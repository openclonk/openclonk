/*
	Grapple Bow
	Author: Maikel
	
	A crossbow which is enabled to fire grappling hooks, also has a winching system.
*/

local aimtime;
local fAiming;

local iArrowMesh;
local iAnimLoad;
local iDrawAnim;
local iCloseAnim;

local help; // Help object, the clonk is attached to this object.

public func SetHelp(object tohelp)
{
	help = tohelp;
	return;
}

public func GetCarryMode() { return CARRY_HandBack; }

public func GetCarrySpecial(clonk) { if(fAiming) return "pos_hand2"; }

/* +++++++++++ Controls ++++++++++++++ */

// holding callbacks are made
public func HoldingEnabled() { return true; }

public func ControlUseCancel(object clonk, int x, int y)
{
	// Cut rope, or otherwise remove helper object.
	if (help)
	{
		var rope = help->GetRope();
		if (rope)
			rope->BreakRope();
		else
			help->RemoveObject();
		return true;
	}
	return true;
}

public func ControlUseStart(object clonk, int x, int y)
{
	// Cut rope, or otherwise remove helper object.
	if (help)
	{
		var rope = help->GetRope();
		if (rope)
			rope->BreakRope();
		else
			help->RemoveObject();
		return true;
	}

	// Reset the clonk (important, if the last aiming wasn't finished)
	ClearScheduleCall(this, "Shoot");
	ResetClonk(clonk);
	
	aimtime = 5;
	
	// Start aiming
	fAiming = 1;
	// walk slow
	AddEffect("IntWalkSlow", clonk, 1, 0, this);
	
	// Setting the hands as blocked, so that no other items are carried in the hands
	clonk->SetHandAction(1);
	// Update, that the Clonk takes the bow in the right hand (see GetCarrySpecial)
	clonk->UpdateAttach();
	// Attach the arrow during the animation
	ScheduleCall(this, "AddArrow", 5*aimtime/20, 1, clonk);
	iAnimLoad = clonk->PlayAnimation("BowLoadArms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("BowLoadArms"), aimtime, ANIM_Remove), Anim_Const(1000));
	iDrawAnim = PlayAnimation("Draw", 6, Anim_Linear(0, 0, GetAnimationLength("Draw"), aimtime, ANIM_Hold), Anim_Const(1000));

	clonk->SetTurnType(1, 1);
	
	return true;
}

public func ControlUseHolding(object clonk, int x, int y)
{
	// check procedure
	if(!ClonkCanAim(clonk))
	{
		clonk->CancelUse();
		return true;
	}

	// angle
	var angle = Angle(0,0,x,y);
	angle = Normalize(angle,-180);

	if(aimtime > 0)
	{
		if(aimtime == 30) Sound("GetArrow*.ogg");
		if(aimtime == 12) Sound("BowLoad*.ogg");
	
		aimtime--;
		if(aimtime == 1)
		{
			// Stop loading and start aiming
			StopAnimation(iDrawAnim);
			iAnimLoad = clonk->PlayAnimation("BowAimArms", 10, Anim_Const(0), Anim_Const(1000));
			iDrawAnim = PlayAnimation("Draw", 6, Anim_Const(GetAnimationLength("Draw")), Anim_Const(1000));
			clonk->SetAnimationPosition(iAnimLoad, Anim_Const(2000*Abs(90)/180));
		}
	}
	else 
	{
		if(Abs(angle) > 160) angle = 160;
		// Adjust the aiming position
		var pos = clonk->GetAnimationPosition(iAnimLoad);
		pos += BoundBy(2000*Abs(angle)/180-pos, -100, 100);
		clonk->SetAnimationPosition(iAnimLoad, Anim_Const(pos));
	}
	return true;
}

public func ControlUseStop(object clonk, int x, int y)
{
	if (!fAiming)
		return true;
	fAiming = 0;
	StopAnimation(iDrawAnim);
	clonk->DetachMesh(iArrowMesh);
	// not done reloading or out of range: cancel
	var angle = clonk->GetAnimationPosition(iAnimLoad)*180/2000;
	if(!clonk->GetDir()) angle = 360-angle;
	if(aimtime > 0 || !ClonkAimLimit(clonk,angle))
	{
		ResetClonk(clonk);
		return true;
	}

	// shoot
	var hook = CreateObject(GrappleHook, 0, 0, NO_OWNER);
	hook->Launch(angle, 100, clonk, this);
	Sound("BowShoot*.ogg");


	// Open the hand to let the string go and play the fire animation
	var iShootTime = 20;
	iDrawAnim = PlayAnimation("Fire", 6, Anim_Linear(0, 0, GetAnimationLength("Fire"), iShootTime, ANIM_Hold), Anim_Const(1000));
	iCloseAnim = clonk->PlayAnimation("Close1Hand", 11, Anim_Const(0), Anim_Const(1000));
	// Reset everything after the animation
	ScheduleCall(this, "Shoot", iShootTime, 0, clonk);
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	fAiming = 0;
	ResetClonk(clonk);
	return true;
}


/* ++++++++ Animation functions ++++++++ */

public func ResetClonk(clonk)
{
	// Already aiming angain? Don't remove Actions
	if(fAiming) return;

	clonk->SetTurnType(0, -1);

	clonk->SetHandAction(0);
	
	if(iArrowMesh != nil)
		clonk->DetachMesh(iArrowMesh);
	iArrowMesh = nil;

	if(iCloseAnim != nil)
		clonk->StopAnimation(iCloseAnim);
	iCloseAnim = nil;
	
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	if(iDrawAnim != nil)
		StopAnimation(iDrawAnim);
	iDrawAnim = nil;
	
	clonk->UpdateAttach();
	RemoveEffect("IntWalkSlow", clonk);
}

public func Shoot(object clonk, int x, int y)
{
	ResetClonk(clonk);
}

public func AddArrow(clonk)
{
	if(!fAiming) return;
	iArrowMesh = clonk->AttachMesh(HelpArrow, "pos_hand1", "main", nil);
}

/* ++++++++ Helper functions ++++++++ */

private func ClonkAimLimit(object clonk, int angle)
{
	angle = Normalize(angle,-180);
	if(Abs(angle) > 160) return false;
	if(clonk->GetDir() == 1 && angle < 0) return false;
	if(clonk->GetDir() == 0 && angle > 0) return false;
	return true;
}

private func ClonkCanAim(object clonk)
{
	var p = clonk->GetProcedure();
	//if(clonk->GetHandAction()) return false;
	if(p != "WALK" && p != "ATTACH" && p != "FLIGHT") return false;
	return true;
}

/* +++++++++++ Slow walk +++++++++++ */

func FxIntWalkSlowStart(pTarget, iNumber, fTmp)
{
	pTarget->SetPhysical("Walk", 30000, PHYS_StackTemporary);
}

func FxIntWalkSlowStop(pTarget, iNumber)
{
	pTarget->ResetPhysical("Walk");
}


protected func Definition(def) {
	SetProperty("Name", "$Name$", def);
	SetProperty("PerspectiveR", 15000, def);
}
