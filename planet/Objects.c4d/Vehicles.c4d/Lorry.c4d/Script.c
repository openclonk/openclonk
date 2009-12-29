/*-- Lore --*/

#strict 2

/* Status */

public func IsLorry() { return 1; }

public func IsToolProduct() { return 1; }

/* Steuerung */

protected func ContactLeft()
{
  if(Stuck() && !Random(5)) SetRDir(RandomX(-7, +7));
}

protected func ContactRight()
{
  if(Stuck() && !Random(5)) SetRDir(RandomX(-7, +7));
}

private func ControlElevator(string szCommand, object pObject)
{
  // Objekte an dieser Position überprüfen
  for(var pElev in FindObjects(Find_AtPoint(1,1), Find_OCF(OCF_Grab), Find_NoContainer()))
    // Im Fahrstuhlkorb
    if(pElev->~IsElevator())
      // Steuerung an Fahrstuhl weiterleiten
      return pElev->Call(szCommand,pObject);
  return 0;
}

private func ControlElevatorMovement(string szCommand, object pObject)
{
  // Objekte an dieser Position überprüfen
  for(var pElev in FindObjects(Find_AtPoint(1,1), Find_OCF(OCF_Grab), Find_NoContainer()))
    // Fahrstuhlkorb gefunden
    if(pElev->~IsElevator())
      // Lore ist angehalten
      if (GetComDir() == COMD_Stop)
        // Steuerung an Fahrstuhl weiterleiten
        return pElev->Call(szCommand,pObject);
  return 0;
}

private func ControlElevatorStop(string szCommand, object pObject)
{
  // Objekte an dieser Position überprüfen
  for(var pElev in FindObjects(Find_AtPoint(1,1), Find_OCF(OCF_Grab), Find_NoContainer()))
    // Fahrstuhlkorb gefunden
    if(pElev->~IsElevator())
      // Fahrstuhlkorb bewegt sich
      if (pElev->GetComDir() != COMD_Stop)
      {
        // Fahrstuhlkorb anhalten
        pElev->Call(szCommand,pObject); 
        // Noch nicht aussteigen
        pObject->SetComDir(COMD_Stop);
        SetComDir(COMD_Stop);
        // Bewegunsbefehl abbrechen
        return 1;
      }
  return 0;
}

/* Füllmengenkontrolle */

private func MaxContents() { return 50; }

protected func RejectCollect(id idObj,object pObj)
{
  if(ContentsCount() < MaxContents()) { Sound("Clonk"); return 0; }
  if(pObj->Contained()) return Message("$TxtLorryisfull$", this);
  if(Abs(GetXDir(pObj))>6) SetYDir(-5,pObj);
  Sound("WoodHit*");
  return 1;
}


/* Automatisches Ausleeren in Gebäuden */

protected func Entrance(object pNewContainer)
  {
  // Nur in Gebäuden (auch Burgteile)
  if (pNewContainer->GetCategory() & (C4D_StaticBack | C4D_Structure))
    // Nicht, wenn es das Gebäude verbietet
    if (!pNewContainer->~NoLorryEjection(this) && !pNewContainer->~IsStaircase())
      {
      // Lore entleeren
      pNewContainer->GrabContents(this);
      }
  }

/* Einlade-Helfer */

protected func ContextLoadUp(object pClonk)
{
  [$TxtLoadUp$|Image=LRY1]
  // Alte Kommandos des Clonks löschen
  pClonk->SetCommand("None");
  // Lore ist bereits voll
  if (ContentsCount() >= MaxContents())
    return Message("$TxtLorryisfull$", this);
  // Maximale noch mögliche Zuladung bestimmen
  var iMaxLoad = MaxContents() - ContentsCount();
  // Frei liegende Gegenstände in der Umgebung automatisch einladen
  var iRange = 60;
  var pObj, iCount;
  for(pObj in FindObjects(Find_InRect(-iRange, -iRange/2, iRange*2, iRange*2), Find_OCF(OCF_Collectible), Find_NoContainer()))
    if (pObj->GetOCF() & OCF_Available)
    {
      // Maximale Zuladung berücksichtigen (auch die noch vom Clonk kommt)
      if (++iCount > iMaxLoad - pClonk->ContentsCount()) break;
      // Einladen
      pClonk->AddCommand("Put", this, 0,0, pObj);
    }
  // Der Clonk soll seine getragenen Objekte auch einladen (ansonsten legt er sie nur irgendwo ab)
  for (var i = 0; i < Min(pClonk->ContentsCount(), iMaxLoad); i++)
    pClonk->AddCommand("Put", this, 0,0, pClonk->Contents(i));
}

func Definition(def) {
  SetProperty("ActMap", {
Drive = {
Prototype = Action,
Name = "Drive",
Procedure = DFA_NONE,
Directions = 2,
FlipDir = 1,
Length = 20,
Delay = 2,
X = 0,
Y = 0,
Wdt = 22,
Hgt = 16,
NextAction = "Drive",
//Animation = "Drive",
},  }, def);
  SetProperty("Name", "$Name$", def);
}
