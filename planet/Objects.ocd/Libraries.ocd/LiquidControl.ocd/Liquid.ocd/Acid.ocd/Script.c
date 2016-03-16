#include Library_Liquid

func GetLiquidType() { return "Acid"; }

func Disperse()
{
	DisperseMaterial(GetLiquidType(), GetLiquidAmount());
	_inherited(...);
}

local Name="$Name$";