/*-- Ice --*/

#strict 2


protected func Initialize()
{
	//SetGraphics(Format("%d.24", Random(2)));
}

protected func Check()
{
	if(GetTemperature() > 0) Melt();
	if(GetTemperature() <= 0 && GetMaterial()==Material("Water")) Freeze();
}

private func Melt()
{
	CastPXS("Water",2,0);
	DoCon(-2);
}

private func Freeze()
{
	DoCon(2);
	var i=2;
	while(i>0) ExtractLiquid() && i=--i;
}

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}
