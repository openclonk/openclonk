/**
	Ladder Climbing
	Gives the ability to clonks climb on ladders.

	@author Randrian
*/

local jump_startcall;
local no_ladder_counter;

public func GetTurnPhase() { return _inherited(...); }

public func Definition(proplist def) 
{
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
		},
		WallJump = {
			Prototype = def.ActMap.WallJump,
			StartCall = "StartSearchLadder",
			// save old phasecall of jump
			StartCallLadderOverloaded = def.ActMap.WallJump.StartCall
		}
	};
	_inherited(def);
}

public func StartScale()
{
	// Should be overloaded, and add a climb animation here
	return _inherited(...);
}

public func StartSearchLadder()
{
	// Call the overwriten old phase call.
	if (GetAction() == "Jump" && this.ActMap.Jump.StartCallLadderOverloaded)
		Call(this.ActMap.Jump.StartCallLadderOverloaded);
	if (GetAction() == "WallJump" && this.ActMap.WallJump.StartCallLadderOverloaded)
		Call(this.ActMap.WallJump.StartCallLadderOverloaded);
	if (!GetEffect("InSearchLadder", this))
		AddEffect("IntSearchLadder", this, 1, 5, this);
	FxIntSearchLadderTimer();
	return;
}

public func GetLadderScaleAnimation()
{
	var animation = _inherited(...);
	if (animation) 
		return animation;
	return "Scale";
}

public func FxIntSearchLadderTimer(object target, proplist effect, int time)
{
	// Only search for a ladder if jumping.
	if (GetAction() != "Jump" && GetAction() != "WallJump") 
		return FX_Execute_Kill;
		
	// Don't search for ladders if the counter is non-zero.
	if (no_ladder_counter > 0)
	{
		no_ladder_counter--;
		return FX_OK;	
	}
		
	// Find a ladder.
	var ladder;
	for (ladder in FindObjects(Find_AtRect(-5, -10, 10, 8), Find_Func("IsLadder"), Find_NoContainer(), Find_Layer(GetObjectLayer())))
	{
		if (!ladder->~CanNotBeClimbed()) 
		{
			SetAction("Climb");
			ladder->~OnLadderGrab(this);
			PlayAnimation(GetLadderScaleAnimation(), CLONK_ANIM_SLOT_Movement, Anim_Y(0, GetAnimationLength(GetLadderScaleAnimation()), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			AddEffect("IntClimbControl", this, 1, 1, this, nil, ladder);
			return FX_Execute_Kill;		
		}
	}
	return FX_OK;
}

public func FxIntSearchLadderStop(object target, proplist effect, reason, tmp)
{
	if (tmp) 
		return FX_OK;
	no_ladder_counter = 0;
	return FX_OK;
}

public func FxIntClimbControlStart(object target, proplist effect, tmp, object ladder)
{
	if (tmp) 
		return FX_OK;
	effect.ladder = ladder;
	SetXDir(0);
	SetYDir(0);
	SetComDir(COMD_Stop);
	effect.odd = 0; // odd or even segment?
	SetHandAction(1);
	SetTurnType(1);
	return FX_OK;
}

public func SetTurnType() { return _inherited(...); }
public func SetHandAction() { return _inherited(...); }

public func LadderStep(target, effect, fUp)
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
				return false;
			}
			no_ladder_counter = 5;
			SetAction("Jump");
			SetXDir(-5+10*GetDir());
			SetYDir(-5);
			return false;
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
				return false;
			}
			no_ladder_counter = 5;
			SetAction("Jump");
			return false;
		}
	}
	if (effect.ladder == nil) return false;
	return true;
}

public func FxIntClimbControlTimer(object target, proplist effect, int time)
{
	if (GetAction() != "Climb" || Contained()) 
		return FX_Execute_Kill;
	if (effect.ladder && effect.ladder->~CanNotBeClimbed(true)) 
		effect.ladder = nil;
	if (!effect.ladder)
	{
		no_ladder_counter = 5;
		SetAction("Jump");
		SetXDir(-5 + 10 * GetDir());
		SetYDir(-5);
		return FX_Execute_Kill;
	}

	// Progress
	var step = 0;
	if (GetComDir() == COMD_Down) 
		step = -1;
	if (GetComDir() == COMD_Up) 
		step = 1;

	if (step && !LadderStep(target, effect, step))
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
				SetXDir(-5 + 10 * GetDir());
				SetYDir(-5);
				no_ladder_counter = 10;
			}
		}
		return FX_Execute_Kill;
	}
	var data = effect.ladder->GetLadderData();
	var startx = data[0], starty = data[1], endx = data[2], endy = data[3], angle = data[4];
	var x = startx + (endx-startx) * effect.pos / 100 + 5000 - 100 * GetTurnPhase();
	var y = starty + (endy-starty) * effect.pos / 100;
	var lx = LadderToLandscapeCoordinates(x);
	var ly = LadderToLandscapeCoordinates(y);
	var old_x = GetX(), old_y = GetY();
	if (Abs(old_x - lx) + Abs(old_y - ly) > 10 && time > 1)
		return FX_Execute_Kill;
	SetPosition(lx, ly);
	SetXDir(0);
	SetYDir(0);
	SetLadderRotation(-angle, x - GetX() * 1000, y - GetY() * 1000);
	if (Stuck())
	{
		var dir = -1;
		if (GetDir() == DIR_Left) 
			dir = 1;
		for (var i = 1; i <= 5; i++)
		{
			SetPosition(LadderToLandscapeCoordinates(x) + i * dir, LadderToLandscapeCoordinates(y));
			if (!Stuck()) break;
		}
		if (Stuck()) SetPosition(LadderToLandscapeCoordinates(x) + 5 * dir, LadderToLandscapeCoordinates(y));
	}
	if (Stuck())
	{
		// Revert Position and step
		SetPosition(old_x, old_y);
		if (step) 
			LadderStep(target, effect, -step);
		// if we are to far left or right try to turn
		if (GetDir() == DIR_Left && LadderToLandscapeCoordinates(x) - 2 > GetX())
		{
			SetComDir(COMD_Right);
			SetDir(DIR_Right);
		}
		else if (GetDir() == DIR_Right && LadderToLandscapeCoordinates(x)+2 < GetX())
		{
			SetComDir(COMD_Left);
			SetDir(DIR_Left);
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
			var pos = effect.pos * length / 200 + length / 2 * effect.odd;
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
	}
}

private func LadderToLandscapeCoordinates(int x)
{
	// Round to the next thousand.
	return (x + 500) / 1000;
}

public func FxIntClimbControlStop(target, effect)
{
	if (GetAction() == "Climb") 
		SetAction("Walk");
	SetLadderRotation(0);
	SetHandAction(0);
}

public func FxIntClimbControlControl(object target, proplist effect, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	// Only handle movement controls.
	if (ctrl != CON_Up && ctrl != CON_Down && ctrl != CON_Right && ctrl != CON_Left) 
		return false;
	// Perform actions on key down and not on release.
	if (release) 
		return false;
			
	if (ctrl == CON_Up)
		SetComDir(COMD_Up);
	else if (ctrl == CON_Down) 
		SetComDir(COMD_Down);
	
	else if (ctrl == CON_Left)
	{
		if (GetDir() == DIR_Left)
		{
			if (GetComDir() == COMD_Stop)
			{
				SetPosition(GetX() - 10, GetY());
				if (!Stuck())
				{
					SetComDir(COMD_Right);
					SetDir(DIR_Right);
				}
				SetPosition(GetX() + 10, GetY());
			}
			SetComDir(COMD_Stop);
		}
		else
		{
			no_ladder_counter = 5;
			if (GetComDir() == COMD_Up)
				this->ObjectCommand("Jump");
			else
				this->ObjectComLetGo(-10);
		}
	}
	else if (ctrl == CON_Right)
	{
		if (GetDir() == DIR_Right)
		{
			if (GetComDir() == COMD_Stop)
			{
				SetPosition(GetX() + 10, GetY());
				if (!Stuck())
				{
					SetComDir(COMD_Left);
					SetDir(DIR_Left);
				}
				SetPosition(GetX() - 10, GetY());
			}
			SetComDir(COMD_Stop);
		}
		else
		{
			no_ladder_counter = 5;
			if (GetComDir() == COMD_Up)
				this->ObjectCommand("Jump");
			else
				this->ObjectComLetGo(10);
		}
	}
	return true;
}

public func SetLadderRotation(int r, int xoff, int yoff) 
{
	SetMeshTransformation(Trans_Mul(Trans_Translate(0, -10000), Trans_Rotate(-r, 0, 0, 1), Trans_Translate(xoff, 10000 + yoff)), 5);
	return;
}

// Defined to prevent an error, because SetMeshTransformation is overloaded by the clonk.
func SetMeshTransformation() { return _inherited(...); }
