#include Library_Liquid

func GetLiquidType() { return "Lava"; }

func Disperse()
{
	DisperseMaterial(GetLiquidType(), GetLiquidAmount());
	_inherited(...);
}

local Name="$Name$";