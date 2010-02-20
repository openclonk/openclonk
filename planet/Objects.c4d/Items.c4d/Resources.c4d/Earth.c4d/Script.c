/*--- Earth ---*/

protected func Initialize()
{
	SetGraphics(Format("%d.8", Random(5)));
}

protected func Hit()
{
  CastPXS("Earth", 200, 18);
  Sound("EarthHit*");
  RemoveObject();
  return 1;
}

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}