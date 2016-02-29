/*-- Ice --*/

protected func Hit()
{
	Sound("Hits::IceHit?");
}

protected func Construction()
{
	var graphic = Random(5);
	if (graphic)
		SetGraphics(Format("%d", graphic));
	AddTimer("Check", 30);
}

protected func Check()
{
	if (GetTemperature() <= 0 && GetMaterial() == Material("Water") && GetCon() < 100)
		Freeze();
	// Don't do anything af
	if (GetTemperature() > 0)
		Melt();
}

private func Melt()
{
	CastPXS("Water", 2, 0);
	DoCon(-1);
}

private func Freeze()
{
	ExtractMaterialAmount(0, 0, Material("Water"), 2);
	DoCon(1);
}

func GetLiquidType() { return "Water"; }
func GetLiquidAmount() { return GetCon()*2; }

// Insertion of liquid into ice is not possible
func PutLiquid(string liquid_name, int amount, object source)
{
	return 0;
}

// Removes liquid for production, for example.
func RemoveLiquid(string liquid_name, int amount, object destination)
{
	if (amount < 0)
	{
		FatalError(Format("You can remove positive amounts of liquid only, got %d", amount));
	}

	// default parameters if nothing is provided: the current material and level
	liquid_name = liquid_name ?? GetLiquidType();
	amount = amount ?? GetLiquidAmount();

	//Wrong material?
	if (!WildcardMatch(GetLiquidType(), liquid_name))
		return [GetLiquidType(), 0];

	amount = Min(amount, GetLiquidAmount());
	DoCon(-(amount + 1)/2);
	return [liquid_name, amount];
}


local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Plane = 450;