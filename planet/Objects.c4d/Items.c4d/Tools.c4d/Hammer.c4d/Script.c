/*-- Hammer --*/

#strict 2

private func Hit()
{
  Sound("RockHit");
  return 1;
}

public func ControlUse(object pByClonk, int iX, int iY)
{ 
  // Stop clonk
  pByClonk->SetComDir(COMD_Stop);
  // Is the clonk able to build?
  if (pByClonk && !pByClonk->GetPhysical("CanConstruct", PHYS_Current))
    { 
     PlayerMessage(pByClonk->GetController(), "$TxtCantConstruct$", this, pByClonk->GetName()); 
     return false; 
    }
  // Create menu and fill it with the player's plans
  CreateMenu(CXCN, pByClonk, this(), 1, "$TxtNoconstructionplansa$");
  var idType; var i = 0;
  while (idType = GetPlrKnowledge(pByClonk->GetOwner(), 0, i++, C4D_Structure))
    AddMenuItem("$TxtConstructions$", "CreateConstructionSite", idType, pByClonk);
  return 1;
}

protected func CreateConstructionSite(idType)
{
  // Only when the clonk is standing and outdoors
  if (Contained()->GetAction() != "Walk") return 0;
  if (Contained()->Contained()) return 0;
  // Check if the building can be build here
  if (idType->~RejectConstruction(0, 10, Contained()) ) return 0;
  // Set owner for CreateConstruction
  SetOwner(Contained()->GetOwner());
  // Create construction site
  var pSite;
  if (!(pSite = CreateConstruction(idType, 0, 10, Contained()->GetOwner(), 1, 1,1))) return 0;
  // Message
  Message("$TxtConstructions$", Contained(), pSite->GetName());
  return 1;
}

public func IsTool() { return 1; }

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
}
