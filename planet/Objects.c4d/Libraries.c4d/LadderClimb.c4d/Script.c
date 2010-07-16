/*--
	Ladder climbing
	Authors: Randrian

	Gives the ability to climb on ladders
--*/

local jump_startcall;
local no_ladder_counter;

func GetTurnPhase() { return _inherited(...); }

func Definition(def) {
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
	}, GetProperty("ActMap"));
	// save old phasecall of jump
	var jump_startcall = GetProperty("StartCall", GetProperty("Jump", GetProperty("ActMap")));
	// and add new one
	SetProperty("StartCall", "StartSearchLadder", GetProperty("Jump", GetProperty("ActMap")));
	SetProperty("StartCallLadderOverloaded", jump_startcall, GetProperty("Jump", GetProperty("ActMap")));
	_inherited(def);
}

func StartScale()
{
	// Should be overloaded, and add a climb animation here
	return _inherited(...);
}

func StartSearchLadder()
{
	// call overwriten old phase call
	if (GetProperty("StartCallLadderOverloaded", GetProperty("Jump", GetProperty("ActMap"))))
		Call(GetProperty("StartCallLadderOverloaded", GetProperty("Jump", GetProperty("ActMap"))));
	if (!GetEffect("InSearchLadder", this))
	{
		AddEffect("IntSearchLadder", this, 1, 5, this);
	}
	FxIntSearchLadderTimer();
}

func GetLadderScaleAnimation()
{
	var animation = _inherited(...);
	if(animation) return animation;
	return "Scale";
}

func FxIntSearchLadderTimer(target, number, time)
{
	if (GetAction() != "Jump") return -1;
	var ladder;
	if (!no_ladder_counter) 
		ladder = FindObject(Find_AtRect(-5,-5,10,10), Find_Func("IsLadder"));
	else 
		no_ladder_counter--;
	// Found ladder?
	if (ladder != nil)
	{
		SetAction("Climb");
		ladder->~OnLadderGrab(this);
		PlayAnimation(GetLadderScaleAnimation(), 5, Anim_Y(0, GetAnimationLength(GetLadderScaleAnimation()), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		AddEffect("IntClimbControl", this, 1, 1, this, 0, ladder);
		return -1;
	}
}

func FxIntSearchLadderStop(target, number, reason, tmp)
{
	if (tmp) return;
	no_ladder_counter = 0; 
}

func FxIntClimbControlStart(target, number, tmp, ladder)
{
	if (tmp) return;
	EffectVar(0, target, number) = ladder;
	SetXDir(0); 
	SetYDir(0);
	SetComDir(COMD_Stop);
	EffectVar(2, target, number) = 0; // odd or even segment?
	SetHandAction(1);
	SetTurnType(1);
}

func SetTurnType() { return _inherited(...); }
func SetHandAction() { return _inherited(...); }

func LadderStep(target, number, fUp)
{
	if (fUp == 1)
	{
		EffectVar(1, target, number) += 10;
		if (EffectVar(1, target, number) > 100)
		{
			EffectVar(1, target, number) = 0;
			EffectVar(0, target, number) = EffectVar(0, target, number)->GetNextLadder();
			EffectVar(2, target, number) = !EffectVar(2, target, number);
		}
		if (EffectVar(0, target, number) == nil)
		{
			var contact = GetContact(-1);
			if (contact & CNAT_Left || contact & CNAT_Right)
			{
				SetAction("Scale");
				return 0;
			}
			no_ladder_counter = 5;
			SetAction("Jump");
			SetXDir(-5+10*GetDir());
			SetYDir(-5);
			return 0;
		}
	}
	else
	{
		EffectVar(1, target, number) -= 10;
		if(EffectVar(1, target, number) < 0)
		{
			EffectVar(1, target, number) = 100;
			EffectVar(0, target, number) = EffectVar(0, target, number)->GetPreviousLadder();
			EffectVar(2, target, number) = !EffectVar(2, target, number);
		}
		if (EffectVar(0, target, number) == nil)
		{
			var contact = GetContact(-1);
			if (contact & CNAT_Left || contact & CNAT_Right)
			{
				SetAction("Scale");
				return 0;
			}
			no_ladder_counter = 5;
			SetAction("Jump");
			return 0;
		}
	}
	if (EffectVar(0, target, number) == nil) return 0;
	return true;
}

func FxIntClimbControlTimer(target, number)
{
	if (GetAction() != "Climb") return -1;
	if(!EffectVar(0, target, number))
	{
		SetAction("Jump");
		SetXDir(-5+10*GetDir());
		SetYDir(-5);
		return -1;
	}
	// Progress
	var step = 0;
	if (GetComDir() == COMD_Down) step = -1;
	if (GetComDir() == COMD_Up)   step = 1;

	if (step && LadderStep(target, number, step) == 0)
	{
		var contact = GetContact(-1);
		if (contact & CNAT_Left || contact & CNAT_Right)
				SetAction("Scale");
		else
		{
			no_ladder_counter = 5;
			SetAction("Jump");
			if (step == 1) // For Up add some speed
			{
				SetXDir(-5+10*GetDir());
				SetYDir(-5);
				no_ladder_counter = 10;
			}
		}
		return -1;
	}
	var startx, starty, endx, endy, angle;
	EffectVar(0, target, number)->GetLadderData(startx, starty, endx, endy, angle);
	var x = startx + (endx-startx)*EffectVar(1, target, number)/100+5000-100*GetTurnPhase();
	var y = starty + (endy-starty)*EffectVar(1, target, number)/100;
	var old_x = GetX(), old_y = GetY();
	SetPosition(LadderToLandscapeCoordinates(x), LadderToLandscapeCoordinates(y));
	SetXDir(0); SetYDir(0);
	SetLadderRotation(-angle, x-GetX()*1000, y-GetY()*1000);//EffectVar(2, target, number));
	if (Stuck())
	{
		var dir = -1;
		if (GetDir() == 0) dir = 1;
		for (var i = 1; i <= 5; i++)
		{
			SetPosition(LadderToLandscapeCoordinates(x)+i*dir, LadderToLandscapeCoordinates(y));
			if (!Stuck()) break;
		}
		if (Stuck()) SetPosition(LadderToLandscapeCoordinates(x)+5*dir, LadderToLandscapeCoordinates(y));
	}
	if (Stuck())
	{
		// Revert Position and step
		SetPosition(old_x, old_y);
		if (step) LadderStep(target, number, -step);
		// if we are to far left or right try to turn
		if (GetDir() == 0 && LadderToLandscapeCoordinates(x) > GetX())
		{
			SetComDir(COMD_Right);
			SetDir(1);
		}
		else if (GetDir() == 1 && LadderToLandscapeCoordinates(x) < GetX())
		{
			SetComDir(COMD_Left);
			SetDir(0);
		}
	}
	else EffectVar(0, target, number)->~OnLadderClimb(this);
	// Make the animation synchron with movement TODO: this only makes the feet synchronous for the arms the animation has to be adapted
	var animation = GetRootAnimation(5);
	if (animation != nil)
	{
		if (GetAnimationName(animation) != nil)
		{
			var length = GetAnimationLength(GetAnimationName(animation));
			SetAnimationPosition(animation, Anim_Const(EffectVar(1, target, number)*length/200+length/2*EffectVar(2, target, number)));
		}
	}
	var contact = GetContact(-1);
	if (contact)
	{
		if (contact & CNAT_Top && GetComDir() == COMD_Up)
		{
			SetAction("Hangle");
			if (GetDir() == 0)
			{
				SetComDir(COMD_Right);
				SetDir(1);
			}
			else
			{
				SetComDir(COMD_Left);
				SetDir(0);
			}
			return -1;
		}
		if (contact & CNAT_Bottom && GetComDir() == COMD_Down)
		{
			SetAction("Walk");
			return -1;
		}
/*		if(contact & CNAT_Left || contact & CNAT_Right)
		{
			SetAction("Scale");
			return -1;
		}*/
	}
}

func LadderToLandscapeCoordinates(int x)
{
	return (x+500)/1000; // round to the next thousand
}

func FxIntClimbControlStop(target, number)
{
	SetLadderRotation(0);
	SetHandAction(0);
}

func FxIntClimbControlControl(target, number, ctrl, x,y,strength, repeat, release)
{
	if (ctrl != CON_Up && ctrl != CON_Down && ctrl != CON_Right && ctrl != CON_Left) return;
	if (release == 1) return;
	if (ctrl == CON_Up)   SetComDir(COMD_Up);
	else if (ctrl == CON_Down) SetComDir(COMD_Down);
	else if (ctrl == CON_Left)
	{
		if (GetDir() == 0)
		{
			if (GetComDir() == COMD_Stop)
			{
				SetPosition(GetX()-10, GetY());
				if (!Stuck())
				{
					SetComDir(COMD_Right);
					SetDir(1);
				}
				SetPosition(GetX()+10, GetY());
			}
			SetComDir(COMD_Stop);
		}
		else
		{
			no_ladder_counter = 5;
			SetAction("Jump");
			SetXDir(-15);
		}
	}
	else if (ctrl == CON_Right)
	{
		if (GetDir() == 1)
		{
			if (GetComDir() == COMD_Stop)
			{
				SetPosition(GetX()+10, GetY());
				if (!Stuck())
				{
					SetComDir(COMD_Left);
					SetDir(0);
				}
				SetPosition(GetX()-10, GetY());
			}
			SetComDir(COMD_Stop);
		}
		else
		{
			no_ladder_counter = 5;
			SetAction("Jump");
			SetXDir(+15);
		}
	}
	return 1;
}

func SetLadderRotation (int r, int xoff, int yoff) {
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(-r,0,0,1), Trans_Translate(xoff,yoff)));
	return;
	var fsin=Sin(r, 1000), fcos=Cos(r, 1000);
	// set matrix values
	SetObjDrawTransform (
		+fcos, +fsin, xoff, //(1000-fcos)*xoff - fsin*yoff,
		-fsin, +fcos, yoff, //(1000-fcos)*yoff + fsin*xoff,
	);
}