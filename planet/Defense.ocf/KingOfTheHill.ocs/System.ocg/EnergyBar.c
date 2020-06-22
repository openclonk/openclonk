// Add energy bar and hitpoints to flagpoles.

#appendto Flagpole

public func Initialize()
{
	AddEnergyBar();
	return _inherited(...);
}

local HitPoints = 400;
