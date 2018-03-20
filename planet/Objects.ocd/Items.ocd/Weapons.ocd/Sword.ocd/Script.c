/**
	Sword
	Standard melee weapon.
*/

#include Library_MeleeWeapon

static const Sword_Standard_StrikingLength = 15; // in frames

local movement_effect;
local magic_number;
local carry_bone;

/*-- Engine Callbacks --*/

func Initialize()
{
	PlayAnimation("Base", 5, Anim_Const(0), Anim_Const(1000));
	return _inherited(...);
}

func Hit()
{
	Sound("Hits::Materials::Metal::LightMetalHit?");
}

func Departure(object container)
{
	// Always end the movement impairing effect when exiting
	if (movement_effect)
	{
		RemoveEffect(nil, container, movement_effect);
		movement_effect = nil;
	}
}

/*-- Callbacks --*/

public func OnWeaponHitCheckStop(clonk)
{
	carry_bone = nil;
	clonk->UpdateAttach();
	if (GetEffect("SwordStrikeSpeedUp", clonk))
		RemoveEffect("SwordStrikeSpeedUp", clonk);
	
	if (clonk->IsJumping())
	{
		if (!GetEffect("Fall", clonk))
			AddEffect("Fall", clonk, 1, 1, clonk);
	}
	
	if (GetEffect("SwordStrikeStop", clonk))
		RemoveEffect("SwordStrikeStop", clonk);
	
	return;
}

/*-- Usage --*/

public func RejectUse(object clonk)
{
	return !clonk->HasHandAction() || !CanStrikeWithWeapon(clonk) || !(clonk->IsWalking() || clonk->IsJumping());
}

public func ControlUse(object clonk, int x, int y)
{
	var slow = GetEffect("SwordStrikeSlow", clonk);
	
	var arm = "R";
	carry_bone = "pos_hand2";
	if (clonk->GetHandPosByItemPos(clonk->GetItemPos(this)) == 1)
	{
		arm = "L";
		carry_bone = "pos_hand1";
	}
	var rand = Random(2) + 1;
	var animation = Format("SwordSlash%d.%s", rand, arm);
	var animation_sword = Format("Strike%d", rand);
	var downwards_stab = false;
	
	// figure out the kind of attack to use
	var length = Sword_Standard_StrikingLength;
	if (clonk->IsWalking())
	{
		if (!GetEffect("SwordStrikeStop", clonk))
			movement_effect = AddEffect("SwordStrikeStop", clonk, 2, length, this);
	}
	else if (clonk->IsJumping())
	{
		rand = 1;
		if (clonk->GetYDir() < -5)
		{
			rand = 2;
		}
		animation = Format("SwordJump%d.%s", rand, arm);
		
		if (!slow && !GetEffect("DelayTranslateVelocity", clonk))
		{
			// check whether the player aims below the Clonk
			var a = Angle(0, 0, x, y);
			var x_dir = Sin(a, 60);
			
			if (Inside(a, 35 + 90, 35 + 180))
				// the player aims downwards
				if ((BoundBy(x_dir, -1, 1) == BoundBy(clonk->GetXDir(), -1, 1)) || (clonk->GetXDir() == 0))
				// the player aims into the direction the Clonk is already jumping
				{
					clonk->SetXDir(x_dir);
					clonk->SetYDir(-Cos(a, 60));
					AddEffect("DelayTranslateVelocity", clonk, 2, 3, nil, Library_MeleeWeapon);
					
					// modified animation
					length = 50;
					animation = Format("SwordSlash1.%s", arm);
					downwards_stab = true;
					
					if (GetEffect("Fall", clonk))
						RemoveEffect("Fall", clonk);
					
					// visual effect
					AddEffect("VisualJumpStrike", clonk, 1, 2, nil, Sword);
				}
		}
	}
	
	if (!downwards_stab)
	{
		PlayWeaponAnimation(clonk, animation, 10, Anim_Linear(0, 0, clonk->GetAnimationLength(animation), length, ANIM_Remove), Anim_Const(1000));
		PlayAnimation(animation_sword, 10, Anim_Linear(0, 0, GetAnimationLength(animation_sword), length, ANIM_Remove), Anim_Const(1000));
	}
	else
	{
		PlayWeaponAnimation(clonk, animation, 10, Anim_Linear(0, 0, (clonk->GetAnimationLength(animation) * 3) / 4, 5, ANIM_Hold), Anim_Const(1000));
		PlayAnimation(animation_sword, 10, Anim_Linear(GetAnimationLength(animation_sword) / 2, 0, GetAnimationLength(animation_sword), length, ANIM_Remove), Anim_Const(1000));
	}
	clonk->UpdateAttach();
	
	// this means that the sword can only hit an object every X frames
	// change it to something that changes every strike if you want the sword to be able to hit the same enemy with different
	// strikes regardless of the time in between
	magic_number = ObjectNumber();
	StartWeaponHitCheckEffect(clonk, length, 1);
	
	this->Sound("Objects::Weapons::WeaponSwing?");
	return true;
}

func FxVisualJumpStrikeStart(target, effect, temp)
{
	if(temp) return;
	effect.x_add = 20;
	if (target->GetXDir() < 0)
	{
		effect.x_add *= -1;
	}
	effect.visual = CreateObjectAbove(Sword_JumpEffect, 0, 0, nil);
	effect.visual->Point({x = target->GetX() + effect.x_add, y = target->GetY() + 10}, {x = target->GetX() + effect.x_add, y = target->GetY() + 10});
}

func FxVisualJumpStrikeTimer(target, effect, time)
{
	if (!target->~IsJumping())
	{
		effect.visual->FadeOut();
		effect.visual = nil;
		return -1;
	}
	effect.visual->Point(nil, {x = target->GetX() + effect.x_add, y = target->GetY() + 10});
}

func FxVisualJumpStrikeStop(target, effect, reason, temp)
{
	if(temp) return;
	if(!effect.visual) return;
	effect.visual->FadeOut();
}

func SwordDamage(int shield)
{
	return (100 - shield) * 9 * 1000 / 100;
}

func CheckStrike(iTime)
{
	//if(iTime < 20) return;
	var offset_x = 7;
	var offset_y = 0;
	if (Contained()->GetDir() == DIR_Left)
		offset_x *= -1;
	
	if (!(Contained()->GetContact(-1) & CNAT_Bottom))
		offset_y = 10;
	
	var width = 15;
	var height = 20;
	var angle = 0;
	
	var doBash = Abs(Contained()->GetXDir()) > 5 || Abs(Contained()->GetYDir()) > 5;
	if (!doBash)
	{
		doBash = Contained()->GetContact(-1) & CNAT_Bottom;
	}
	
	if (doBash)
	{
		if (Contained()->GetDir() == DIR_Left)
			angle = -(Max(5, Abs(Contained()->GetXDir())));
		else
			angle = (Max(5, Abs(Contained()->GetXDir())));
	}
	
	for(var obj in FindObjects(Find_AtRect(offset_x - width/2, offset_y - height/2, width, height),
							   Find_NoContainer(),
							   Find_Exclude(Contained()),
							   Find_Layer(GetObjectLayer())))
	{
		if (obj->~IsProjectileTarget(this, Contained()))
		{
			var effect_name = Format("HasBeenHitBySwordEffect%d", magic_number);
			var sword_name = Format("HasBeenHitBySword%d", this->ObjectNumber());
			var first = true;
			// don't hit objects twice
			if (!GetEffect(effect_name, obj))
			{
				AddEffect(effect_name, obj, 1, Sword_Standard_StrikingLength);
				
				if (GetEffect(sword_name, obj))
				{
					//Log("successive hit");
					first = false;
				}
				else
				{
					//Log("first hit overall");
					AddEffect(sword_name, obj, 1, 40);
				}
				

				// Reduce damage by shield
				var shield = ApplyShieldFactor(Contained(), obj, 0); // damage out of scope?
				if (shield == 100)
					continue;
				
				// Sound before damage to prevent null pointer access if callbacks delete this
				Sound("Objects::Weapons::WeaponHit?", false);
				
				// fixed damage (9)
				var damage = SwordDamage(shield);
				WeaponDamage(obj, damage, FX_Call_EngGetPunched, true);
				
				// object has not been deleted?
				if (obj)
				{
					if (offset_y)
						ApplyWeaponBash(obj, 100, 0);
					else if (!first)
						ApplyWeaponBash(obj, damage / 50, Angle(0, 0, angle, -10));
					else if (!offset_y)
						DoWeaponSlow(obj, 300);
					
					// Particle effect
					var particle =
					{
						Size = 20,
						BlitMode = GFX_BLIT_Additive,
						Attach = ATTACH_Front | ATTACH_MoveRelative,
						Phase = PV_Linear(0, 3)
					};
					
					if (Contained()->GetDir() == DIR_Left)
					{
						particle.Phase = PV_Linear(4, 7);
					}
					obj->CreateParticle("SwordSlice", RandomX(-1, 1), RandomX(-1, 1), 0, 0, 6, particle);
				}
				
				// and done. We can only hit one target
				break;
			}
		}
	}
}

func FxSwordStrikeStopStart(pTarget, effect, iTemp)
{
	if(iTemp) return;
	pTarget->PushActionSpeed("Walk", (pTarget.ActMap.Walk.Speed) / 100);
}

func FxSwordStrikeStopStop(pTarget, effect, iCause, iTemp)
{
	if(iTemp) return;
	pTarget->PopActionSpeed("Walk");
	if (this)
		movement_effect = nil;
}

func FxSwordStrikeStopTimer(pTarget, effect)
{
	return -1;
}

func FxSwordStrikeSpeedUpStart(pTarget, effect, iTemp)
{
	pTarget->PushActionSpeed("Walk", pTarget.ActMap.Walk.Speed * 3);
	pTarget.ActMap.Walk.Accel = 210;
}

func FxSwordStrikeSpeedUpTimer(pTarget, effect, iEffectTime)
{
	if (!pTarget->GetContact(-1) & CNAT_Bottom)
		return -1;
	if (iEffectTime > 35 * 2)
		return -1;
}

func FxSwordStrikeSpeedUpStop(pTarget, effect, iCause, iTemp)
{
	pTarget->PopActionSpeed("Walk");
	if(iTemp) return;
	if(!pTarget->GetAlive()) return;
	
	AddEffect("SwordStrikeSlow", pTarget, 1, 5, nil, Sword, effect.Time);
}

func FxSwordStrikeSlowStart(pTarget, effect, iTemp, iTime)
{
	pTarget->PushActionSpeed("Walk", pTarget.ActMap.Walk.Speed / 3);
	if(iTemp) return;
	effect.starttime = iTime;
}

func FxSwordStrikeSlowTimer(pTarget, effect, iEffectTime)
{
	if (iEffectTime > effect.starttime)
		return -1;
}

func FxSwordStrikeSlowStop(pTarget, effect, iCause, iTemp)
{
	pTarget->PopActionSpeed("Walk");
}

/*-- Production --*/

public func IsWeapon() { return true; }
public func IsArmoryProduct() { return true; }

/*-- Display --*/

public func GetCarryMode(object clonk, bool idle, bool nohand)
{
	if (idle)
		return CARRY_Sword;
	
	return CARRY_HandBack;
}

public func GetCarrySpecial(clonk) { return carry_bone; }

public func GetCarryTransform(clonk, sec, back)
{
	if (sec) return Trans_Mul(Trans_Rotate(130, 0, 0, 1), Trans_Translate(-3500, 0, 2800));

	if(back) return Trans_Mul(Trans_Rotate(180,0,1,0), Trans_Rotate(-90,1,0,0), Trans_Translate(-7000,0,0));
	return Trans_Rotate(-90, 1, 0, 0);
}

func Definition(def)
{
	SetProperty("PictureTransformation", Trans_Rotate(20, 0, 0, 1), def);
}

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local Components = {Wood = 1, Metal = 1};
