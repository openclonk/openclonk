/*-- Club --*/

#include Library_MeleeWeapon

private func Hit()
{
  Sound("WoodHit");
}

public func GetCarryMode() { return CARRY_HandBack; }
//public func GetCarryTransform() { return Trans_Scale(130); }

public func GetCarrySpecial(clonk) { if(fAiming) return "pos_hand2"; }

public func HoldingEnabled() { return true; }

local fAiming;
local iAim1;
local iAim2;
local iAimKnot;

local iWait;

public func ControlUseStart(object clonk, int x, int y)
{
	// Not finished last strike?
	if(fAiming == 1)
	{
		// Wait
		iWait = 1;
		return true;
	}
	iWait = 0;
	fAiming = 1;

	clonk->SetHandAction(1); // Setting the hands as blocked, so that no other items are carried in the hands
  clonk->UpdateAttach(); // Update, that the Clonk takes the bow in the right hand (see GetCarrySpecial)

	var iLoopTime = 35*3;
	iAim1 = clonk->PlayAnimation("BatAimArms",  10, Anim_Linear(0, 0, clonk->GetAnimationLength("BatAimArms"),  iLoopTime, ANIM_Loop), Anim_Const(1000));
	iAim2 = clonk->PlayAnimation("BatAim2Arms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("BatAim2Arms"), iLoopTime, ANIM_Loop), Anim_Const(1000), iAim1);
	iAimKnot = iAim2 + 1;

	AddEffect("IntWalkSlow", clonk, 1, 0, 0, Bow);

	return true;
}

public func ControlUseHolding(object clonk, int x, int y)
{
	// Still waiting?
	if(iWait)
	{
		// Last move finished?
		if(fAiming == 0)
			ControlUseStart(clonk, x, y);
		// do nothing and wait
		return;
	}
	// angle
	var angle = Angle(0,0,x,y);
	angle = Normalize(angle,-180);

	clonk->SetAnimationWeight(iAimKnot, Anim_Const(Abs(angle)*1000/180));

}

protected func ControlUseStop(object clonk, int x, int y)
{
	if(iWait) return;
	var iStrikeTime = 35/2;
	iAim1 = clonk->PlayAnimation("BatStrikeArms",  10, Anim_Linear(0, 0, clonk->GetAnimationLength("BatStrikeArms"),  iStrikeTime, ANIM_Remove), Anim_Const(1000));
	iAim2 = clonk->PlayAnimation("BatStrike2Arms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("BatStrike2Arms"), iStrikeTime, ANIM_Remove), Anim_Const(1000), iAim1);
	iAimKnot = iAim2 + 1;

	var angle = Angle(0,0,x,y);
	angle = Normalize(angle,-180);

	clonk->SetAnimationWeight(iAimKnot, Anim_Const(Abs(angle)*1000/180));

	ScheduleCall(this, "EndStrike", iStrikeTime, 0, clonk);
	// At frame 6 of the animation the bat reaches it's hit point here the hit should take place TODO: Zapper!
	ScheduleCall(this, "DoStrike", iStrikeTime*600/(clonk->GetAnimationLength("BatStrikeArms")), 0, clonk, angle);
}

public func EndStrike(object clonk, int x, int y)
{
	fAiming = 0;
	ResetClonk(clonk);
}

func DoStrike(clonk, angle)
{
	var x=Sin(angle, 7);
	var y=-Cos(angle, 7);
	
	for(var obj in FindObjects(Find_Distance(7, x, y), Find_Or(Find_OCF(OCF_Alive), Find_Category(C4D_Object)), Find_Exclude(clonk), Find_NoContainer()))
	{
		if(obj->GetOCF() & OCF_Alive)
		{
			ApplyWeaponBash(obj, 400, angle);
		}
		else
		{
			obj->SetXDir((obj->GetXDir(100) + Sin(angle, 2000)) / 2, 100);
			obj->SetYDir((obj->GetYDir(100) - Cos(angle, 2000)) / 2, 100);
		}
	}
}

protected func ControlUseCancel(object clonk, int x, int y)
{
  fAiming = 0;
  ResetClonk(clonk);
}

public func ResetClonk(clonk)
{
	// Already aiming angain? Don't remove Actions
	if(fAiming) return;

	clonk->SetHandAction(0);

  clonk->StopAnimation(clonk->GetRootAnimation(10));

	clonk->UpdateAttach();
	RemoveEffect("IntWalkSlow", clonk);
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
	SetProperty("PerspectiveR", 14000, def);
  SetProperty("PerspectiveTheta", 20, def);
  SetProperty("PerspectivePhi", 40, def);
}
