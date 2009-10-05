/*-- Axe --*/

#strict 2

private func Hit()
{
  Sound("RockHit");
  return 1;
}

public func ControlUse(object pByClonk, int iX, int iY)
{ 
  var pTree;
  if( pTree = FindObject(Find_AtPoint(iX, iY), Find_Func("IsTree") && Find_Func("IsStanding")) )
    {
      pByClonk->SetCommand("Chop", pTree);
      Sound("KnightConfirm*");
    }
  else{
    if(pByClonk->GetAction() == "Chop")
      pByClonk->SetAction("Idle");
      }
  return 1;
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}
