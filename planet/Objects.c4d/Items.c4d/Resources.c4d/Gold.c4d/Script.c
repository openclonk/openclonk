/*--- Gold ---*/

#strict 2

protected func Hit()
{
   Sound("RockHit*");
  return 1;
}

  func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
func Definition(def) {
  SetProperty("Collectible", 1, def);
}
