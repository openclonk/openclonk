/*--- Der Stein ---*/

#strict 2

protected func Hit()
{
  Sound("RockHit*");
  return 1;
}

func IsAlchemContainer() { return true; }
func AlchemProcessTime() { return 160; }
func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}
