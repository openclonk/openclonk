/*-- Cooked Mushroom --*/

func Hit()
{
	Sound("GeneralHit?");
}

/* Eating */

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->Eat(this);
}

public func NutritionalValue() { return 25; }

public func IsKitchenProduct() { return true; }
public func GetFuelNeed() { return 50; }

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = 1;
local Rebuy = 1;