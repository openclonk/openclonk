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
local fx_hook;

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

	fx_hook = AddEffect("IntGrappleControl", clonk, 1, 1, this);
	if(clonk->GetAction() == "Jump")
	{
		rope->AdjustClonkMovement();
		rope->ConnectPull();
		fx_hook.var5 = 1;
		fx_hook.var6 = 10;
	}
}

public func Hit(x, y)
{
	Stick();
}

// rotate arrow according to speed
public func FxInFlightStart(object target, effect, int temp)
{
	if(temp) return;
	effect.var0 = target->GetX();
	effect.var1 = target->GetY();
}
public func FxInFlightTimer(object target, effect, int time)
{
	var oldx = effect.var0;
	var oldy = effect.var1;
	var newx = GetX();
	var newy = GetY();
	
	// and additionally, we need one check: If the arrow has no speed
	// anymore but is still in flight, we'll remove the hit check
	if(oldx == newx && oldy == newy)
	{
		// but we give the arrow 5 frames to speed up again
		effect.var2++;
		if(effect.var2 >= 10)
			return Hit();
	}
	else
		effect.var2 = 0;

	// rotate arrow according to speed
	var anglediff = Normalize(Angle(oldx,oldy,newx,newy)-GetR(),-180);
	SetRDir(anglediff/2);
	effect.var0 = newx;
	effect.var1 = newy;
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
	RemoveEffect(0, clonk, fx_hook);
	RemoveObject();
	return;
}

local Name = "$Name$";


/*-- Grapple rope controls --*/

public func FxIntGrappleControlControl(object target, fxnum, ctrl, x,y,strength, repeat, release)
{
	// Cancel this effect if clonk is now attached to something
	if (target->GetProcedure() == "ATTACH") {
		RemoveEffect(nil,target,fxnum);
		return false;
	}

	if(ctrl != CON_Up && ctrl != CON_Down && ctrl != CON_Right && ctrl != CON_Left) return false;

	if(ctrl == CON_Right)
	{
		fxnum.var3 = !release;
	}
	if(ctrl == CON_Left)
	{
		fxnum.var2 = !release;
	}
	if(ctrl == CON_Up)
	{
		fxnum.var0 = !release;
		if(target->GetAction() == "Jump" && !release && pull)
			rope->ConnectPull();
	}
	if(ctrl == CON_Down)
	{
		fxnum.var1 = !release;
	}
	
	// never swallow the control
	return false;
}

local iSwingAnimation;

// Effect for smooth movement.
public func FxIntGrappleControlTimer(object target, fxnum, int time)
{
	// Cancel this effect if clonk is now attached to something
	// this check is also in the timer because on a high control rate
	// (higher than 1 actually), the timer could be called first
	if (target->GetProcedure() == "ATTACH")
		return -1;
	// Also cancel if the clonk is contained
	if (target->Contained())
		return -1;

	// EffectVars:
	// 0 - movement up
	// 1 - movement down
	// 2 - movement left
	// 3 - movement right
	// 4 -
	// 5 -
	// 6 -
		
	// Movement.
	if (fxnum.var0)
		if (rope && time%2 == 0)
			rope->DoLength(-1);
	if (fxnum.var1)
		if (rope)
			rope->DoLength(+1);
	if (fxnum.var2)
	{
		rope->DoSpeed(-10);
	}
	if (fxnum.var3)
	{
		rope->DoSpeed(+10);
	}

	if(target->GetAction() == "Tumble" && target->GetActTime() > 10)
		target->SetAction("Jump");

	if(target->GetAction() != "Jump")
	{
		if(rope->GetConnectStatus())
			rope->ConnectLoose();
	}
	
	if(target->GetAction() == "Jump" && rope->PullObjects() && !fxnum.var6)
	{
		if(!fxnum.var4)
		{
			target->SetTurnType(1);
			if(!target->GetHandAction())
				target->SetHandAction(-1);
		}
		target->SetObjDrawTransform(1000, 0, 3000*(1-2*target->GetDir()), 0, 1000);

		if(fxnum.var0)
		{
			if(fxnum.var4 != 2)
			{
				fxnum.var4 = 2;
				target->PlayAnimation("RopeClimb", 10, Anim_Linear(target->GetAnimationLength("RopeClimb")/2, 0, target->GetAnimationLength("RopeClimb"), 35), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			}
		}
		else if(fxnum.var1)
		{
			if(fxnum.var4 != 3)
			{
				fxnum.var4 = 3;
				target->PlayAnimation("RopeDown", 10, Anim_Const(0), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			}
		}
		else if(fxnum.var2 || fxnum.var3)
		{
			var start = target->GetAnimationLength("RopeSwing")/2;
			var length = target->GetAnimationLength("RopeSwing");
			var dir = 0;
			if( (fxnum.var2 && !target->GetDir())
				|| (!fxnum.var2 && target->GetDir())
				) dir = 1;
			if(fxnum.var4 != 4+dir)
			{
				iSwingAnimation = target->PlayAnimation("RopeSwing", 10, Anim_Linear(start, length*dir, length*(!dir), 35, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
				fxnum.var4 = 4+dir;
			}
		}
		else if(fxnum.var4 != 1)
		{
			fxnum.var4 = 1;
			target->PlayAnimation("OnRope", 10, Anim_Linear(0, 0, target->GetAnimationLength("OnRope"), 35*2, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		}
		var angle = rope->GetClonkAngle();
		var off = rope->GetClonkOff();
		target->SetMeshTransformation(Trans_Mul(Trans_Translate(0, -10000), Trans_Rotate(angle,0,0,1), Trans_Translate(-off[0],-off[1]+10000)), 2);
	}
	else if(fxnum.var4)
	{
		target->SetMeshTransformation(0, 2);
		target->SetObjDrawTransform(1000, 0, 0, 0, 1000);
		target->StopAnimation(target->GetRootAnimation(10));
		if(!target->GetHandAction())
				target->SetHandAction(0);
		fxnum.var4 = 0;
	}

	if(fxnum.var6) fxnum.var6--;
	
	return FX_OK;
}

public func FxIntGrappleControlStop(object target, fxnum, int reason, int tmp)
{
	if(tmp) return;
	target->SetTurnType(0);
	target->SetMeshTransformation(0, 2);
	target->StopAnimation(target->GetRootAnimation(10));
	target->SetObjDrawTransform();
	if(!target->GetHandAction())
		target->SetHandAction(0);
	
	// grapple hook == this:
	// if the hook is not already drawing in, break the rope
	if(!GetEffect("DrawIn", this->GetRope()))
	{
		this->GetRope()->BreakRope();
	}
}

public func NoWindjarForce() {	return true; }
