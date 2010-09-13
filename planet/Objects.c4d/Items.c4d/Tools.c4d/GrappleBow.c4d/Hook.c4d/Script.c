/*
	Grapple Hook
	Author: Randrian
	
	The hook can be shot with the grappling bow.
	On impact the hook will stick to the ground
	The hook also controls the swinging controls for the clonk
*/

local rope; // The rope is the connection between the hook
local clonk;
local pull;
local grappler;
local effect;

public func ArrowStrength() { return 10; }

public func GetRope() { return rope; }

public func New(object new_clonk, object new_rope)
{
	SetObjDrawTransform(0, 1, 0, 0, 0, 0, 0); // Hide
	clonk = new_clonk;
	rope = new_rope;
}

public func Launch(int angle, int str, object shooter, object bow)
{
	SetObjDrawTransform(0, 1, 0, 0, 0, 0, 0); // Hide
	Exit();

	pull = 0;
		
	// Create rope
	rope = CreateObject(GrappleRope, 0, 0, NO_OWNER);

	rope->Connect(this, bow);
	rope->ConnectLoose();
	clonk = shooter;
	grappler = bow;

	var xdir = Sin(angle,str);
	var ydir = Cos(angle,-str);
	SetXDir(xdir);
	SetYDir(ydir);
	SetR(angle);
	Sound("ArrowShoot*.ogg");
	
	AddEffect("InFlight", this, 1, 1, this);
}

public func Destruction()
{
	if(rope)
		rope->HookRemoved();
}

private func Stick()
{
	if (GetEffect("InFlight",this))
	{
		Sound("ArrowHitGround.ogg");
	
		RemoveEffect("InFlight",this);
	
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

		rope->HockAnchored();
		for(var obj in FindObjects(Find_ID(GrappleBow), Find_Container(clonk)))
			if(obj != grappler)
				obj->DrawRopeIn();
//		StartPull();
		ScheduleCall(this, "StartPull", 5); // TODO
	}
}

public func StartPull()
{
	pull = 1;

	effect = AddEffect("IntGrappleControl", clonk, 1, 1, this);
	if(clonk->GetAction() == "Jump")
	{
		rope->AdjustClonkMovement();
		rope->ConnectPull();
		EffectVar(5, clonk, GetEffect("IntGrappleControl", clonk)) = 1;
		EffectVar(6, clonk, GetEffect("IntGrappleControl", clonk)) = 10;
	}
}

public func Hit(x, y)
{
	Stick();
}

// rotate arrow according to speed
public func FxInFlightStart(object target, int effect, int temp)
{
	if(temp) return;
	EffectVar(0,target,effect) = target->GetX();
	EffectVar(1,target,effect) = target->GetY();
}
public func FxInFlightTimer(object target, int effect, int time)
{
	var oldx = EffectVar(0,target,effect);
	var oldy = EffectVar(1,target,effect);
	var newx = GetX();
	var newy = GetY();
	
	// and additionally, we need one check: If the arrow has no speed
	// anymore but is still in flight, we'll remove the hit check
	if(oldx == newx && oldy == newy)
	{
		// but we give the arrow 5 frames to speed up again
		EffectVar(2,target,effect)++;
		if(EffectVar(2,target,effect) >= 10)
			return Hit();
	}
	else
		EffectVar(2,target,effect) = 0;

	// rotate arrow according to speed
	var anglediff = Normalize(Angle(oldx,oldy,newx,newy)-GetR(),-180);
	SetRDir(anglediff/2);
	EffectVar(0,target,effect) = newx;
	EffectVar(1,target,effect) = newy;
	return;
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
	RemoveEffect(0, clonk, effect);
	RemoveObject();
	return;
}

local Name = "$Name$";


/*-- Grapple rope controls --*/

public func FxIntGrappleControlControl(object target, int fxnum, ctrl, x,y,strength, repeat, release)
{
	if(ctrl != CON_Up && ctrl != CON_Down && ctrl != CON_Right && ctrl != CON_Left) return;

	if(ctrl == CON_Right)
	{
		EffectVar(3, target, fxnum) = !release;
	}
	if(ctrl == CON_Left)
	{
		EffectVar(2, target, fxnum) = !release;
	}
	if(ctrl == CON_Up)
	{
		EffectVar(0, target, fxnum) = !release;
		if(target->GetAction() == "Jump" && !release && pull)
			rope->ConnectPull();
	}
	if(ctrl == CON_Down)
	{
		EffectVar(1, target, fxnum) = !release;
	}
}

local iSwingAnimation;

// Effect for smooth movement.
public func FxIntGrappleControlTimer(object target, int fxnum, int time)
{

	// Movement.
	if (EffectVar(0, target, fxnum))
		if (rope && time%2 == 0)
			rope->DoLength(-1);
	if (EffectVar(1, target, fxnum))
		if (rope)
			rope->DoLength(+1);
	if (EffectVar(2, target, fxnum))
	{
		rope->DoSpeed(-15);
	}
	if (EffectVar(3, target, fxnum))
	{
		rope->DoSpeed(+15);
	}

	if(target->GetAction() == "Tumble" && target->GetActTime() > 10)
		target->SetAction("Jump");

	if(target->GetAction() != "Jump")
	{
		if(rope->GetConnectStatus())
			rope->ConnectLoose();
	}
	
	if(target->GetAction() == "Jump" && rope->PullObjects() && !EffectVar(6, target, fxnum))
	{
		if(!EffectVar(4, target, fxnum))
		{
			target->SetTurnType(1);
			if(!target->GetHandAction())
				target->SetHandAction(-1);
		}
		target->SetObjDrawTransform(1000, 0, 3000*(1-2*target->GetDir()), 0, 1000);

		if(EffectVar(0, target, fxnum))
		{
			if(EffectVar(4, target, fxnum) != 2)
			{
				EffectVar(4, target, fxnum) = 2;
				target->PlayAnimation("RopeClimb", 10, Anim_Linear(target->GetAnimationLength("RopeClimb")/2, 0, target->GetAnimationLength("RopeClimb"), 35), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			}
		}
		else if(EffectVar(1, target, fxnum))
		{
			if(EffectVar(4, target, fxnum) != 3)
			{
				EffectVar(4, target, fxnum) = 3;
				target->PlayAnimation("RopeDown", 10, Anim_Const(0), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			}
		}
		else if(EffectVar(2, target, fxnum) || EffectVar(3, target, fxnum))
		{
			var start = target->GetAnimationLength("RopeSwing")/2;
			var length = target->GetAnimationLength("RopeSwing");
			var dir = 0;
			if( (EffectVar(2, target, fxnum) && !target->GetDir())
				|| (!EffectVar(2, target, fxnum) && target->GetDir())
				) dir = 1;
			if(EffectVar(4, target, fxnum) != 4+dir)
			{
				iSwingAnimation = target->PlayAnimation("RopeSwing", 10, Anim_Linear(start, length*dir, length*(!dir), 35, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
				EffectVar(4, target, fxnum) = 4+dir;
			}
		}
		else if(EffectVar(4, target, fxnum) != 1)
		{
			EffectVar(4, target, fxnum) = 1;
			target->PlayAnimation("OnRope", 10, Anim_Linear(0, 0, target->GetAnimationLength("OnRope"), 35*2, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		}
		var angle = rope->GetClonkAngle();
		var off = rope->GetClonkOff();
		target->SetProperty("MeshTransformation", Trans_Mul(Trans_Translate(0, -10000), Trans_Rotate(angle,0,0,1), Trans_Translate(-off[0],-off[1]+10000)));
	}
	else if(EffectVar(4, target, fxnum))
	{
		target->SetProperty("MeshTransformation");
		target->SetObjDrawTransform(1000, 0, 0, 0, 1000);
		target->StopAnimation(target->GetRootAnimation(10));
		if(!target->GetHandAction())
				target->SetHandAction(0);
		EffectVar(4, target, fxnum) = 0;
	}

	if(EffectVar(6, target, fxnum)) EffectVar(6, target, fxnum)--;
	
	return FX_OK;
}

public func FxIntGrappleControlStop(object target, int fxnum, int reason, int tmp)
{
	if(tmp) return;
	target->SetTurnType(0);
	target->SetProperty("MeshTransformation");
	target->StopAnimation(target->GetRootAnimation(10));
	target->SetObjDrawTransform();
	if(!target->GetHandAction())
		target->SetHandAction(0);
}