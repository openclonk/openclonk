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
		Melt();
}

private func Melt()
{
	CastPXS("Water", 2, 0);
	DoCon(-2);
}

private func Hit()
{
	CastPXS("Snow", GetCon() * 4, 18);
	RemoveObject();
}

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
local Plane = 450;