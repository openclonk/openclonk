/*-- Ice --*/

protected func Construction()
{
	var graphic = Random(5);
	if(graphic)
		SetGraphics(Format("%d",graphic));
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
	var i=2;
	while(i>0)
	{
		ExtractLiquid();
		i=--i;
	}
}

local Collectible = 1;
local Name = "$Name$";
