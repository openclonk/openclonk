/*-- 
	Sell Gems
	Author: Sven2
	
	Sell n gems at flagpole.
--*/


#include Library_Goal

local gems_to_sell;

protected func Initialize()
{
	gems_to_sell = 20; // default
	return inherited(...);
}

func SetTargetAmount(int new_amount)
{
	gems_to_sell = new_amount;
	return true;
}

func OnGemSold()
{
	// A gem was sold. Subtract.
	gems_to_sell = Max(gems_to_sell-1);
	return true;
}


/* Scenario saving */

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (gems_to_sell) props->AddCall("Goal", this, "SetTargetAmount", gems_to_sell);
	return true;
}


/*-- Goal interface --*/

// The goal is fulfilled if no more gems need to be sold
public func IsFulfilled()
{
	return (gems_to_sell<=0);
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
		message = Format("@$MsgGoalUnfulfilled$", gems_to_sell);

	CustomMessage(message, nil, plr, 0, 16 + 64, 0xffffff, GUI_MenuDeco, this, MSG_HCenter);
	return;
}

protected func FxGoalMessageStart() {}

public func GetShortDescription(int plr)
{
	// Show acquired wealth compared to goal.
	var clr = RGB(255, 0, 0);
	if (gems_to_sell<=0)
		clr = RGB(0, 255, 0);
	var msg = Format("<c %x>%d</c>{{%i}}", clr, gems_to_sell, Ruby);
	return msg;
}

/*-- Proplist --*/

local Name = "$Name$";
