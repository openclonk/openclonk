/*--- Gold Bar ---*/

protected func Hit()
{
	Sound("Hits::GeneralHit?");
	return 1;
}

public func IsFoundryProduct() { return true; }
public func GetFuelNeed() { return 100; }
public func IsValuable(){ return true; }
public func QueryRejectRebuy(){ return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Plane = 480;
local Components = {Nugget = 3};