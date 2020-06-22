/*-- Snowball --*/

protected func Construction()
{
	var graphic = Random(5);
	if (graphic)
		SetGraphics(Format("%d", graphic));
	AddTimer("Check", 30);
}

protected func Check()
{
	if (GetTemperature() > 0)
		Melt(1);
}

private func Melt(int strength)
{
	CastPXS("Water", 2 * strength, 0);
	DoCon(-2 * strength);
}

public func OnInIncendiaryMaterial()
{
	// Melt a bit faster in incendiary materials.
	return Melt(8);
}

private func Hit()
{
	CastPXS("Snow", GetCon() * 4, 18);
	RemoveObject();
}

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local MaterialIncinerate = true;
local Plane = 450;