/*-- Sulphur --*/

#strict 2

protected func Initialize()
{
	SetGraphics(Format("%.8",Random(2)));
}

protected func Hit()
{
	Sound("CrystalHit*");
}

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}
