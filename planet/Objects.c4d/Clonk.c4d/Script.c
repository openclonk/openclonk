/*-- Der Clonk --*/

// selectable by HUD
#include HUDS
// standard controls
#include L_CO

// un-comment them as soon as the new controls work with context menus etc.^
// Context menu
//#include L_CM
// Auto production
//#include L_AP

local pInventory;

/* Initialization */

protected func Construction()
{
  _inherited(...);
  // shovel...
  var shov = CreateObject(SHVL,0,0,GetOwner());
  Collect(shov,2);
  // Clonks mit Magiephysikal aus fehlerhaften Szenarien korrigieren
  if (GetID () == CLNK)
    if (GetPhysical ("Magic", 1))
      SetPhysical ("Magic", 0, 1);
  SetAction("Walk");
  SetDir(Random(2));
  // Broadcast für Spielregeln
  GameCallEx("OnClonkCreation", this);
}



/* Bei Hinzuf�gen zu der Crew eines Spielers */

protected func Recruitment(int iPlr) {
  // Broadcast f�r Crew
  GameCallEx("OnClonkRecruitment", this, iPlr);
  
  return _inherited(iPlr,...);
}

protected func DeRecruitment(int iPlr) {
  // Broadcast f�r Crew
  GameCallEx("OnClonkDeRecruitment", this, iPlr);
  
  return _inherited(iPlr,...);
}

/*
protected func ControlCommand(szCommand, pTarget, iTx, iTy, pTarget2, Data)
{
  // Kommando MoveTo an Pferd weiterleiten
  if (szCommand == "MoveTo")
    if (IsRiding())
      return GetActionTarget()->~ControlCommand(szCommand, pTarget, iTx, iTy);
  // Anderes Kommando beim Reiten: absteigen (Ausnahme: Context)
  if (IsRiding() && szCommand != "Context")
  {
    GetActionTarget()->SetComDir(COMD_Stop);
    GetActionTarget()->~ControlDownDouble(this);
  }
  // RejectConstruction Callback beim Bauen durch Drag'n'Drop aus einem Gebaeude-Menu
  if(szCommand == "Construct")
  {
    if(Data->~RejectConstruction(iTx - GetX(), iTy - GetY(), this) )
    {
      return 1;
    }
  }
  // Kein �berladenes Kommando
  return 0;
}
*/

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


/* Food and drinks :-) */  

public func Drink(object pDrink)
{
  // Trinkaktion setzen, wenn vorhanden
  if (GetActMapVal("Name", "Drink"))
    SetAction("Drink");
  // Attention: don't do anything with the potion
  // normally it deletes itself.
}

/* Actions */

private func Riding()
{
  // change dir of horse
  SetDir(GetActionTarget()->GetDir());
  // horse is still, the clonk too please!
  if (GetActionTarget()->~IsStill())
  {
    if (GetAction() != "RideStill")
      SetAction("RideStill");
  }
  // the horse is not still... the clonk too please!
  else
    if (GetAction() != "Ride")
      SetAction("Ride");
  return 1;
}

private func Fighting()
{
  if (!Random(2)) SetAction("Punch");
}

private func Punching()
{
  if (!Random(3)) Sound("Kime*");
  if (!Random(5)) Sound("Punch*");
  if (!Random(2)) return;
  GetActionTarget()->Punch();
  return;
}
  
private func Chopping()
{
  if (!GetActTime()) return;
  Sound("Chop*");
  CastParticles("Dust",Random(3)+1,6,-8+16*GetDir(),1,10,12);
}
  
private func Building()
{
  if (!Random(2)) Sound("Build*");
}

private func Processing()
{
  Sound("Build1");
}

private func Digging()
{
  Sound("Dig*");
}

protected func Scaling()
{
  var szDesiredAction;
  if (GetYDir()>0) szDesiredAction = "ScaleDown"; else szDesiredAction = "Scale";
  if (GetAction() != szDesiredAction) SetAction(szDesiredAction);
}


/* Ereignisse */
  
protected func CatchBlow()
{
  if (GetAction() == "Dead") return;
  if (!Random(5)) Hurt();
}
  
protected func Hurt()
{
  Sound("Hurt*");
}
  
protected func Grab(object pTarget, bool fGrab)
{
  Sound("Grab");
}

protected func Get()
{
  Sound("Grab");
}

protected func Put()
{
  Sound("Grab");
}

protected func Death(int iKilledBy)
{
  // Info-Broadcasts f�r sterbende Clonks
  GameCallEx("OnClonkDeath", this, iKilledBy);
  
  // Der Broadcast k�nnte seltsame Dinge gemacht haben: Clonk ist noch tot?
  if (GetAlive()) return;
  
  Sound("Die");
  DeathAnnounce();
  // Letztes Mannschaftsmitglied tot: Script benachrichtigen
  if (!GetCrew(GetOwner()))
    GameCallEx("RelaunchPlayer",GetOwner());
  return;
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
  return;
}

protected func DeepBreath()
{
  Sound("Breath");
}
  
protected func CheckStuck()
{                   
  // Verhindert Festh�ngen am Mittelvertex
  if(!GetXDir()) if(Abs(GetYDir()) < 5)
    if(GBackSolid(0, 3))
      SetPosition(GetX(), GetY() + 1);
}

/* Status */

public func IsClonk() { return true; }


// Test to synchronize the walkanimation with the movement
local OldPos;

static CLNK_WalkStates; // TODO: Well wasn't there once a patch, allowing arrys to be assigned to global static?
static CLNK_HangleStates;
static CLNK_SwimStates;

func StartWalk()
{
	if(CLNK_WalkStates == nil)
		CLNK_WalkStates = ["Stand", "Walk", "Run"];
	if(!GetEffect("IntWalk", this))
		AddEffect("IntWalk", this, 1, 1, this);
}

func StopWalk()
{
	if(GetAction() != "Walk") RemoveEffect("IntWalk", this);
}

func FxIntWalkStart(pTarget, iNumber)
{
	AnimationPlay("Stand", 0);
	AnimationPlay("Walk", 0);
	AnimationPlay("Run", 0);
	EffectVar(0, pTarget, iNumber) = 0; // Phase
	EffectVar(1, pTarget, iNumber) = 1000; // Stand weight
	EffectVar(2, pTarget, iNumber) = 0; // Walk weight
	EffectVar(3, pTarget, iNumber) = 0; // Run weight
	EffectVar(4, pTarget, iNumber) = 0; // Oldstate
	EffectVar(5, pTarget, iNumber) = 0; // Save wether the last frame was COMD_Stop
}

func FxIntWalkStop(pTarget, iNumber)
{
	AnimationStop("Stand");
	AnimationStop("Walk");
	AnimationStop("Run");
}

func FxIntWalkTimer(pTarget, iNumber, iTime)
{
//	if(GetAction() != "Walk") return -1;
	var iSpeed = Distance(0,0,GetXDir(),GetYDir());
  var iState = 0;

	// Play stand animation when not moving
	if(iSpeed < 1 && EffectVar(5, pTarget, iNumber))
	{
		AnimationSetState("Stand", ((iTime/5)%11)*100, nil);
		iState = 1;
	}
	// When moving slowly play synchronized with movement walk
	else if(iSpeed < 10)
	{
		EffectVar(0, pTarget, iNumber) +=  iSpeed*25/(16*1);
		if(EffectVar(0, pTarget, iNumber) > 250) EffectVar(0, pTarget, iNumber) -= 250;

		AnimationSetState("Walk", EffectVar(0, pTarget, iNumber)*10, nil);
		iState = 2;
	}
	// When moving fast play run
	else
	{
		if(EffectVar(4, pTarget, iNumber) != 3)
			EffectVar(0, pTarget, iNumber) = 190; // start with frame 190 (feet on the floor)
		else
  		EffectVar(0, pTarget, iNumber) += iSpeed*25/(16*3);
		if(EffectVar(0, pTarget, iNumber) > 250) EffectVar(0, pTarget, iNumber) -= 250;

		AnimationSetState("Run", EffectVar(0, pTarget, iNumber)*10, nil);
		iState = 3;
	}

  // Save wether he have COMD_Stop or not. So a single frame with COMD_Stop keeps the movement
  if(GetComDir() == COMD_Stop) EffectVar(5, pTarget, iNumber) = 1;
	else EffectVar(5, pTarget, iNumber) = 0;
	
	// Blend between the animations: The actuall animations gains weight till it reaches 1000
	// the other animations lose weight until they are at 0
	for(var i = 1; i <= 3; i++)
	{
		if(i == iState)
		{
			if(EffectVar(i, pTarget, iNumber) < 1000)
				EffectVar(i, pTarget, iNumber) += 200;
		}
		else
		{
			if(EffectVar(i, pTarget, iNumber) > 0)
				EffectVar(i, pTarget, iNumber) -= 200;
		}
		AnimationSetState(CLNK_WalkStates[i-1], nil, EffectVar(i, pTarget, iNumber));
	}
	EffectVar(4, pTarget, iNumber) = iState;
}

func StartHangle()
{
	if(CLNK_HangleStates == nil)
		CLNK_HangleStates = ["HangleStand", "Hangle"];
	if(!GetEffect("IntHangle", this))
		AddEffect("IntHangle", this, 1, 1, this);
}

func StopHangle()
{
	if(GetAction() != "Hangle") RemoveEffect("IntHangle", this);
}

func FxIntHangleStart(pTarget, iNumber)
{
	AnimationPlay("Hangle", 0);
	AnimationPlay("HangleStand", 1000);
	EffectVar(0, pTarget, iNumber) = 0; // Phase
	EffectVar(1, pTarget, iNumber) = 1000; // HangleStand weight
	EffectVar(2, pTarget, iNumber) = 0; // Hangle weight
	EffectVar(4, pTarget, iNumber) = 0; // Oldstate
	EffectVar(5, pTarget, iNumber) = 0; // Remember if the last frame had COMD_Stop (so a single COMD_Stop frame doesn't stop the movement)
	EffectVar(6, pTarget, iNumber) = 1; // Scedule Stop
	EffectVar(7, pTarget, iNumber) = 0; // Hanging Pose (Front to player or Back)
}

func FxIntHangleStop(pTarget, iNumber)
{
	AnimationStop("Hangle");
	AnimationStop("HangleStand");
}

func FxIntHangleTimer(pTarget, iNumber, iTime)
{
  // Make a cosine movment speed (the clonk only moves whem he makes a "stroke")
  var iSpeed = 50-Cos(EffectVar(0, pTarget, iNumber)*360*2/1000, 50);
	SetPhysical("Hangle", 600*iSpeed, 2);
	var iState = 0;

  // Continue movement, if the clonk still has momentum
	if(GetComDir() == COMD_Stop && iSpeed>10)
	{
		EffectVar(6, pTarget, iNumber) = 1;
		if(GetDir())
			SetComDir(COMD_Right);
		else
  	  SetComDir(COMD_Left);
	}
	// Stop movement if clonk has lost his momentum
	else if(EffectVar(6, pTarget, iNumber))
	{
    EffectVar(6, pTarget, iNumber) = 0;
		SetComDir(COMD_Stop);
		// and remeber the pose (front or back)
		if(EffectVar(0, pTarget, iNumber) > 250 && EffectVar(0, pTarget, iNumber) < 750)
			EffectVar(7, pTarget, iNumber) = 1;
		else
			EffectVar(7, pTarget, iNumber) = 0;
	}
	
	// Play stand animation when not moving
	if(GetComDir() == COMD_Stop && EffectVar(5, pTarget, iNumber))
	{
		AnimationSetState("HangleStand", ((iTime/5)%21)*100+4000*EffectVar(7, pTarget, iNumber), nil);
		iState = 1;
	}
	// When moving
	else
	{
		var iSpeed = 5;
		if(EffectVar(4, pTarget, iNumber) !=  2)
			EffectVar(0, pTarget, iNumber) = 100+500*EffectVar(7, pTarget, iNumber); // start with frame 100 or from the back hanging pose frame 600
		else EffectVar(0, pTarget, iNumber) +=  iSpeed*100/(14*2);
		if(EffectVar(0, pTarget, iNumber) > 1000) EffectVar(0, pTarget, iNumber) -= 1000;

		AnimationSetState("Hangle", EffectVar(0, pTarget, iNumber)*10, nil);
		iState = 2;
	}

  // Save wether he have COMD_Stop or not. So a single frame with COMD_Stop keeps the movement
  if(GetComDir() == COMD_Stop) EffectVar(5, pTarget, iNumber) = 1;
	else EffectVar(5, pTarget, iNumber) = 0;

	// Blend between the animations: The actuall animations gains weight till it reaches 1000
	// the other animations lose weight until they are at 0
	for(var i = 1; i <= 2; i++)
	{
		if(i == iState)
		{
			if(EffectVar(i, pTarget, iNumber) < 1000)
				EffectVar(i, pTarget, iNumber) += 200;
		}
		else
		{
			if(EffectVar(i, pTarget, iNumber) > 0)
				EffectVar(i, pTarget, iNumber) -= 200;
		}
		AnimationSetState(CLNK_HangleStates[i-1], nil, EffectVar(i, pTarget, iNumber));
	}
	EffectVar(4, pTarget, iNumber) = iState;
}

func StartSwim()
{
	if(CLNK_SwimStates == nil)
		CLNK_SwimStates = ["SwimStand", "Swim", "SwimDiveUp", "SwinDiveDown"];
	if(!GetEffect("IntSwim", this))
		AddEffect("IntSwim", this, 1, 1, this);
}

func StopSwim()
{
	if(GetAction() != "Swim") RemoveEffect("IntSwim", this);
}

func FxIntSwimStart(pTarget, iNumber)
{
	AnimationPlay("SwimStand", 1000);
	AnimationPlay("Swim", 0);
	AnimationPlay("SwimDiveUp", 0);
	AnimationPlay("SwimDiveDown", 0);
	EffectVar(0, pTarget, iNumber) = 0; // Phase
	EffectVar(1, pTarget, iNumber) = 1000; // Stand weight
	EffectVar(2, pTarget, iNumber) = 0; // Walk weight
	EffectVar(3, pTarget, iNumber) = 0; // Run weight
	EffectVar(4, pTarget, iNumber) = 0; // Oldstate
	EffectVar(5, pTarget, iNumber) = 0; // Save wether the last frame was COMD_Stop
	EffectVar(6, pTarget, iNumber) = 0; // OldRot
}

func FxIntSwimStop(pTarget, iNumber)
{
	AnimationStop("SwimStand");
	AnimationStop("Swim");
	AnimationStop("SwimDiveUp");
	AnimationStop("SwimDiveDown");
}

func FxIntSwimTimer(pTarget, iNumber, iTime)
{
	DoEnergy(1); //TODO Remove this! Endless Energy while diving is only for the testers

	var iSpeed = Distance(0,0,GetXDir(),GetYDir());
  var iState = 0;

	// Play stand animation when not moving
	if(iSpeed < 1 && EffectVar(5, pTarget, iNumber) && !GBackSemiSolid(0, -4))
	{
		AnimationSetState("SwimStand", ((iTime/5)%20)*100, nil);
		iState = 1;
	}
	// When moving slowly play synchronized with movement walk
	else if(!GBackSemiSolid(0, -4))
	{
		EffectVar(0, pTarget, iNumber) +=  iSpeed*40/(16*2);
		if(EffectVar(0, pTarget, iNumber) > 400) EffectVar(0, pTarget, iNumber) -= 400;

		AnimationSetState("Swim",     EffectVar(0, pTarget, iNumber)*10, nil);
		iState = 2;
	}
	// Diving
	else
	{
		EffectVar(0, pTarget, iNumber) +=  iSpeed*40/(16*2);
		if(EffectVar(0, pTarget, iNumber) > 400) EffectVar(0, pTarget, iNumber) -= 400;

		AnimationSetState("SwimDiveUp",   ((iTime/2)%20)*100, nil);
		AnimationSetState("SwimDiveDown", ((iTime/2)%20)*100, nil);
		iState = 3;
	}

  // Save wether he have COMD_Stop or not. So a single frame with COMD_Stop keeps the movement
  if(GetComDir() == COMD_Stop) EffectVar(5, pTarget, iNumber) = 1;
	else EffectVar(5, pTarget, iNumber) = 0;

	// Blend between the animations: The actuall animations gains weight till it reaches 1000
	// the other animations lose weight until they are at 0
	for(var i = 1; i <= 3; i++)
	{
		if(i == iState)
		{
			if(EffectVar(i, pTarget, iNumber) < 1000)
				EffectVar(i, pTarget, iNumber) += 100;
		}
		else
		{
			if(EffectVar(i, pTarget, iNumber) > 0)
				EffectVar(i, pTarget, iNumber) -= 100;
		}
		AnimationSetState(CLNK_SwimStates[i-1], nil, EffectVar(i, pTarget, iNumber));
	}
	// Adjust Swim direction
	if(iSpeed > 1)
	{
	  var iRot = Angle(-Abs(GetXDir()), GetYDir());
		EffectVar(6, pTarget, iNumber) += BoundBy(iRot-EffectVar(6, pTarget, iNumber), -4, 4);
	}
	iRot = EffectVar(6, pTarget, iNumber);
	AnimationSetState("SwimDiveUp",     nil, EffectVar(3, pTarget, iNumber)*iRot/180);
	AnimationSetState("SwimDiveDown", nil, EffectVar(3, pTarget, iNumber)-EffectVar(3, pTarget, iNumber)*iRot/180);
			
	EffectVar(4, pTarget, iNumber) = iState;
}

/* Act Map */

func Definition(def) {
  SetProperty("ActMap", {

Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Directions = 2,
	FlipDir = 1,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Walk",
	StartCall = "StartWalk",
	AbortCall = "StopWalk",
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
	Hgt = 20,
	NextAction = "Dig",
	StartCall = "Digging",
	DigFree = 11,
	InLiquidAction = "Swim",
},
Bridge = {
	Prototype = Action,
	Name = "Bridge",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 1,
	Length = 16,
	Delay = 1,
	X = 0,
	Y = 60,
	Wdt = 8,
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
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 80,
	Wdt = 8,
	Hgt = 20,
	OffX = 0,
	OffY = 3,
	StartCall = "StartSwim",
	AbortCall = "StopSwim",
},
Hangle = {
	Prototype = Action,
	Name = "Hangle",
	Procedure = DFA_HANGLE,
	Directions = 2,
	FlipDir = 1,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 100,
	Wdt = 8,
	Hgt = 20,
	OffX = 0,
	OffY = 3,
	NextAction = "Hangle",
	StartCall = "StartHangle",
	AbortCall = "StopHangle",
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
	Wdt = 8,
	Hgt = 20,
	NextAction = "Hold",
	InLiquidAction = "Swim",
	PhaseCall = "CheckStuck",
	Animation = "Jump",
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
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
	Wdt = 8,
	Hgt = 20,
	NextAction = "Ride",
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
	Wdt = 8,
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
	Wdt = 8,
	Hgt = 20,
	NextAction = "Walk",
},  }, def);
  SetProperty("Name", "Clonk", def);
}

