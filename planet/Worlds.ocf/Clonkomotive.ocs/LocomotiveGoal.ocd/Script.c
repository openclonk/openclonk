/** 
	Locomotive Transport Goal
	Player must drive the train to a certain position.
	
	@author Sven2, Pyrit
*/


#include Library_Goal

local goal_rect;

public func SetGoalRect(int x, int y, int wdt, int hgt)
{
	goal_rect = Rectangle(x, y, wdt, hgt);
	return true;
}

/*-- Goal interface --*/

// The goal is fulfilled if the train is somewhere in a rectangle.
public func IsFulfilled()
{
	return FindObject(Find_ID(Locomotive), Find_InRect(AbsX(goal_rect.x), AbsY(goal_rect.y), goal_rect.wdt, goal_rect.hgt));
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


/*-- Proplist --*/

local Name = "$Name$";
