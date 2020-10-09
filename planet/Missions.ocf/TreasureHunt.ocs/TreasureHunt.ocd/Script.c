/*--
		Treasure Hunt
		Author: Sven2
		
		Must find the treasure and sell it
--*/


#include Library_Goal

local has_gem_task, got_oil, is_fulfilled;

public func IsFulfilled() { return is_fulfilled; }

public func OnGotGemTask()
{
	SetGraphics("Hunt");
	SetName("$Name2$");
	has_gem_task = true;
	return true;
}
public func OnTreasureSold()
{
	SetGraphics(nil);
	SetName("$Name$");
	got_oil = true;
	return true;
}
public func OnOilDelivered()
{
	is_fulfilled = true;
	return true;
}

public func GetDescription(int plr)
{
	var desc;
	if (is_fulfilled)
		desc = "$MsgGoalFulfilled$";
	else if (got_oil)
		desc = "$MsgGotOil$";
	else if (has_gem_task)
		desc = "$MsgGoalUnFulfilled$";
	else
		desc = "$MsgGoalOil$";
	if (has_gem_task || g_num_goldbars) desc = Format("%s|%s", desc, Format("$MsgSideGoal$", g_num_goldbars, MAX_GOLD_BARS));
	return desc;
}

public func GetShortDescription(int plr) {}

/*-- Proplist --*/
local Name = "$Name$";
