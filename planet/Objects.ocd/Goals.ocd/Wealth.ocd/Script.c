/*-- 
	Wealth
	Author: Maikel
	
	Each player much reach the specified wealth to complete the goal.
--*/


#include Library_Goal

local wealth_goal;

protected func Initialize()
{
	wealth_goal = 0;
	return inherited(...);
}

/*-- Wealth goal --*/

public func SetWealthGoal(int wealth)
{
	wealth_goal = Max(0, wealth);
	return;
}

public func GetWealthGoal()
{
	return wealth_goal;
}


/* Scenario saving */

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	var v = GetWealthGoal();
	if (v) props->AddCall("Goal", this, "SetWealthGoal", v);
	return true;
}



/*-- Goal interface --*/

// The goal is fulfilled if all players have the specfied wealth.
public func IsFulfilled()
{
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		if (GetWealth(plr) < GetWealthGoal())
			return false;
	}
	// Goal fulfilled.
	return true;
}

public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
		message = Format("$MsgGoalFulfilled$", GetWealthGoal());	
	else
		message = Format("$MsgGoalUnfulfilled$", GetWealth(plr), GetWealthGoal());

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
		message = Format("@$MsgGoalFulfilled$", GetWealthGoal());	
	else
		message = Format("@$MsgGoalUnfulfilled$", GetWealth(plr), GetWealthGoal());

	CustomMessage(message, nil, plr, 0, 16 + 64, 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	return;
}

protected func FxGoalMessageStart() {}

public func GetShortDescription(int plr)
{
	// Show acquired wealth compared to goal.
	var wealth = GetWealth(plr);
	var goal = GetWealthGoal();
	var clr = RGB(255, 0, 0);
	if (wealth >= goal)
		clr = RGB(0, 255, 0);
	var msg = Format("<c %x>%d</c>{{%i}}", clr, goal, Icon_Wealth);
	return msg;
}

/*-- Proplist --*/

local Name = "$Name$";
