/*--- Earth ---*/

protected func Construction()
{
	var graphic = Random(5);
	if(graphic)
		SetGraphics(Format("%d",graphic));
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