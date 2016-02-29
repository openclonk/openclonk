#include Library_Liquid

func GetLiquidType() { return "Water"; }

func Disperse()
{
	DisperseMaterial(IsLiquid(), GetLiquidAmount());
	_inherited(...);
}

local Name="$Name$";