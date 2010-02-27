/*-- Shield --*/

#include L_WN

private func Hit()
{
  Sound("WoodHit"); //TODO Some metal sond
}

local iAngle;
// TODO:
// The clonk must be able to duck behind the shield
// when he ducks, he is at least invulnerable against spears and arrows,
// other bonuses perhaps too

public func ControlUseStart(object clonk, int x, int y)
{
	if(!CanStrikeWithWeapon(clonk)) return true;
	
	// figure out the kind of attack to use
	var length=0;
	if(clonk->IsWalking())
	{
		length=10;
	} else
	if(clonk->IsJumping())
	{
		if(clonk->GetYDir() < 0) length=10;
		else length=GetJumpLength(clonk);
	}
	else return true;

	PlayWeaponAnimation(clonk, "Strike2Arms", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("Strike2Arms"), length, ANIM_Hold), Anim_Const(1000));
	StartWeaponHitCheckEffect(clonk, -1, 1);
	iAngle=Angle(0,0, x,y);
	return true;
}

func ControlUseHolding(clonk, x, y)
{
	var angle=Angle(0,0, x,y);
	if(clonk->GetDir() == DIR_Left)
	{
		if(!Inside(angle, 180, 360)) return;
	}
	else if(!Inside(angle, 0, 180)) return;
	iAngle=angle;
}

func ControlUseStop(object clonk, int x, int y)
{
	StopWeaponHitCheckEffect(clonk);
	StopWeaponAnimation(clonk);
}

func OnWeaponHitCheckStop()
{
	return;
}

func CheckStrike(iTime)
{
	var found=false;
	for(var obj in FindObjects(Find_Distance(20), Find_Category(C4D_Object), Find_NoContainer(), Find_Exclude(Contained())))
	{

		if(obj->GetX() > GetX())
		{
			if(obj->GetXDir() > 0) continue;
		}
		else if(obj->GetXDir() < 0) continue;

		var a=Angle(GetX(), GetY(), obj->GetX(), obj->GetY());
		if(!Inside(a, iAngle-45, iAngle+45))
			if(!Inside(a+360, iAngle-45, iAngle+45)) continue;

		var speed=Sqrt((obj->GetXDir(100)**2) + (obj->GetYDir(100)**2));
		if(Abs(iAngle-a) > Abs(iAngle-(a+360)))
			a=((a+360) + iAngle)/2;
		if(!speed) continue;
		if(speed > 5000) continue;
		speed=Max(150, speed-200);
		obj->SetXDir(+Sin(a, speed), 100);
		obj->SetYDir(-Cos(a, speed), 100); 
	}
	/*for(var cnt=0;cnt<10;++cnt)
	{
		CreateParticle("Spark", Sin(iAngle, 4*cnt), -Cos(iAngle, 4*cnt), 0, 0, 50, RGB(255,255,255));
	}*/
}

func HitByWeapon(pFrom, iDamage)
{
	if(pFrom->GetX() > GetX())
	{
		if(!Inside(iAngle, 0, 180)) return 0;
	}
	else if(!Inside(iAngle, 180, 360)) return 0;
		
	// bash him hard!
	ApplyWeaponBash(pFrom, 50, iAngle);
	
	// shield factor
	return 50;
}

public func HoldingEnabled() { return true; }

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryTransform(clonk, sec, back)
{
	if(mTrans != nil) return mTrans;
	if(!sec)
	{
		if(back) return Trans_Mul(Trans_Rotate(-90, 0, 0, 1),Trans_Translate(0,0,-400));
		return nil;
	}
	if(back) return Trans_Mul(Trans_Mul(Trans_Rotate(90, 0, 0, 1),Trans_Rotate(180, 0, 1)),Trans_Translate(0,0,-400));
	return Trans_Rotate(180,1,0,0);
}

local mTrans;

func Definition(def) {
  SetProperty("Collectible", 1, def);
  SetProperty("Name", "$Name$", def);
  SetProperty("PerspectiveR", 9500, def);
  SetProperty("PerspectiveTheta", 15, def);
  SetProperty("PerspectivePhi", 20, def);
}
