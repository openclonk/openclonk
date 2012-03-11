/*--
	Plane part: Engine
	Author: Sven2

	Used to construct the plane
--*/

private func Hit()
{
	Sound("WoodHit");
}

func IsToolProduct() { return true; }

public func Definition(proplist def)
{
}

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
