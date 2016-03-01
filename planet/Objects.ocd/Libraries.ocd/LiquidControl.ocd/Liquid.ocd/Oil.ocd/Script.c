#include Library_Liquid

func GetLiquidType() { return "Oil"; }

func Disperse()
{
	DisperseMaterial(IsLiquid(), GetLiquidAmount());
	_inherited(...);
}

func IsFuel(){ return true;}
func GetFuelAmount(int requested_amount)
{
	return Min(requested_amount, GetLiquidAmount());
}

func OnFuelRemoved(int amount)
{
	DoStackCount(-amount);
	return true;
}

local Name="$Name$";