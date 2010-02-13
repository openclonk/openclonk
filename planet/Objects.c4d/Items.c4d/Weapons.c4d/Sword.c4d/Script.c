/*-- Sword --*/

private func Hit()
{
  Sound("WoodHit"); //TODO Some metal sond
}

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }

local fAttack;
local iAnimStrike;

static const SWOR_StrikeTime = 35;

public func ControlUse(object clonk, int x, int y)
{
	if(!fAttack)
	{
		Message("!Attack!", this);
		fAttack = 1;
//		if(iAnimStrike) clonk->StopAnimation(iAnimStrike);
		iAnimStrike = clonk->PlayAnimation("StrikeArms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("StrikeArms"), SWOR_StrikeTime, ANIM_Remove), Anim_Const(1000));
		ScheduleCall(this, "DoStrike", 15*SWOR_StrikeTime/20, 1, clonk);  // Do Damage at animation frame 15 of 20
		ScheduleCall(this, "StrikeEnd", SWOR_StrikeTime, 1, clonk);
	}
	return true;
}

public func DoStrike(clonk)
{
	if(!fAttack) return;
	clonk->CreateParticle("Blast", 20*(-1+2*clonk->GetDir()), 0, 0, 0, 20);
	// TODO Make Damage!
}

public func StrikeEnd(clonk)
{
	clonk->StopAnimation(iAnimStrike);
	fAttack = 0;
}

func FxIntSwordStart(pTarget, iNumber, fTmp)
{
	if(fTmp) return;
//	pTarget->SetPhysical("Walk", 20000, 2);
	fAttack = 0;
}

func FxIntSwordTimer(pTarget, iNumber, iTime)
{
	if(pTarget->GetAction() != "Walk" && pTarget->GetAction() != "Jump") return -1;
}

func FxIntSwordStop(pTarget, iNumber, iReason, fTmp)
{
	if(fTmp) return;
	if(fAttack) StrikeEnd(pTarget);
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("PerspectiveR", 15000, def);
  SetProperty("PerspectiveTheta", 10, def);
  SetProperty("PerspectivePhi", -10, def);
}