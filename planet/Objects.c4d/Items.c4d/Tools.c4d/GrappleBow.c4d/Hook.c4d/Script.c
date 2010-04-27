/*
	Grapple Hook
	Author: Maikel
	
	The hook can be shot with the grappling bow. 
	On impact the hook will 
*/

local rope; // The rope is the connection between the hook and the helper object.
local help; // The clonk is attached to the helper object.

public func ArrowStrength() { return 10; }

protected func Construction()
{
	SetR(90);
	return _inherited(...);
}

public func GetRope() { return rope; }

public func Launch(int angle, int str, object shooter, object bow)
{
	Exit();
		
	// Create rope and helper object.
	rope = CreateObject(GrappleRope, 0, 0, NO_OWNER);
//	help = CreateObject(GrappleHelp, 0, 0, NO_OWNER);

	rope->Connect(this, bow);
/*	help->SetBow(bow);
	help->SetClonk(shooter);
	help->SetRope(rope);
	bow->SetHelp(help);*/

	var xdir = Sin(angle,str);
	var ydir = Cos(angle,-str);
	SetXDir(xdir);
	SetYDir(ydir);
	SetR(angle);
	Sound("ArrowShoot*.ogg");
	
	AddEffect("InFlight", this, 1, 1, this);

	AddEffect("IntGrappleControl", shooter, 1, 1, this);
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
	
		var x = Sin(GetR(), +9);
		var y = Cos(GetR(), -9);
		if(GBackSolid(x,y) && 1)
		{
			//Log("Attach hook");
			// stick in landscape
			SetVertex(2,VTX_Y,-12,2);
			SetVertex(3,VTX_X,-3,2);
			SetVertex(3,VTX_Y,-9,2);
			SetVertex(4,VTX_X,+3,2);
			SetVertex(4,VTX_Y,-6,2);
			SetVertex(5,VTX_X,-3,2);
			SetVertex(5,VTX_Y,-6,2);
			SetVertex(6,VTX_X,+3,2);
			SetVertex(6,VTX_Y,-6,2);
		}

		if (rope)
			ScheduleCall(rope, "ConnectPull", 5);
//			rope->ConnectPull();
		if (help)
			help->HangClonkOntoMe();
	}
}

public func Hit()
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
	RemoveObject();
	return;
}

protected func Definition(def) {
	SetProperty("Name", "$Name$", def);
}


/*-- Grapple rope controls --*/

public func FxIntGrappleControlControl(object target, int fxnum, ctrl, x,y,strength, repeat, release)
{
	if(ctrl != CON_Up && ctrl != CON_Down && ctrl != CON_Right && ctrl != CON_Left) return;

	if(ctrl == CON_Right)
	{
//		target->SetDir(DIR_Right);
		EffectVar(3, target, fxnum) = !release;
	}
	if(ctrl == CON_Left)
	{
//		target->SetDir(DIR_Right);
		EffectVar(2, target, fxnum) = !release;
	}
	if(ctrl == CON_Up)
	{
//		target->SetDir(DIR_Right);
		EffectVar(0, target, fxnum) = !release;
	}
	if(ctrl == CON_Down)
	{
//		target->SetDir(DIR_Right);
		EffectVar(1, target, fxnum) = !release;
	}
}

// Effect for smooth movement.
public func FxIntGrappleControlTimer(object target, int fxnum, int time)
{
/*	if (!EffectVar(0, target, fxnum) && !EffectVar(1, target, fxnum)
		&& !EffectVar(2, target, fxnum) && !EffectVar(3, target, fxnum))
		return 0;*/

	// Movement.
	if (EffectVar(0, target, fxnum))
		if (rope)
			rope->DoLength(-1);
	if (EffectVar(1, target, fxnum))
		if (rope)
			rope->DoLength(+1);
	if (EffectVar(2, target, fxnum))
		SetXDir(GetXDir(100) - 20, 100);
	if (EffectVar(3, target, fxnum))
		SetXDir(GetXDir(100) + 20, 100);

	if(target->GetAction() == "Jump")
	{
		if(!EffectVar(4, target, fxnum))
		{
			target->SetTurnType(1);
			target->SetObjDrawTransform(1000, 0, 3000*(1-2*target->GetDir()), 0, 1000);
		}
		var xoff = +rope->GetPartXOffset(-1)*10, yoff = rope->GetPartYOffset(-1)*10;
		target->SetObjDrawTransform(1000, 0, 3000*(1-2*target->GetDir())+xoff, 0, 1000, yoff);

		if(EffectVar(4, target, fxnum) != 2 && EffectVar(0, target, fxnum))
		{
			EffectVar(4, target, fxnum) = 2;
			target->PlayAnimation("RopeClimb", 10, Anim_Linear(target->GetAnimationLength("RopeClimb")/2, 0, target->GetAnimationLength("RopeClimb"), 35, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		}
		if(EffectVar(4, target, fxnum) != 1 && !EffectVar(0, target, fxnum))
		{
			EffectVar(4, target, fxnum) = 1;
			target->PlayAnimation("OnRope", 10, Anim_Const(0), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		}
	}
	else if(EffectVar(4, target, fxnum))
	{
		target->StopAnimation(target->GetRootAnimation(10));
		EffectVar(4, target, fxnum) = 0;
	}
	
	return FX_OK;
}