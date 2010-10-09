/*-- Sword --*/

#include Library_MeleeWeapon

private func Hit()
{
	Sound("WoodHit"); //TODO Some metal sond
}

public func Initialize()
{
	PlayAnimation("Base", 5, Anim_Const(0), Anim_Const(1000));
	return _inherited(...);
}

public func GetCarryMode() { return CARRY_HandBack; }
public func GetCarryBone() { return "main"; }
public func GetCarrySpecial(clonk) { return carry_bone; }
public func GetCarryTransform() { return Trans_Rotate(90, 0, 1, 0); }


public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

local magic_number;
local carry_bone;
public func ControlUseStart(object clonk, int x, int y)
{
	if(!CanStrikeWithWeapon(clonk)) return true;
	var slow=GetEffect("SwordStrikeSlow", clonk);

	var arm = "R";
	carry_bone = "pos_hand2";
	if(clonk->GetItemPos(this) == 1)
	{
		arm = "L";
		carry_bone = "pos_hand1";
	}
	var rand = Random(2)+1;
	var animation = Format("SwordSlash%d.%s", rand, arm);
	if(clonk->GetAction() == "Jump")
	{
		rand = 1;
		if(clonk->GetYDir() < -5) rand = 2;
		animation = Format("SwordJump%d.%s",rand,arm);
	}
	var animation_sword = Format("Strike%d", rand);
	
	// figure out the kind of attack to use
	var length=15;
	if(clonk->IsWalking())
	{
		//length=20;
		/*if(!GetEffect("SwordStrikeSpeedUp", clonk) && !slow)
			AddEffect("SwordStrikeSpeedUp", clonk, 1, 5, this);*/
		if(!GetEffect("SwordStrikeStop", clonk, 0))
			AddEffect("SwordStrikeStop", clonk, 2, 50, this);
	} else
	if(clonk->IsJumping())
	{
		//if(clonk->GetYDir() < 0) length=20;
		//else length=GetJumpLength(clonk);
		//length=40;
		
		if(!slow)
		if(!GetEffect("DelayTranslateVelocity", clonk))
		{
			//TranslateVelocity(clonk, Angle(0, 0, x,y), 0, 300, 1);
			var a=Angle(0, 0, x,y);
			if(Inside(a, 35+90, 35+180))
			{
				clonk->SetXDir(Sin(a, 60));
				clonk->SetYDir(-Cos(a, 60));
				AddEffect("DelayTranslateVelocity", clonk, 2, 3, nil, Library_MeleeWeapon);
				animation = Format("SwordJump3.%s",arm);
			}
		}
	}
	//else return true;*/
	if(!clonk->IsWalking() && !clonk->IsJumping()) return true;

	PlayWeaponAnimation(clonk, animation, 10, Anim_Linear(0, 0, clonk->GetAnimationLength(animation), length, ANIM_Remove), Anim_Const(1000));
	PlayAnimation(animation_sword, 10, Anim_Linear(0, 0, GetAnimationLength(animation_sword), length, ANIM_Remove), Anim_Const(1000));
	clonk->UpdateAttach();
	
	magic_number=((magic_number+1)%10) + (ObjectNumber()*10);
	StartWeaponHitCheckEffect(clonk, length, 1);
	
	this->Sound(Format("WeaponSwing%d.ogg", 1+Random(3)), false, nil, nil, nil);
	return true;
}



func ControlUseStop(object clonk, int x, int y)
{
	//StopWeaponHitCheckEffect(clonk);
}

func OnWeaponHitCheckStop(clonk)
{
	carry_bone = nil;
	clonk->UpdateAttach();
	if(GetEffect("SwordStrikeSpeedUp", clonk))
		RemoveEffect("SwordStrikeSpeedUp", clonk);
	//if(GetEffect("DelayTranslateVelocity", clonk))
	//	RemoveEffect("DelayTranslateVelocity", clonk);
	return;
}

func WeaponStrikeExpired()
{
	//if(Contained())
	//	this->ScheduleCall(this, "ControlUseStart", 1, 0, Contained(), 0, 0);
	if(GetEffect("SwordStrikeStop", Contained()))
		RemoveEffect("SwordStrikeStop", Contained());
}

func CheckStrike(iTime)
{
	//if(iTime < 20) return;
	var  offset_x=10;
	var offset_y=0;
	if(Contained()->GetDir() == DIR_Left) offset_x*=-1;
	
	if(!(Contained()->GetContact(-1) & CNAT_Bottom))
		offset_y=10;
	
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
	
	for(var obj in FindObjects(Find_AtRect(offset_x - width/2, offset_y - height/2, width, height), Find_OCF(OCF_Alive), Find_NoContainer(), Find_Exclude(Contained()), Find_Layer(GetObjectLayer())))
	{
		// check for second hit
		var effect_name=Format("HasBeenHitBySwordEffect%d", magic_number);
		var sword_name=Format("HasBeenHitBySword%d", this->ObjectNumber());
		var first=true;
		if(GetEffect(effect_name, obj))
		{
			//Log("already hit");
			continue;
		} else
		{
			//Log("first hit by this effect");
			AddEffect(effect_name, obj, 1, 60 /* arbitrary */, 0, 0);
			
			if(GetEffect(sword_name, obj))
			{
				//Log("successive hit");
				first=false;
			}
			else
			{
				//Log("first hit overall");
				AddEffect(sword_name, obj, 1, 40, 0, 0);
			}
		}
		
		found=true;
		
		/*if(iTime < 20)
		{
			DoWeaponSlow(obj, 800);
			continue;
		}*/
		
		/*var velocity=GetRelativeVelocity(Contained(), obj) * 2;
		velocity+= slowedVelocity / 10;
		velocity=velocity*3;*/
		//if(velocity > 300) velocity=300;
		
		var shield=ApplyShieldFactor(Contained(), obj, 0); // damage out of scope?
		if(shield == 100)
			continue;
		// fixed damage for now, not taking velocity into account
		var damage=(((100-shield)*(125 * 1000) / 100) / 15);
		obj->DoEnergy(-damage, true, 0, Contained()->GetOwner());
		
		
		if(offset_y)
			ApplyWeaponBash(obj, 100, 0);
		else
			if(!first)
				ApplyWeaponBash(obj, damage/50, Angle(0, 0, angle, -10));
		else
			if(!offset_y)
				DoWeaponSlow(obj, 300);
		
		
		// particle
		var x=-1;
		var p="Slice2";
		if(Contained()->GetDir() == DIR_Right)
		{
			x=1;
			p="Slice1";
		} 

		CreateParticle(p, AbsX(obj->GetX())+RandomX(-1,1), AbsY(obj->GetY())+RandomX(-1,1), 0, 0, 100, RGB(255,255,255), obj);
	}
	if(found)
	{
		/*if(iTime < 20)
		{
			DoWeaponSlow(Contained(), 1000);
		}
		else*/
		{
			this->Sound(Format("WeaponHit%d.ogg", 1+Random(3)), false);
			//if(doBash)
			//	DoWeaponSlow(Contained(), 2000);
			//this->StopWeaponHitCheckEffect(Contained());
		}
	}
}

func FxSwordStrikeStopStart(pTarget, iEffectNumber, iTemp)
{
	pTarget->SetPhysical("Walk", (pTarget->GetPhysical("Walk", 0) * 1)/100, PHYS_StackTemporary);
	if(iTemp) return;
}

func FxSwordStrikeStopStop(pTarget, iEffectNumber, iCause, iTemp)
{
	pTarget->ResetPhysical("Walk");
	if(iTemp) return;
}

func FxSwordStrikeStopTimer(pTarget, iEffectNumber)
{
	return 1;
}

func FxSwordStrikeSpeedUpStart(pTarget, iEffectNumber, iTemp)
{
	
	pTarget->SetPhysical("Walk", pTarget->GetPhysical("Walk", 0) * 3, PHYS_StackTemporary);
	if(iTemp) return;
	var dir=-1;
	if(pTarget->GetDir() == DIR_Right) dir=1;
	pTarget->SetXDir(pTarget->GetPhysical("Walk")*dir, 100000);
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
	pTarget->SetPhysical("Walk", pTarget->GetPhysical("Walk", 0) / 3, PHYS_StackTemporary);
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

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("Description", "$Description$", def);
	SetProperty("PictureTransformation",Trans_Rotate(20, 0, 0, 1),def);
}
