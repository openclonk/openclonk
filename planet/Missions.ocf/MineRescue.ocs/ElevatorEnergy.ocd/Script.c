/**
	Energy supply
	A given object has to be supplied with energy.
	
	@author ck
*/

#include Library_Goal

local target;

protected func Initialize()
{
	target = 0;
	return inherited(...);
}

// Set the target object to be supplied with energy
public func SetTarget(object target_)
{
	target = target_;
}

// The goal is fulfilled if the target has been supplied with energy.
public func IsFulfilled()
{
	// Get the power network for the target.
	var network = GetPowerSystem()->GetPowerNetwork(target);
	if (!network)
		return false;
	return network->GetBarePowerAvailable() > 0;
}

public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
		message = "$MsgGoalFulfilled$";		
	else
		message = "$MsgGoalUnFulfilled$";
	return message;
}

// Shows or hides a message window with information.
public func Activate(int plr)
{
	// If goal message open -> hide it.
	if (GetEffect("GoalMessage", this))
	{
		CustomMessage("", nil, plr, nil, nil, nil, nil, nil, MSG_HCenter);
		RemoveEffect("GoalMessage", this);
		return;
	}

	// Otherwise open a new message.
	AddEffect("GoalMessage", this, 100, 0, this);
	var message;
	if (IsFulfilled())
		message = "@$MsgGoalFulfilled$";
	else
		message = "@$MsgGoalUnFulfilled$";

	CustomMessage(message, nil, plr, 0, 16 + 64, 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	return;
}

protected func FxGoalMessageStart() {}

public func GetShortDescription(int plr)
{
	return "";
}

/*-- Proplist --*/

local Name = "$Name$";
