#include Library_Liquid

func GetLiquidType() { return "Lava"; }

func Disperse()
{
	DisperseMaterial(IsLiquid(), GetLiquidAmount());
	_inherited(...);
}

local Name="$Name$";