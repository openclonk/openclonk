/*-- 
	Javelin
	Author: Ringwaul
	
	A simple but dangerous throwing weapon.
--*/

#include L_ST
public func MaxStackCount() { return 3; }

local fAiming;
local power;

local iAim;
local fWait;

public func GetCarryMode(clonk) { if(fAiming >= 0) return CARRY_HandBack; }
public func GetCarryBone() { return "Javelin"; }
public func GetCarrySpecial(clonk) { if(fAiming >= 0) return "pos_hand2"; }
public func GetCarryTransform() { if(fAiming == 1) return Trans_Rotate(180, 1, 0, 0); }

public func ControlUseStart(object pClonk, int x, int y)
{
	// if the clonk doesn't have an action where he can use it's hands do nothing
	if(!pClonk->HasHandAction())
	{
		fWait = 1;
		return true;
	}
	fWait = false;
	fAiming=true;
	pClonk->SetHandAction(1);
	pClonk->UpdateAttach();
	iAim = pClonk->PlayAnimation("SpearAimArms", 10, Anim_Const(0), Anim_Const(1000));

	var angle = Angle(0,0,x,y);
	angle = Normalize(angle,-180);
	pClonk->SetAnimationPosition(iAim, Anim_Const(Abs(angle)*pClonk->GetAnimationLength("SpearAimArms")/180));
	return 1;
}

public func HoldingEnabled() { return true; }

protected func ControlUseHolding(object pClonk, int ix, int iy)
{
	if(fWait)
	{
		if(pClonk->HasHandAction())
			ControlUseStart(pClonk, ix, iy);
		return 0;
	}
	var p = pClonk->GetProcedure();
	if(p != "WALK" && p != "FLIGHT")
	{
		fAiming = 0;
		ResetClonk(pClonk);
		fWait = 1;
		return 1;
	}
//	if(power<30) power=power+2;
//	Message("%d",pClonk,power+30);

	// angle
	var angle = Angle(0,0,ix,iy);
	angle = Normalize(angle,-180);

	var iTargetPosition = Abs(angle)*pClonk->GetAnimationLength("SpearAimArms")/180;
	var iPos = pClonk->GetAnimationPosition(iAim);
	iPos += BoundBy(iTargetPosition-iPos, -50, 50);
	pClonk->SetAnimationPosition(iAim, Anim_Const(iPos));
	if( (pClonk->GetComDir() == COMD_Stop && !pClonk->GetXDir()) || pClonk->GetAction() == "Jump")
	{
		if(pClonk->GetDir() == 1 && angle < 0) pClonk->SetDir(0);
		else if(pClonk->GetDir() == 0 && angle > 0) pClonk->SetDir(1);
	}
	return 1;
}

static const JAVE_ThrowTime = 16;

protected func ControlUseStop(object pClonk, int ix, int iy)
{
	if(fWait) return;
	var p = pClonk->GetProcedure();
	if(p != "WALK" && p != "FLIGHT")
	{
		fAiming = 0;
		ResetClonk(pClonk);
		return 1;
	}

	var angle = pClonk->GetAnimationPosition(iAim)*180/(pClonk->GetAnimationLength("SpearAimArms"));
	if(!pClonk->GetDir()) angle = -angle;

	var iThrowtime = JAVE_ThrowTime;
	if(Abs(angle) < 90)
	{
		iAim = pClonk->PlayAnimation("SpearThrow2Arms",  10, Anim_Linear(0, 0, pClonk->GetAnimationLength("SpearThrow2Arms" ), iThrowtime), Anim_Const(1000));
		iAim = pClonk->PlayAnimation("SpearThrowArms", 10, Anim_Linear(0, 0, pClonk->GetAnimationLength("SpearThrowArms"), iThrowtime), Anim_Const(1000), iAim);
		pClonk->SetAnimationWeight(iAim+1, Anim_Const(1000*Abs(angle)/90));
	}
	else
	{
		iAim = pClonk->PlayAnimation("SpearThrowArms",  10, Anim_Linear(0, 0, pClonk->GetAnimationLength("SpearThrowArms" ), iThrowtime), Anim_Const(1000));
		iAim = pClonk->PlayAnimation("SpearThrow3Arms", 10, Anim_Linear(0, 0, pClonk->GetAnimationLength("SpearThrow3Arms"), iThrowtime), Anim_Const(1000), iAim);
		pClonk->SetAnimationWeight(iAim+1, Anim_Const(1000*(Abs(angle)-90)/90));
	}

	ScheduleCall(this, "DoThrow", iThrowtime/2, 1, pClonk, angle);
}

protected func ControlUseCancel(object clonk, int x, int y)
{
  fAiming = 0;
  ResetClonk(clonk);
}

public func DoThrow(object pClonk, int angle)
{
	var javelin=TakeObject();
	javelin->LaunchProjectile(angle+RandomX(-1, 1), 6, 90);
	javelin->AddEffect("Flight",javelin,1,1,javelin,nil);
	javelin->AddEffect("HitCheck",javelin,1,1,nil,nil,pClonk);
	
	power=0;
	
	fAiming = -1;
  pClonk->UpdateAttach();
	ScheduleCall(this, "ResetClonk", JAVE_ThrowTime/2, 1, pClonk);
}

public func ResetClonk(clonk)
{
	// Already aiming angain? Don't remove Actions
	if(fAiming == 1) return;
	fAiming = 0;

	clonk->SetHandAction(0);

  clonk->StopAnimation(clonk->GetRootAnimation(10));

	clonk->UpdateAttach();
}

protected func JavelinStrength() { return 14; }

//slightly modified HitObject() from arrow
public func HitObject(object obj)
{
	var speed = Sqrt(GetXDir()*GetXDir()+GetYDir()*GetYDir());

	if(obj->~QueryCatchBlow(this)) return;
  
	// arrow hit
	obj->~OnArrowHit(this);
	if(!this) return;
	// ouch!
	var dmg = JavelinStrength()*speed/100;
	if(obj->GetAlive())
	{
		obj->DoEnergy(-dmg);
	    obj->~CatchBlow(-dmg,this);
	}
	
	RemoveEffect("HitCheck",this);

	// target could have done something with this arrow
	if(!this) return;
	
	// tumble target
    if(obj->GetAlive())
    {
		obj->SetAction("Tumble");
		obj->SetSpeed(obj->GetXDir()+GetXDir()/3,obj->GetYDir()+GetYDir()/3-1);
    }
}

private func Hit()
{	
	Sound("WoodHit");
	SetSpeed();
	RemoveEffect("Flight",this);
	RemoveEffect("HitCheck",this);
}

protected func FxFlightStart(object pTarget, int iEffectNumber)
{
	pTarget->SetProperty("Collectible",0);
	pTarget->SetR(Angle(0,0,pTarget->GetXDir(),pTarget->GetYDir()));
}

protected func FxFlightTimer(object pTarget,int iEffectNumber, int iEffectTime)
{
	//Using Newton's arrow rotation. This would be much easier if we had tan^-1 :(
	var oldx = EffectVar(0,pTarget,iEffectNumber);
	var oldy = EffectVar(1,pTarget,iEffectNumber);
	var newx = GetX();
	var newy = GetY();

	var anglediff = Normalize(Angle(oldx,oldy,newx,newy)-GetR(),-180);
	pTarget->SetRDir(anglediff/2);
	pTarget->EffectVar(0,pTarget,iEffectNumber) = newx;
	pTarget->EffectVar(1,pTarget,iEffectNumber) = newy;
	pTarget->SetR(Angle(0,0,pTarget->GetXDir(),pTarget->GetYDir()));
}

protected func FxFlightStop(object pTarget,int iEffectNumber)
{
	pTarget->SetProperty("Collectible", 1);
}

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("PerspectiveR", 15000, def);
  SetProperty("PerspectiveTheta", 10, def);
  SetProperty("PerspectivePhi", -10, def);
}