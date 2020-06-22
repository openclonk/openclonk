/**
	Concrete
	Represents a single material pixel of concrete.
	
	@author Maikel
*/

#include Library_Liquid


public func GetLiquidType() { return "Concrete"; }

public func GetLiquidMaterial() { return "Granite"; }

public func Disperse(int angle, int strength)
{
	DisperseMaterial(GetLiquidMaterial(), GetLiquidAmount(), strength, angle);
	_inherited(angle, strength, ...);
}


/*-- Production --*/

public func IsFoundryProduct() { return true; }

public func GetProductionTime() { return 120; }

public func Construction(object creator)
{
	var res = _inherited(creator, ...);
	// If the concrete is created by the foundry we can safely assume it has been produced
	// and set the stack count to 200. The only exceptions are CreateContents script calls.
	if (creator && creator->~IsProducer())
		SetStackCount(200);
	return res;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Components = {Rock = 1, Water = 100};