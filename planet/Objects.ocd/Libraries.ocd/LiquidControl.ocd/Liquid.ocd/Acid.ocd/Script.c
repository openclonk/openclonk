#include Library_Liquid

func GetLiquidType() { return "Acid"; }

func Disperse()
{
	DisperseMaterial(IsLiquid(), GetLiquidAmount());
	_inherited(...);
}

local Name="$Name$";