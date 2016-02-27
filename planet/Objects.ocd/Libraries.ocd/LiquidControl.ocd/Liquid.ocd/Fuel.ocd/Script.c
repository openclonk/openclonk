#include Library_Liquid

func IsLiquid() { return "Fuel"; }
func IsFuel(){ return GetLiquidAmount();}

func Disperse()
{
	_inherited(...);
}

local Name="$Name$";