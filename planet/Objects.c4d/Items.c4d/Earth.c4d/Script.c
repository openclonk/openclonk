/*--- Earth ---*/

#strict 2

protected func Hit()
{
  CastPXS("Earth", 225, 18);
  Sound("EarthHit*");
  RemoveObject();
  return 1;
}

func IsAlchemContainer() { return true; }
func AlchemProcessTime() { return 120; }

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}