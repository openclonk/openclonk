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
		Freeze(1);
	if (GetTemperature() > 0)
		Melt(1);
}

private func Melt(int strength)
{
	CastPXS("Water", 2 * strength, 0);
	DoCon(-strength);
}

private func Freeze(int strength)
{
	ExtractMaterialAmount(0, 0, Material("Water"), 2 * strength);
	DoCon(strength);
}

public func OnInIncendiaryMaterial()
{
	// Melt a bit faster in incendiary materials.
	return Melt(8);
}

func CanConvertToLiquidType() { return "Water"; }
func GetLiquidAmount() { return GetCon()*2; }

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local MaterialIncinerate = true;
local Plane = 450;