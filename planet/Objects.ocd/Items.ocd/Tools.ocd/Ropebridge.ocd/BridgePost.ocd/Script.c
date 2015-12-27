/**
	Ropebridge Post
	This serves as the end of a ropebridge, at which the bridge can be retracted.
	
	@author Randrian
*/

local double;
local bridge;
local is_static;

public func Initialize()
{
	// Only create a double if there is not already post at this location.
	if (FindObject(Find_ID(GetID()), Find_Exclude(this), Find_AtPoint()))
		return;
	if (!double)
	{
		double = CreateObject(GetID());
		double.Plane = 600;
		double->SetAction("Attach", this);
		double->SetGraphics("Foreground", GetID());
	}
	return;
}

public func Turn(int dir)
{
	var turn_dir = 1;
	if (dir == DIR_Right)
		turn_dir = -1;
	SetObjDrawTransform(1000 * turn_dir, 0, 0, 0, 1000);
	if (double)
		double->SetObjDrawTransform(1000 * turn_dir, 0, 0, 0, 1000);
	return;
}

public func SetBridge(object to_bridge)
{
	bridge = to_bridge;
	return;
}

public func SetStatic(bool to_static)
{
	is_static = to_static;
	return;
}

public func Destruction()
{
	if (double)
		double->RemoveObject();
	if (bridge)
		bridge->OnBridgePostDestruction();
	return;
}


/*-- Interaction --*/

public func Interact(object clonk)
{
	// Only an interaction for the real post, not the double.
	if (is_static || !double)
		return true;
	if (!bridge)
	{
		RemoveObject();
		return true;
	}
	// Make bridge retract itself.
	// TODO: implement animation, etc.
	bridge->RemoveObject();
	clonk->CreateObjectAbove(Ropebridge, 0, clonk->GetBottom());
	return true;
}

public func IsInteractable(object clonk)
{
	return !is_static && !!double && clonk->IsWalking();
}

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$MsgRetractBridge$", IconName = nil, IconID = nil, Selected = false };
}


/*-- Properties --*/

local ActMap = {
	Attach = {
		Prototype = Action,
		Name = "Attach",
		Procedure = DFA_ATTACH,
		FacetBase = 1,
	},
};
local Name = "$Name$";
local Description = "$Description$";
local Plane = 600;
	
