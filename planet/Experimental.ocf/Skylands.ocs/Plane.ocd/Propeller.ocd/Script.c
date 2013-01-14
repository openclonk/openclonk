/*--
	Plane part: Propeller
	Author: Sven2

	Used to construct the plane
--*/

#include Library_PlanePart

private func Hit()
{
	Sound("WoodHit");
}

public func Definition(proplist def)
{
}

public func IsPlanePart() { return true; }

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local HitPoints = 20;
