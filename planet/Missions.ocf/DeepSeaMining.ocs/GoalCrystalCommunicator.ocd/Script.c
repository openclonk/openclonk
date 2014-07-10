/*-- 
	Build crystal communicator
	Author: Sven2
	
	Player must build the crystal communicator
--*/


#include Library_Goal


/*-- Goal interface --*/

// The goal is fulfilled if the communicator has been built
public func IsFulfilled()
{
	return ObjectCount(Find_ID(CrystalCommunicator));
}

public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
		message = "$MsgGoalFulfilled$";		
	else
		message = "$MsgGoalUnfulfilled$";
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
	{
		message = "@$MsgGoalFulfilled$";		
	}
	else
	{
		message = "@$MsgGoalUnfulfilled$";
	}
	CustomMessage(message, nil, plr, 0, 16 + 64, 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	return;
}

protected func FxGoalMessageStart() {}

//public func GetShortDescription(int plr) { return ""; }

/*-- Proplist --*/

local Name = "$Name$";
