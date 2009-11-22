/*-- Der Clonk --*/

#strict 2

// Context menu
#include L_CM
// Auto production
#include L_AP

local pInventory;

/* Initialisierung */

protected func Initialize()
{
  // Create Inventoryobject
  pInventory = CreateObject(INVT, 0, 0, GetOwner());
  CreateContents(EMPT);
  CreateContents(SHVL);
  // Clonks mit Magiephysikal aus fehlerhaften Szenarien korrigieren
  if (GetID () == CLNK)
    if (GetPhysical ("Magic", 1))
      SetPhysical ("Magic", 0, 1);
  SetAction("Walk");
  SetDir(Random(2));
  // Broadcast für Spielregeln
  GameCallEx("OnClonkCreation", this);
  return 1;
}

protected func Swimming()
{
  if(GBackSemiSolid(0, -4))
    SetAction("Swim2");
}

protected func Swimming2()
{
  if(!GBackSemiSolid(0, -4))
    SetAction("Swim");
}

/* Bei Hinzuf�gen zu der Crew eines Spielers */

protected func Recruitment(int iPlr) {
  // Broadcast f�r Crew
  GameCallEx("OnClonkRecruitment", this, iPlr);
}

/* Steuerung */

protected func ControlLeft()
{
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlLeft")) return 1;
  // Steuerung an Pferd weiterleiten
  if (IsRiding()) return GetActionTarget()->~ControlLeft(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlLeftDouble()
  {
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlLeftDouble")) return true;
  }

protected func ControlRightDouble()
  {
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlRightDouble")) return true;
  }

protected func ControlLeftReleased()
{
  // Steuerung an Pferd weiterleiten
  if (IsRiding()) return GetActionTarget()->~ControlLeftReleased(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlRight()
{
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlRight")) return 1;
  // Steuerung an Pferd weiterleiten
  if (IsRiding())  return GetActionTarget()->~ControlRight(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlRightReleased()
{
  // Steuerung an Pferd weiterleiten
  if (IsRiding()) return GetActionTarget()->~ControlRightReleased(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlUp()
{
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlUp")) return 1;
  // Steuerung an Pferd weiterleiten
  if (IsRiding())  return GetActionTarget()->~ControlUp(this);
  // Bei JnR Delfinsprung
  if(GetPlrCoreJumpAndRunControl(GetController()))
    DolphinJump();
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlUpReleased()
{
  // Steuerung an Pferd weiterleiten
  if (IsRiding()) return GetActionTarget()->~ControlUpReleased(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlUpDouble() 
{
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlUpDouble")) return 1;
  DolphinJump();
}

private func DolphinJump()
{
  // nur wenn an Meeresoberfl�che
  if(!InLiquid()) return 0;
  if(GBackSemiSolid(0,-1)) return 0;
  // Nicht wenn deaktiviert (z.B. Ohnmacht)
  if (GetActMapVal("ObjectDisabled", GetAction())) return false;
  // herausspringen
  SetPosition(GetX(),GetY()-1);
  SetAction("Jump");
  SetSpeed(GetXDir(),-BoundBy(GetPhysical("Swim")/2500,24,38));
  var iX=GetX(),iY=GetY(),iXDir=GetXDir(),iYDir=GetYDir();
  // Wenn Sprung im Wasser endet und das Wasser tief genug ist, Kopfsprung machen
  if(SimFlight(iX,iY,iXDir,iYDir,25,50))
    if(GBackLiquid(iX-GetX(),iY-GetY()) && GBackLiquid(iX-GetX(),iY+9-GetY()))
      SetAction("Dive");
}

protected func ControlDown()
{
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlDown")) return 1;
  // Steuerung an Pferd weiterleiten
  if (IsRiding())  return GetActionTarget()->~ControlDown(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlDownReleased()
{
  // Steuerung an Pferd weiterleiten
  if (IsRiding()) return GetActionTarget()->~ControlDownReleased(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlDownSingle()
{
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlDownSingle")) return 1;
  // Steuerung an Pferd weiterleiten
  if (IsRiding())  return GetActionTarget()->~ControlDownSingle(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlDownDouble()
{
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlDownDouble")) return 1;
  // Steuerung an Pferd weiterleiten
  if (IsRiding())  return GetActionTarget()->~ControlDownDouble(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlDig()
{
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlDig")) return 1;
  // Steuerung an Pferd weiterleiten
  if (IsRiding())  return GetActionTarget()->~ControlDig(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlDigReleased()
{
  // Steuerung an Pferd weiterleiten
  if (IsRiding()) return GetActionTarget()->~ControlDigReleased(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlDigSingle()
{
  // Steuerung an Pferd weiterleiten
  if (IsRiding())  return GetActionTarget()->~ControlDigSingle(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlDigDouble()
{
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlDigDouble")) return 1;
  // Steuerung an Pferd weiterleiten
  if (IsRiding())  return GetActionTarget()->~ControlDigDouble(this);
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlThrow()
{
  // Bei vorherigem Doppel-Stop nur Ablegen
  if (GetPlrDownDouble(GetOwner())) return 0;
  // Steuerung an Effekt weitergeben 
  if (Control2Effect("ControlThrow")) return 1;
  // Reiten und Werfen
  if (IsRiding())
    if (Contents(0))
    {
      SetAction("RideThrow");
      return 1;
    }
  // Keine �berladene Steuerung
  return 0;
}

protected func ControlUpdate(object self, int comdir, bool dig, bool throw)
{
  // Steuerung an Pferd weiterleiten
  if(IsRiding()) return GetActionTarget()->~ControlUpdate(self, comdir, dig, throw);
  // Keine �berladene Steuerung
  return 0;
}

/* Verwandlung */

private func RedefinePhysical(szPhys, idTo)
{
  // Physical-Werte ermitteln
  var physDefFrom = GetID()->GetPhysical(szPhys),
      physDefTo   = idTo->GetPhysical(szPhys),
      physCurr    = GetPhysical(szPhys);
  // Neuen Wert berechnen
  var physNew; if (physDefTo) physNew=BoundBy(physDefTo-physDefFrom+physCurr, 0, 100000);
  // Neuen Wert f�r den Reset immer tempor�r setzen, selbst wenn keine �nderung besteht, damit der Reset richtig funktioniert
  SetPhysical(szPhys, physNew, PHYS_StackTemporary);
  // Fertig
  return 1;
}

protected func FxIntRedefineStart(object trg, int num, int tmp, id idTo)
  {
  // Ziel-ID in Effektvariable
  if (tmp)
    idTo = EffectVar(0, trg, num);
  else
    {
    EffectVar(0, trg, num) = idTo;
    EffectVar(1, trg, num) = GetID();
    }
  // Physicals anpassen
  RedefinePhysical("Energy", idTo);
  RedefinePhysical("Breath", idTo);
  RedefinePhysical("Walk", idTo);
  RedefinePhysical("Jump", idTo);
  RedefinePhysical("Scale", idTo);
  RedefinePhysical("Hangle", idTo);
  RedefinePhysical("Dig", idTo);
  RedefinePhysical("Swim", idTo);
  RedefinePhysical("Throw", idTo);
  RedefinePhysical("Push", idTo);
  RedefinePhysical("Fight", idTo);
  RedefinePhysical("Magic", idTo);
  RedefinePhysical("Float", idTo);
  /*if (GetRank()<4) RedefinePhysical("CanScale", idTo);
  if (GetRank()<6) RedefinePhysical("CanHangle", idTo);*/ // z.Z. k�nnen es alle
  RedefinePhysical("CanDig", idTo);
  RedefinePhysical("CanConstruct", idTo);
  RedefinePhysical("CanChop", idTo);
  RedefinePhysical("CanSwimDig", idTo);
  RedefinePhysical("CorrosionResist", idTo);
  RedefinePhysical("BreatheWater", idTo);
  // Damit Aufwertungen zu nicht-Magiern keine Zauberenergie �brig lassen
  if (GetPhysical("Magic")/1000 < GetMagicEnergy()) DoMagicEnergy(GetPhysical("Magic")/1000-GetMagicEnergy());
  // Echtes Redefine nur bei echten Aufrufen (hat zu viele Nebenwirkungen)
  if (tmp) return FX_OK;
  Redefine(idTo);
  // Fertig
  return FX_OK;
  }
  
protected func FxIntRedefineStop(object trg, int num, int iReason, bool tmp)
  {
  // Physicals wiederherstellen
  ResetPhysical("BreatheWater");
  ResetPhysical("CorrosionResist");
  ResetPhysical("CanSwimDig");
  ResetPhysical("CanChop");
  ResetPhysical("CanConstruct");
  ResetPhysical("CanDig");
  ResetPhysical("Float");
  ResetPhysical("Magic");
  ResetPhysical("Fight");
  ResetPhysical("Push");
  ResetPhysical("Throw");
  ResetPhysical("Swim");
  ResetPhysical("Dig");
  ResetPhysical("Hangle");
  ResetPhysical("Scale");
  ResetPhysical("Jump");
  ResetPhysical("Walk");
  ResetPhysical("Breath");
  ResetPhysical("Energy");
  // Keine R�ck�nderung bei tempor�ren Aufrufen oder beim Tod/L�schen
  if (tmp || iReason) return;
  // Damit Aufwertungen von nicht-Magiern keine Zauberenergie �brig lassen
  if (GetPhysical("Magic")/1000 < GetMagicEnergy()) DoMagicEnergy(GetPhysical("Magic")/1000-GetMagicEnergy());
  // OK; alte Definition wiederherstellen
  Redefine(EffectVar(1, trg, num));
  }

public func Redefine2(idTo)
{
  if (GetID() == idTo) return true;
  RemoveEffect("IntRedefine", this);
  if (GetID() == idTo) return true;
  return !!AddEffect("IntRedefine", this, 10, 0, this, 0, idTo);
}

public func Redefine(idTo)
{
  // Aktivit�tsdaten sichern
  var phs=GetPhase(),act=GetAction();
  // Umwandeln
  ChangeDef(idTo);
  // Aktivit�t wiederherstellen
  var chg=SetAction(act);
  if (!chg) SetAction("Walk");
  if (chg) SetPhase(phs);
  // Fertig
  return 1;
}

/* Essen */  

public func Feed(iLevel)
{
  DoEnergy(iLevel);
  Sound("ClonkMunch");
    return 1;
}

/* Aktionen */

private func Riding()
{
  // Richtung an die des Pferdes anpassen
  SetDir(GetActionTarget()->GetDir());
  // Pferd steht still: Clonk soll auch still sitzen
  if (GetActionTarget()->~IsStill())
  {
    if (GetAction() != "RideStill")
      SetAction("RideStill");
  }
  // Pferd steht nicht still: Clonk soll auch nicht still sitzen
  else
    if (GetAction() != "Ride")
      SetAction("Ride");
  return 1;
}

private func Throwing()
{
  // Erstes Inhaltsobjekt werfen
  var pObj = Contents(0);
  // Wurfparameter berechnen
  var iX, iY, iR, iXDir, iYDir, iRDir;
  iX = 0; if (!GetDir()) iX = -iX;
  iY = -10;
  iR = Random(360);
  iXDir = GetPhysical("Throw") / 25000; if(!GetDir()) iXDir = -iXDir;
  iYDir = -GetPhysical("Throw") / 25000;
  iRDir = Random(40) - 20;
  // Reitet? Eigengeschwindigkeit addieren
  if (GetActionTarget())
  {
    iXDir += GetActionTarget()->GetXDir() / 10;
    iYDir += GetActionTarget()->GetYDir() / 10;
  }
  // Werfen!
  pObj->Exit(iX, iY, iR, iXDir, iYDir, iRDir);  
  // Fertig
  return 1;  
}

private func Fighting()
{
  if (!Random(2)) SetAction("Punch");
  return 1;
}

private func Punching()
{
  if (!Random(3)) Sound("Kime*");
  if (!Random(5)) Sound("Punch*");
  if (!Random(2)) return 1;
  GetActionTarget()->Punch();
  return 1;
}
  
private func Chopping()
{
  if (!GetActTime()) return; // Erster Schlag kein Sound. Clonk holt noch aus.
  Sound("Chop*");
  CastParticles("Dust",Random(3)+1,6,-8+16*GetDir(),1,10,12);
  return 1;
}
  
private func Building()
{
  if (!Random(2)) Sound("Build*");
  return 1;
}

private func Processing()
{
  Sound("Build1");
  return 1;
}

private func Digging()
{
  Sound("Dig*");
  return 1;
}

protected func Scaling()
{
  var szDesiredAction;
  if (GetYDir()>0) szDesiredAction = "ScaleDown"; else szDesiredAction = "Scale";
  if (GetAction() != szDesiredAction) SetAction(szDesiredAction);
  return 1;   
}

/* Ereignisse */
  
protected func CatchBlow()
{
  if (GetAction() == "Dead") return 0;
  if (!Random(5)) Hurt();
  return 1;
}
  
protected func Hurt()
{
  Sound("Hurt*");
  return 1;
}
  
protected func Grab(object pTarget, bool fGrab)
{
  Sound("Grab");
  return 1;
}

protected func Get()
{
  Sound("Grab");
  return 1;
}

protected func Put()
{
  Sound("Grab");
  return 1;
}

protected func Death(int iKilledBy)
{
  // Info-Broadcasts f�r sterbende Clonks
  GameCallEx("OnClonkDeath", this, iKilledBy);
  
  // Der Broadcast k�nnte seltsame Dinge gemacht haben: Clonk ist noch tot?
  if (GetAlive()) return;
  
  // den Beutel fallenlassen
  if(GetAlchemBag()) GetAlchemBag()->~Loose();

  Sound("Die");
  DeathAnnounce();
  // Letztes Mannschaftsmitglied tot: Script benachrichtigen
  if (!GetCrew(GetOwner()))
    GameCallEx("RelaunchPlayer",GetOwner());
  return 1;
}

protected func Destruction()
{
  // Clonk war noch nicht tot: Jetzt ist er es
  if (GetAlive())
    GameCallEx("OnClonkDeath", this, GetKiller());
  // Dies ist das letztes Mannschaftsmitglied: Script benachrichtigen
  if (GetCrew(GetOwner()) == this)
    if (GetCrewCount(GetOwner()) == 1)
      //Nur wenn der Spieler noch lebt und nicht gerade eleminiert wird
      if (GetPlayerName(GetOwner()))
        {
        GameCallEx("RelaunchPlayer",GetOwner());
        }
  return 1;
}

protected func DeepBreath()
{
  Sound("Breath");
  return 1; 
}
  
protected func CheckStuck()
{                   
  // Verhindert Festh�ngen am Mittelvertex
  if(!GetXDir()) if(Abs(GetYDir()) < 5)
    if(GBackSolid(0, 3))
      SetPosition(GetX(), GetY() + 1);
}

/* Status */

public func IsClonk() { return 1; }

/* Hilfsfunktion */

public func ContainedCall(string strFunction, object pTarget)
{
  // Erst das betreffende Geb�ude betreten, dann die Zielfunktion aufrufen 
  SetCommand("Call", pTarget, this, 0, nil, strFunction);
  AddCommand(this, "Enter", pTarget);
}

/* Trinken */

public func Drink(object pDrink)
{
  // Trinkaktion setzen, wenn vorhanden
  if (GetActMapVal("Name", "Drink"))
    SetAction("Drink");
  // Vorsicht: erstmal nichts mit pDrink machen,
  // die Potions l�schen sich meist selber...
}

/* New collection behavior */
public func Collection2(object pObj)
{
  if(pObj->GetID() != EMPT)
    pInventory->AddItem(pObj);
  // HACK: only to hide the engine inventory
  pObj->SetClrModulation(RGBa(Random(255),Random(255),Random(255),255));
  UpdateInventorySelection();
}

public func Ejection(object pObj)
{
  if(pObj->GetID() != EMPT)
  {
    pInventory->RemItem(pObj);
    // Try to get next item
    var i;
    do
    {
      pInventory->SelectNext();
      i++;
    }
    while(!pInventory->GetSelectedObj() && i<4)
  }
  // HACK: only to hide the engine inventory
  pObj->SetClrModulation(RGB(255,255,255));
  UpdateInventorySelection();
}

/* Inventorychange */

protected func CrewSelection(bool fDeselect, bool fCursorOnly)
{
  pInventory->Show(GetCursor(GetOwner()) != this);
}

protected func ControlSpecial()
{
  pInventory->SelectNext();
  UpdateInventorySelection();
}

private func UpdateInventorySelection()
{
  var pObj = pInventory->GetSelectedObj();
  if(!pObj) pObj = FindContents(EMPT);
  var iSave;
  while(ScrollContents() != pObj && iSave < 10) iSave++;
//  if(iSave == 10) Log("ERROR: Inventory doesn't match");
}

/* Einsammeln */

public func RejectCollect(id idObject, object pObject)
{
  // Objekt kann gepackt werden
  // automatisches Packen aber nur wenn die Paktteile nicht extra gez�hlt werden
  if(!IsSpecialItem(pObject)) if(pObject->~JoinPack(this)) return 1;
    
  // Objektaufnahme mit Limit verhindern, wenn bereits genug getragen
  if(pObject->~CarryLimit() && ContentsCount(idObject) >= pObject->~CarryLimit() ) return 1;
    
  // Spezialitem?
  var i, iCount;
  if(i = IsSpecialItem(pObject))
  {
    // Noch genug Platz f�r das ganze Packet?
    if(GetSpecialCount(GetMaxSpecialCount(i-1))+Max(pObject->~PackCount(),1)<=GetMaxSpecialCount(i-1, 1)) return 0;
    iCount = GetMaxSpecialCount(i-1, 1)-GetSpecialCount(GetMaxSpecialCount(i-1));
    // Ansonten so viel wie geht rein
    if(pObject->~SplitPack(pObject->~PackCount()-iCount)) return 0;
    else return 1;
  }
  
  return !pInventory->FreeSpace(pObject);//GetNonSpecialCount()>=MaxContentsCount();
}

/* Itemlimit */
public func MaxContentsCount() { return 3; }

public func GetMaxSpecialCount(iIndex, fAmount)
{
  // Hier k�nnten Spezialbehandlungen von Itemgruppen definiert werden
  // wie z.B. zu dem Inventar noch 30 Pfeile aufnehmen (siehe auch Ritter)
  //  if(iIndex == 0) { if(fAmount) return(30); return("IsArrow"); }
}

/* Liefert die Gesamtzahl eines Objekt(paket)typs */ 
private func GetObjectCount(idObj) 
  { 
  var idUnpackedObj; 
  if (idUnpackedObj = idObj->~UnpackTo()) 
    // Auch verschachtelte Pakete mitz�hlen 
    return GetObjectCount(idUnpackedObj) * (idObj->~PackCount()||1);
  // Ansonsten ist es nur ein Objekt 
  return 1; 
  }

/* Spezialgegenst�nde im Inventar z�hlen */ 
private func GetSpecialCount(szTest) 
  { 
  var iCnt, pObj; 
  // Einzelne Pfeile... 
  for(var i = 0; pObj = Contents(i); i++) 
    if(pObj->Call(szTest)) 
      iCnt++; 
  // Pakete... 
  for(var i = 0; pObj = Contents(i); i++) 
    if(pObj->~UnpackTo())
      if(DefinitionCall(pObj->~UnpackTo(), szTest))
        iCnt += GetObjectCount(pObj); 
  // Wert zur�ckgeben 
  return iCnt; 
  }
  
/* Testen eines Objektes */
private func IsSpecialItem(pObj)
{
  // Spezialitem?
  var j=-1;
  while(GetMaxSpecialCount(++j, 1))
    if(pObj->GetMaxSpecialCount(j))
      return j+1;
  // Spezialitempacket?
  if(pObj->~UnpackTo())
  {
    j=-1;
    while(GetMaxSpecialCount(++j, 1))
      if(DefinitionCall(pObj->~UnpackTo(), GetMaxSpecialCount(j)))
        return j+1;
  }
}

/* Anzahl an normalen Objekten */
private func GetNonSpecialCount() 
  { 
  var iCnt, pObj; 
  // Inventar einzeln auf nicht-Spezial �berpr�fen 
  for(var i = 0; pObj = Contents(i); i++)
    // Spezialitems nicht z�hlen
    if(!IsSpecialItem(pObj))
        iCnt++;
   
  // Wert zur�ckgeben 
  return iCnt; 
  } 

/* Effektsteuerung */

private func Control2Effect(string szControl)
  {
  // Von Effektzahl abw�rts z�hlen
  var i = GetEffectCount(nil, this), iEffect;
  var res;
  while (i--)
    {
    // Effekte mit Control im Namen benachrichtigen	  
    iEffect = GetEffect("*Control*", this, i);
    //  Message("%s", this, GetEffect(0, this, iEffect, 1));
    if ( GetEffect(nil, this, iEffect, 1) )
      res += EffectCall(this, iEffect, szControl);
    }
  return res;
  }


/* Pfeile */

// Pfeilpaket aufteilen 
public func SplitPack2Components(pPack) 
  { 
  // Aufteilen 
  if(!pPack->~Unpack(this) ) Split2Components(pPack); 
  // Fertig, Erfolg 
  return 1; 
  }
 
/* Pfeil aus dem Inventar nehmen */ 
public func GetArrow() 
  { 
  // Einzelne Pfeile suchen 
  var pObj, pArrow; 
  for(var i = 0; pObj = Contents(i); i++) 
    if(pObj->~IsArrow()) 
      return pObj; 
  // Bei Bedarf Pakete aufteilen 
  for(var i = 0; pObj = Contents(i); i++) 
    if(pObj->~IsArrowPack())
    {
      // Pfeil aus Paket verwenden
      if(pArrow = pObj->~GetItem()) return pArrow;
      // oder bei alten Pfeilen Paket aufteilen
      if (SplitPack2Components(pObj))
        return FindSingleArrow();
    }
  // Keine Pfeile gefunden 
  return 0; 
  } 

public func FindSingleArrow() 
  { 
  // Einzelne Pfeile suchen 
  var pObj; 
  for(var i = 0; pObj = Contents(i); i++) 
    if(pObj->~IsArrow()) 
      return pObj; 
  // Keiner gefunden 
  return 0; 
  } 
  
public func GetComboArrow() 
  { 
  // Pfeile als Komboobjekt: Nur wenn das erste Inventarobjekt ein Pfeil ist
  var pObj = Contents(0), pArrow;
  if (!pObj) return;
  if(pObj->~IsArrow()) return pObj; 
  // Bei Bedarf Pakete aufteilen 
  if(pObj->~IsArrowPack())
    {
    // Pfeil aus Paket verwenden
    if(pArrow = pObj->~GetItem()) return pArrow;
    // oder bei alten Pfeilen Paket aufteilen
    if (SplitPack2Components(pObj))
      return FindSingleArrow();
    }
  // Keine Pfeile gefunden 
  return 0;
  } 

/* Pfeile im Inventar z�hlen */ 
private func GetArrowCount() 
  {
   return GetSpecialCount("IsArrow");
  }


/* Zauberei - ben�tigt, wenn der Clonk Zielzauber z.B. aus dem Zauberturm zaubert */

public func SpellFailed(id idSpell, object pByCaller)
{
  // Umleiten an eigentliche Zauberquelle? (Buch, Zauberturm, etc.)
  var pSpellOrigin = pAimedSpellOrigin;
  pAimedSpellOrigin = 0;
  if (pSpellOrigin && pSpellOrigin != this)
    // Auch bei nicht erfolgreicher Umleitung abbrechen: Das zaubernde Objekt hat im Normalfall die Zutaten/Zauberenergie f�r den
    // Zauber bereit gestellt, und diese sollten nicht an den Clonk zur�ck gegeben werden
    return (pSpellOrigin->~SpellFailed(idSpell, this));
  // Magieenergie zur�ckgeben
  DoMagicEnergy(idSpell->GetDefValue(), true);
  // Alchemische Zutaten zur�ckgeben
  if(ObjectCount(ALCO)) IncreaseAlchem(idSpell);
}

public func SpellSucceeded(id idSpell, object pByCaller)
{
  // Umleiten an eigentliche Zauberquelle? (Buch, Zauberturm, etc.)
  var pSpellOrigin = pAimedSpellOrigin;
  pAimedSpellOrigin = 0;
  if (pSpellOrigin && pSpellOrigin != this)
    // Auch bei nicht erfolgreicher Umleitung abbrechen: Das zaubernde Objekt hat im Normalfall das Magietraining schon erledigt
    return (pSpellOrigin->~SpellSucceeded(idSpell, this));
  // Globaler Aufruf f�r Zauber
  OnClonkSucceededSpell(idSpell);
}

// Der Clonk kann von sich aus nicht zaubern und hat keine Aktivit�ten daf�r
private func SetMagicAction(id idForSpell) {}
private func SetCastAction() {}
private func EndMagicAction() {}

func Definition(def) {
  SetProperty("ActMap", {
Walk = {
Prototype = Action,
Name = "Walk",
Procedure = DFA_WALK,
Directions = 2,
FlipDir = 1,
Length = 16,
Delay = 15,
X = 0,
Y = 0,
Wdt = 16,
Hgt = 20,
NextAction = "Walk",
InLiquidAction = "Swim",
},
StillTrans1 = {
Prototype = Action,
Name = "StillTrans1",
Procedure = DFA_THROW,
Directions = 2,
FlipDir = 1,
Length = 4,
Delay = 2,
X = 0,
Y = 280,
Wdt = 16,
Hgt = 20,
NextAction = "Still",
InLiquidAction = "Swim",
},
Still = {
Prototype = Action,
Name = "Still",
Procedure = DFA_THROW,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 10,
X = 64,
Y = 280,
Wdt = 16,
Hgt = 20,
NextAction = "Still",
InLiquidAction = "Swim",
},
StillTrans2 = {
Prototype = Action,
Name = "StillTrans2",
Procedure = DFA_THROW,
Directions = 2,
Reverse = 1,
FlipDir = 1,
Length = 4,
Delay = 2,
X = 192,
Y = 280,
Wdt = 16,
Hgt = 20,
NextAction = "Still",
InLiquidAction = "Swim",
},
Scale = {
Prototype = Action,
Name = "Scale",
Procedure = DFA_SCALE,
Directions = 2,
FlipDir = 1,
Length = 16,
Delay = 15,
X = 0,
Y = 20,
Wdt = 16,
Hgt = 20,
OffX = 2,
OffY = 0,
NextAction = "Scale",
StartCall = "Scaling",
},
ScaleDown = {
Prototype = Action,
Name = "ScaleDown",
Procedure = DFA_SCALE,
Directions = 2,
FlipDir = 1,
Length = 16,
Delay = 15,
X = 0,
Y = 20,
Wdt = 16,
Hgt = 20,
OffX = 2,
OffY = 0,
Reverse = 1,
NextAction = "ScaleDown",
StartCall = "Scaling",
},
Tumble = {
Prototype = Action,
Name = "Tumble",
Procedure = DFA_FLIGHT,
Directions = 2,
FlipDir = 1,
Length = 16,
Delay = 1,
X = 0,
Y = 40,
Wdt = 16,
Hgt = 20,
NextAction = "Tumble",
ObjectDisabled = 1,
InLiquidAction = "Swim",
EndCall = "CheckStuck",
},
Dig = {
Prototype = Action,
Name = "Dig",
Procedure = DFA_DIG,
Directions = 2,
FlipDir = 1,
Length = 16,
Delay = 15,
X = 0,
Y = 60,
Wdt = 16,
Hgt = 20,
NextAction = "Dig",
StartCall = "Digging",
DigFree = 11,
InLiquidAction = "Swim",
},
Bridge = {
Prototype = Action,
Name = "Bridge",
Procedure = DFA_BRIDGE,
Directions = 2,
FlipDir = 1,
Length = 16,
Delay = 1,
X = 0,
Y = 60,
Wdt = 16,
Hgt = 20,
NextAction = "Bridge",
StartCall = "Digging",
InLiquidAction = "Swim",
},
Swim = {
Prototype = Action,
Name = "Swim",
Procedure = DFA_SWIM,
Directions = 2,
FlipDir = 1,
Length = 12,
Delay = 15,
X = 0,
Y = 80,
Wdt = 20,
Hgt = 20,
OffX = 0,
OffY = 1,
NextAction = "Swim",
StartCall = "Swimming",
},
Swim2 = {
Prototype = Action,
Name = "Swim2",
Procedure = DFA_SWIM,
Directions = 2,
FlipDir = 1,
Length = 12,
Delay = 15,
X = 0,
Y = 300,
Wdt = 20,
Hgt = 20,
OffX = 0,
OffY = 1,
NextAction = "Swim2",
StartCall = "Swimming2",
},
Hangle = {
Prototype = Action,
Name = "Hangle",
Procedure = DFA_HANGLE,
Directions = 2,
FlipDir = 1,
Length = 11,
Delay = 16,
X = 0,
Y = 100,
Wdt = 16,
Hgt = 20,
OffX = 0,
OffY = 3,
NextAction = "Hangle",
InLiquidAction = "Swim",
},
Jump = {
Prototype = Action,
Name = "Jump",
Procedure = DFA_FLIGHT,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 3,
X = 0,
Y = 120,
Wdt = 16,
Hgt = 20,
NextAction = "Hold",
InLiquidAction = "Swim",
PhaseCall = "CheckStuck",
},
KneelDown = {
Prototype = Action,
Name = "KneelDown",
Procedure = DFA_KNEEL,
Directions = 2,
FlipDir = 1,
Length = 4,
Delay = 1,
X = 0,
Y = 140,
Wdt = 16,
Hgt = 20,
NextAction = "KneelUp",
},
KneelUp = {
Prototype = Action,
Name = "KneelUp",
Procedure = DFA_KNEEL,
Directions = 2,
FlipDir = 1,
Length = 4,
Delay = 1,
X = 64,
Y = 140,
Wdt = 16,
Hgt = 20,
NextAction = "Walk",
},
Dive = {
Prototype = Action,
Name = "Dive",
Procedure = DFA_FLIGHT,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 4,
X = 0,
Y = 160,
Wdt = 16,
Hgt = 20,
NextAction = "Hold",
ObjectDisabled = 1,
InLiquidAction = "Swim",
PhaseCall = "CheckStuck",
},
FlatUp = {
Prototype = Action,
Name = "FlatUp",
Procedure = DFA_KNEEL,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 1,
X = 0,
Y = 180,
Wdt = 16,
Hgt = 20,
NextAction = "KneelUp",
ObjectDisabled = 1,
},
Throw = {
Prototype = Action,
Name = "Throw",
Procedure = DFA_THROW,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 1,
X = 0,
Y = 200,
Wdt = 16,
Hgt = 20,
NextAction = "Walk",
InLiquidAction = "Swim",
},
Punch = {
Prototype = Action,
Name = "Punch",
Procedure = DFA_FIGHT,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 2,
X = 0,
Y = 220,
Wdt = 16,
Hgt = 20,
NextAction = "Fight",
EndCall = "Punching",
ObjectDisabled = 1,
},
Dead = {
Prototype = Action,
Name = "Dead",
Directions = 2,
FlipDir = 1,
X = 0,
Y = 240,
Wdt = 16,
Hgt = 20,
Length = 6,
Delay = 3,
NextAction = "Hold",
NoOtherAction = 1,
ObjectDisabled = 1,
},
Ride = {
Prototype = Action,
Name = "Ride",
Procedure = DFA_ATTACH,
Directions = 2,
FlipDir = 1,
Length = 4,
Delay = 3,
X = 128,
Y = 120,
Wdt = 16,
Hgt = 20,
NextAction = "Ride",
StartCall = "Riding",
InLiquidAction = "Swim",
},
RideStill = {
Prototype = Action,
Name = "RideStill",
Procedure = DFA_ATTACH,
Directions = 2,
FlipDir = 1,
Length = 1,
Delay = 10,
X = 128,
Y = 120,
Wdt = 16,
Hgt = 20,
NextAction = "RideStill",
StartCall = "Riding",
InLiquidAction = "Swim",
},
Push = {
Prototype = Action,
Name = "Push",
Procedure = DFA_PUSH,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 15,
X = 128,
Y = 140,
Wdt = 16,
Hgt = 20,
NextAction = "Push",
InLiquidAction = "Swim",
},
Chop = {
Prototype = Action,
Name = "Chop",
Procedure = DFA_CHOP,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 3,
X = 128,
Y = 160,
Wdt = 16,
Hgt = 20,
NextAction = "Chop",
StartCall = "Chopping",
InLiquidAction = "Swim",
},
Fight = {
Prototype = Action,
Name = "Fight",
Procedure = DFA_FIGHT,
Directions = 2,
FlipDir = 1,
Length = 7,
Delay = 4,
X = 128,
Y = 180,
Wdt = 16,
Hgt = 20,
NextAction = "Fight",
StartCall = "Fighting",
ObjectDisabled = 1,
},
GetPunched = {
Prototype = Action,
Name = "GetPunched",
Procedure = DFA_FIGHT,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 3,
X = 128,
Y = 200,
Wdt = 16,
Hgt = 20,
NextAction = "Fight",
ObjectDisabled = 1,
},
Build = {
Prototype = Action,
Name = "Build",
Procedure = DFA_BUILD,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 2,
X = 128,
Y = 220,
Wdt = 16,
Hgt = 20,
NextAction = "Build",
StartCall = "Building",
InLiquidAction = "Swim",
},
RideThrow = {
Prototype = Action,
Name = "RideThrow",
Procedure = DFA_ATTACH,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 1,
X = 128,
Y = 240,
Wdt = 16,
Hgt = 20,
NextAction = "Ride",
StartCall = "Throwing",
InLiquidAction = "Swim",
},
Process = {
Prototype = Action,
Name = "Process",
Procedure = DFA_THROW,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 3,
X = 0,
Y = 260,
Wdt = 16,
Hgt = 20,
NextAction = "Process",
EndCall = "Processing",
},
Drink = {
Prototype = Action,
Name = "Drink",
Procedure = DFA_THROW,
Directions = 2,
FlipDir = 1,
Length = 8,
Delay = 3,
X = 128,
Y = 260,
Wdt = 16,
Hgt = 20,
NextAction = "Walk",
},  }, def);
  SetProperty("Name", "Clonk", def);
}
