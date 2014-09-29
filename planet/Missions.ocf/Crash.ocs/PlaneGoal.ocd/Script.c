/*--
		Plane goal
		Author: Maikel
		
		The plane has to be retrieved and transported back to the pilot.
--*/


#include Library_Goal

local is_fulfilled = false;
local is_outro_stated = false;

protected func Initialize()
{
	return inherited(...);
}

public func IsFulfilled()
{
	// already done?
	if (is_fulfilled || is_outro_stated) return is_fulfilled;
	// not done yet. do fulfillment check
	var cabin = FindObject(Find_ID(WoodenCabin));
	if (!cabin)
		return false;
	var plane = FindObject(Find_ID(Plane));
	if (!plane)
		return false;
	// Plane has to be brought to the wooden cabin.
	if (ObjectDistance(plane, cabin) < 200)
	{
		is_outro_stated = true;
		StartSequence("Outro", 0, this, plane);
		// wait for end of outro for fulfillment
		return false;
	}
	return false;
}

public func GetDescription(int plr)
{
	var message;
	if (IsFulfilled())
		message = "$MsgGoalFulfilled$";		
	else
		message = "$MsgGoalUnFulfilled$";
	return message;
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

public func SetFulfilled()
{
	is_fulfilled = true;
	return true;
}

/*-- Proplist --*/
local Name = "$Name$";
