/**
	Constructor
	Library for objects which are able to construct structures.
	
	@author Maikel
*/


public func IsConstructor() { return true; }


public func ControlUseStart(object clonk, int x, int y)
{
	// Is the clonk able to construct?
	if(clonk->GetProcedure() != "WALK")
	{
		clonk->CancelUse();
		return true;
	}
	// Is the clonk at an construction site?
	// TODO: check for multiple objects
	var structure = FindObject(Find_Category(C4D_Structure), Find_Or(Find_Distance(20), Find_AtPoint()), Find_Layer(GetObjectLayer()));
	if (structure)
	{
		if (structure->GetDamage() > 0)
		{
			Repair(clonk, structure);
			return true;
		}
	}
	
	// Otherwise create a menu with possible structures to build.
	clonk->CreateConstructionMenu(this, true);
	clonk->CancelUse();
	return true;
}

public func HoldingEnabled() { return true; }

public func ControlUseHolding(object clonk, int x, int y)
{
	// Is the clonk still able to construct?
	if (clonk->GetProcedure() != "WALK")
	{
		clonk->CancelUse();
		return true;
	}
	// Is the clonk still at an construction site?
	var structure = FindObject(Find_Category(C4D_Structure), Find_Or(Find_Distance(20), Find_AtPoint()), Find_Layer(GetObjectLayer()));
	if (structure)
	{	
		if (structure->GetDamage() > 0)
		{
			Repair(clonk, structure);
			return true;
		}
	}	
	clonk->CancelUse();
	return true;
}

private func ShowConstructionMaterial(object clonk, object structure)
{
	var mat_msg = "$TxtNeeds$";
	var structure_id = structure->GetID();
	var comp, index = 0;
	while (comp = structure->GetComponent(nil, index))
	{
		var current_amount = structure->GetComponent(comp);
		var max_amount = GetComponent(comp, nil, nil, structure_id);
		mat_msg = Format("%s %dx{{%i}}", mat_msg, Max(0, max_amount - current_amount), comp);
		index++;
	}
	clonk->Message(mat_msg);
	return;
}


private func Repair(object clonk, object structure)
{

}

/** Gives a list of ids of the players knowledge.
*/
public func GetConstructionPlans(int plr)
{
	var construction_plans = [];
	var construct_id, index = 0;
	while (construct_id = GetPlrKnowledge(plr, nil, index++, C4D_Structure))
		construction_plans[index-1] = construct_id;
	return construction_plans;
}

/* Construction preview */

func ShowConstructionPreview(object clonk, id structure_id)
{
	AddEffect("ControlConstructionPreview", clonk, 1, 0, this, nil, structure_id, clonk);
	SetPlayerControlEnabled(clonk->GetOwner(), CON_Aim, true);
	return true;
}

func FxControlConstructionPreviewStart(object clonk, effect, int temp, id structure_id, object clonk)
{
	if (temp) return;

	effect.structure = structure_id;
	effect.flipable = !structure_id->~NoConstructionFlip();
	effect.preview = structure_id->~CreateConstructionPreview(clonk);
	if (!effect.preview) effect.preview = CreateObjectAbove(ConstructionPreviewer, AbsX(clonk->GetX()), AbsY(clonk->GetY()), clonk->GetOwner());
	effect.preview->Set(structure_id, clonk);
}

// Called by Control2Effect
func FxControlConstructionPreviewControl(object clonk, effect, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (ctrl != CON_Aim)
	{
		// CON_Use is accept
		if (ctrl == CON_Use)
			CreateConstructionSite(clonk, effect.structure, AbsX(effect.preview->GetX()), AbsY(effect.preview->GetY() + effect.preview.dimension_y/2), effect.preview.blocked, effect.preview.direction, effect.preview.stick_to);
		// movement is allowed
		else if (IsMovementControl(ctrl))
			return false;
		// Flipping
		// this is actually realized twice. Once as an Extra-Interaction in the clonk, and here. We don't want the Clonk to get any non-movement controls though, so we handle it here too.
		// (yes, this means that actionbar-hotkeys wont work for it. However clicking the button will.)
		else if (IsInteractionControl(ctrl))
		{
			if (release)
				effect.preview->Flip();
			return true;
		}

		// everything else declines
		RemoveEffect("ControlConstructionPreview", clonk, effect);
		return true;
	}
		
	effect.preview->Reposition(x, y);
	return true;
}

func FxControlConstructionPreviewStop(object clonk, effect, int reason, bool temp)
{
	if (temp) return;

	effect.preview->RemoveObject();
	SetPlayerControlEnabled(clonk->GetOwner(), CON_Aim, false);
}

/* Construction */

func CreateConstructionSite(object clonk, id structure_id, int x, int y, bool blocked, int dir, object stick_to)
{
	// Only when the clonk is standing and outdoors
	if (clonk->GetAction() != "Walk")
		return false;
	if (clonk->Contained())
		return false;
	// Check if the building can be build here
	if (structure_id->~RejectConstruction(x, y, clonk)) 
		return false;
	if (blocked)
	{
		CustomMessage("$TxtNoSiteHere$", this, clonk->GetOwner(), nil,nil, RGB(255,0,0)); 
		return false;
	}
	// intersection-check with all other construction sites... bah
	for(var other_site in FindObjects(Find_ID(ConstructionSite)))
	{
		if(!(other_site->GetLeftEdge()   > GetX()+x+structure_id->GetDefWidth()/2  ||
		     other_site->GetRightEdge()  < GetX()+x-structure_id->GetDefWidth()/2  ||
		     other_site->GetTopEdge()    > GetY()+y+structure_id->GetDefHeight()/2 ||
		     other_site->GetBottomEdge() < GetY()+y-structure_id->GetDefHeight()/2 ))
		{
			CustomMessage(Format("$TxtBlocked$",other_site->GetName()), this, clonk->GetOwner(), nil,nil, RGB(255,0,0));
			return false;
		}
	}
	
	// Set owner for CreateConstruction
	SetOwner(clonk->GetOwner());
	// Create construction site
	var site;
	site = CreateObjectAbove(ConstructionSite, x, y, Contained()->GetOwner());
	/* note: this is necessary to have the site at the exact position x,y. Otherwise, for reasons I don't know, the
	   ConstructionSite seems to move 2 pixels downwards (on ConstructionSite::Construction() it is still the
	   original position) which leads to that the CheckConstructionSite function gets different parameters later
	   when the real construction should be created which of course could mean that it returns something else. (#1012)
	   - Newton
	*/
	site->SetPosition(GetX()+x,GetY()+y);
	
	// Randomize sign rotation
	site -> SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(RandomX(-30, 30), 0, 1, 0), Trans_Rotate(RandomX(-10, 10), 1, 0, 0)));
	site -> PlayAnimation("LeftToRight", 1, Anim_Const(RandomX(0, GetAnimationLength("LeftToRight"))), Anim_Const(500));
	
	site -> Set(structure_id, dir, stick_to);
	//if(!(site = CreateConstruction(structure_id, x, y, Contained()->GetOwner(), 1, 1, 1)))
		//return false;
	
	// check for material
	var comp, index = 0;
	var mat;
	var w = structure_id->GetDefWidth()+10;
	var h = structure_id->GetDefHeight()+10;

	while (comp = GetComponent(nil, index, nil, structure_id))
	{
		// find material
		var count_needed = GetComponent(comp, nil, nil, structure_id);
		index++;
		
		mat = CreateArray();
		// 1. look for stuff in the clonk
		mat[0] = FindObjects(Find_ID(comp), Find_Container(clonk));
		// 2. look for stuff lying around
		mat[1] = clonk->FindObjects(Find_ID(comp), Find_NoContainer(), Find_InRect(-w/2, -h/2, w,h));
		// 3. look for stuff in nearby lorries/containers
		var i = 2;
		for(var cont in clonk->FindObjects(Find_Or(Find_Func("IsLorry"), Find_Func("IsContainer")), Find_InRect(-w/2, -h/2, w,h)))
			mat[i] = FindObjects(Find_ID(comp), Find_Container(cont));
		// move it
		for(var mat2 in mat)
		{
			for(var o in mat2)
			{
				if(count_needed <= 0)
					break;
				o->Exit();
				o->Enter(site);
				count_needed--;
			}
		}
	}

	
	// Message
	clonk->Message("$TxtConstructions$", structure_id->GetName());
	return true;
}
