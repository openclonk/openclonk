#include Library_Liquid

func GetLiquidType() { return "Oil"; }

func Disperse()
{
	DisperseMaterial(GetLiquidType(), GetLiquidAmount());
	_inherited(...);
}

func IsFuel(){ return true;}
func GetFuelAmount(int requested_amount)
{
	requested_amount = requested_amount ?? GetLiquidAmount();
	return Min(requested_amount, GetLiquidAmount());
}

func OnFuelRemoved(int amount)
{
	DoStackCount(-amount);
	return true;
}

local Name="$Name$";