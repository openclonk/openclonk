/*-- Bread --*/

#include Library_Edible

protected func Hit()
{
	Sound("Hits::GeneralHit?");
}

/* Eating */

public func NutritionalValue() { return 50; }
public func IsKitchenProduct() { return true; }
public func GetFuelNeed() { return 50; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Components = {Flour = 1, Water = 50};
