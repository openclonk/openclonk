/*--
		Plane goal
		Author: Maikel
		
		The plane has to be retrieved and transported back to the pilot.
--*/


#include Library_Goal

protected func Initialize()
{
	return inherited(...);
}

public func IsFulfilled()
{
	var cabin = FindObject(Find_ID(WoodenCabin));
	if (!cabin)
		return false;
	var plane = FindObject(Find_ID(Plane));
	if (!plane)
		return false;
	// Plane has to be brought to the wooden cabin.
	if (ObjectDistance(plane, cabin) < 80)
		return true;
	return false;	
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
