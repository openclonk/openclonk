/*-- Wolf --*/

#include Library_Animal

public func IsPossessible() { return 1; }

/* Initialize */

protected func Initialize() { return Birth(); }

/* TimerCall with AI-control */

protected func Activity()
{
	// With possession the ai control isn't needed
	if (GetEffect("PossessionSpell", this)) return;

	// The following actions only outdoor
	if (Contained()) return;

	// Just when walking or swiming
	if (GetAction() != "Walk" && GetAction() != "Swim") return;

	// Emerge, if the breath is short
	if (InLiquid() && GetBreath() <= 25 && GetComDir() != COMD_Up)
		SetComDir(COMD_Up);

	// Do nothing
	if (Random(2)) return;

	// Reproduction
	if (!Random(ReproductionRate()))
		Reproduction();

	// Jump
	if (GetAction() == "Walk")
		if (!Random(3))
			return Jump();

	// Turn
	if (Random(2)) return TurnRight();
	return TurnLeft();
}

/* Contact */

protected func ContactLeft()
{
	// With possession the ai control isn't needed
	if (GetEffect("PossessionSpell", this)) return;
	
	return TurnRight();
}

protected func ContactRight()
{
	// With possession the ai control isn't needed
	if (GetEffect("PossessionSpell", this)) return;
	
	return TurnLeft();
}

/* Actions */

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

private func DigFree()
{
	SetAction("DigFree");
	SetComDir(3+GetDir()*4);
	return 1;
}

/* Damage */

protected func CatchBlow()
{
	if (GetAction() == "Dead") return 0;
	if (!Random(3)) Sound("WolfHurt"); // TODO Get Sound
	return 1;
}

protected func Death()
{
	Sound("WolfDead"); // TODO Get Sound
	SetDir(0);
	SetAction("Dead");
	return 1;
}

/* Reproduction */

private func ReproductionRate() { return 1000; } // The chance that in one timer invervall a reproduction takes place

/* Control with possession */

protected func ControlCommand(szCommand, pTarget, iTx, iTy)
{
	// Move command
	if (szCommand == "MoveTo")
		return SetCommand(szCommand, pTarget, iTx, iTy);
	return 0;
}

protected func ContainedLeft(object caller)
{
	[$TxtMovement$]
	SetCommand("None");
	return 1;
}

protected func ContainedRight(object caller)
{
	[$TxtMovement$]
	SetCommand("None");
	return 1;
}

protected func ContainedUp(object caller)
{
	[$TxtMovement$]
	SetCommand("None");
	
	if (GetAction() == "Swim")
	{
		return 1;
	}
	
	Jump();
	return 1;
}

protected func ContainedDown(object caller)
{
	[$TxtMovement$]
	SetCommand("None");
	if(Contained()) SetCommand("Exit");
	if (GetAction() == "Swim")
	{
		return 1;
	}
	
	return 1;
}

/* JumpAndRun Control */

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

public func ContainedUpdate(object self, int comdir, bool dig, bool throw)
{
	if(GetAction() == "Swim")
	{
		SetComDir(comdir);
		ClearScheduleCall(this(), "ClearDir");
		if(comdir == COMD_Down || comdir == COMD_Up) ScheduleCall(this(), "ClearDir", 1, (Abs(GetXDir())+1)/2, true);
		if(comdir == COMD_Left || comdir == COMD_Right) ScheduleCall(this(), "ClearDir", 1, (Abs(GetYDir())+1)/2, false);
	}
	else if(GetAction() == "Dig")
	{
		if(comdir == COMD_Stop && dig) return;
		SetComDir(comdir);
	}
	else
	{
		if(comdir == COMD_UpRight || comdir == COMD_DownRight) comdir = COMD_Right;
		if(comdir == COMD_Up || comdir == COMD_Down) comdir = COMD_Stop;
		if(comdir == COMD_UpLeft || comdir == COMD_DownLeft) comdir = COMD_Left;

		if(comdir == COMD_Right) TurnRight();
		else if(comdir == COMD_Left) TurnLeft();
		else SetComDir(comdir);
	}

	return 1;
}

protected func ContainedThrow()
{
	[$TxtDrop$]
	var iEffectNumber, pSorcerer;
	if (iEffectNumber = GetEffect("PossessionSpell", this))
		if (pSorcerer = EffectVar(0, this, iEffectNumber))
		{
			if (pSorcerer->Contents()) pSorcerer->Contents()->Exit(0,0,6);
			AddEffect("IntCollectionDelay", this(), 1, 70);
		}
	return 1;
}

protected func ContainedDigDouble()
{
	[$TxtLeave$]
	RemoveEffect("PossessionSpell", this);
	return 1;
}

local ActMap = {
Walk = {
	Prototype = Action,
	Name = "Walk",
	Procedure = DFA_WALK,
	Accel = 16,
	Decel = 22,
	Speed = 224,
	Directions = 2,
	FlipDir = 1,
	Length = 14,
	Delay = 20,
	X = 0,
	Y = 0,
	Wdt = 26,
	Hgt = 18,
	NextAction = "Walk",
	Animation = "Walk",
},
Run = {
	Prototype = Action,
	Name = "Run",
	Procedure = DFA_WALK,
	Accel = 16,
	Decel = 22,
	Speed = 224,
	Directions = 2,
	FlipDir = 1,
	Length = 18,
	Delay = 15,
	X = 0,
	Y = 18,
	Wdt = 26,
	Hgt = 18,
	NextAction = "Run",
	Animation = "Run",
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
	Y = 0,
	Wdt = 26,
	Hgt = 18,
	NextAction = "Hold",
	InLiquidAction = "Swim",
	Animation = "Walk",
},
Snarl = {
	Prototype = Action,
	Name = "Snarl",
	Procedure = DFA_NONE,
	Attach = 8,
	Directions = 2,
	FlipDir = 1,
	Length = 12,
	Delay = 1,
	X = 0,
	Y = 18*2,
	Wdt = 26,
	Hgt = 18,
	NextAction = "Hold",
	InLiquidAction = "Swim",
	Animation = "Snarl",
},
Lunge = {
	Prototype = Action,
	Name = "Lunge",
	Procedure = DFA_FLIGHT,
	Directions = 2,
	FlipDir = 1,
	Length = 22,
	Delay = 1,
	X = 0,
	Y = 18*3,
	Wdt = 26,
	Hgt = 18,
	NextAction = "Hold",
	InLiquidAction = "Swim",
	Animation = "Lunge",
},
Land = {
	Prototype = Action,
	Name = "Land",
	Procedure = DFA_NONE,
	Attach = 8,
	Directions = 2,
	FlipDir = 1,
	Length = 17,
	Delay = 1,
	X = 0,
	Y = 18*4,
	Wdt = 26,
	Hgt = 18,
	NextAction = "Walk",
	InLiquidAction = "Swim",
	Animation = "Land",
},
Swim = {
	Prototype = Action,
	Name = "Swim",
	Procedure = DFA_SWIM,
	Speed = 96,
	Accel = 7,
	Directions = 2,
	FlipDir = 1,
	Length = 16,
	Delay = 5,
	X = 0,
	Y = 0,
	Wdt = 26,
	Hgt = 18,
	NextAction = "Swim",
	//TurnAction = "Turn",
	StartCall = "HitCheck",
	Animation = "Walk",
},
Wait = {
	Prototype = Action,
	Name = "Wait",
	Procedure = DFA_NONE,
	Attach = 8,
	Directions = 2,
	FlipDir = 1,
	Length = 10,
	Delay = 1,
	X = 0,
	Y = 18*6,
	Wdt = 26,
	Hgt = 18,
	NextAction = "Walk",
	InLiquidAction = "Swim",
	Animation = "Idle",
},
Howl = {
	Prototype = Action,
	Name = "Howl",
	Procedure = DFA_NONE,
	Attach = 8,
	Directions = 2,
	FlipDir = 1,
	Length = 45,
	Delay = 1,
	X = 0,
	Y = 18*7,
	Wdt = 26,
	Hgt = 18,
	NextAction = "Walk",
	InLiquidAction = "Swim",
	Animation = "Howl",
},
Bite = {
	Prototype = Action,
	Name = "Bite",
	Attach = 8,
	Procedure = DFA_NONE,
	Directions = 2,
	FlipDir = 1,
	Length = 12,
	Delay = 15,
	X = 0,
	Y = 18*5,
	Wdt = 26,
	Hgt = 18,
	NextAction = "Walk",
	InLiquidAction = "Swim",
	Animation = "Bite",
},
Dead = {
	Prototype = Action,
	Name = "Dead",
	Directions = 2,
	FlipDir = 1,
	X = 0,
	Y = 18*8,
	Wdt = 18,
	Hgt = 18,
	Length = 10,
	Delay = 3,
	NextAction = "Hold",
	NoOtherAction = 1,
	ObjectDisabled = 1,
	Animation = "Death",
},
};
local Name = "$Name$";
