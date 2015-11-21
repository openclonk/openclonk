/*--
		Build plane goal
		Author: Sven2
		
		All plane parts have to be brought to the construction site
--*/

#include Library_Goal

local progress = 0;

private func Initialize()
{
	return inherited(...);
}

public func IsFulfilled()
{
	return ObjectCount(Find_ID(Airplane));
}

public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
		message = "$MsgGoalFulfilled$";
	else
	{
		message = "$MsgGoalUnFulfilled$";
		if (!progress)
			message = Format("%s|$MsgGoalSkids$", message);
		if (progress == 1)
			message = Format("%s|$MsgGoalChassis$", message);
		if (progress == 2)
			message = Format("%s|$MsgGoalWings$", message);
		if (progress == 3)
			message = Format("%s|$MsgGoalEngine$", message);
		if (progress == 4)
			message = Format("%s|$MsgGoalPropeller$", message);
	}
	return message;
}

public func Activate(int byplr)
{
	MessageWindow(GetDescription(byplr), byplr);
}

public func GetShortDescription(int plr)
{
	var next_part = Airplane_Skids;
	if (progress == 1)
		next_part = Airplane_Chassis;
	if (progress == 2)
		next_part = Airplane_Wings;
	if (progress == 3)
		next_part = Airplane_Engine;
	if (progress == 4)
		next_part = Airplane_Propeller;
	return Format("{{%i}}", next_part);
}

public func OnPlanePartAdded()
{
	progress++;
}

/*-- Proplist --*/
local Name = "$Name$";
