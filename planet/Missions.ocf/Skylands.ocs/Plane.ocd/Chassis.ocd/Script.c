/*--
	Plane part: Chassis
	Author: Sven2

	Used to construct the plane
--*/

#include Library_PlanePart
#include Library_ElevatorControl

private func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit*");
}

public func Definition(proplist def)
{
}

public func IsPlanePart() { return true; }

local Collectible = false;
local Name = "$Name$";
local Description = "$Description$";
local Touchable = 1; // Later, this could be done with the lift tower. There is no working lift tower at the moment though :(
local HitPoints = 20;
