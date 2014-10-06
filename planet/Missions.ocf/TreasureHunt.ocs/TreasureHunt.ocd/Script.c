/*--
		Treasure Hunt
		Author: Sven2
		
		Must find the treasure and sell it
--*/


#include Library_Goal

local has_gem_task, got_oil, is_fulfilled;

public func IsFulfilled() { return is_fulfilled; }

public func OnGotGemTask() { SetGraphics("Hunt"); SetName("$Name2$"); return has_gem_task = true; }
public func OnTreasureSold() { SetGraphics(nil); SetName("$Name$"); return got_oil = true; }
public func OnOilDelivered() { return is_fulfilled = true; }

public func GetDescription(int plr)
{
	if (is_fulfilled)
		return "$MsgGoalFulfilled$";
	else if (got_oil)
		return "$MsgGotOil$";
	else if (has_gem_task)
		return "$MsgGoalUnFulfilled$";
	else
		return "$MsgGoalOil$";
}

public func Activate(int byplr)
{
	var desc = GetDescription(byplr);
	if (has_gem_task) desc = Format("%s|%s", desc, Format("$MsgSideGoal$", g_num_goldbars, MAX_GOLD_BARS));
	ToggleGoalMessage(desc, byplr);
	return true;
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
public func GetShortDescription(int plr) {}

/*-- Proplist --*/
local Name = "$Name$";
