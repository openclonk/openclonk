/*--- The Log ---*/

#include Library_Flammable

protected func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit?");
	return 1;
}

func Incineration()
{
	SetClrModulation(RGB(48, 32, 32));
	_inherited(...);
}

public func IsFuel() { return true; }
public func GetFuelAmount(int requested_amount) 
{ 
    // disregard the parameter, because only a complete chunk should be removed 
	if (this != Wood) return GetCon() / 2;
	return 50;
}
public func IsSawmillProduct() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 5;
local ContactIncinerate = 1;
local Plane = 470;