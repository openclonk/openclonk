/**
	No Energy Rule
	If this rule is activated, power consumers do not need energy supply 
	anymore to operate. This rule informs existing consumers about its 
	presence and removal. Consumers created while this rule is active 
	will themselves set the no power need.
	
	@author Zapper, Maikel
*/


protected func Initialize()
{
	// Don't do anything if this is not the first rule of this type.
	if (ObjectCount(Find_ID(Rule_NoPowerNeed)) > 1) 
		return;
		
	// Find all consumers and tell them to have no power need.
	for (var consumer in FindObjects(Find_Func("IsPowerConsumer")))
		consumer->SetNoPowerNeed(true);
	return;
}

protected func Destruction()
{
	// If this is not the last copy of this rule do nothing. 
	if (ObjectCount(Find_ID(Rule_NoPowerNeed)) > 1)
		return;
	
	// Find all consumers and tell them to have power need again.
	for (var consumer in FindObjects(Find_Func("IsPowerConsumer")))
		consumer->SetNoPowerNeed(false);
	return;
}

public func Activate(int by_plr)
{
	MessageWindow(this.Description, by_plr);
	return true;
}

local Name = "$Name$";
local Description = "$Description$";
local Visibility = VIS_Editor;
local EditorPlacementLimit = 1; // Rules are to be placed only once
