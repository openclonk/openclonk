/*-- Hammer --*/

private func Hit()
{
	Sound("RockHit");
	return 1;
}

public func GetCarryMode()	{	return CARRY_HandBack;	}
public func GetCarryBone()	{	return "main";	}
public func GetCarryTransform()	{	return Trans_Rotate(90,0,1,0);	}

public func ControlUseStart(object clonk, int x, int y)
{
	// Is the clonk able to construct?
	if(clonk->GetProcedure() != "WALK")
	{
		clonk->CancelUse();
		return true;
	}
	// Is the clonk at an construction site?
	var structure = FindObject(Find_Category(C4D_Structure), Find_Or(Find_Distance(20), Find_AtPoint()), Find_Layer(GetObjectLayer()));
	if (structure)
	{
		if (structure->GetCon() < 100)
		{
			Construct(clonk, structure);
			return true;
		}
		if (structure->GetDamage() > 0)
		{
			Repair(clonk, structure);
			return true;
		}
		clonk->CancelUse();
		return true;
	}
	
	// Otherwise create a menu with possible structures to build.
	clonk->CreateMenu(Hammer, this, 1, "$TxtNoconstructionplansa$");
	var structure_id, index = 0;
	while (structure_id = GetPlrKnowledge(clonk->GetOwner(), 0, index++, C4D_Structure))
		clonk->AddMenuItem("$TxtConstructions$", "CreateConstructionSite", structure_id);
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
		if (structure->GetCon() < 100)
		{
			Construct(clonk, structure);
			return true;
		}
		if (structure->GetDamage() > 0)
		{
			Repair(clonk, structure);
			return true;
		}
	}	
	clonk->CancelUse();
	return true;
}

private func Construct(object clonk, object structure)
{
	// Look for missing components.
	var structure_id = structure->GetID();
	var con = structure->GetCon();
	var comp, index = 0;
	while (comp = structure->GetComponent(nil, index))
	{
		var current_amount = structure->GetComponent(comp);
		var max_amount = GetComponent(comp, nil, nil, structure_id);
		// Check if there is enough material for current con.
		if (100 * current_amount / max_amount < con)
		{
			var content = FindObject(Find_Container(clonk), Find_ID(comp));
			if (!content)
			{
				clonk->Message("Construction needs {{%i}}", comp);
				clonk->CancelUse();
				return;
			}
			clonk->Message("Used {{%i}}", comp);
			content->RemoveObject();
			structure->SetComponent(comp, current_amount+1);			
		}
		index++;
	}
	structure->DoCon(1);
	clonk->Message("Constructing %d%", structure->GetCon());
	return;
}

private func Repair(object clonk, object structure)
{



	return;
}

protected func CreateConstructionSite(id structure_id)
{
	var clonk = Contained();
	// Only when the clonk is standing and outdoors
	if (clonk->GetAction() != "Walk")
		return false;
	if (clonk->Contained()) 
		return false;
	// Check if the building can be build here
	if (structure_id->~RejectConstruction(0, 10, clonk)) 
		return false;
	// Set owner for CreateConstruction
	SetOwner(clonk->GetOwner());
	// Create construction site
	var site;
	if (!(site = CreateConstruction(structure_id, 0, 10, Contained()->GetOwner(), 1, 1,1)))
		return false;
	// Message
	clonk->Message("$TxtConstructions$", site->GetName());
	return true;
}

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Rotate(20,1,0,1),def);
}
local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
