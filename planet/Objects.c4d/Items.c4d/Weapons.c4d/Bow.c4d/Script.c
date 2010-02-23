/*
	Bow
	Author: Newton
	
	The standard bow. This object is a standard projectile weapon
	with an extra slot.
*/

// has extra slot
#include L_ES

local aimtime;
local fAiming;

local iArrowMesh;
local iAnimLoad;
local iDrawAnim;
local iCloseAnim;

public func GetCarryMode() { return CARRY_HandBack; }

public func GetCarrySpecial(clonk) { if(fAiming) return "pos_hand2"; }


/* +++++++++++ Controls ++++++++++++++ */

// holding callbacks are made
public func HoldingEnabled() { return true; }

public func ControlUseStart(object clonk, int x, int y)
{
	// Reset the clonk (important, if the last aiming wasn't finished)
	ClearScheduleCall(this, "Shoot");
	ResetClonk(clonk);
	
	// check for ammo
	if(!Contents(0))
	{
		// reload
		var obj;
		if(obj = FindObject(Find_Container(clonk), Find_Func("IsArrow")))
		{
			obj->Enter(this);
		}
	}
	aimtime = 30;
	
	if(!Contents(0))
	{
		// + sound or message that he doesnt have arrows anymore
		clonk->CancelUse();
		return true;
	}
	
	
	
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
		// Turn clonk if aiming in the other direction and he isn't moving
		if(!ClonkAimLimit(clonk,angle))
			if(clonk->GetComDir() == COMD_Stop && !clonk->GetXDir())
			{
				if(clonk->GetDir() == 1 && angle < 0) clonk->SetDir(0);
				else if(clonk->GetDir() == 0 && angle > 0) clonk->SetDir(1);
			}
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
	if(Contents(0))
	{
		if(Contents(0)->~IsArrow())
		{
			var arrow = Contents(0)->TakeObject();
			arrow->Launch(angle,100,clonk);
			Sound("BowShoot*.ogg");
		}
	}

	// Open the hand to let the string go and play the fire animation
	var iShootTime = 35/2;
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
	iArrowMesh = clonk->AttachMesh(HARW, "pos_hand1", "main", nil);
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

/* +++++++++++ Various callbacks +++++++++ */

func RejectCollect(id arrowid, object arrows)
{
	// arrows are not arrows? decline!
	if(!(arrows->~IsArrow())) return true;
}

func Selection()
{
	Sound("DrawBow.ogg");
}

func Deselection()
{
	Sound("PutAwayBow.ogg");
}

func Definition(def) {
	SetProperty("Name", "$Name$", def);
	SetProperty("PerspectiveR", 15000, def);
}
