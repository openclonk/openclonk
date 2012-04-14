/*--
	Melee weapons
	Author: Zapper
	
	Generic weapon template
--*/


/*local fDrawn;
local fAttack;
local iAnimStrike;*/

//local iWeaponChargeType;
//local hWeaponMesh;
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

func FxDelayTranslateVelocityStart(pTarget, effect, temp, p1)
{
	if(temp) return;
	//effect.var0 = p1; // old flinging behavior
}

func FxDelayTranslateVelocityTimer(pTarget, effect, iEffectTime)
{
	//if(iEffectNumber.var0)
	//	if(iEffectTime > iEffectNumber.var0) return -1;
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
	
	/*	var x, y;
		var time = Min(iEffectTime, strikeTime);
		var step=(((time*100) / GetStrikeTime()) * 180) / 100;
		x=-Cos(step, 10);
		y=-Sin(step, 6)-3;
		if(GetChargeType() == L_WN_down) y*=-1;
		if(pTarget->GetDir() == DIR_Left) x *= -1;
		CreateParticle("Blast", x, y, 0, 0, 20);
		
		var strength = 0;
		if(iEffectTime <= GetStrikeTime())
			strength = (((iEffectTime*100)/GetStrikeTime()) * this->WeaponSharpness()) / 100;
		var bash = EffectCall(pTarget, iEffectNumber, "GetBash");
		var bashAngle = Angle(0, 0, pTarget->GetXDir(), pTarget->GetYDir());
		var found = false;
		for(var obj in FindObjects(Find_AtPoint(x, y), Find_OCF(OCF_Alive), Find_Exclude(pTarget)))
		{
			var b = bash;
			var b2;
			var effect;
			if(effect = GetEffect("IntWeaponCharge", obj))
				if(pTarget->GetDir() != obj->GetDir()) // facing each other
									b2 = EffectCall(0, effect, "GetBash");
		
			var xVel=Abs(pTarget->GetXDir());
			if(BoundBy(pTarget->GetXDir(), -1, 1) != BoundBy(obj->GetXDir(), -1, 1))
				xVel+=Abs(obj->GetXDir());
			else xVel-=Abs(obj->GetXDir());
			
			var yVel=Abs(pTarget->GetYDir());
			if(BoundBy(pTarget->GetYDir(), -1, 1) != BoundBy(obj->GetYDir(), -1, 1))
				yVel+=Abs(obj->GetYDir());
			else yVel-=Abs(obj->GetYDir());
			b = (b*Sqrt((xVel**2)+(yVel**2)));
			
			var dmg = ((2*strength) + b) / 3;
			if(b > strength) dmg = ((2*b) + strength) / 3;
			var angle = 0;
			if(bash != 0) angle = bashAngle;
			dmg = Max(0, dmg - b2);
			
			if(dmg)
				AddEffect("IntIsBeingStruck", obj, 1, 1, 0, GetID(), dmg, angle);
			Log("b: %d  strength: %d  b2: %d dmg: %d xVel: %d yVel: %d", b, strength, b2, dmg);
			found = true;
		}
		
		if(found)
		{
			var velocity = Sqrt((pTarget->GetXDir())**2 + (pTarget->GetYDir())**2);
			velocity = Min(1, velocity-1);
			pTarget->SetSpeed(Sin(bashAngle, velocity), -Cos(bashAngle, velocity));
		}
		
		if(iEffectTime == GetStrikeTime()+1)
		{
			effect.Interval = 3;
			effect.Time = 0;
		}*/
}

func FxIntWeaponChargeStop(pTarget, effect, iReason, iTemp)
{
	if(iTemp) return;
	if(!pTarget) return;
	if(this)
	{
		this->StopWeaponAnimation(pTarget);
		//this->DetachWeaponMesh(pTarget);
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
	//return (this->WeaponBash() * Sqrt((pTarget->GetXDir())**2+(pTarget->GetYDir())**2)) / 1000;
}

func FxIntWeaponChargeHitByWeapon(pTarget, effect)
{
	return this->~HitByWeapon(...);
}

func FxIntIsBeingStruckStart(pTarget, effect, iTemp, iDamage, angle)
{
	if(iTemp) return;
	effect.delay = 3;
	effect.damage = iDamage;
	effect.angle = angle;
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
		//if(iEffectNumber.var1 > 20) iEffectNumber.var1 = 20;
		pTarget->SetXDir(Sin(effect.angle, effect.damage ), 100);
		pTarget->SetYDir(-Abs(Cos(effect.angle, effect.damage )), 100);
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
	/*if(iWeaponChargeType == L_WN_straight) return "StrikeArms";
	if(iWeaponChargeType == L_WN_up) return "Strike2Arms";
	if(iWeaponChargeType == L_WN_down) return "Strike3Arms";
	DebugLog("WARNING: L_WN::GetStrikeAnimation() called at the wrong time!");*/
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

/*func StartWeaponAnimation(pTarget)
{
	if(hWeaponAnimStrike) StopWeaponAnimation(pTarget);
	hWeaponAnimStrike = pTarget->PlayAnimation(GetStrikeAnimation(), 10, Anim_Linear(0, 0, pTarget->GetAnimationLength(GetStrikeAnimation()), GetStrikeTime(), ANIM_Remove), Anim_Const(1000));
}*/

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



/*
func AttachWeaponMesh(pTarget)
{
	if(hWeaponMesh)
	{
		DetachWeaponMesh(pTarget);
	}
	hWeaponMesh = pTarget->AttachMesh(GetID(), "pos_hand1", "Main", 130);
}

func DetachWeaponMesh(pTarget)
{
	if(!hWeaponMesh) return;
	pTarget->DetachMesh(hWeaponMesh);
	hWeaponMesh = 0;
}
*/


func GetJumpLength(pClonk)
{
	if(!pClonk->~IsJumping()) return 0;
	var x = pClonk->GetX(), y = pClonk->GetY(), xDir = pClonk->GetXDir(), yDir = pClonk->GetYDir();
	//crash! var l=SimFlight(x, y, xDir, yDir, 0, 0, 100, 0); //SimFlight behavior changed (10/1/10)
	var l=0;
	if(!l) return 50;
	else
	{
		// not a very good approximation. I really think SimFlight should return the number of iterations
		return Sqrt((x - pClonk->GetX()) ** 2 + (y - pClonk->GetY()) ** 2);
	}
	return -1;
}

func ApplyShieldFactor(pFrom, pTo, damage)
{
	// totally prevent the strike?
	if(pTo->~QueryCatchBlow(pFrom)) return 100;
	
	var state=0;
	var shield=-1;
	//for(var i = GetEffectCount(0, pTo); i--;)
	for(;state <= 1;state++)
	{
		var effect_name="*Shield*";
		if(state == 1) effect_name="IntWeaponCharge";
		var iEffect;
		var i=0;
		while(iEffect=GetEffect(effect_name, pTo, i++))
		{
			//iEffect = GetEffect("*Shield*", pTo, i);
			//if(!iEffect) iEffect = GetEffect("IntWeaponCharge", pTo, i);
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
	AddEffect("IntWeaponCharge", pClonk, 10, iInterval, this, 0, iLength);
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

func ApplyWeaponBash(pTo, int strength, angle)
{
	AddEffect("IntIsBeingStruck", pTo, 2, 1, nil, GetID(), strength, angle);
}

func TranslateVelocity(object pTarget, int angle, int iLimited, int iExtraVelocity)
{
	var speed=Sqrt((pTarget->GetXDir(100) ** 2) + (pTarget->GetYDir(100) ** 2)) + iExtraVelocity;
	var a=Angle(0, 0, pTarget->GetXDir(), pTarget->GetYDir());
	
	if(iLimited)
	{
		//if(Abs(angle-a) > Abs(angle-(a+360)))
		//	a=(a+360);
		angle+=360;
		a+=360;
		angle=BoundBy(angle, a-iLimited, a+iLimited);
	}
	
	pTarget->SetXDir(Sin(angle, speed), 100);
	pTarget->SetYDir(-Cos(angle, speed), 100);
}
