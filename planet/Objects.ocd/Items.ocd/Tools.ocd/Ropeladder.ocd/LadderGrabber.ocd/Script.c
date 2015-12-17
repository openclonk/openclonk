/** 
	Ropeladder Grabber 
	Allows for picking up the rope ladder at its base.
	
	@author Randrian.
*/


public func Interact(object clonk)
{
	if (GetActionTarget())
		GetActionTarget()->StartRollUp();
	else
		RemoveObject();
	return true;
}

public func IsInteractable(object clonk)
{
	return clonk->GetProcedure() == "WALK" || clonk->GetProcedure() == "SCALE" || clonk->GetProcedure() == "HANGLE";
}

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$GrabLadder$", IconName = nil, IconID = nil, Selected = false };
}

public func SaveScenarioObject() { return false; }


/*-- Properties --*/

local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH,
	},
};
local Name = "$Name$";
