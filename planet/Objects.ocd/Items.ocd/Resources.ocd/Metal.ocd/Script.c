/*--- Metal ---*/

protected func Construction()
{
	if(GBackSemiSolid())
		SetGraphics("Old");
}

protected func Hit()
{
	Sound("GeneralHit?");
	return 1;
}

public func IsFoundryProduct() { return true; }
public func GetFuelNeed() { return 100; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = true;
local Plane = 470;