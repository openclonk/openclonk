/**
	Oil
	Represents a single material pixel of oil.
	
	@author Marky
*/

#include Library_Liquid


public func GetLiquidType() { return "Oil"; }

public func Disperse(int angle, int strength)
{
	DisperseMaterial(GetLiquidType(), GetLiquidAmount(), strength, angle);
	_inherited(angle, strength, ...);
}

public func IsFuel() { return true; }

public func GetFuelAmount(int requested_amount)
{
	requested_amount = requested_amount ?? GetLiquidAmount();
	return Min(requested_amount, GetLiquidAmount());
}

public func OnFuelRemoved(int amount)
{
	DoStackCount(-amount);
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$"; 