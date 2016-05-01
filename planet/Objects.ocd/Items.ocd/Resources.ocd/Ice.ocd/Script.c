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

func CanConvertToLiquidType() { return "Water"; }
func GetLiquidAmount() { return GetCon()*2; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Plane = 450;