/*--
	Melee weapons
	Author: Zapper
	
	Generic weapon template
--*/


local hWeaponAnimStrike;

func Construction()
{
	hWeaponAnimStrike = nil;
	return _inherited(...);
}

func CheckStrike()
{
	return _inherited(...);
}

public func CanStrikeWithWeapon(clonk)
{
	if(!(clonk->~IsClonk())) return false;
	
	if(GetEffect("*WeaponCooldown*", clonk))
	{
		return false;
	}
	
	if(GetEffect("*WeaponCharge", clonk))
	{
		return false;
	}
	return true;
}


func FxDelayTranslateVelocityTimer(pTarget, effect, iEffectTime)
{
	if(pTarget->GetContact(-1) & CNAT_Bottom) return -1;
	return 1;
}

func FxDelayTranslateVelocityStop(pTarget, effect, reason, temp)
{
	if(temp) return;
	if(!(pTarget->GetContact(-1) & CNAT_Bottom))
	if(Sqrt(pTarget->GetXDir()**2 + pTarget->GetYDir()**2) > 10)
		pTarget->SetSpeed(pTarget->GetXDir()/3, pTarget->GetYDir()/3);
}

func FxIntWeaponChargeStart(pTarget, effect, iTemp, length)
{
	if(iTemp) return;
	
	// saved velocity
	effect.velocity = 0;
	// save length
	effect.length = length;
}

func FxIntWeaponChargeTimer(pTarget, effect, iEffectTime)
{
	if(this->Contained() != pTarget) return -1;
	if(!pTarget->~IsWalking() && !pTarget->~IsJumping()) return -1;
	var strikeTime=effect.length;
	if(strikeTime != -1 && iEffectTime > strikeTime)
	{
		this->~WeaponStrikeExpired();
		return -1;
	}
	this->CheckStrike(iEffectTime);
}

func FxIntWeaponChargeStop(pTarget, effect, iReason, iTemp)
{
	if(iTemp) return;
	if(!pTarget) return;
	if(this)
	{
		this->StopWeaponAnimation(pTarget);
		this->~OnWeaponHitCheckStop(pTarget);
	}
	
}

func FxIntWeaponChargeAddWeaponSlow(pTarget, effect, iStrength)
{
	effect.slow += iStrength;
}

func FxIntWeaponChargeGetWeaponSlow(pTarget, effect)
{
	return effect.slow;
}

func FxIntWeaponChargeGetBash(pTarget, effect)
{
	return 0;
}

func FxIntWeaponChargeHitByWeapon(pTarget, effect)
{
	return this->~HitByWeapon(...);
}

func FxIntIsBeingStruckStart(pTarget, effect, iTemp, iDamage, angle, object from)
{
	if(iTemp) return;
	effect.delay = 3;
	effect.damage = iDamage;
	effect.angle = angle;
	effect.from = from;
	effect.from_player = NO_OWNER;
	if (from)
		effect.from_player = from->GetOwner();
}

func FxIntIsBeingStruckTimer(pTarget, effect, iEffectTime)
{
	if(effect.delay -- == 0)
	{
		// FALCON PUNCH
		if(pTarget->GetContact(-1) & CNAT_Bottom)
		{
			if(!pTarget->Stuck())
			{
				pTarget->SetPosition(pTarget->GetX(), pTarget->GetY()-1);
				if(pTarget->Stuck()) pTarget->SetPosition(pTarget->GetX(), pTarget->GetY()+1);
			}
			if(effect.damage > 60)
				pTarget->Fling();
		}

		pTarget->SetXDir(Sin(effect.angle, effect.damage ), 100);
		pTarget->SetYDir(-Abs(Cos(effect.angle, effect.damage )), 100);
		
		// in case the object is flung down a cliff
		if (effect.from_player != NO_OWNER)
			pTarget->SetKiller(effect.from_player);
		return -1;
	}
	
	return true;
}

func FxIntIsBeingStruckEffect(string szNewEffectName, object pTarget)
{
	if(szNewEffectName == "IntIsBeingStruck") return -2;
	return 0;
}

func FxIntIsBeingStruckAdd (object pTarget, effect, string szNewEffectName, int iNewEffectTimer, int damage, int angle)
{
	// reset delay
	effect.delay = 3;
	
	// add damage!
	if(damage > effect.damage)
		effect.damage = damage;
	else
	effect.var1 = (effect.damage*2 + damage) / 2;
	
	// check angle
	if(!effect.angle)
		effect.angle = angle;
	else if(angle)
		// should actually set the new angle to the average between the old and the new one. but I don't feel like doing such calculations now
		// let's see if anyone notices it
		effect.angle = angle;
}

func GetStrikeAnimation()
{
	return "StrikeArms";
}


func StopWeaponAnimation(pTarget)
{
	if(hWeaponAnimStrike == nil) return;
	pTarget->StopAnimation(hWeaponAnimStrike);
	hWeaponAnimStrike = nil;
}

func PlayWeaponAnimation(pTarget)
{
	if(hWeaponAnimStrike != nil) StopWeaponAnimation(pTarget);
	hWeaponAnimStrike = pTarget->PlayAnimation(...);
}


func GetRelativeVelocity(pObject1, pObject2)
{
	var b=0;
	var xVel=Abs(pObject1->GetXDir());
	if(BoundBy(pObject1->GetXDir(), -1, 1) != BoundBy(pObject2->GetXDir(), -1, 1))
		xVel+=Abs(pObject2->GetXDir());
	else xVel-=Abs(pObject2->GetXDir());
	
	var yVel=Abs(pObject1->GetYDir());
	if(BoundBy(pObject1->GetYDir(), -1, 1) != BoundBy(pObject2->GetYDir(), -1, 1))
		yVel+=Abs(pObject2->GetYDir());
	else yVel-=Abs(pObject2->GetYDir());
	b = Sqrt((xVel**2)+(yVel**2));
	return b;
}

func ApplyShieldFactor(pFrom, pTo, damage)
{
	// totally prevent the strike?
	if(pTo->~QueryCatchBlow(pFrom)) return 100;
	
	var state=0;
	var shield=-1;
	for(;state <= 1;state++)
	{
		var effect_name="*Shield*";
		if(state == 1) effect_name="IntWeaponCharge";
		var iEffect;
		var i=0;
		while(iEffect=GetEffect(effect_name, pTo, i++))
		{
			var s=EffectCall(pTo, iEffect, "HitByWeapon", pFrom, damage);
			if(s && shield == -1) shield=s;
			else if(s)
			{
				shield=(100-(((100-s)*(100-shield))/100));
			}
			
		}
	}
	
	
	if(shield == -1) return 0;
	return shield;
}

func StartWeaponHitCheckEffect(pClonk, iLength, iInterval)
{
	AddEffect("IntWeaponCharge", pClonk, 10, iInterval, this, nil, iLength);
}

func StopWeaponHitCheckEffect(pClonk)
{
	if(GetEffect("IntWeaponCharge", pClonk))
		RemoveEffect("IntWeaponCharge", pClonk);
}

func DoWeaponSlow(pClonk, iStrength)
{
	var e=GetEffect("IntWeaponCharge", pClonk);
	var s=Sqrt( (pClonk->GetXDir(1000)) ** 2 + (pClonk->GetYDir(1000)) ** 2);
	var angle=Angle(0,0,pClonk->GetXDir(), pClonk->GetYDir());
	
	s-=iStrength;
	if(s < 0) s=0;
	pClonk->SetXDir(Sin(angle, s), 1000);
	pClonk->SetYDir(-Cos(angle, s), 1000);
	
	if(e)
		EffectCall(nil, e, "AddWeaponSlow", iStrength);
	return true;
}

func GetWeaponSlow(pClonk)
{
	var e=GetEffect("IntWeaponCharge", pClonk);
	if(!e) return 0;
	return EffectCall(nil, e, "GetWeaponSlow");
}

func ApplyWeaponBash(pTo, int strength, angle, object from)
{
	from = from ?? this;
	AddEffect("IntIsBeingStruck", pTo, 2, 1, nil, GetID(), strength, angle, from);
}

func TranslateVelocity(object pTarget, int angle, int iLimited, int iExtraVelocity)
{
	var speed=Sqrt((pTarget->GetXDir(100) ** 2) + (pTarget->GetYDir(100) ** 2)) + iExtraVelocity;
	var a=Angle(0, 0, pTarget->GetXDir(), pTarget->GetYDir());
	
	if(iLimited)
	{
		angle+=360;
		a+=360;
		angle=BoundBy(angle, a-iLimited, a+iLimited);
	}
	
	pTarget->SetXDir(Sin(angle, speed), 100);
	pTarget->SetYDir(-Cos(angle, speed), 100);
}
