/*-- Sword --*/

#include Library_MeleeWeapon

private func Hit()
{
  Sound("WoodHit"); //TODO Some metal sond
}

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }


public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("PerspectiveR", 15000, def);
  SetProperty("PerspectiveTheta", 10, def);
  SetProperty("PerspectivePhi", -10, def);
}

public func ControlUseStart(object clonk, int x, int y)
{
	if(!CanStrikeWithWeapon(clonk)) return true;
	var slow=GetEffect("SwordStrikeSlow", clonk);
	
	// figure out the kind of attack to use
	var length=0;
	if(clonk->IsWalking())
	{
		length=30;
		if(!GetEffect("SwordStrikeSpeedUp", clonk) && !slow)
			AddEffect("SwordStrikeSpeedUp", clonk,  1, 5, this);
	} else
	if(clonk->IsJumping())
	{
		if(clonk->GetYDir() < 0) length=40;
		else length=GetJumpLength(clonk);
		
		if(!slow)
		if(!GetEffect("DelayTranslateVelocity", clonk))
		{
			TranslateVelocity(clonk, Angle(0, 0, x,y), 90);
			AddEffect("DelayTranslateVelocity", clonk, 1, 35*2);
		}
	}
	else return true;

	PlayWeaponAnimation(clonk, "StrikeArms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("StrikeArms"), length, ANIM_Remove), Anim_Const(1000));
	StartWeaponHitCheckEffect(clonk, length, 1);
	return true;
}


func ControlUseStop(object clonk, int x, int y)
{
	StopWeaponHitCheckEffect(clonk);
}

func OnWeaponHitCheckStop(clonk)
{
	if(GetEffect("SwordStrikeSpeedUp", clonk))
		RemoveEffect("SwordStrikeSpeedUp", clonk);
	return;
}

func WeaponStrikeExpired()
{
	if(Contained())
		this->ScheduleCall(this, "ControlUseStart", 1, 0, Contained(), 0, 0);
}

func CheckStrike(iTime)
{
	var width=8;
	var height=20;
	var slowedVelocity=GetWeaponSlow(Contained());
	var found=false;
	var angle=0;
	
	var doBash=Abs(Contained()->GetXDir()) > 5 || Abs(Contained()->GetYDir()) > 5;
	if(!doBash) doBash=Contained()->GetContact(-1) & CNAT_Bottom;
	
	if(doBash)
	{
		if(Contained()->GetDir() == DIR_Left)
			angle=-(Max(5, Abs(Contained()->GetXDir())));
		else angle=(Max(5, Abs(Contained()->GetXDir())));
	}
	
	for(var obj in FindObjects(Find_AtRect(-width/2, -height/2, width, height), Find_OCF(OCF_Living), Find_NoContainer(), Find_Exclude(Contained())))
	{
		found=true;
		var velocity=GetRelativeVelocity(Contained(), obj) * 2;
		velocity+= slowedVelocity / 10;
		velocity=Max(5, velocity);
		if(velocity > 30) velocity=30;
		
		var shield=ApplyShieldFactor(Contained(), obj, damage);
		if(shield == 100)
			continue;
		
		var damage=((100-shield)*(5000 * velocity) / 100) / 100;
		obj->DoEnergy(-damage,  true, 0, Contained()->GetOwner());
		if(doBash)
			ApplyWeaponBash(obj, velocity, Angle(0, 0, angle, Contained()->GetYDir()));
		
		
		if(doBash)
			DoWeaponSlow(obj, 10);
	}
	if(found && doBash) DoWeaponSlow(Contained(), 20);
}


func FxSwordStrikeSpeedUpStart(pTarget, iEffectNumber, iTemp)
{
	//if(iTemp) return;
	pTarget->SetPhysical("Walk", pTarget->GetPhysical("Walk", 0) * 2, PHYS_StackTemporary);
}

func FxSwordStrikeSpeedUpTimer(pTarget, iEffectNumber, iEffectTime)
{
	if(!pTarget->GetContact( -1) & CNAT_Bottom)
		return -1;
	if(iEffectTime > 35*2) return -1;
}

func FxSwordStrikeSpeedUpStop(pTarget, iEffectNumber, iCause, iTemp)
{
	pTarget->ResetPhysical("Walk");
	//pTarget->SetPhysical("Walk", pTarget->GetPhysical("Walk", 0) / 2, PHYS_Temporary);
	if(iTemp) return;
	if(!pTarget->GetAlive()) return;
	
	var time=GetEffect(0, 0, iEffectNumber, 6);
	AddEffect("SwordStrikeSlow", pTarget, 1, 5, 0, Sword, time);
}

func FxSwordStrikeSlowStart(pTarget, iEffectNumber, iTemp, iTime)
{
	pTarget->SetPhysical("Walk", pTarget->GetPhysical("Walk", 0) / 2, PHYS_StackTemporary);
	if(iTemp) return;
	EffectVar(0, pTarget, iEffectNumber) = iTime;
}

func FxSwordStrikeSlowTimer(pTarget, iEffectNumber, iEffectTime)
{
	if(iEffectTime > EffectVar(0, pTarget, iEffectNumber)) return -1;
}

func FxSwordStrikeSlowStop(pTarget, iEffectNumber, iCause, iTemp)
{
	pTarget->ResetPhysical("Walk");
	//pTarget->SetPhysical("Walk", pTarget->GetPhysical("Walk", 0) * 2, PHYS_Temporary);
}
