/*-- 
	Acid drilling
	Author: Sven2
	
	Player must fill a small basin with acid.
--*/


#include Library_Goal

local basin_x, basin_y;

public func SetBasinPosition(int x, int y)
{
	SetPosition(0, 0);
	basin_x = x; basin_y = y;
	return true;
}

// Scenario saving
public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (basin_x || basin_y) props->AddCall("Goal", this, "SetBasinPosition", basin_x, basin_y);
	return true;
}


/*-- Goal interface --*/

// The goal is fulfilled if a statue has been assigned and it's repaired.
public func IsFulfilled()
{
	return GetMaterial(basin_x, basin_y) == Material("Acid");
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
