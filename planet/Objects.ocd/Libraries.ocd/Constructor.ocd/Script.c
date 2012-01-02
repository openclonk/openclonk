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
	
	// Otherwise create a menu with possible structures to build.
	clonk->CreateConstructionMenu(this);
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

}

/** Gives a list of ids of the players knowledge.
*/
public func GetConstructionPlans(int plr)
{
	var construction_plans = [];
	var construct_id, index = 0;
	while (construct_id = GetPlrKnowledge(plr, 0, index++, C4D_Structure))
		construction_plans[index-1] = construct_id;
	return construction_plans;
}

protected func CreateConstructionSite(object clonk, id structure_id)
{
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
	if (!(site = CreateConstruction(structure_id, 0, 10, Contained()->GetOwner(), 1, 1, 1)))
		return false;
	// Message
	clonk->Message("$TxtConstructions$", site->GetName());
	return true;
}

