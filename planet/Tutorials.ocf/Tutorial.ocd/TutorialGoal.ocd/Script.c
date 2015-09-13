/**
	Tutorial goal
	The goal is fulfilled if the tutorial script says so. This is just a placeholder object, effectively.
	
	@author Maikel
*/


#include Library_Goal

local fulfilled;

protected func Initialize()
{
	fulfilled = false;
	return inherited(...);
}

public func IsFulfilled()
{
	return fulfilled;
}

public func Fulfill()
{
	fulfilled = true;
	return;
}

public func GetDescription(int plr)
{
	return this.Description;
}

/*-- Proplist --*/
local Name = "$Name$";
local Description = "$Description$";