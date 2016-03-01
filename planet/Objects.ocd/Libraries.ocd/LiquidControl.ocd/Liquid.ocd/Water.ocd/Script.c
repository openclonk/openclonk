#include Library_Liquid

func GetLiquidType() { return "Water"; }

func Disperse()
{
	DisperseMaterial(GetLiquidType(), GetLiquidAmount());
	_inherited(...);
}

local Name="$Name$";