#include Library_Liquid

func GetLiquidType() { return "Oil"; }

func Disperse()
{
	DisperseMaterial(IsLiquid(), GetLiquidAmount());
	_inherited(...);
}

local Name="$Name$";