/*--- The Log ---*/

protected func Hit()
{
	Sound("WoodHit?");
	return 1;
}

func Incineration()
{
	SetClrModulation (RGB(48, 32, 32));
}

public func IsFuel() { return true; }
public func GetFuelAmount() { return 50; }
public func IsSawmillProduct() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local BlastIncinerate = 5;
local ContactIncinerate = 1;
local Plane = 470;