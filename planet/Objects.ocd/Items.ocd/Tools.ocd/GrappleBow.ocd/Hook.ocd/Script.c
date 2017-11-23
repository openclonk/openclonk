/**
	Grapple Hook
	The hook can be shot with the grappling bow. On impact the hook will stick to the ground.
	The hook also controls the swinging controls for the clonk.
	
	@author Randrian
*/

local rope; // The rope is the connection between the hook and the bow.
local clonk;
local pull;
local grappler;
local fx_hook;

public func GetRope() { return rope; }

public func New(object new_clonk, object new_rope)
{
	// Hook graphics are handled by rope.
	this.Visibility = VIS_None;
	clonk = new_clonk;
	rope = new_rope;
}

public func Launch(int angle, int str, object shooter, object bow)
{
	// Hook graphics are handled by rope.
	this.Visibility = VIS_None;
	Exit();

	pull = false;
		
	// Create rope.
	rope = CreateObject(GrappleRope);

	rope->Connect(this, bow);
	rope->ConnectLoose();
	clonk = shooter;
	grappler = bow;

	var xdir = Sin(angle, str);
	var ydir = Cos(angle, -str);
	SetXDir(xdir);
	SetYDir(ydir);
	SetR(angle);
	Sound("Objects::Arrow::Shoot?");
	
	AddEffect("InFlight", this, 1, 1, this);
}

public func Destruction()
{
	if (rope)
		rope->HookRemoved();
}

private func Stick()
{
	if (GetEffect("InFlight",this))
	{
		Sound("Objects::Arrow::HitGround");
		RemoveEffect("InFlight", this);
	
		SetXDir(0);
		SetYDir(0);
		SetRDir(0);

		// Stick in landscape (vertex 3-7)
		SetVertex(2, VTX_X,  0, 2);
		SetVertex(2, VTX_Y, -6, 2);
		SetVertex(3, VTX_X, -3, 2);
		SetVertex(3, VTX_Y, -4, 2);
		SetVertex(4, VTX_X,  3, 2);
		SetVertex(4, VTX_Y, -4, 2);
		SetVertex(5, VTX_X,  4, 2);
		SetVertex(5, VTX_Y, -1, 2);
		SetVertex(6, VTX_X, -4, 2);
		SetVertex(6, VTX_Y, -1, 2);
		
		// Stick successful?
		if (!Stuck())
		{
			// If not, draw in to prevent hook from dragging you down
			if (grappler) grappler->DrawRopeIn();
			return true;
		}

		rope->HookAnchored();
		
		// Draw in possible other active grapplers the clonk is using once this hook hits a solid area and sticks.
		for (var obj in FindObjects(Find_ID(GrappleBow), Find_Container(clonk)))
			if (obj != grappler)
				obj->DrawRopeIn();
				
		ScheduleCall(this, "StartPull", 5); // TODO
	}
}

public func StartPull()
{
	pull = true;
	fx_hook = AddEffect("IntGrappleControl", clonk, 1, 1, this);
	if (clonk->GetAction() == "Jump")
	{
		rope->AdjustClonkMovement();
		rope->ConnectPull();
		fx_hook.var5 = 1;
		fx_hook.var6 = 10;
	}
}

public func Hit()
{
	Stick();
}

public func FxInFlightStart(object target, proplist effect, int temp)
{
	if(temp) return;
	effect.x = target->GetX();
	effect.y = target->GetY();
}

public func FxInFlightTimer(object target, proplist effect, int time)
{
	var oldx = effect.x;
	var oldy = effect.y;
	var newx = GetX();
	var newy = GetY();
	
	// and additionally, we need one check: If the hook has no speed
	// anymore but is still in flight, we'll remove the hit check
	if (oldx == newx && oldy == newy)
	{
		// but we give the arrow 5 frames to speed up again
		effect.countdown++;
		if (effect.countdown >= 10)
			return Hit();
	}
	else
		effect.countdown = 0;

	// Rotate hook according to speed
	var anglediff = Normalize(Angle(oldx, oldy ,newx, newy) - GetR(), -180);
	SetRDir(anglediff / 2);
	effect.x = newx;
	effect.y = newy;
	return FX_OK;
}

public func Entrance(object container)
{
	if(container->GetID() == GrappleBow) return;
	if (rope)
		rope->BreakRope();
	RemoveObject();
	return;
}

public func OnRopeBreak()
{
	// Remove control effect for the grapple bow, but only if it exists.
	// Otherwise RemoveEffect with fx_hook == nil removes another effect in the clonk.
	if (fx_hook)
		RemoveEffect(nil, clonk, fx_hook);
	RemoveObject();
	return;
}

/*-- Grapple rope controls --*/

public func FxIntGrappleControlControl(object target, proplist effect, int ctrl, int x, int y, int strength, bool repeat, int status)
{
	if (status == CONS_Moved) return false;
	var release = status == CONS_Up;
	// Cancel this effect if clonk is now attached to something.
	if (target->GetProcedure() == "ATTACH") 
	{
		RemoveEffect(nil, target, effect);
		return false;
	}

	if (ctrl != CON_Up && ctrl != CON_Down && ctrl != CON_Right && ctrl != CON_Left)
		return false;

	if (ctrl == CON_Right)
	{
		effect.mv_right = !release;
		if (release)
		{
			if (effect.lastkey == CON_Right)
			{
		    	target->SetDir(0);
		    	target->UpdateTurnRotation();
			}
			effect.lastkey = CON_Right;
			effect.keyTimer = 10;
		}
	}
	if (ctrl == CON_Left)
	{
		effect.mv_left = !release;
		if (release)
		{
			if (effect.lastkey == CON_Left)
			{
		    	target->SetDir(1);
		    	target->UpdateTurnRotation();
			}
			effect.lastkey = CON_Left;
			effect.keyTimer = 10;
		}
	}
	if (ctrl == CON_Up)
	{
		effect.mv_up = !release;
		if ((target->GetAction() == "Jump" || target->GetAction() == "WallJump") && !release && pull)
			rope->ConnectPull();
	}
	if (ctrl == CON_Down)
	{
		effect.mv_down = !release;
	}
	
	// Never swallow the control.
	return false;
}

// Effect for smooth movement.
public func FxIntGrappleControlTimer(object target, proplist effect, int time)
{
	// Cancel this effect if clonk is now attached to something
	// this check is also in the timer because on a high control rate
	// (higher than 1 actually), the timer could be called first
	if (target->GetProcedure() == "ATTACH")
		return FX_Execute_Kill;
	// Also cancel if the clonk is contained
	if (target->Contained())
		return FX_Execute_Kill;
		
	if (effect.keyTimer)
	{
		effect.keyTimer--;
		if (effect.keyTimer == 0)
			effect.lastkey = 0;
	}
	// Movement.
	if (effect.mv_up && rope)
    {
		var iSpeed = 10 - Cos(target->GetAnimationPosition(effect.Climb) * 360 * 2 / target->GetAnimationLength("RopeClimb") - 45, 10);
		effect.speedCounter += iSpeed;
		if (effect.speedCounter > 20)
		{
			rope->DoLength(-1);
			effect.speedCounter -= 20;
      }
    }
	if (effect.mv_down)
		if (rope)
			rope->DoLength(+1);
	if (effect.mv_left)
	{
		rope->DoSpeed(-10);
	}
	if (effect.mv_right)
	{
		rope->DoSpeed(+10);
	}

	if (target->GetAction() == "Tumble" && target->GetActTime() > 10)
		target->SetAction("Jump");

	if (target->GetAction() != "Jump" && target->GetAction() != "WallJump")
	{
		if (rope->GetConnectStatus())
			rope->ConnectLoose();
	}
	
	if ((target->GetAction() == "Jump" || target->GetAction() == "WallJump") && rope->PullObjects() && !effect.var6)
	{
		if (!effect.ani_mode)
		{
			target->SetTurnType(1);
			if (!target->GetHandAction())
				target->SetHandAction(-1);
		}

		if (effect.mv_up)
		{
			if (effect.ani_mode != 2)
			{
				effect.ani_mode = 2;
				effect.Climb = target->PlayAnimation("RopeClimb", CLONK_ANIM_SLOT_Arms, Anim_Linear(target->GetAnimationLength("RopeClimb")/2, 0, target->GetAnimationLength("RopeClimb"), 35), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
				effect.speedCounter = 0;
			}
		}
		else if (effect.mv_down)
		{
			if (effect.ani_mode != 3)
			{
				effect.ani_mode = 3;
				target->PlayAnimation("RopeDown", CLONK_ANIM_SLOT_Arms, Anim_Const(0), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			}
		}
		else if (effect.mv_left || effect.mv_right)
		{
			var length = target->GetAnimationLength("RopeSwing");
			var start = length / 2;
			var dir = 0;
			if ((effect.mv_left && !target->GetDir()) || (!effect.mv_left && target->GetDir()))
				dir = 1;
			if (effect.ani_mode != 4 + dir)
			{
				target->PlayAnimation("RopeSwing", CLONK_ANIM_SLOT_Arms, Anim_Linear(start, length * dir, length*(!dir), 35, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
				effect.ani_mode = 4 + dir;
			}
		}
		else if (effect.ani_mode != 1)
		{
			effect.ani_mode = 1;
			target->PlayAnimation("OnRope", CLONK_ANIM_SLOT_Arms, Anim_Linear(0, 0, target->GetAnimationLength("OnRope"), 35 * 2, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		}
		var angle = rope->GetClonkAngle();
		var off = rope->GetClonkOff();
		target->SetMeshTransformation(Trans_Translate(-off[0] * 10 + 3000 * (1 - 2 * target->GetDir()), -off[1] * 10), CLONK_MESH_TRANSFORM_SLOT_Translation_Hook);
    	target->SetMeshTransformation(Trans_RotX(angle, 500, 11000), CLONK_MESH_TRANSFORM_SLOT_Rotation_Hook);
	}
	else if (effect.ani_mode)
	{
		target->SetMeshTransformation(0, CLONK_MESH_TRANSFORM_SLOT_Translation_Hook);
		target->SetMeshTransformation(0, CLONK_MESH_TRANSFORM_SLOT_Rotation_Hook);
		target->StopAnimation(target->GetRootAnimation(10));
		if (!target->GetHandAction())
			target->SetHandAction(0);
		effect.ani_mode = 0;
	}

	if (effect.var6) 
		effect.var6--;
	
	return FX_OK;
}

public func FxIntGrappleControlOnCarryHeavyPickUp(object target, proplist effect, object heavy_object)
{
	// Remove the control effect when a carry-heavy object is picked up.
	// The rope will then be drawn in automatically.
	RemoveEffect(nil, target, effect);
	return;
}

public func FxIntGrappleControlRejectCarryHeavyPickUp(object target, proplist effect, object heavy_object)
{
	// Block picking up carry-heavy objects when this clonk is hanging on a rope.
	if (rope->PullObjects())
		return true;
	return false;
}

public func FxIntGrappleControlStop(object target, proplist effect, int reason, int tmp)
{
	if (tmp) 
		return FX_OK;
	target->SetTurnType(0);
	target->SetMeshTransformation(0, CLONK_MESH_TRANSFORM_SLOT_Translation_Hook);
 	target->SetMeshTransformation(0, CLONK_MESH_TRANSFORM_SLOT_Rotation_Hook);
	target->StopAnimation(target->GetRootAnimation(10));
	if (!target->GetHandAction())
		target->SetHandAction(0);
	
	// If the hook is not already drawing in, break the rope.
	if (!GetEffect("DrawIn", this->GetRope()))
	{
		this->GetRope()->BreakRope();
	}
	return FX_OK;
}

private func Trans_RotX(int rotation, int ox, int oy)
{
	return Trans_Mul(Trans_Translate(-ox, -oy), Trans_Rotate(rotation, 0, 0, 1), Trans_Translate(ox, oy));
}

public func RejectWindbagForce() { return true; }

// Only the grappler is stored.
public func SaveScenarioObject() { return false; }

/*-- Properties --*/


local Name = "$Name$";
local Plane = 300;

