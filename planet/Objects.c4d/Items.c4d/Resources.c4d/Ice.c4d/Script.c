/*-- Ice --*/

protected func Initialize()
{
	SetGraphics(Format("%d.8", Random(5)));
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

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}
