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
	// cooldown?
	if(!CanStrikeWithWeapon(clonk)) return true;
	
	// if the clonk doesn't have an action where he can use it's hands do nothing
	if(!clonk->HasHandAction())
		return true;
		
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
	var  offset_x=7;
	var offset_y=0;
	if(Contained()->GetDir() == DIR_Left) offset_x*=-1;
	
	if(!(Contained()->GetContact(-1) & CNAT_Bottom))
		offset_y=10;
	
	var width=10;
	var height=20;
	var angle=0;
	
	var doBash=Abs(Contained()->GetXDir()) > 5 || Abs(Contained()->GetYDir()) > 5;
	if(!doBash) doBash=Contained()->GetContact(-1) & CNAT_Bottom;
	
	if(doBash)
	{
		if(Contained()->GetDir() == DIR_Left)
			angle=-(Max(5, Abs(Contained()->GetXDir())));
		else angle=(Max(5, Abs(Contained()->GetXDir())));
	}
	
	for(var obj in FindObjects(Find_AtRect(offset_x - width/2, offset_y - height/2, width, height),
							   Find_NoContainer(),
							   Find_Exclude(Contained()),
							   Find_Layer(GetObjectLayer())))
	{
		if (obj->~IsProjectileTarget(this, Contained()) || obj->GetOCF() & OCF_Alive)
		{
			var effect_name=Format("HasBeenHitBySwordEffect%d", magic_number);
			var sword_name=Format("HasBeenHitBySword%d", this->ObjectNumber());
			var first=true;
			// don't hit objects twice
			if(!GetEffect(effect_name, obj))
			{
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

				
				// Reduce damage by shield
				var shield=ApplyShieldFactor(Contained(), obj, 0); // damage out of scope?
				if(shield == 100)
					continue;
					
				// fixed damage (9)
				var damage=((100-shield)*9*1000 / 100);
				ProjectileHit(obj, damage, ProjectileHit_no_query_catch_blow_callback | ProjectileHit_exact_damage | ProjectileHit_no_on_projectile_hit_callback, FX_Call_EngGetPunched);
				
				// object has not been deleted?
				if(obj)
				{
					if(offset_y)
						ApplyWeaponBash(obj, 100, 0);
					else
						if(!first)
							ApplyWeaponBash(obj, damage/50, Angle(0, 0, angle, -10));
					else
						if(!offset_y)
							DoWeaponSlow(obj, 300);
					
					// Particle effect
					var x=-1;
					var p="Slice2";
					if(Contained()->GetDir() == DIR_Right)
					{
						x=1;
						p="Slice1";
					} 
					CreateParticle(p, AbsX(obj->GetX())+RandomX(-1,1), AbsY(obj->GetY())+RandomX(-1,1), 0, 0, 100, RGB(255,255,255), obj);
				}
				
				// sound and done. We can only hit one target
				Sound(Format("WeaponHit%d.ogg", 1+Random(3)), false);
				break;
			}
		}
	}

}

func FxSwordStrikeStopStart(pTarget, effect, iTemp)
{
	pTarget->PushActionSpeed("Walk", (pTarget.ActMap.Walk.Speed)/100);
	if(iTemp) return;
}

func FxSwordStrikeStopStop(pTarget, effect, iCause, iTemp)
{
	pTarget->PopActionSpeed("Walk");
	if(iTemp) return;
}

func FxSwordStrikeStopTimer(pTarget, effect)
{
	return 1;
}

func FxSwordStrikeSpeedUpStart(pTarget, effect, iTemp)
{
	pTarget->PushActionSpeed("Walk", pTarget.ActMap.Walk.Speed * 3);
	pTarget.ActMap.Walk.Accel = 210;
}

func FxSwordStrikeSpeedUpTimer(pTarget, effect, iEffectTime)
{
	if(!pTarget->GetContact( -1) & CNAT_Bottom)
		return -1;
	if(iEffectTime > 35*2) return -1;
}

func FxSwordStrikeSpeedUpStop(pTarget, effect, iCause, iTemp)
{
	pTarget->PopActionSpeed("Walk");
	if(iTemp) return;
	if(!pTarget->GetAlive()) return;
	
	AddEffect("SwordStrikeSlow", pTarget, 1, 5, 0, Sword, effect.Time);
}

func FxSwordStrikeSlowStart(pTarget, effect, iTemp, iTime)
{
	pTarget->PushActionSpeed("Walk", pTarget.ActMap.Walk.Speed / 3);
	if(iTemp) return;
	effect.var0 = iTime;
}

func FxSwordStrikeSlowTimer(pTarget, effect, iEffectTime)
{
	if(iEffectTime > effect.var0) return -1;
}

func FxSwordStrikeSlowStop(pTarget, effect, iCause, iTemp)
{
	pTarget->PopActionSpeed("Walk");
}

func Definition(def) {
	SetProperty("Collectible", 1, def);
	SetProperty("Name", "$Name$", def);
	SetProperty("Description", "$Description$", def);
	SetProperty("PictureTransformation",Trans_Rotate(20, 0, 0, 1),def);
}
