/**
	Cooked Mushroom
	Yummier meal than an uncooked mushroom.
*/

/*-- Engine Callbacks --*/

func Construction()
{
	this.MeshTransformation = Trans_Rotate(RandomX(0, 359), 0, 1, 0);
}

func Hit()
{
	Sound("Hits::GeneralHit?");
}

/*-- Eating --*/

public func ControlUse(object clonk)
{
	clonk->Eat(this);
	return true;
}

public func NutritionalValue() { return 25; }

/*-- Production --*/

public func IsKitchenProduct() { return true; }
public func GetFuelNeed() { return 50; }

/*-- Display --*/

public func GetCarryMode()
{
	return CARRY_Hand;
}

public func GetCarryTransform()
{
	return Trans_Scale(750);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local Components = {Mushroom = 1};