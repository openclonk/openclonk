/*-- Cooked Mushroom --*/


protected func Construction()
{
	this.MeshTransformation = Trans_Rotate(RandomX(0, 359), 0, 1, 0);
}

func Hit()
{
	Sound("Hits::GeneralHit?");
}

/* Eating */

protected func ControlUse(object clonk)
{
	clonk->Eat(this);
	return true;
}

public func NutritionalValue() { return 25; }

public func IsKitchenProduct() { return true; }
public func GetFuelNeed() { return 50; }

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = true;