/*-- Bread --*/

protected func Hit()
{
	Sound("Hits::GeneralHit?");
}

/* Eating */

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->Eat(this);
	return true;
}

public func NutritionalValue() { return 50; }
public func IsKitchenProduct() { return true; }
public func GetFuelNeed() { return 50; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;