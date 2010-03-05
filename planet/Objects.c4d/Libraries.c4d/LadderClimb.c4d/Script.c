/*--
	Ladder climbing
	Authors: Randrian

	Gives the ability to climb on ladders
--*/

#strict 2

local jump_startcall;
local no_ladder_counter;

func Initialize()
{
	// add action
	SetProperty("Climb", {
		Prototype = Action,
		Name = "Climb",
		Directions = 2,
		Length = 0,
		Delay = 0,
		Wdt = 8,
		Hgt = 20,
		Procedure = DFA_FLOAT,
		StartCall = "StartScale",
	}, GetProperty("ActMap"));
	// save old phasecall of jump
	jump_startcall = GetProperty("StartCall", GetProperty("Jump", GetProperty("ActMap")));
	// and add new one
	SetProperty("StartCall", "StartSearchLadder", GetProperty("Jump", GetProperty("ActMap")));
	return _inherited(...);
}

func StartScale()
{
	// Should be overloaded, and add a climb animation here
	return _inherited(...);
}

func StartSearchLadder()
{
	// call overwriten old phase call
	if(jump_startcall != nil && jump_startcall != "StartSearchLadder")
		Call(jump_startcall);
	ScheduleCall(this, "SearchLadder", 1);
}

func SearchLadder()
{
	var ladder;
	if(!no_ladder_counter) ladder = FindObject(Find_AtRect(-5,-5,10,10), Find_Func("IsLadder"));
	else no_ladder_counter--;
	// Found ladder?
	if(ladder != nil)
	{
		SetAction("Climb");
		Message("Ladder", this);
		ladder->~OnLadderGrab(this);
		AddEffect("IntClimbControl", this, 1, 1, this, 0, ladder);
		return true;
	}
	ScheduleCall(this, "SearchLadder", 1);
}

func FxIntClimbControlStart(target, number, tmp, ladder)
{
	if(tmp) return;
	EffectVar(0, target, number) = ladder;
	SetXDir(0); SetYDir(0);
	SetComDir(COMD_Stop);
	no_ladder_counter = 20;
	EffectVar(2, target, number) = 0; // angle
}

func FxIntClimbControlTimer(target, number)
{
	if(GetAction() != "Climb") return -1;
	// Progress
	if(GetComDir() == COMD_Up)
	{
		EffectVar(1, target, number) += 10;
		if(EffectVar(1, target, number) > 100)
		{
			EffectVar(1, target, number) = 0;
			EffectVar(0, target, number) = EffectVar(0, target, number)->GetNextLadder();
		}
		if(EffectVar(0, target, number) == nil)
		{
			SetAction("Jump");
			SetXDir(-5+10*GetDir());
			SetYDir(-5);
			return -1;
		}
		if(EffectVar(0, target, number)) EffectVar(0, target, number)->~OnLadderClimb(this);
	}
	if(GetComDir() == COMD_Down)
	{
		EffectVar(1, target, number) -= 10;
		if(EffectVar(1, target, number) < 0)
		{
			EffectVar(1, target, number) = 100;
			EffectVar(0, target, number) = EffectVar(0, target, number)->GetPreviousLadder();
		}
		if(EffectVar(0, target, number) == nil)
		{
			SetAction("Jump");
			return -1;
		}
		if(EffectVar(0, target, number)) EffectVar(0, target, number)->~OnLadderClimb(this);
	}
	var startx, starty, endx, endy, angle;
	EffectVar(0, target, number)->GetLadderData(startx, starty, endx, endy, angle);
	var x = startx + (endx-startx)*EffectVar(1, target, number)/100+5-10*GetDir();
	var y = starty + (endy-starty)*EffectVar(1, target, number)/100;
	var old_x = GetX(), old_y = GetY();
	SetPosition(x, y);
	SetDTRotation(-angle);//EffectVar(2, target, number));
	if(Stuck())
	{
		SetPosition(old_x, old_y);
	}
	var contact = GetContact(-1);
	if(contact)
	{
		if(contact & CNAT_Top)
		{
			SetAction("Hangle");
			return -1;
		}
		if(contact & CNAT_Bottom)
		{
			SetAction("Walk");
			return -1;
		}
		if(contact & CNAT_Left || contact & CNAT_Right)
		{
			SetAction("Scale");
			return -1;
		}
	}
}

func FxIntClimbControlStop(target, number)
{
	SetDTRotation(0);
}

func FxIntClimbControlControl(target, number, ctrl, x,y,strength, repeat, release)
{
	if(ctrl != CON_Up && ctrl != CON_Down && ctrl != CON_Right && ctrl != CON_Left) return;
	if(release == 1) return;
//	Log("%v %v %v %v %v %v", ctrl, x, y, strength, repeat, release);
	if(ctrl == CON_Up)   SetComDir(COMD_Up);
	else if(ctrl == CON_Down) SetComDir(COMD_Down);
	else if(ctrl == CON_Left)
	{
		if(GetDir() == 0)
		{
			if(GetComDir() == COMD_Stop)
			{
				SetComDir(COMD_Right);
				SetDir(1);
			}
			SetComDir(COMD_Stop);
		}
		else
		{
			SetAction("Jump");
			SetXDir(-5);
		}
	}
	else if(ctrl == CON_Right)
	{
		if(GetDir() == 1)
		{
			if(GetComDir() == COMD_Stop)
			{
				SetComDir(COMD_Left);
				SetDir(0);
			}
			SetComDir(COMD_Stop);
		}
		else
		{
		SetAction("Jump");
		SetXDir(+5);
		}
	}
	return 1;
}

global func SetDTRotation (int r, int xoff, int yoff) {
  var fsin=Sin(r, 1000), fcos=Cos(r, 1000);
  // set matrix values
  SetObjDrawTransform (
    +fcos, +fsin, (1000-fcos)*xoff - fsin*yoff,
    -fsin, +fcos, (1000-fcos)*yoff + fsin*xoff,
  );
}