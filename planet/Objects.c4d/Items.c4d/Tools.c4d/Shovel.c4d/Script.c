/*-- Shovel --*/

#strict 2

private func Hit()
{
  Sound("WoodHit");
  return 1;
}

public func ControlUse(object pByClonk, int iX, int iY)
{
  pByClonk->SetCommand("Dig", 0, iX, iY);
  Sound("KnightConfirm*");
  return 1;
}

public func IsTool() { return 1; }

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}