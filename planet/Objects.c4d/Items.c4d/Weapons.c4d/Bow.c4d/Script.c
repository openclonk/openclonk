/*
	Bow
	Author: Newton
	
	The standard bow. This object is a standard projectile weapon
	with an extra slot.
*/

// has extra slot
#include L_ES

local aimtime;
local iMesh;
local iAnimLoad;
local iArrowMesh;

local fAiming;

local iDrawAnim;

public func GetCarryMode() { return CARRY_HandBack; }
//public func GetCarryTransform() { return Trans_Mul(Trans_Mul(Trans_Mul(Trans_Rotate(90,0,10,0), Trans_Rotate(90,10,0,0)),Trans_Scale(1300)), Trans_Translate(0, -600, 0)); }
//public func GetCarryBone() { return "Handle"; }

public func GetCarrySpecial(clonk) { if(fAiming) return "pos_hand2"; }

public func HoldingEnabled() { return true; }

protected func ControlUseStart(object clonk, int x, int y)
{
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
	
	fAiming = 1;
  clonk->UpdateAttach();
	ScheduleCall(this, "AddArrow", 5*aimtime/20, 1, clonk);
	iAnimLoad = clonk->PlayAnimation("BowLoadArms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("BowLoadArms"), aimtime, ANIM_Remove), Anim_Const(1000));
  iDrawAnim = clonk->PlayAnimation("Draw", 6, Anim_Linear(0, 0, GetAnimationLength("Draw"), aimtime, ANIM_Hold), Anim_Const(1000), nil, clonk->GetHandMesh(this));
	return true;
}

public func AddArrow(clonk)
{
	if(!fAiming) return;
	iArrowMesh = clonk->AttachMesh(HARW, "pos_hand1", "main", Trans_Scale(2000, 1000, 2000));
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
	// adapt aiming animation
	// ...
	if(aimtime > 0) aimtime--;


	if(aimtime > 0)
	{
		if(aimtime == 1)
		{
			clonk->StopAnimation(iDrawAnim, clonk->GetHandMesh(this));
			iAnimLoad = clonk->PlayAnimation("BowAimArms", 10, Anim_Const(0), Anim_Const(1000));
      iDrawAnim = clonk->PlayAnimation("Draw", 6, Anim_Const(GetAnimationLength("Draw")), Anim_Const(1000), nil, clonk->GetHandMesh(this));
			clonk->SetAnimationPosition(iAnimLoad, Anim_Const(2000*Abs(90)/180));
		}
	}
	else if(!ClonkAimLimit(clonk,angle))
		;
	else
	{
		if(angle > 180) angle -= 360;
		var pos = clonk->GetAnimationPosition(iAnimLoad);
		pos += BoundBy(2000*Abs(angle)/180-pos, -100, 100);
		clonk->SetAnimationPosition(iAnimLoad, Anim_Const(pos));
	}

	return true;
}

protected func ControlUseStop(object clonk, int x, int y)
{
	fAiming = 0;
	clonk->StopAnimation(iDrawAnim, clonk->GetHandMesh(this));
	clonk->DetachMesh(iArrowMesh);

	// "canceled"	
	var angle = Angle(0,0,x,y);
	if(aimtime > 0 || !ClonkAimLimit(clonk,angle))
	{
		clonk->StopAnimation(clonk->GetRootAnimation(10));
  	clonk->UpdateAttach();
		return true;
	}
	
	if(Contents(0))
	{
		if(Contents(0)->~IsArrow())
		{
			var arrow = Contents(0)->TakeObject();
			arrow->Launch(angle,100,clonk);
		}
	}
	var iShootTime = 35/2;
	iDrawAnim = clonk->PlayAnimation("Fire", 6, Anim_Linear(0, 0, GetAnimationLength("Fire"), iShootTime, ANIM_Hold), Anim_Const(1000), nil, clonk->GetHandMesh(this));
	ScheduleCall(this, "Shoot", iShootTime, 0, clonk);
	return true;
}

public func Shoot(object clonk, int x, int y)
{
	// Already aiming angain? Don't remove Actions
	if(fAiming) return;
  clonk->StopAnimation(clonk->GetRootAnimation(10));
	clonk->StopAnimation(iDrawAnim, clonk->GetHandMesh(this));
	clonk->UpdateAttach();
}



protected func ControlUseCancel(object clonk, int x, int y)
{
  fAiming = 0;
  clonk->DetachMesh(iArrowMesh);
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	clonk->UpdateAttach();
	Message("",clonk);
}

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
	if(p != "WALK" && p != "ATTACH" && p != "FLIGHT") return false;
	return true;
}

func RejectCollect(id arrowid, object arrows)
{
	// arrows are not arrows? decline!
	if(!(arrows->~IsArrow())) return true;
}

func Definition(def) {
  SetProperty("Name", "$Name$", def);
}
