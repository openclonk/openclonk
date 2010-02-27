/*-- Sword --*/

#include L_WN

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
	
	// figure out the kind of attack to use
	var length=0;
	if(clonk->IsWalking())
	{
		length=30;
	} else
	if(clonk->IsJumping())
	{
		if(clonk->GetYDir() < 0) length=40;
		else length=GetJumpLength(clonk);
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

func OnWeaponHitCheckStop()
{
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
	
	if(Contained()->GetDir() == DIR_Left)
		angle=-(Max(5, Abs(Contained()->GetXDir())));
	else angle=(Max(5, Abs(Contained()->GetXDir())));
	
	for(var obj in FindObjects(Find_AtRect(-width/2, -height/2, width, height), Find_OCF(OCF_Living), Find_NoContainer(), Find_Exclude(Contained())))
	{
		var velocity=GetRelativeVelocity(Contained(), obj) * 2;
		velocity+= slowedVelocity / 10;
		velocity=Max(5, velocity);
		if(velocity > 30) velocity=30;
		
		var shield=ApplyShieldFactor(Contained(), obj, damage);
		
		var damage=((100-shield)*(5000 * velocity) / 100) / 100;
		obj->DoEnergy(-damage,  true, 0, Contained()->GetOwner());
		ApplyWeaponBash(obj, velocity, Angle(0, 0, angle, Contained()->GetYDir()));
		found=true;
		DoWeaponSlow(obj, 10);
	}
	if(found) DoWeaponSlow(Contained(), 20);
}
