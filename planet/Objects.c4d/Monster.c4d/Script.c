/*-- Monster --*/

#strict 2
//#include ANIM

public func IsPossessible() { return 1; }

/* Initialisierung */

protected func Initialize() { SetAction("Walk"); }

/* TimerCall mit KI-Steuerung */

private func Activity()
{
  // Die KI-Steuerung wird bei Besessenheit nicht gebraucht
  if (GetEffect("PossessionSpell", this)) return;

  // Nichts machen
  if (Random(2) || (GetAction() != "Walk" && GetAction() != "Swim")) return 1;

  // Springen
/*  if (GetAction() == "Walk")
   if (!Random(3)) return DoJump();

  // Umsehen
  if (GetAction() == "Walk")
   if (!Random(8)) return SetAction("LookUp");*/

  // Umdrehen
  if (Random(2)) return TurnRight();
  return TurnLeft();
}

/* Kontakte */

protected func ContactLeft()
{
  // Die KI-Steuerung wird bei Besessenheit nicht gebraucht
  if (GetEffect("PossessionSpell", this)) return;
  
  return TurnRight();
}

protected func ContactRight()
{
  // Die KI-Steuerung wird bei Besessenheit nicht gebraucht
  if (GetEffect("PossessionSpell", this)) return;
  
  return TurnLeft();
}

/* Aktionen */

public func TurnRight()
{
  if (Stuck() || (GetAction() != "Walk" && GetAction() != "Swim")) return;
  if (GetXDir() < 0) SetXDir(0);
  SetDir(DIR_Right);
  SetComDir(COMD_Right); 
  return 1;
}

public func TurnLeft()
{
  if (Stuck() || (GetAction() != "Walk" && GetAction() != "Swim")) return;
  if (GetXDir() > 0) SetXDir(0);
  SetDir(DIR_Left);
  SetComDir(COMD_Left); 
  return 1;
}

private func HitCheck()
{
/*  var obj;
  if (obj = FindObject(0, +1,0,0,0, OCF_Prey, 0,0, NoContainer()))
   Punch(obj, 10);*/
  return 1;
}

public func DoJump()
{
  if (GetAction() != "Walk") return;
  if (Random(2)) Sound("Growl*");
  Jump();
}

/* Einwirkungen */

protected func Death()
{
  Sound("DeathGrowl");
  SetDir(DIR_Left);
  //ChangeDef(DMNS);
  SetAction("Dead");
  return 1;
}

/* Vermehrung */

private func ReproductionRate() { return 2000; } // Die Chance, dass in einem Timerintervall eine Vermehrung stattfindet
private func MaxAnimalCount()   { return 4; }    // Maximale Tieranzahl im Umkreis

/* Steuerung durch Besessenheit */

protected func ControlCommand(szCommand, pTarget, iTx, iTy)
{
 // Bewegungskommando
 if (szCommand == "MoveTo")
  return SetCommand(this,szCommand, pTarget, iTx, iTy);
 return 0;
}

protected func ContainedLeft(object caller)
{
 [$TxtMovement$]
 SetCommand(this, "None");
 if(!GetPlrCoreJumpAndRunControl(caller->GetController()))
  TurnLeft();
 return 1;
}

protected func ContainedRight(object caller)
{
 [$TxtMovement$]
 SetCommand(this, "None");
 if(!GetPlrCoreJumpAndRunControl(caller->GetController()))
  TurnRight();
 return 1;
}

protected func ContainedUp(object caller)
{
 [$TxtMovement$]
 SetCommand(this, "None");
 if(Contained()) return SetCommand(this, "Exit");
 if (GetAction() == "Swim")
 {
  if(!GetPlrCoreJumpAndRunControl(caller->GetController()))
   SetComDir(COMD_Up);

  return 1;
 }

 DoJump();
 return 1;
}

protected func ContainedDown(object caller)
{
 [$TxtMovement$]
 SetCommand(this, "None");

 if (GetAction() == "Swim")
 {
  if(!GetPlrCoreJumpAndRunControl(caller->GetController()))
   SetComDir(COMD_Down);
  return 1;
 }

 if (GetAction() == "Walk")
  SetAction("LookUp");
  
 return 1;
}

/* JumpAndRun Steuerung */

private func ClearDir(bool fX)
{
  if(fX && GetXDir())
  {
    if(GetXDir() > 0) SetXDir(Max(GetXDir() - 2, 0));
    else SetXDir(Min(GetXDir() + 2, 0));
  }
  if(!fX && GetYDir())
  {
    if(GetYDir() > 0) SetYDir(Max(GetYDir() - 2, 0));
    else SetYDir(Min(GetYDir() + 2, 0));
  }
}

public func ContainedUpdate(object self, int comdir)
{
  if(GetAction() == "Swim")
  {
    SetComDir(comdir);
    ClearScheduleCall(this, "ClearDir");
    if(comdir == COMD_Down || comdir == COMD_Up) ScheduleCall(this, "ClearDir", 1, (Abs(GetXDir())+1)/2, true);
    if(comdir == COMD_Left || comdir == COMD_Right) ScheduleCall(this, "ClearDir", 1, (Abs(GetYDir())+1)/2, false);
  }
  else
  {
    if(comdir == COMD_UpRight || comdir == COMD_DownRight) comdir = COMD_Right;
    if(comdir == COMD_Up || comdir == COMD_Down) comdir = COMD_Stop;
    if(comdir == COMD_UpLeft || comdir == COMD_DownLeft) comdir = COMD_Left;
    SetComDir(comdir);
  }

  return 1;
}

protected func ContainedThrow() { return 1; }

protected func ContainedDigDouble()
{
 [$TxtLeave$]
 RemoveEffect("PossessionSpell", this);
 return 1;
}
func Definition(def) {
  SetProperty("ActMap", {
Walk = {
Prototype = Action,
Name = "Walk",
Procedure = DFA_WALK,
Directions = 2,
FlipDir = 1,
Length = 16,
Delay = 5,
X = 0,
Y = 0,
Wdt = 48,
Hgt = 34,
NextAction = "Walk",
TurnAction = "Turn",
StartCall = "HitCheck",
InLiquidAction = "Swim",
Animation = "Walk",
},
Turn = {
Prototype = Action,
Name = "Turn",
Procedure = DFA_NONE,
Directions = 2,
FlipDir = 1,
Length = 7,
Delay = 2,
X = 0,
Y = 68,
Wdt = 48,
Hgt = 34,
NextAction = "Walk",
},
Jump = {
Prototype = Action,
Name = "Jump",
Procedure = DFA_FLIGHT,
Directions = 2,
FlipDir = 1,
Length = 17,
Delay = 1,
X = 0,
Y = 34,
Wdt = 48,
Hgt = 34,
NextAction = "Hold",
InLiquidAction = "Swim",
},
Swim = {
Prototype = Action,
Name = "Swim",
Procedure = DFA_SWIM,
Directions = 2,
FlipDir = 1,
Length = 16,
Delay = 5,
X = 0,
Y = 0,
Wdt = 48,
Hgt = 34,
NextAction = "Swim",
TurnAction = "Turn",
StartCall = "HitCheck",
},
LookUp = {
Prototype = Action,
Name = "LookUp",
Procedure = DFA_NONE,
Attach = 8,
Directions = 2,
FlipDir = 1,
Length = 12,
Delay = 1,
X = 0,
Y = 102,
Wdt = 48,
Hgt = 34,
NextAction = "Look",
InLiquidAction = "Swim",
},
Look = {
Prototype = Action,
Name = "Look",
Attach = 8,
Procedure = DFA_NONE,
Directions = 2,
FlipDir = 1,
Delay = 15,
X = 528,
Y = 102,
Wdt = 48,
Hgt = 34,
NextAction = "LookAway",
InLiquidAction = "Swim",
},
LookAway = {
Prototype = Action,
Name = "LookAway",
Attach = 8,
Procedure = DFA_NONE,
Directions = 2,
FlipDir = 1,
Length = 12,
Delay = 1,
Reverse = 1,
X = 0,
Y = 102,
Wdt = 48,
Hgt = 34,
NextAction = "Walk",
InLiquidAction = "Swim",
},  }, def);
  SetProperty("Name", "$Name$", def);
}
