/*-- Ice --*/

protected func Hit()
{
	Sound("CrystalHit?");
}

protected func Construction()
{
	var graphic = Random(5);
	if(graphic)
		SetGraphics(Format("%d",graphic));
	AddTimer("Check", 30);
}

protected func Check()
{
	if(GetTemperature() > 0) Melt();
	if(GetTemperature() <= 0 && GetMaterial()==Material("Water") && GetCon()<100) Freeze();
}

private func Melt()
{
	CastPXS("Water",2,0);
	DoCon(-1);
}

private func Freeze()
{
	DoCon(1);
	ExtractMaterialAmount(0,0,Material("Water"),2);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
