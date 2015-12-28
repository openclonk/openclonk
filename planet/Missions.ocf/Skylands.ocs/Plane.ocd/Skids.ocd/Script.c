/*--
	Plane part: Skids
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
local Touchable = 1;
local HitPoints = 20;
