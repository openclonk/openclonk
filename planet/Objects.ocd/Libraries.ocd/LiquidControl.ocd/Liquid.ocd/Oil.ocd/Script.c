#include Library_Liquid

func IsLiquid() { return "Oil"; }

func Disperse()
{
	DisperseMaterial(IsLiquid(), GetLiquidAmount());
	_inherited(...);
}

local Name="$Name$";