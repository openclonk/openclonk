/**
	Ladder Climbing
	Gives the ability to clonks climb on ladders, to be included by the clonk.

	@author Randrian
*/

public func Definition(proplist def) 
{
	// Only add action if included by clonk.
	if (!def.ActMap)
		return _inherited(def);
	// Add actions for climbing and overload jumping actions to search for ladders.
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
			// Save the old phasecall of the jump.
			StartCallLadderOverloaded = def.ActMap.Jump.StartCall
		},
		WallJump = {
			Prototype = def.ActMap.WallJump,
			StartCall = "StartSearchLadder",
			// Save the old phasecall of the wall jump.
			StartCallLadderOverloaded = def.ActMap.WallJump.StartCall
		}
	};
	return _inherited(def);
}

public func GetTurnPhase() { return _inherited(...); }
public func SetTurnType() { return _inherited(...); }
public func SetHandAction() { return _inherited(...); }
// Should be overloaded, and add a climb animation here
public func StartScale() { return _inherited(...); }


/*-- Ladder Searching --*/

public func StartSearchLadder()
{
	// Call the overwriten old phase call.
	if (GetAction() == "Jump" && this.ActMap.Jump.StartCallLadderOverloaded)
		Call(this.ActMap.Jump.StartCallLadderOverloaded);
	if (GetAction() == "WallJump" && this.ActMap.WallJump.StartCallLadderOverloaded)
		Call(this.ActMap.WallJump.StartCallLadderOverloaded);
	// Add an effect to search for ladders.
	ScheduleCall(this, this.AddSearchLadderEffect, 1, 1);
	return;
}

public func AddSearchLadderEffect()
{
	if (!GetEffect("IntSearchLadder", this))
		AddEffect("IntSearchLadder", this, 1, 2, this);
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
		
	// Find a ladder which can be climbed.
	var ladder;
	for (ladder in FindObjects(Find_AtRect(-5, -10, 10, 8), Find_NoContainer(), Find_Property("IsLadder"), Find_Layer(GetObjectLayer())))
	{
		// Don't climb ladders that are blocked.
		if (ladder->~CanNotBeClimbed(false, this) || IsBlockedLadder(ladder))
			continue;
		
		SetAction("Climb");
		ladder->~OnLadderGrab(this);
		PlayAnimation(GetLadderScaleAnimation(), CLONK_ANIM_SLOT_Movement, Anim_Y(0, GetAnimationLength(GetLadderScaleAnimation()), 0, 15), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		AddEffect("IntClimbControl", this, 1, 1, this, nil, ladder);
		return FX_Execute_Kill;
	}
	return FX_OK;
}

public func FxIntSearchLadderStop(object target, proplist effect, reason, tmp)
{
	if (tmp) 
		return FX_OK;
	return FX_OK;
}


/*-- Ladder Block --*/

private func AddLadderBlock(object ladder, int duration)
{
	AddEffect("IntBlockLadder", this, 100, duration, this, nil, ladder);
	return;
}

private func IsBlockedLadder(object ladder)
{
	var index = 0;
	var fx;
	while (fx = GetEffect("IntBlockLadder", this, index++))
		if (fx.ladder && fx.ladder->IsSameLadder(ladder))
			return true;
	return false;
}

public func FxIntBlockLadderStart(object target, effect fx, int tmp, object to_ladder)
{
	if (tmp)
		return FX_OK;
	fx.ladder = to_ladder;
	return FX_OK;
}

public func FxIntBlockLadderTimer(object target, effect fx, int time)
{
	return FX_Execute_Kill;
}


/*-- Ladder Control --*/

public func FxIntClimbControlStart(object target, effect fx, int tmp, object ladder)
{
	if (tmp) 
		return FX_OK;
	fx.ladder = ladder;
	SetXDir(0);
	SetYDir(0);
	SetComDir(COMD_Stop);
	// Start on an even segment.
	fx.odd = 0;
	// Correctly initalize the relative y-position of the clonk to the segment.
	var data = fx.ladder->GetLadderData();
	var sy = data[1], ey = data[3];
	var cy = GetY(1000);
	var posy = 0;
	if (ey - sy != 0)
		posy = 100 * (cy - sy) / (ey - sy);
	fx.pos = BoundBy(posy, 0, 100);
	// Set some stuff for the clonk.
	SetHandAction(1);
	SetTurnType(1);
	return FX_OK;
}

public func LadderStep(object target, effect fx, int climb_direction)
{
	if (climb_direction != 1 && climb_direction != -1)
		return fx.ladder != nil;
	// Store old segment to forward to blocking.	
	var old_ladder_segment = fx.ladder;
	// Increase position depending on direction and move to new segment if needed.
	fx.pos += 10 * climb_direction;
	if (fx.pos > 100)
	{
		fx.pos = 0;
		fx.ladder = fx.ladder->GetNextLadder();
		fx.odd = !fx.odd;
	}
	if (fx.pos < 0)
	{
		fx.pos = 100;
		fx.ladder = fx.ladder->GetPreviousLadder();
		fx.odd = !fx.odd;
	}
	// If no ladder has been found scale or jump off.
	if (fx.ladder == nil)
	{
		var contact = GetContact(-1);
		if (contact & CNAT_Left || contact & CNAT_Right)
		{
			SetAction("Scale");
			old_ladder_segment->~OnLadderReleased(this);
			return false;
		}
		AddLadderBlock(old_ladder_segment, 10);
		SetAction("Jump");
		// Increase speed if moving up.
		if (climb_direction == 1)
		{
			SetXDir(-5 + 10 * GetDir());
			SetYDir(-5);
		}
		old_ladder_segment->~OnLadderReleased(this);
		return false;
	}
	return true;
}

public func FxIntClimbControlTimer(object target, effect fx, int time)
{
	if (GetAction() != "Climb" || Contained()) 
		return FX_Execute_Kill;
	if (!fx.ladder || fx.ladder->~CanNotBeClimbed(true, this)) 
	{
		AddLadderBlock(fx.ladder, 5);
		SetAction("Jump");
		SetXDir(-5 + 10 * GetDir());
		SetYDir(-5);
		return FX_Execute_Kill;
	}
	
	// Progress movement in the controlled direction.
	var climb_direction = 0;
	if (GetComDir() == COMD_Down) 
		climb_direction = -1;
	if (GetComDir() == COMD_Up) 
		climb_direction = 1;
	// LadderStep advances the ladder segment or the ladder position.
	if (climb_direction && !LadderStep(target, fx, climb_direction))
		return FX_Execute_Kill;

	// Move the clonk along the ladder according to the new pos/segment.
	var data = fx.ladder->GetLadderData();
	var startx = data[0], starty = data[1], endx = data[2], endy = data[3], angle = data[4];
	var x = startx + (endx - startx) * fx.pos / 100 + 5000 - 100 * GetTurnPhase();
	var y = starty + (endy - starty) * fx.pos / 100;
	var lx = LadderToLandscapeCoordinates(x);
	var ly = LadderToLandscapeCoordinates(y);
	var old_x = GetX(), old_y = GetY();
	if (Abs(old_x - lx) + Abs(old_y - ly) > 10 && time > 1)
		return FX_Execute_Kill;
	SetPosition(lx, ly);
	SetXDir(0);
	SetYDir(0);
	SetLadderRotation(-angle, x - GetX() * 1000, y - GetY() * 1000);

	// Handle if the clonk gets stuck.
	if (Stuck())
	{
		var dir = -1;
		if (GetDir() == DIR_Left) 
			dir = 1;
		for (var i = 1; i <= 5; i++)
		{
			SetPosition(LadderToLandscapeCoordinates(x) + i * dir, LadderToLandscapeCoordinates(y));
			if (!Stuck())
				break;
		}
		if (Stuck()) 
			SetPosition(LadderToLandscapeCoordinates(x) + 5 * dir, LadderToLandscapeCoordinates(y));
	}
	if (Stuck())
	{
		// Revert Position and step.
		SetPosition(old_x, old_y);
		if (climb_direction) 
			LadderStep(target, fx, -climb_direction);
		// If we are too far left or right try to turn.
		if (GetDir() == DIR_Left && LadderToLandscapeCoordinates(x) - 2 > GetX())
		{
			SetComDir(COMD_Right);
			SetDir(DIR_Right);
		}
		else if (GetDir() == DIR_Right && LadderToLandscapeCoordinates(x) + 2 < GetX())
		{
			SetComDir(COMD_Left);
			SetDir(DIR_Left);
		}
	}
	else
	{
		fx.ladder->~OnLadderClimb(this);
	}
	// Make the animation synchron with movement.
	// TODO: this only makes the feet synchronous for the arms the animation has to be adapted.
	var animation = GetRootAnimation(5);
	if (animation != nil)
	{
		if (GetAnimationName(animation) != nil)
		{
			var length = GetAnimationLength(GetAnimationName(animation));
			var pos = fx.pos * length / 200 + length / 2 * fx.odd;
			SetAnimationPosition(animation, Anim_Const(pos));
		}
	}
	// Start walking or hangling if the clonk makes contact with the floor or ceiling.
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
			return FX_Execute_Kill;
		}
		if (contact & CNAT_Bottom && GetComDir() == COMD_Down)
		{
			SetAction("Walk");
			return FX_Execute_Kill;
		}
	}
	return FX_OK;
}

private func LadderToLandscapeCoordinates(int x)
{
	// Round to the next thousand.
	return (x + 500) / 1000;
}

public func FxIntClimbControlStop(object target, effect fx, int reason, bool tmp)
{
	if (tmp) 
		return FX_OK;
	if (GetAction() == "Climb") 
		SetAction("Walk");
	if (fx.ladder)
		fx.ladder->~OnLadderReleased(this);
	SetLadderRotation(0);
	SetHandAction(0);
	return FX_OK;
}

public func FxIntClimbControlControl(object target, effect fx, int ctrl, int x, int y, int strength, bool repeat, int status)
{
	// Only handle movement controls.
	if (ctrl != CON_Up && ctrl != CON_Down && ctrl != CON_Right && ctrl != CON_Left) 
		return false;
	// Perform actions on key down and not on release.
	if (status != CONS_Down) 
		return false;
	// Move up and down by setting com dir.	
	if (ctrl == CON_Up)
	{
		SetComDir(COMD_Up);
		return true;
	}
	if (ctrl == CON_Down)
	{
		SetComDir(COMD_Down);
		return true;
	}
	
	// Handle left and right controls.
	if (ctrl == CON_Left)
	{
		if (GetDir() == DIR_Left)
		{
			// Switch to the other side of the ladder.
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
			// Let go of the ladder and remove the effect.
			AddLadderBlock(fx.ladder, 5);
			if (GetComDir() == COMD_Up)
				this->ObjectCommand("Jump");
			else
				this->ObjectComLetGo(-10);
			RemoveEffect(nil, target, fx);
		}
		return true;
	}
	
	if (ctrl == CON_Right)
	{
		if (GetDir() == DIR_Right)
		{
			// Switch to the other side of the ladder.
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
			// Let go of the ladder and remove the effect.
			AddLadderBlock(fx.ladder, 5);
			if (GetComDir() == COMD_Up)
				this->ObjectCommand("Jump");
			else
				this->ObjectComLetGo(10);
			RemoveEffect(nil, target, fx);
		}
		return true;
	}
	return true;
}

public func SetLadderRotation(int r, int xoff, int yoff) 
{
	SetMeshTransformation(Trans_Mul(Trans_Translate(0, -10000), Trans_Rotate(-r, 0, 0, 1), Trans_Translate(xoff, 10000 + yoff)), CLONK_MESH_TRANSFORM_SLOT_Rotation_Ladder);
	return;
}

// Defined to prevent an error, because SetMeshTransformation is overloaded by the clonk.
func SetMeshTransformation() { return _inherited(...); }
