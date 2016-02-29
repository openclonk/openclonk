#include Library_Liquid

func GetLiquidType() { return "Fuel"; }

func Disperse()
{
	_inherited(...);
}

local Name="$Name$";