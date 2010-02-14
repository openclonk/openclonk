/*-- Ore --*/

#strict 2

protected func Initialize()
{
	SetGraphics(Format("%d.8",Random(2)));
}

protected func Hit()
{
	Sound("RockHit");
}

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}
