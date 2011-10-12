/*-- Cooked Mushroom --*/

func Hit()
{
	Sound("WoodHit?");
}

/* Eating */

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->Eat(this);
}

public func NutritionalValue() { return 15; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Rebuy = 1;