/*-- Der Clonk --*/

// selectable by HUD
#include HUDS
// standard controls
#include L_CO

// un-comment them as soon as the new controls work with context menus etc.^
// Context menu
//#include L_CMAniF
// Auto production
//#include L_AP

local pInventory;

/* Initialization */

protected func Construction()
{
  _inherited(...);
  // shovel...
  var shov = CreateObject(SHVL,0,0,GetOwner());
  Collect(shov,false,2);
  // Clonks mit Magiephysikal aus fehlerhaften Szenarien korrigieren
  if (GetID () == CLNK)
    if (GetPhysical ("Magic", 1))
      SetPhysical ("Magic", 0, 1);
  SetAction("Walk");
  SetDir(Random(2));
  // Broadcast für Spielregeln
  GameCallEx("OnClonkCreation", this);

	AddEffect("IntTurn", this, 1, 1, this);
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

// TODO: Make this more sophisticated, readd turn animation and other
// adaptions
public func IsClonk() { return true; }

/* Carry items on the clonk */

local iHandMesh;
local fHandAction;
local fBothHanded;
// GetSelected

func OnSelectionChanged(int oldslot, int newslot, bool secondaryslot)
{
	AttachHandItem(secondaryslot);
	return _inherited(oldslot, newslot, secondaryslot);
}
func OnSlotEmpty(int slot)
{
	if(GetSelected(0) == slot) AttachHandItem(0);
	if(GetSelected(1) == slot) AttachHandItem(1);
	return _inherited(slot);
}
func OnSlotFull(int slot)
{
	if(GetSelected(0) == slot) AttachHandItem(0);
	if(GetSelected(1) == slot) AttachHandItem(1);
	return _inherited(slot);
}

public func DetachObject(object obj)
{
	if(GetSelectedItem(0) == obj)
		DetachHandItem(0);
	if(GetSelectedItem(1) == obj)
		DetachHandItem(1);
}

func DetachHandItem(bool secondary)
{
	if(iHandMesh[secondary])
		DetachMesh(iHandMesh[secondary]);
	iHandMesh[secondary] = 0;
}

func AttachHandItem(bool secondary)
{
	if(!iHandMesh) iHandMesh = [0,0];
	DetachHandItem(secondary);
	UpdateAttach();	
}

func UpdateAttach()
{
	StopAnimation(GetRootAnimation(6));
	DoUpdateAttach(0);
	DoUpdateAttach(1);
}

func DoUpdateAttach(bool sec)
{
	var obj = GetSelectedItem(sec);
	if(!obj) return;
	var iAttachMode = obj->~GetCarryMode(this);
	if(iAttachMode == CARRY_None) return;

	if(iHandMesh[sec])
	{
  	DetachMesh(iHandMesh[sec]);
		iHandMesh[sec] = 0;
	}

	var bone = "main";
	if(obj->~GetCarryBone()) bone = obj->~GetCarryBone(this);
	var trans = obj->~GetCarryTransform(this);

	var pos_hand = "pos_hand2";
	if(sec) pos_hand = "pos_hand1";
	var pos_back = "pos_back1";
	if(sec) pos_back = "pos_back2";
	var closehand = "Close2Hand";
	if(sec) closehand = "Close1Hand";

	if(!sec) fBothHanded = 0;

	var special = obj->~GetCarrySpecial(this);
	if(special)
	{
		iHandMesh[sec] = AttachMesh(obj, special, bone, trans);
		iAttachMode = 0;
	}

	if(iAttachMode == CARRY_Hand)
	{
		if(HasHandAction(sec))
		{
  		iHandMesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
			PlayAnimation(closehand, 6, Anim_Const(GetAnimationLength(closehand)), Anim_Const(1000));
		}
		else
			; // Don't display
	}
	else if(iAttachMode == CARRY_HandBack)
	{
		if(HasHandAction(sec))
		{
  		iHandMesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
			PlayAnimation(closehand, 6, Anim_Const(GetAnimationLength(closehand)), Anim_Const(1000));
		}
		else
			iHandMesh[sec] = AttachMesh(obj, pos_back, bone, trans);
	}
	else if(iAttachMode == CARRY_HandAlways)
	{
		iHandMesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
		PlayAnimation(closehand, 6, Anim_Const(GetAnimationLength(closehand)), Anim_Const(1000));
	}
	else if(iAttachMode == CARRY_Back)
	{
		iHandMesh[sec] = AttachMesh(obj, pos_back, bone, trans);
	}
	else if(iAttachMode == CARRY_BothHands)
	{
		if(sec) return;
		if(HasHandAction(sec))
		{
			iHandMesh[sec] = AttachMesh(obj, pos_hand, bone, trans);
			PlayAnimation("CarryArms", 6, Anim_Const(obj->~GetCarryPhase(this)), Anim_Const(1000));
			fBothHanded = 1;
		}
		else
			; // Don't display
	}
}

public func GetHandMesh(object obj)
{
	if(GetSelectedItem() == obj)
	  return iHandMesh[0];
	if(GetSelectedItem(1) == obj)
	  return iHandMesh[1];
}

static const CARRY_None = 0;
static const CARRY_Hand         = 1;
static const CARRY_HandBack     = 2;
static const CARRY_HandAlways   = 3;
static const CARRY_Back         = 4;
static const CARRY_BothHands		= 5;

func HasHandAction(sec)
{
	if(sec && fBothHanded)
		return false;
	if( (GetAction() == "Walk" || GetAction() == "Jump") && !fHandAction )
		return true;
	return false;
}

public func SetHandAction(bool fNewValue)
{
	if(fNewValue)
		fHandAction = 1;
	else
		fHandAction = 0;
}

public func GetHandAction()
{
	if(fHandAction == 1)
		return true;
	return false;
}

/*
// Test to synchronize the walkanimation with the movement
local OldPos;

static CLNK_WalkStates; // TODO: Well wasn't there once a patch, allowing arrys to be assigned to global static?
static CLNK_HangleStates;
static CLNK_SwimStates;
*/

/* Turn */
local iTurnAction;
local iTurnAction2;
local iTurnAction3;

local iTurnKnot1;
local iTurnKnot2;

func FxIntTurnStart(pTarget, iNumber, fTmp)
{
	if(fTmp) return;
	EffectVar(0, pTarget, iNumber) = GetDirection();
	var iTurnPos = 0;
	if(EffectVar(0, pTarget, iNumber) == COMD_Right) iTurnPos = 1;

	iTurnAction  = PlayAnimation("TurnRoot120", 1, Anim_Const(iTurnPos*GetAnimationLength("TurnRoot120")), Anim_Const(1000));
	iTurnAction2 = PlayAnimation("TurnRoot180", 1, Anim_Const(iTurnPos*GetAnimationLength("TurnRoot180")), Anim_Const(1000), iTurnAction);
	iTurnKnot1 = iTurnAction2+1;
	iTurnAction3 = PlayAnimation("TurnRoot240", 1, Anim_Const(iTurnPos*GetAnimationLength("TurnRoot240")), Anim_Const(1000), iTurnAction2);
	iTurnKnot2 = iTurnAction3+1;

	EffectVar(1, pTarget, iNumber) = 0;
}

func FxIntTurnTimer(pTarget, iNumber, iTime)
{
	// Check wether the clonk wants to turn (Not when he wants to stop)
  if(EffectVar(0, pTarget, iNumber) != GetDirection())
	{
		var iTurnTime = 10;
		if(EffectVar(0, pTarget, iNumber) == COMD_Right)
		{
			SetAnimationPosition(iTurnAction,  Anim_Linear(GetAnimationLength("TurnRoot120"), GetAnimationLength("TurnRoot120"), 0, iTurnTime, ANIM_Hold));
			SetAnimationPosition(iTurnAction2, Anim_Linear(GetAnimationLength("TurnRoot180"), GetAnimationLength("TurnRoot180"), 0, iTurnTime, ANIM_Hold));
			SetAnimationPosition(iTurnAction3, Anim_Linear(GetAnimationLength("TurnRoot240"), GetAnimationLength("TurnRoot240"), 0, iTurnTime, ANIM_Hold));
		}
		else
		{
			SetAnimationPosition(iTurnAction,  Anim_Linear(0, 0, GetAnimationLength("TurnRoot120"), iTurnTime, ANIM_Hold));
			SetAnimationPosition(iTurnAction2, Anim_Linear(0, 0, GetAnimationLength("TurnRoot180"), iTurnTime, ANIM_Hold));
			SetAnimationPosition(iTurnAction3, Anim_Linear(0, 0, GetAnimationLength("TurnRoot240"), iTurnTime, ANIM_Hold));
		}
		// Save new ComDir
		EffectVar(0, pTarget, iNumber) = GetDirection();
		EffectVar(1, pTarget, iNumber) = iTurnTime;
	}
	// Turning
	if(EffectVar(1, pTarget, iNumber))
	{
		EffectVar(1, pTarget, iNumber)--;
		if(EffectVar(1, pTarget, iNumber) == 0)
		{
			SetAnimationPosition(iTurnAction,  Anim_Const(GetAnimationLength("TurnRoot120")*(GetDirection()==COMD_Right)));
			SetAnimationPosition(iTurnAction2, Anim_Const(GetAnimationLength("TurnRoot180")*(GetDirection()==COMD_Right)));
			SetAnimationPosition(iTurnAction3, Anim_Const(GetAnimationLength("TurnRoot240")*(GetDirection()==COMD_Right)));
		}
	}
}

func SetTurnType(iIndex)
{
	if(iIndex == 0)
	{
		if(GetAnimationWeight(iTurnKnot1) > 0)
			SetAnimationWeight(iTurnKnot1, Anim_Linear(GetAnimationWeight(iTurnKnot1),1000,0,10,ANIM_Hold));
	}
	if(iIndex == 1)
	{
		if(GetAnimationWeight(iTurnKnot1) < 1000)
			SetAnimationWeight(iTurnKnot1, Anim_Linear(GetAnimationWeight(iTurnKnot1),0,1000,10,ANIM_Hold));
		if(GetAnimationWeight(iTurnKnot2) > 0)
			SetAnimationWeight(iTurnKnot2, Anim_Linear(GetAnimationWeight(iTurnKnot2),1000,0,10,ANIM_Hold));
	}
	if(iIndex == 2)
	{
		if(GetAnimationWeight(iTurnKnot2) > 0)
			SetAnimationWeight(iTurnKnot2, Anim_Linear(GetAnimationWeight(iTurnKnot2),0,1000,10,ANIM_Hold));
	}
}

func GetDirection()
{
	// Get direction from ComDir
	if(ComDirLike(GetComDir(), COMD_Right)) return COMD_Right;
	else if(ComDirLike(GetComDir(), COMD_Left)) return COMD_Left;
	// if ComDir hasn't a direction, use GetDir
	if(GetDir()==DIR_Right) return COMD_Right;
	else return COMD_Left;
}

/* Walk */

static const CLNK_WalkStand = "Stand";
static const CLNK_WalkWalk  = "Walk";
static const CLNK_WalkRun   = "Run";

func StartWalk()
{
//	if(CLNK_WalkStates == nil)
//		CLNK_WalkStates = ["Stand", "Walk", "Run", "StandTurn", "RunTurn"];
	if(!GetEffect("IntWalk", this))
		AddEffect("IntWalk", this, 1, 1, this);
}

func StopWalk()
{
	if(GetAction() != "Walk") RemoveEffect("IntWalk", this);
}

func GetCurrentWalkAnimation()
{
	var velocity = Distance(0,0,GetXDir(),GetYDir());
	if(velocity < 1) return CLNK_WalkStand;
	if(velocity < 10) return CLNK_WalkWalk;
	return CLNK_WalkRun;
}

func GetWalkAnimationPosition(string anim)
{
	// TODO: Choose proper starting positions, depending on the current
	// animation and its position: For Stand->Walk or Stand->Run, start
	// with a frame where one of the clonk's feets is on the ground and
	// the other one is in the air. For Walk->Run and Run->Walk, fade to
	// a state where its feets are at a similar position (just taking
	// over previous animation's position might work, using
	// GetAnimationPosition()). Walk->Stand is arbitrary I guess.
	// First parameter of Anim_Linear/Anim_AbsX is initial position.
	// Movement synchronization might also be tweaked somewhat as well.
	if(anim == CLNK_WalkStand)
		return Anim_Linear(0, 0, GetAnimationLength(anim), 35, ANIM_Loop);
	else if(anim == CLNK_WalkWalk)
		return Anim_AbsX(0, 0, GetAnimationLength(anim), 20);
	else if(anim == CLNK_WalkRun)
		return Anim_AbsX(0, 0, GetAnimationLength(anim), 50);
}

func FxIntWalkStart(pTarget, iNumber, fTmp)
{
	if(fTmp) return;
	// Always start in Stand for now... should maybe fade properly from previous animation instead
	var anim = "Stand";  //GetCurrentWalkAnimation();
	EffectVar(0, pTarget, iNumber) = anim;
	EffectVar(1, pTarget, iNumber) = PlayAnimation(anim, 5, GetWalkAnimationPosition(anim), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn
	SetTurnType(0);
}

func FxIntWalkStop(pTarget, iNumber, fTmp)
{
	if(fTmp) return;

	// Remove all
//	StopAnimation(GetRootAnimation(5));

	// Update carried items
	UpdateAttach();
}

func FxIntWalkTimer(pTarget, iNumber)
{
/*	if(EffectVar(4, pTarget, iNumber))
	{
		EffectVar(4, pTarget, iNumber)--;
		if(EffectVar(4, pTarget, iNumber) == 0)
			SetAnimationPosition(iTurnAction, Anim_Const(1200*(GetDirection()==COMD_Right)));
	}*/
	var anim = GetCurrentWalkAnimation();
	if(anim != EffectVar(0, pTarget, iNumber) && !EffectVar(4, pTarget, iNumber))
	{
		EffectVar(0, pTarget, iNumber) = anim;
		EffectVar(1, pTarget, iNumber) = PlayAnimation(anim, 5, GetWalkAnimationPosition(anim), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	}
/*	// Check wether the clonk wants to turn (Not when he wants to stop)
	if(EffectVar(17, pTarget, iNumber) != GetDirection())
	{
		var iTurnTime = 10;
//		if(Distance(0,0,GetXDir(),GetYDir()) < 10) //TODO Run turn animation
//		{
		if(EffectVar(17, pTarget, iNumber) == COMD_Right)
		{
			EffectVar(0, pTarget, iNumber) = PlayAnimation("StandTurn", 5, Anim_Linear(0, 0, 2000, iTurnTime, ANIM_Hold), Anim_Linear(0, 0, 1000, 2, ANIM_Remove));
			SetAnimationPosition(iTurnAction, Anim_Linear(1200, 1200, 0, iTurnTime, ANIM_Hold));
		}
		else
		{
			EffectVar(0, pTarget, iNumber) = PlayAnimation("StandTurn", 5, Anim_Linear(3000, 3000, 5000, iTurnTime, ANIM_Hold), Anim_Linear(0, 0, 1000, 2, ANIM_Remove));
			SetAnimationPosition(iTurnAction, Anim_Linear(0, 0, 1200, iTurnTime, ANIM_Hold));
		}
//		}
		//else
		//	EffectVar(0, pTarget, iNumber) = PlayAnimation("RunTurn", 5, Anim_Linear(0, 0, 2400, iTurnTime, ANIM_Hold), Anim_Linear(0, 0, 1000, 2, ANIM_Remove));
		// Save new ComDir
		EffectVar(17, pTarget, iNumber) = GetDirection();
		EffectVar(4, pTarget, iNumber) = iTurnTime;
	}*/
}

func StartStand()
{
	PlayAnimation(CLNK_WalkStand, 5, GetWalkAnimationPosition(CLNK_WalkStand), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
}

/*
func FxIntWalkStart(pTarget, iNumber, fTmp)
{
	if(fTmp) return;
	for(var i = 0; i < GetLength(CLNK_WalkStates); i++)
		AnimationPlay(CLNK_WalkStates[i], 0);
	EffectVar(0, pTarget, iNumber) = 0; // Phase
	EffectVar(1, pTarget, iNumber) = 1000; // Stand weight
	EffectVar(2, pTarget, iNumber) = 0; // Walk weight
	EffectVar(3, pTarget, iNumber) = 0; // Run weight
	EffectVar(4, pTarget, iNumber) = 0;
	EffectVar(5, pTarget, iNumber) = 0;
	EffectVar(14, pTarget, iNumber) = 0; // Oldstate
	EffectVar(15, pTarget, iNumber) = 0; // Save wether the last frame was COMD_Stop

	EffectVar(17, pTarget, iNumber) = GetComDir(); // OldDir
	if(GetComDir() == COMD_Stop)
	{
		if(GetDir()) EffectVar(17, pTarget, iNumber) = COMD_Right;
		else EffectVar(17, pTarget, iNumber) = COMD_Left;
	}
	EffectVar(18, pTarget, iNumber) = 0; // Turn Phase
	EffectVar(19, pTarget, iNumber) = 0; // Wether to use Run turn or not
}

func FxIntWalkStop(pTarget, iNumber, iReason, fTmp)
{
	if(fTmp) return;
	for(var i = 0; i < GetLength(CLNK_WalkStates); i++)
		AnimationStop(CLNK_WalkStates[i]);
}

func FxIntWalkTimer(pTarget, iNumber, iTime)
{
	var iSpeed = Distance(0,0,GetXDir(),GetYDir());
  var iState = 0;

	// Check wether the clonk wants to turn
  if(EffectVar(17, pTarget, iNumber) != GetComDir())
	{
		// Not when he wants to stop
		if(GetComDir()!= COMD_Stop)
		{
			// Save new ComDir and start turn
			EffectVar(17, pTarget, iNumber) = GetComDir();
			EffectVar(18, pTarget, iNumber) = 1;
			// The weight of run and stand goes to their turning actions
			EffectVar(5, pTarget, iNumber) = EffectVar(3, pTarget, iNumber);
			EffectVar(3, pTarget, iNumber) = 0;
			EffectVar(4, pTarget, iNumber) = EffectVar(1, pTarget, iNumber);
			EffectVar(1, pTarget, iNumber) = 0;
			// Decide wether to use StandTurn or RunTurn
			if(iSpeed < 10)
				EffectVar(19, pTarget, iNumber) = 0;
			else
				EffectVar(19, pTarget, iNumber) = 1;
		}
	}
	// Turning
	if(EffectVar(18, pTarget, iNumber))
	{
		// Play animations
		AnimationSetState("StandTurn", EffectVar(18, pTarget, iNumber)*100, nil);
		AnimationSetState("RunTurn", EffectVar(18, pTarget, iNumber)*100, nil);
		// 
		if( ( EffectVar(17, pTarget, iNumber) == COMD_Left && GetDir() )
			|| ( EffectVar(17, pTarget, iNumber) == COMD_Right && !GetDir() ) )
			{
				SetObjDrawTransform(-1000, 0, 0, 0, 1000);
				//AnimationSetState("RunTurn", EffectVar(18, pTarget, iNumber)*100+2400, nil);
			}
			else SetObjDrawTransform(1000, 0, 0, 0, 1000);
		EffectVar(18, pTarget, iNumber) += 2;
		if(EffectVar(18, pTarget, iNumber) >= 24)
			EffectVar(18, pTarget, iNumber) = 0;
		iState = 4 + EffectVar(19, pTarget, iNumber);
	}
	// Play stand animation when not moving
	else if(iSpeed < 1 && EffectVar(15, pTarget, iNumber))
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
		if(EffectVar(14, pTarget, iNumber) != 3)
		{
			if(EffectVar(14, pTarget, iNumber) == 5)
				EffectVar(0, pTarget, iNumber) = 60; // start with frame 190 (feet on the floor)
			else
  			EffectVar(0, pTarget, iNumber) = 190; // start with frame 190 (feet on the floor)
		}
		else
  		EffectVar(0, pTarget, iNumber) += iSpeed*25/(16*3);
		if(EffectVar(0, pTarget, iNumber) > 250) EffectVar(0, pTarget, iNumber) -= 250;

		AnimationSetState("Run", EffectVar(0, pTarget, iNumber)*10, nil);
		iState = 3;
	}

  // Save wether he have COMD_Stop or not. So a single frame with COMD_Stop keeps the movement
  if(GetComDir() == COMD_Stop) EffectVar(15, pTarget, iNumber) = 1;
	else EffectVar(15, pTarget, iNumber) = 0;
	
	// Blend between the animations: The actuall animations gains weight till it reaches 1000
	// the other animations lose weight until they are at 0
	for(var i = 1; i <= 5; i++)
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
	EffectVar(14, pTarget, iNumber) = iState;
}
*/


/* Scale */

func StartScale()
{
	// TODO: Tweak animation speed
	PlayAnimation("Scale", 5, Anim_Y(0, GetAnimationLength("Scale"), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Set proper turn type
	SetTurnType(1);
	// Update carried items
	UpdateAttach();
}

/* Jump */

func StartJump()
{
	// TODO: Tweak animation speed
	PlayAnimation("Jump", 5, Anim_Linear(0, 0, GetAnimationLength("Jump"), 8*3, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
}

/* Hangle */

func StartHangle()
{
/*	if(CLNK_HangleStates == nil)
		CLNK_HangleStates = ["HangleStand", "Hangle"];*/
	if(!GetEffect("IntHangle", this))
		AddEffect("IntHangle", this, 1, 1, this);
	// Set proper turn type
	SetTurnType(1);
	// Update carried items
	UpdateAttach();
}

func StopHangle()
{
	if(GetAction() != "Hangle") RemoveEffect("IntHangle", this);
}

func FxIntHangleStart(pTarget, iNumber, fTmp)
{
	EffectVar(10, pTarget, iNumber) = GetPhysical("Hangle");
	if(fTmp) return;

	// EffectVars:
	// 0: whether the clonk is currently moving or not (<=> current animation is Hangle or HangleStand)
	// 1: Current animation number
	// 6: Player requested the clonk to stop 
	// 7: Whether the HangleStand animation is shown front-facing or back-facing
	// 10: Previous Hangle physical

	EffectVar(1, pTarget, iNumber) = PlayAnimation("HangleStand", 5, Anim_Linear(0, 0, 2000, 100, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

}

func FxIntHangleStop(pTarget, iNumber, iReasonm, fTmp)
{
	SetPhysical("Hangle", EffectVar(10, pTarget, iNumber), 2);
	if(fTmp) return;
}

func FxIntHangleTimer(pTarget, iNumber, iTime)
{
	// (TODO: Instead of EffectVar(0, pTarget, iNumber) we should be able
	// to query the current animation... maybe via a to-be-implemented
	// GetAnimationName() engine function.

	// If we are currently moving
	if(EffectVar(0, pTarget, iNumber))
	{
		// Use a cosine-shaped movement speed (the clonk only moves when he makes a "stroke")
		var iSpeed = 50-Cos(GetAnimationPosition(EffectVar(1, pTarget, iNumber))/10*360*2/1000, 50);
		SetPhysical("Hangle", EffectVar(10, pTarget, iNumber)/50*iSpeed, 2);

		// Exec movement animation (TODO: Use Anim_Linear?)
		var position = GetAnimationPosition(EffectVar(1, pTarget, iNumber));
		position += (EffectVar(10, pTarget, iNumber)/6000*1000/(14*2));

		SetAnimationPosition(EffectVar(1, pTarget, iNumber), Anim_Const(position % GetAnimationLength("Hangle")));

		// Continue movement, if the clonk still has momentum
		if(GetComDir() == COMD_Stop && iSpeed>10)
		{
			// Make it stop after the current movement
			EffectVar(6, pTarget, iNumber) = 1;

			if(GetDir())
				SetComDir(COMD_Right);
			else
				SetComDir(COMD_Left);
		}
		// Stop movement if the clonk has lost his momentum
		else if(iSpeed <= 10 && (GetComDir() == COMD_Stop || EffectVar(6, pTarget, iNumber)))
		{
			EffectVar(6, pTarget, iNumber) = 0;
			SetComDir(COMD_Stop);

			// and remeber the pose (front or back)
			if(GetAnimationPosition(EffectVar(1, pTarget, iNumber)) > 2500 && GetAnimationPosition(EffectVar(1, pTarget, iNumber)) < 7500)
				EffectVar(7, pTarget, iNumber) = 1;
			else
				EffectVar(7, pTarget, iNumber) = 0;

			// Change to HangleStand animation
			var begin = 4000*EffectVar(7, pTarget, iNumber);
			var end = 2000+begin;
			EffectVar(1, pTarget, iNumber) = PlayAnimation("HangleStand", 5, Anim_Linear(begin, begin, end, 100, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			EffectVar(0, pTarget, iNumber) = 0;
		}
	}
	else
	{
		// We are currently not moving
		if(GetComDir() != COMD_Stop)
		{
			// Switch to move
			EffectVar(0, pTarget, iNumber) = 1;
			// start with frame 100 or from the back hanging pose frame 600
			var begin = 10*(100 + 500*EffectVar(7, pTarget, iNumber));
			EffectVar(1, pTarget, iNumber) = PlayAnimation("Hangle", 5, Anim_Const(begin), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		}
	}
}

/* Swim */

func StartSwim()
{
/*	if(CLNK_SwimStates == nil)
		CLNK_SwimStates = ["SwimStand", "Swim", "SwimDive", "SwimTurn", "SwimDiveTurn", "SwimDiveUp", "SwimDiveDown"];*/
	if(!GetEffect("IntSwim", this))
		AddEffect("IntSwim", this, 1, 1, this);
}

func StopSwim()
{
	if(GetAction() != "Swim") RemoveEffect("IntSwim", this);
}

func FxIntSwimStart(pTarget, iNumber, fTmp)
{
	if(fTmp) return;

	EffectVar(0, pTarget, iNumber) = "SwimStand";
	EffectVar(1, pTarget, iNumber) = PlayAnimation("SwimStand", 5, Anim_Linear(0, 0, GetAnimationLength("SwimStand"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
/*
	for(var i = 0; i < GetLength(CLNK_SwimStates); i++)
  	AnimationPlay(CLNK_SwimStates[i], 0);
	EffectVar(0, pTarget, iNumber) = 0; // Phase
	EffectVar(1, pTarget, iNumber) = 1000; // Stand weight
	EffectVar(2, pTarget, iNumber) = 0; // Walk weight
	EffectVar(3, pTarget, iNumber) = 0; // Run weight
	EffectVar(4, pTarget, iNumber) = 0; // Oldstate
	EffectVar(5, pTarget, iNumber) = 0; // Save wether the last frame was COMD_Stop
	EffectVar(6, pTarget, iNumber) = 0; // OldRot

	EffectVar(7, pTarget, iNumber) = GetDir(); // OldDir
	EffectVar(8, pTarget, iNumber) = 0; // Turn Phase
	AnimationSetState("SwimStand", 0, 1000);*/

	// Set proper turn type
	SetTurnType(0);
	// Update carried items
	UpdateAttach();
	SetAnimationWeight(iTurnKnot2, Anim_Const(1000));
}

func FxIntSwimStop(pTarget, iNumber, iReason, fTmp)
{
	if(fTmp) return;
//	StopAnimation(GetRootAnimation(5));
}

func FxIntSwimTimer(pTarget, iNumber, iTime)
{
//	DoEnergy(1); //TODO Remove this! Endless Energy while diving is only for the testers

	var iSpeed = Distance(0,0,GetXDir(),GetYDir());

	// TODO: Smaller transition time between dive<->swim, keep 15 for swimstand<->swim/swimstand<->dive

	// Play stand animation when not moving
	if(Abs(GetXDir()) < 1 && !GBackSemiSolid(0, -4))
	{
		if(EffectVar(0, pTarget, iNumber) != "SwimStand")
		{
			EffectVar(0, pTarget, iNumber) = "SwimStand";
			EffectVar(1, pTarget, iNumber) = PlayAnimation("SwimStand", 5, Anim_Linear(0, 0, GetAnimationLength("SwimStand"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
		}
		SetAnimationWeight(iTurnKnot1, Anim_Const(0));
	}
	// Swimming
	else if(!GBackSemiSolid(0, -4))
	{
		// Animation speed by X
		if(EffectVar(0, pTarget, iNumber) != "Swim")
		{
			EffectVar(0, pTarget, iNumber) = "Swim";
			// TODO: Determine starting position from previous animation
			PlayAnimation("Swim", 5, Anim_AbsX(0, 0, GetAnimationLength("Swim"), 25), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
		}
		SetAnimationWeight(iTurnKnot1, Anim_Const(0));
	}
	// Diving
	else
	{
		if(EffectVar(0, pTarget, iNumber) != "SwimDive")
		{
			EffectVar(0, pTarget, iNumber) = "SwimDive";
			// TODO: Determine starting position from previous animation
			EffectVar(2, pTarget, iNumber) = PlayAnimation("SwimDiveUp", 5, Anim_Linear(0, 0, GetAnimationLength("SwimDiveUp"), 40, ANIM_Loop), Anim_Linear(0, 0, 1000, 15, ANIM_Remove));
			EffectVar(3, pTarget, iNumber) = PlayAnimation("SwimDiveDown", 5, Anim_Linear(0, 0, GetAnimationLength("SwimDiveDown"), 40, ANIM_Loop), Anim_Const(500), EffectVar(2, pTarget, iNumber));
			EffectVar(1, pTarget, iNumber) = EffectVar(3, pTarget, iNumber) + 1;

			// TODO: This should depend on which animation we come from
			// Guess for SwimStand we should fade from 0, otherwise from 90.
			EffectVar(4, pTarget, iNumber) = 90;
		}

		if(iSpeed)
		{
			var iRot = Angle(-Abs(GetXDir()), GetYDir());
			EffectVar(4, pTarget, iNumber) += BoundBy(iRot - EffectVar(4, pTarget, iNumber), -4, 4);
		}

		// TODO: Shouldn't weight go by sin^2 or cos^2 instead of linear in angle?
		var weight = 1000*EffectVar(4, pTarget, iNumber)/180;
		SetAnimationWeight(EffectVar(1, pTarget, iNumber), Anim_Const(1000 - weight));
		SetAnimationWeight(iTurnKnot1, Anim_Const(1000 - weight));
	}
}

/*
func FxIntScaleStart(pTarget, iNumber, fTmp)
{
	if(fTmp) return;
	AnimationPlay("Scale", 1000);
	EffectVar(0, pTarget, iNumber) = 0; // Phase
	EffectVar(1, pTarget, iNumber) = 1000; // Stand weight
	EffectVar(2, pTarget, iNumber) = 0; // Walk weight
	EffectVar(3, pTarget, iNumber) = 0; // Run weight
	EffectVar(4, pTarget, iNumber) = 0; // Oldstate
	EffectVar(5, pTarget, iNumber) = 0; // Save wether the last frame was COMD_Stop
}

func FxIntScaleStop(pTarget, iNumber, iReason, fTmp)
{
	if(fTmp) return;
	AnimationStop("Scale");
}

func FxIntScaleTimer(pTarget, iNumber, iTime)
{
//	if(GetAction() != "Walk") return -1;
	var iSpeed = -GetYDir();
  var iState = 0;

	// Play stand animation when not moving
	if(iSpeed < 1 && EffectVar(5, pTarget, iNumber))
	{
//		AnimationSetState("Stand", ((iTime/5)%11)*100, nil);
		iState = 2;
	}
	// When moving slowly play synchronized with movement walk
	else
	{
		EffectVar(0, pTarget, iNumber) +=  iSpeed*20/(16*1);
		if(EffectVar(0, pTarget, iNumber) < 0) EffectVar(0, pTarget, iNumber) += 200;
		if(EffectVar(0, pTarget, iNumber) > 200) EffectVar(0, pTarget, iNumber) -= 200;

		AnimationSetState("Scale", EffectVar(0, pTarget, iNumber)*10, nil);
		iState = 2;
	}

  // Save wether he have COMD_Stop or not. So a single frame with COMD_Stop keeps the movement
  if(GetComDir() == COMD_Stop) EffectVar(5, pTarget, iNumber) = 1;
	else EffectVar(5, pTarget, iNumber) = 0;

	// Blend between the animations: The actuall animations gains weight till it reaches 1000
	// the other animations lose weight until they are at 0
	for(var i = 1; i <= 1; i++)
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
//		AnimationSetState(CLNK_WalkStates[i-1], nil, EffectVar(i, pTarget, iNumber));
	}
	EffectVar(4, pTarget, iNumber) = iState;
}*/

func StartDigging()
{
	if(!GetEffect("IntDig", this))
		AddEffect("IntDig", this, 1, 1, this);
}

func StopDigging()
{
	if(GetAction() != "Dig") RemoveEffect("IntDig", this);
}

func FxIntDigStart(pTarget, iNumber, fTmp)
{
	if(fTmp) return;
	EffectVar(1, pTarget, iNumber) = PlayAnimation("Dig", 5, Anim_Linear(0, 0, GetAnimationLength("Dig"), 36, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));

	// Update carried items
	UpdateAttach();

  // Sound
	Digging();

	// Set proper turn type
	SetTurnType(0);
}

func FxIntDigTimer(pTarget, iNumber, iTime)
{
	if(iTime % 36 == 0)
	{
		Digging();
	}
	if( (iTime-18) % 36 == 0)
	{
		var pShovel = FindObject(Find_ID(SHVL), Find_Container(this));
		if(!pShovel || !pShovel->IsDigging())
		{
			SetAction("Walk");
			SetComDir(COMD_Stop);
			return -1;
		}
	}
}

func StartDead()
{
	PlayAnimation("Dead", 5, Anim_Linear(0, 0, GetAnimationLength("Dead"), 20, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(1);
}

func StartTumble()
{
	PlayAnimation("Tumble", 5, Anim_Linear(0, 0, GetAnimationLength("Tumble"), 20, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
	// Update carried items
	UpdateAttach();
	// Set proper turn type
	SetTurnType(0);
}

/* Act Map */

func Definition(def) {
  SetProperty("ActMap", {

Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Directions = 2,
	FlipDir = 0,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartWalk",
	AbortCall = "StopWalk",
	InLiquidAction = "Swim",
},
Stand = {
	Prototype = Action,
	Name = "Stand",
	Procedure = DFA_THROW,
	Directions = 2,
	FlipDir = 0,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 0,
	Wdt = 8,
	Hgt = 20,
	StartCall = "StartStand",
	InLiquidAction = "Swim",
},
Scale = {
	Prototype = Action,
	Name = "Scale",
	Procedure = DFA_SCALE,
  Attach = CNAT_MultiAttach,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 20,
	Wdt = 8,
	Hgt = 20,
	OffX = 0,
	OffY = 0,
	StartCall = "StartScale",
},
Tumble = {
	Prototype = Action,
	Name = "Tumble",
	Procedure = DFA_FLIGHT,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 40,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Tumble",
	ObjectDisabled = 1,
	InLiquidAction = "Swim",
	StartCall = "StartTumble",
	EndCall = "CheckStuck",
},
Dig = {
	Prototype = Action,
	Name = "Dig",
	Procedure = DFA_DIG,
	Directions = 2,
	Length = 16,
	Delay = 15*3*0,
	X = 0,
	Y = 60,
	Wdt = 8,
	Hgt = 20,
	NextAction = "Dig",
	StartCall = "StartDigging",
	AbortCall = "StopDigging",
	DigFree = 11,
	InLiquidAction = "Swim",
	Attach = CNAT_Left | CNAT_Right | CNAT_Bottom,
},
Bridge = {
	Prototype = Action,
	Name = "Bridge",
	Procedure = DFA_THROW,
	Directions = 2,
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
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 80,
	Wdt = 8,
	Hgt = 20,
	OffX = 0,
	OffY = 2,
	StartCall = "StartSwim",
	AbortCall = "StopSwim",
},
Hangle = {
	Prototype = Action,
	Name = "Hangle",
	Procedure = DFA_HANGLE,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 100,
	Wdt = 8,
	Hgt = 20,
	OffX = 0,
	OffY = 3,
	StartCall = "StartHangle",
	AbortCall = "StopHangle",
	InLiquidAction = "Swim",
},
Jump = {
	Prototype = Action,
	Name = "Jump",
	Procedure = DFA_FLIGHT,
	Directions = 2,
	Length = 1,
	Delay = 0,
	X = 0,
	Y = 120,
	Wdt = 8,
	Hgt = 20,
	InLiquidAction = "Swim",
	PhaseCall = "CheckStuck",
//	Animation = "Jump",
	StartCall = "StartJump",
},
Dive = {
	Prototype = Action,
	Name = "Dive",
	Procedure = DFA_FLIGHT,
	Directions = 2,
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
Dead = {
	Prototype = Action,
	Name = "Dead",
	Directions = 2,
	X = 0,
	Y = 240,
	Wdt = 8,
	Hgt = 20,
	Length = 1,
	Delay = 0,
	NextAction = "Hold",
	StartCall = "StartDead",
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
}, def);
  SetProperty("Name", "Clonk", def);

  // Set perspective
  SetProperty("PerspectiveR", 12000, def);
  SetProperty("PerspectiveTheta", 20, def);
  SetProperty("PerspectivePhi", 70, def);
}

