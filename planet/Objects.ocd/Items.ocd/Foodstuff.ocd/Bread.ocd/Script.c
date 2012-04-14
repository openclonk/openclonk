/*-- Bread --*/

protected func Hit()
{
	Sound("GeneralHit?");
}

public func IsOvenProduct() { return true; }

/* Eating */

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->Eat(this);
}

public func NutritionalValue() { return 50; }

public func NeedsWater() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = true;