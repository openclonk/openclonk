/*-- Der Clonk --*/

#strict 2


/* Initialisierung */

protected func Initialize()
{
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

/* Bei Hinzufügen zu der Crew eines Spielers */

protected func Recruitment(int iPlr) {
  // Broadcast für Crew
  GameCallEx("OnClonkRecruitment", this, iPlr);
}

/* Kontext */

public func HasConstructMenu() { return HasKnowledge() && GetPhysical("CanConstruct"); }
public func HasKnowledge() { return GetPlrKnowledge(GetOwner(),0,0,C4D_Structure); }
public func HasBase()      { return FindBase(GetOwner()) && GetBase(Contained()) != GetOwner(); }
public func ReleaseAllowed() { return ObjectCount(REAC); }
public func AtConstructionSite() { return !Contained() && FindConstructionSite() && ObjectCount(CNMT); }
public func AtEnergySite() { return !Contained() && FindEnergySite(); }
public func AtTreeToChop() { return !Contained() && FindTree() && GetPhysical("CanChop"); }

public func FindConstructionSite()
{
  return FindObject2(Find_AtRect(-1,-16,2,32), Find_OCF(OCF_Construct), Find_Layer(GetObjectLayer()));
}

public func FindEnergySite()
{
  return FindObject2(Find_AtPoint(), Find_OCF(OCF_PowerConsumer), Find_NoContainer(), Find_Layer(GetObjectLayer()), Find_Func("NeedsEnergy"));
}

public func FindTree()
{
  return FindObject2(Find_AtPoint(), Find_OCF(OCF_Chop), Find_Layer(GetObjectLayer()));
}


/* Steuerung */

public func GetInteractionTarget()
{
  // Contained interaction target
  var container = Contained();
  if (container)
  {
    if (container->GetCategory() & (C4D_Structure | C4D_Vehicle)) return container;
  }
  // Procedure interaction target
  // (Except for FIGHT, of course. You can't control your enemy ;))
  var proc = GetProcedure();
  if (proc == "PUSH" || proc == "PULL" || proc == "BUILD") return GetActionTarget();
  // First contents object interaction target
  return Contents(0);
}

public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
  // Generic movement
  if (inherited(plr, ctrl, x, y, strength, repeat, release)) return true;
  var proc = GetProcedure();
  // Handled by InteractionTarget?
  var interaction_target = GetInteractionTarget();
  if (interaction_target)
  {
    if (interaction_target->ObjectControl(plr, ctrl, x,y, strength, repeat, release)) return true;
  }
  // Dolphin jump
  if (ctrl == CON_DolphinJump) return DolphinJump();
  // Context menu
  else if (ctrl == CON_Context)
  {
    // Context menu of interaction target (fallback to this if no interaction target)
    if (!interaction_target) interaction_target = this;
    SetCommand(this,"Context",0,0,0,interaction_target);
    return ExecuteCommand();
  }
  // Throw
  else if (ctrl == CON_Throw)
  {
    // During Scale+Hangle, this means "Drop". During dig, this means object dig out request. Otherwise, throw.
    if (proc == "DIG")
      return SetActionData(!GetActionData());
    else if (proc == "SCALE" || proc == "HANGLE")
      return PlayerObjectCommand(plr, false, "Drop");
    else
      return PlayerObjectCommand(plr, false, "Throw");
  }
  // Dig
  else if (ctrl == CON_Dig)
  {
    if (proc == "DIG")
    {
      // Currently, another press on dig ends digging. Maybe changed once we have the shovel system?
      SetAction("Walk");
      return true;
    }
    else if (proc == "WALK")
    {
      if (!GetPhysical("Dig")) return false;
      if (!SetAction("Dig")) return false;
      SetActionData(0);
      return true;
    }
    // Can't dig now
    return false;
  }
  // Unhandled
  return false;
}

private func DolphinJump()
{
  // nur wenn an Meeresoberfläche
  if(!InLiquid()) return 0;
  if(GBackSemiSolid(0,-1)) return 0;
  // Nicht wenn deaktiviert (z.B. Ohnmacht)
  if (GetActMapVal("ObjectDisabled", GetAction(), GetID())) return false;
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

protected func ControlCommand(szCommand, pTarget, iTx, iTy, pTarget2, Data)
{
  // Kommando MoveTo an Pferd weiterleiten
  if (szCommand == "MoveTo")
    if (IsRiding())
      return GetActionTarget()->~ControlCommand(szCommand, pTarget, iTx, iTy);
  // Anderes Kommando beim Reiten: absteigen (Ausnahme: Context)
  if (IsRiding() && szCommand != "Context")
  {
    SetComDir(COMD_Stop,GetActionTarget());
    GetActionTarget()->~ControlDownDouble(this);
  }
  // RejectConstruction Callback beim Bauen durch Drag'n'Drop aus einem Gebaeude-Menu
  if(szCommand == "Construct")
  {
    // Data ist eigentlich keine ID, sondern ein C4Value* - Damit ein DirectCall
    // möglich ist, muss sie aber zu einer C4ID gecastet werden.
    if(CastC4ID(Data)->~RejectConstruction(iTx - GetX(), iTy - GetY(), this) )
    {
      return 1;
    }
  }
  // Kein überladenes Kommando
  return 0;
}

public func ControlDownDouble() {} // dummy


/* Verwandlung */

private func RedefinePhysical(szPhys, idTo)
{
  // Physical-Werte ermitteln
  var physDefFrom = GetPhysical(szPhys, 0, 0, GetID()),
      physDefTo   = GetPhysical(szPhys, 0, 0, idTo),
      physCurr    = GetPhysical(szPhys);
  // Neuen Wert berechnen
  var physNew; if (physDefTo) physNew=BoundBy(physDefTo-physDefFrom+physCurr, 0, 100000);
  // Neuen Wert für den Reset immer temporär setzen, selbst wenn keine Änderung besteht, damit der Reset richtig funktioniert
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
  if (GetRank()<6) RedefinePhysical("CanHangle", idTo);*/ // z.Z. können es alle
  RedefinePhysical("CanDig", idTo);
  RedefinePhysical("CanConstruct", idTo);
  RedefinePhysical("CanChop", idTo);
  RedefinePhysical("CanSwimDig", idTo);
  RedefinePhysical("CorrosionResist", idTo);
  RedefinePhysical("BreatheWater", idTo);
  // Damit Aufwertungen zu nicht-Magiern keine Zauberenergie übrig lassen
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
  ResetPhysical(0, "BreatheWater");
  ResetPhysical(0, "CorrosionResist");
  ResetPhysical(0, "CanSwimDig");
  ResetPhysical(0, "CanChop");
  ResetPhysical(0, "CanConstruct");
  ResetPhysical(0, "CanDig");
  ResetPhysical(0, "Float");
  ResetPhysical(0, "Magic");
  ResetPhysical(0, "Fight");
  ResetPhysical(0, "Push");
  ResetPhysical(0, "Throw");
  ResetPhysical(0, "Swim");
  ResetPhysical(0, "Dig");
  ResetPhysical(0, "Hangle");
  ResetPhysical(0, "Scale");
  ResetPhysical(0, "Jump");
  ResetPhysical(0, "Walk");
  ResetPhysical(0, "Breath");
  ResetPhysical(0, "Energy");
  // Keine Rückänderung bei temporären Aufrufen oder beim Tod/Löschen
  if (tmp || iReason) return;
  // Damit Aufwertungen von nicht-Magiern keine Zauberenergie übrig lassen
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
  // Aktivitätsdaten sichern
  var phs=GetPhase(),act=GetAction();
  // Umwandeln
  ChangeDef(idTo);
  // Aktivität wiederherstellen
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
  SetDir(GetDir(GetActionTarget()));
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
    iXDir += GetXDir(GetActionTarget()) / 10;
    iYDir += GetYDir(GetActionTarget()) / 10;
  }
  // Werfen!
  Exit(pObj, iX, iY, iR, iXDir, iYDir, iRDir);  
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
  Punch(GetActionTarget());
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
  // Info-Broadcasts für sterbende Clonks
  GameCallEx("OnClonkDeath", this, iKilledBy);
  
  // Der Broadcast könnte seltsame Dinge gemacht haben: Clonk ist noch tot?
  if (GetAlive()) return;
  
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
  // Verhindert Festhängen am Mittelvertex
  if(!GetXDir()) if(Abs(GetYDir()) < 5)
    if(GBackSolid(0, 3))
      SetPosition(GetX(), GetY() + 1);
}


/* Status */

public func IsRiding()
{
  // Reitet der Clonk?
  return (WildcardMatch(GetAction(), "Ride*"));
}

public func IsClonk() { return 1; }


/* Kontext */

public func ContextRelease(pCaller) 
{
  [$CtxRelease$|Image=CXRL|Condition=ReleaseAllowed]
  FindObject(REAC)->Activate(GetOwner());
  return 1;
}

public func ContextEnergy(pCaller)
{
  [$TxtEnergysupply$|Image=CXEC|Condition=AtEnergySite]
  var pSite; 
  if (pSite = FindEnergySite())
    SetCommand(this, "Energy", pSite);
  return 1;
}

public func ContextConstructionSite(pCaller)
{
  [$CtxConstructionMaterial$|Image=CXCM|Condition=AtConstructionSite]
  var pSite; 
  if (pSite = FindConstructionSite())
    PlayerMessage(GetOwner(), pSite->GetNeededMatStr(), pSite);
  return 1;
}

public func ContextChop(pCaller)
{
  [$CtxChop$|Image=CXCP|Condition=AtTreeToChop]
  var pTree; 
  if (pTree = FindTree())
    SetCommand(this, "Chop", pTree);
  return 1;
}

public func ContextConstruction(pCaller)
{
  [$CtxConstructionDesc$|Image=CXCN|Condition=HasConstructMenu]
  SetCommand(this, "Construct");
  ExecuteCommand();
  return 1;
}

public func ContextHome(pCaller)
{
  [$CtxHomeDesc$|Image=CXHM|Condition=HasBase]
  SetCommand(this, "Home");
  return 1;
}


/* Hilfsfunktion */

public func ContainedCall(string strFunction, object pTarget)
{
  // Erst das betreffende Gebäude betreten, dann die Zielfunktion aufrufen 
  SetCommand(this, "Call", pTarget, this, 0, 0, strFunction);
  AddCommand(this, "Enter", pTarget);
}


/* Callback beim Auswahl aus dem Construct-Kontextmenu */

public func ControlCommandConstruction(target, x, y, target2, def)
{
  // Keine Konstruktion erlaubt?
  if(def->~RejectConstruction(x - GetX(), y - GetY(), this) )
    // Construct-Kommando beenden
    return FinishCommand(this, false, 0) ;
}


/* Automatische Produktion */

public func ControlCommandAcquire(target, x, y, target2, def)
{
  // Falls das Teil rumliegt nur aufsammeln
  var obj = GetAvailableObject (def, target2);
  if (obj) {
    AddEffect("IntNotAvailable", obj, 1, 5, this);
    AddCommand (this, "Get", obj, 0, 0, 0, 40);
    return 1;
  }
  // Gebäude suchen worin man's herstellen kann  
  if (obj = GetProducerOf (def)) {
    AddCommand (this (), "Call", this, 0, 0, 0, 0, "AutoProduction", 0, 1);
    obj -> HowToProduce (this, def);
    return 1;
  }
  AddCommand (this, "Buy", 0, 0, 0, 0, 100, def, 0, C4CMD_Sub);
  return 1;
}

public func AutoProduction() { return 1; }

public func AutoProductionFailed() 
{
  var def = GetCommand (this (), 5, 1);
  if (!FindContents(def)) {
    var obj = GetAvailableObject (def, GetCommand (this (), 4, 1));
    if (obj) {
      AddEffect("IntNotAvailable", obj, 1, 5, this);
      AddCommand (this, "Get", obj,0,0,0,40);
      return 1;
    }
    AddCommand (this, "Buy", 0, 0, 0, 0, 100, GetCommand(this, 5, 1), 0, C4CMD_Sub);
  }
  return 1;
}

public func FxIntNotAvailableStart(target, number)
{
  EffectVar(0, target, number) = this;
}

public func FxIntNotAvailableTimer(target, number)
{
  var clonk = EffectVar(0, target, number);
  // Check wether the clonk still wants to get the object
  for (var i = 0; GetCommand(clonk,0,i); ++i)  {
    if (GetCommand(clonk, 0, i) == "Get" && GetCommand(clonk, 1, i) == target)
      return;
  }
  return FX_Execute_Kill;
}

public func GetProducerOf(def)
{
  return FindObject2(Find_InRect(-500,-250,1000,500), Find_Func("IsProducerOf", this, def), Sort_Distance());
}


/* Trinken */

public func Drink(object pDrink)
{
  // Trinkaktion setzen, wenn vorhanden
  if (GetActMapVal("Name", "Drink"))
    SetAction("Drink");
  // Vorsicht: erstmal nichts mit pDrink machen,
  // die Potions löschen sich meist selber...
}


/* Einsammeln */

public func RejectCollect(id idObject, object pObject)
{
  // Objekt kann gepackt werden
  // automatisches Packen aber nur wenn die Paktteile nicht extra gezählt werden
  if(!IsSpecialItem(pObject)) if(pObject->~JoinPack(this)) return 1;
    
  // Objektaufnahme mit Limit verhindern, wenn bereits genug getragen
  if(pObject->~CarryLimit() && ContentsCount(idObject) >= pObject->~CarryLimit() ) return 1;
    
  // Spezialitem?
  var i, iCount;
  if(i = IsSpecialItem(pObject))
  {
    // Noch genug Platz für das ganze Packet?
    if(GetSpecialCount(GetMaxSpecialCount(i-1))+Max(pObject->~PackCount(),1)<=GetMaxSpecialCount(i-1, 1)) return 0;
    iCount = GetMaxSpecialCount(i-1, 1)-GetSpecialCount(GetMaxSpecialCount(i-1));
    // Ansonten so viel wie geht rein
    if(pObject->~SplitPack(pObject->~PackCount()-iCount)) return 0;
    else return 1;
  }
  
  return GetNonSpecialCount()>=MaxContentsCount();
}


/* Itemlimit */
public func MaxContentsCount() { return 1; }

public func GetMaxSpecialCount(iIndex, fAmount)
{
  // Hier könnten Spezialbehandlungen von Itemgruppen definiert werden
  // wie z.B. zu dem Inventar noch 30 Pfeile aufnehmen (siehe auch Ritter)
  //  if(iIndex == 0) { if(fAmount) return(30); return("IsArrow"); }
}

/* Liefert die Gesamtzahl eines Objekt(paket)typs */ 
private func GetObjectCount(idObj) 
  { 
  var idUnpackedObj; 
  if (idUnpackedObj = idObj->~UnpackTo()) 
    // Auch verschachtelte Pakete mitzählen 
    return GetObjectCount(idUnpackedObj) * idObj->PackCount(); 
  // Ansonsten ist es nur ein Objekt 
  return 1; 
  }

/* Spezialgegenstände im Inventar zählen */ 
private func GetSpecialCount(szTest) 
  { 
  var iCnt, pObj; 
  // Einzelne Pfeile... 
  for(var i = 0; pObj = Contents(i); i++) 
    if(ObjectCall(pObj, szTest)) 
      iCnt++; 
  // Pakete... 
  for(var i = 0; pObj = Contents(i); i++) 
    if(pObj->~UnpackTo())
      if(DefinitionCall(pObj->~UnpackTo(), szTest))
        iCnt += GetObjectCount(pObj); 
  // Wert zurückgeben 
  return iCnt; 
  }
  
/* Testen eines Objektes */
private func IsSpecialItem(pObj)
{
  // Spezialitem?
  var j=-1;
  while(GetMaxSpecialCount(++j, 1))
    if(ObjectCall(pObj, GetMaxSpecialCount(j)))
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
  // Inventar einzeln auf nicht-Spezial überprüfen 
  for(var i = 0; pObj = Contents(i); i++)
    // Spezialitems nicht zählen
    if(!IsSpecialItem(pObj))
        iCnt++;
   
  // Wert zurückgeben 
  return iCnt; 
  } 

/* Reiten */

public func ContextDescend(pCaller) 
{
  [$TxtDescend$|Image=DSCN|Condition=IsRiding]
  DescendVehicle();
}

public func DescendVehicle()
{
  var pOldVehicle = GetActionTarget();
  SetAction("Walk");
  // Feststecken nach Absteigen? Dann besser direkt beim Gefährt absteigen.
  if (Stuck()) if (pOldVehicle)
  {
    var x=GetX(), y=GetY();
    SetPosition(GetX(pOldVehicle), GetY(pOldVehicle));
    if (Stuck())
    {
      // Das Gefährt steckt auch? Dann hilft es alles nichts. Zurück zum Ursprungsort.
      SetPosition(x,y);
    }
  }  
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

/* Pfeile im Inventar zählen */ 
private func GetArrowCount() 
  {
   return GetSpecialCount("IsArrow");
  }


/* Dummies, damit die Engine die Namen kennt... */

func Activate() {}
func HowToProduce() {}
func PackCount() {}
