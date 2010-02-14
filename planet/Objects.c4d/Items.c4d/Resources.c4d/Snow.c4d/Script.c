/*-- Snowball --*/

#strict 2


protected func Initialize()
{
	SetGraphics(Format("%d.8", Random(2)));
}

protected func Check()
{
	if(GetTemperature() > 0) Melt();
}

private func Melt()
{
	CastPXS("Water",2,0);
	DoCon(-2);
}

private func Hit()
{
	CastPXS("Snow", 400,18);
	RemoveObject();
}

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}
