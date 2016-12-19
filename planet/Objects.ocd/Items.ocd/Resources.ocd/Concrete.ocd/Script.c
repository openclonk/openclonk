/**
	Concrete
	Represents a single material pixel of concrete.
	
	@author Maikel
*/

#include Library_Liquid


public func GetLiquidType() { return "Concrete"; }

public func Disperse(int angle, int strength)
{
	DisperseMaterial("Rock", GetLiquidAmount(), strength, angle);
	_inherited(angle, strength, ...);
}


/*-- Production --*/

public func IsFoundryProduct() { return true; }

public func Construction(object creator)
{
	var res = _inherited(creator, ...);
	// If the concrete is created by the foundry we can safely assume it has been produced
	// and set the stack count to 100.
	if (creator->~IsProducer())
		SetStackCount(100);
	return res;
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Components = {Rock = 1, Water = 100};