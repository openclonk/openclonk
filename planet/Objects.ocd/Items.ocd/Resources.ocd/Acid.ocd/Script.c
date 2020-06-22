/**
	Acid
	Represents a single material pixel of acid.
	
	@author Marky
*/

#include Library_Liquid


public func GetLiquidType() { return "Acid"; }

public func Disperse(int angle, int strength)
{
	DisperseMaterial(GetLiquidType(), GetLiquidAmount(), strength, angle);
	_inherited(angle, strength, ...);
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$"; 