/*--
		Treasure Hunt
		Author: Sven2
		
		Must find the treasure and sell it
--*/


#include Library_Goal

local is_fulfilled;

public func IsFulfilled() { return is_fulfilled; }

public func OnTreasureSold() { is_fulfilled = true; }

public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
		message = "$MsgGoalFulfilled$";		
	else
		message = "$MsgGoalUnFulfilled$";
	return message;
}

public func Activate(int byplr)
{
	if (IsFulfilled())
		ToggleGoalMessage("$MsgGoalFulfilled$", byplr);
	else
		ToggleGoalMessage("$MsgGoalUnFulfilled$", byplr);
	return;
}

// Shows or hides a message window with information.
private func ToggleGoalMessage(string msg, int plr)
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
	CustomMessage(Format("@%s", msg), nil, plr, 0, 16 + 64, 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	return;
}

protected func FxGoalMessageStart() {}


public func GetShortDescription(int plr)
{
	return Name;
}

/*-- Proplist --*/
local Name = "$Name$";
