/*--
	Ladder climbing
	Authors: Randrian

	Gives the ability to climb on ladders
--*/

local jump_startcall;
local no_ladder_counter;

func GetTurnPhase() { return _inherited(...); }

func Definition(def) {
	// Only add action if included by clonk.
	if (!def.ActMap)
		return _inherited(def);
	def.ActMap = {
		Prototype = def.ActMap,
		Climb = {
			Prototype = Action,
			Name = "Climb",
			Directions = 2,
			Length = 0,
			Delay = 0,
			Wdt = 8,
			Hgt = 20,
			Procedure = DFA_FLOAT,
		},
		Jump = {
			Prototype = def.ActMap.Jump,
			StartCall = "StartSearchLadder",
			// save old phasecall of jump
			StartCallLadderOverloaded = def.ActMap.Jump.StartCall
		}
	};
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

func FxIntSearchLadderTimer(target, effect, time)
{
	if (GetAction() != "Jump") return -1;

	var ladder;
	if (!no_ladder_counter)
	{
		for(ladder in FindObjects(Find_AtRect(-5,-5,10,10), Find_Func("IsLadder"),
		    Find_NoContainer(), Find_Layer(GetObjectLayer())))
		{
			if(ladder->~CanNotBeClimbed()) continue;
			else break;
		}
		if(ladder && ladder->~CanNotBeClimbed()) ladder = nil;
	}
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

func FxIntSearchLadderStop(target, effect, reason, tmp)
{
	if (tmp) return;
	no_ladder_counter = 0;
}

func FxIntClimbControlStart(target, effect, tmp, ladder)
{
	if (tmp) return;
	effect.ladder = ladder;
	SetXDir(0);
	SetYDir(0);
	SetComDir(COMD_Stop);
	effect.odd = 0; // odd or even segment?
	SetHandAction(1);
	SetTurnType(1);
}

func SetTurnType() { return _inherited(...); }
func SetHandAction() { return _inherited(...); }

func LadderStep(target, effect, fUp)
{
	if (fUp == 1)
	{
		effect.pos += 10;
		if (effect.pos > 100)
		{
			effect.pos = 0;
			effect.ladder = effect.ladder->GetNextLadder();
			effect.odd = !effect.odd;
		}
		if (effect.ladder == nil)
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
		effect.pos -= 10;
		if(effect.pos < 0)
		{
			effect.pos = 100;
			effect.ladder = effect.ladder->GetPreviousLadder();
			effect.odd = !effect.odd;
		}
		if (effect.ladder == nil)
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
	if (effect.ladder == nil) return 0;
	return true;
}

func FxIntClimbControlTimer(target, effect, time)
{
	if (GetAction() != "Climb" || Contained()) return -1;
	if(effect.ladder && effect.ladder->~CanNotBeClimbed(1)) effect.ladder = nil;
	if(!effect.ladder)
	{
		no_ladder_counter = 5;
		SetAction("Jump");
		SetXDir(-5+10*GetDir());
		SetYDir(-5);
		return -1;
	}

	// Progress
	var step = 0;
	if (GetComDir() == COMD_Down) step = -1;
	if (GetComDir() == COMD_Up)   step = 1;

	if (step && LadderStep(target, effect, step) == 0)
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
	var data = effect.ladder->GetLadderData();
	var startx = data[0], starty = data[1], endx = data[2], endy = data[3], angle = data[4];
	var x = startx + (endx-startx)*effect.pos/100+5000-100*GetTurnPhase();
	var y = starty + (endy-starty)*effect.pos/100;
	var lx = LadderToLandscapeCoordinates(x);
	var ly = LadderToLandscapeCoordinates(y);
	var old_x = GetX(), old_y = GetY();
	if(Abs(old_x-lx)+Abs(old_y-ly) > 10 && time > 1)
		return -1;
	SetPosition(lx, ly);
	SetXDir(0); SetYDir(0);
	SetLadderRotation(-angle, x-GetX()*1000, y-GetY()*1000);//effect.odd);
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
		if (step) LadderStep(target, effect, -step);
		// if we are to far left or right try to turn
		if (GetDir() == 0 && LadderToLandscapeCoordinates(x)-2 > GetX())
		{
			SetComDir(COMD_Right);
			SetDir(1);
		}
		else if (GetDir() == 1 && LadderToLandscapeCoordinates(x)+2 < GetX())
		{
			SetComDir(COMD_Left);
			SetDir(0);
		}
	}
	else effect.ladder->~OnLadderClimb(this);
	// Make the animation synchron with movement TODO: this only makes the feet synchronous for the arms the animation has to be adapted
	var animation = GetRootAnimation(5);
	if (animation != nil)
	{
		if (GetAnimationName(animation) != nil)
		{
			var length = GetAnimationLength(GetAnimationName(animation));
			var pos = effect.pos*length/200+length/2*effect.odd;
			//pos = BoundBy(pos, 1, length-1);
			SetAnimationPosition(animation, Anim_Const(pos));
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

func FxIntClimbControlStop(target, effect)
{
	if(GetAction() == "Climb") SetAction("Walk");
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
	SetMeshTransformation(Trans_Mul(Trans_Translate(0, -10000), Trans_Rotate(-r,0,0,1), Trans_Translate(xoff, 10000+yoff)), 5);
//	SetProperty("MeshTransformation", Trans_Mul(Trans_Translate(0, -10000), Trans_Rotate(-r,0,0,1), Trans_Translate(xoff, 10000+yoff)));
	return;
//	var fsin=Sin(r, 1000), fcos=Cos(r, 1000);
//	// set matrix values
//	SetObjDrawTransform (
//		+fcos, +fsin, xoff, //(1000-fcos)*xoff - fsin*yoff,
//		-fsin, +fcos, yoff, //(1000-fcos)*yoff + fsin*xoff,
//	);
}

// Defined to prevent an error.
func SetMeshTransformation() { return _inherited(...); }
