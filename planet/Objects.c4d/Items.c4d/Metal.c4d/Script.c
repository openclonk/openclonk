/*--- Metal ---*/

#strict 2

protected func Hit()
{
   Sound("MetalHit*");
  return 1;
}

  func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
