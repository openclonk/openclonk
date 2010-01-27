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

local fAiming;

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryTransform() { return Trans_Mul(Trans_Mul(Trans_Mul(Trans_Rotate(90,0,10,0), Trans_Rotate(90,10,0,0)),Trans_Scale(1300)), Trans_Translate(0, -600, 0)); }
public func GetCarryBone() { return "Handle"; }

public func GetCarrySpecial(clonk) { if(fAiming) return "pos_hand2"; }

public func HoldingEnabled() { return true; }

protected func ControlUseStart(object clonk, int x, int y)
{
	fAiming = 1;
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
	}
  clonk->UpdateAttach();
	iAnimLoad = clonk->PlayAnimation("BowAimArms", 10, Anim_Const(0), Anim_Const(1000));
	var test = clonk->PlayAnimation("Draw", 6, Anim_Linear(0, 0, GetAnimationLength("Draw"), 35, ANIM_Loop), Anim_Const(1000), nil, clonk->GetHandMesh(this));
	Log("%d", test);
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
	// adapt aiming animation
	// ...
	if(aimtime > 0) aimtime--;

	// debug....
	var points = "";
	for(var i=2; i>=0; --i) { if(i<(aimtime/5)%4) points = Format(".%s",points); else points = Format("%s ",points); }

	if(aimtime > 0)
		Message("Reload arrow%s",clonk,points);
	else if(!ClonkAimLimit(clonk,angle))
		Message("Cannot aim there",clonk);
	else
	{
		if(angle > 180) angle -= 360;
		clonk->SetAnimationPosition(iAnimLoad, Anim_Const(2000*Abs(angle)/180));
		Message("Aiming|%d!!!!!",clonk,angle);
	}

	return true;
}

protected func ControlUseStop(object clonk, int x, int y)
{
	fAiming = 0;
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	clonk->UpdateAttach();
	Message("",clonk);

	// "canceled"
	if(aimtime > 0) return true;
	
	var angle = Angle(0,0,x,y);
	if(!ClonkAimLimit(clonk,angle)) return true;
	
	if(Contents(0))
	{
		if(Contents(0)->~IsArrow())
		{
			var arrow = Contents(0)->TakeObject();
			arrow->Launch(angle,100,clonk);
		}
	}
	return true;
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