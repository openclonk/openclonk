/*--
		Build plane goal
		Author: Sven2
		
		All plane parts have to be brought to the construction site
--*/


#include Library_Goal

protected func Initialize()
{
	return inherited(...);
}

public func IsFulfilled()
{
	return ObjectCount(Find_ID(Plane));
}

public func Activate(int byplr)
{
	if (IsFulfilled())
		MessageWindow("$MsgGoalFulfilled$", byplr);
	else
		MessageWindow("$MsgGoalUnFulfilled$", byplr);
	return;
}

public func GetShortDescription(int plr)
{
	return "{{Plane}}"; // TODO
}

/*-- Proplist --*/
local Name = "$Name$";
