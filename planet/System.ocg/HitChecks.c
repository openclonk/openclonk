/**
	HitCheck.c
	Effect for hit checking.
	Facilitates any hit check of a projectile. The Projectile hits anything
	which is either alive or returns for IsProjectileTarget(object projectile,
	object shooter) true. If the projectile hits something, it calls
	HitObject(object target) in the projectile.

	@author Newton, Boni
*/

global func FxHitCheckStart(object target, proplist effect, int temp, object by_obj, bool never_shooter)
{
	if (temp)
		return;
	effect.x = target->GetX();
	effect.y = target->GetY();
	if (!by_obj || GetType(by_obj) != C4V_C4Object)
		by_obj = target;
	if (by_obj->Contained())
		by_obj = by_obj->Contained();
	effect.shooter = by_obj;
	effect.live = false;
	effect.never_shooter = never_shooter;
	
	// C4D_Object has a hitcheck too -> change to vehicle to supress that.
	if (target->GetCategory() & C4D_Object)
		target->SetCategory((target->GetCategory() - C4D_Object) | C4D_Vehicle);
	return;
}

global func FxHitCheckStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return;
	
	target->SetCategory(target->GetID()->GetCategory());
	return;
}

global func FxHitCheckDoCheck(object target, proplist effect)
{
	var obj;
	// rather search in front of the projectile, since a hit might delete the effect,
	// and clonks can effectively hide in front of walls.
	var oldx = target->GetX();
	var oldy = target->GetY();
	var newx = target->GetX() + target->GetXDir() / 10;
	var newy = target->GetY() + target->GetYDir() / 10;
	var dist = Distance(oldx, oldy, newx, newy);
	
	var shooter = effect.shooter;
	var live = effect.live;
	
	if (live)
		shooter = target;
	
	if (dist <= Max(1, Max(Abs(target->GetXDir()), Abs(target->GetYDir()))) * 2)
	{
		// We search for objects along the line on which we moved since the last check
		// and sort by distance (closer first).
		for (obj in FindObjects(Find_OnLine(oldx, oldy, newx, newy),
								Find_NoContainer(),
								Find_Layer(target->GetObjectLayer()),
								Find_PathFree(target),
								Sort_Distance(oldx, oldy)))
		{
			// Excludes
			if (!obj) continue; // hit callback of one object might have removed other objects
			if (obj == target) continue;
			if (obj == shooter) continue;

			// Unlike in hazard, there is no NOFF rule (yet)
			// CheckEnemy
			//if (!CheckEnemy(obj, target)) continue;

			// IsProjectileTarget will be hit (defaults to true for OCF_Alive).
			if (obj->~IsProjectileTarget(target, shooter))
			{
				target->~HitObject(obj);
				if (!target)
					return;
			}
		}
	}
	return;
}

global func FxHitCheckEffect(string newname)
{
	if (newname == "HitCheck")
		return -2;
	return;
}

global func FxHitCheckAdd(object target, proplist effect, string neweffectname, int newtimer, by_obj, never_shooter)
{
	effect.x = target->GetX();
	effect.y = target->GetY();
	if (!by_obj)
		by_obj = target;
	if (by_obj->Contained())
		by_obj = by_obj->Contained();
	effect.shooter = by_obj;
	effect.live = false;
	effect.never_shooter = never_shooter;
	return;
}

global func FxHitCheckTimer(object target, proplist effect, int time)
{
	EffectCall(target, effect, "DoCheck");
	// It could be that it hit something and removed itself. thus check if target is still there.
	// The effect will be deleted right after this.
	if (!target)
		return -1;
	
	effect.x = target->GetX();
	effect.y = target->GetY();
	var live = effect.live;
	var never_shooter = effect.never_shooter;
	var shooter = effect.shooter;

	// The projectile will be only switched to "live", meaning that it can hit the
	// shooter himself when the shot exited the shape of the shooter one time.
	if (!never_shooter)
	{
		if (!live)
		{
			var ready = true;
			// We search for all objects with the id of our shooter.
			if (shooter)
			{
				if (FindObject(Find_AtPoint(target->GetX(), target->GetY()), Find_InArray([shooter])))
				{
					// we may not switch to "live" yet.
					ready = false;
				}
			}
			// Otherwise, the shot will be live.
			if (ready)
				effect.live = true;
		}
	}
	return;
}

global func IsProjectileTarget(object projectile, object shooter)
{
	return GetOCF() & OCF_Alive;
}

/*
	Checks whether an object is ready to take damage from this object, calling QueryCatchBlow.
*/
global func WeaponCanHit(object target)
{
	if (target->~QueryCatchBlow(this)) return false;
	if (!target || !this) return false;
	return true;
}

/*
	Deals damage to an object, draining either energy for living things or dealing damage otherwise.
	CatchBlow is called on the target if it's alive.
*/
global func WeaponDamage(object target, int damage, int damage_type, bool exact_damage)
{
	if (!this || !target) return;
	
	damage_type = damage_type ?? FX_Call_EngObjHit;
	var true_damage = damage;
	if (exact_damage) true_damage = damage / 1000;
	
	if (target->GetAlive())
	{
		target->DoEnergy(-damage, exact_damage, damage_type, GetController());
		if (!target) return;

		target->~CatchBlow(-true_damage, this);
	}
	else
	{
		target->DoDamage(true_damage, damage_type, GetController());
	}
}

/*
	Tumbles an object based on this object's speed and mass.
	strength = 100 means using 100% of the own mass for tumbling the other object.
*/
global func WeaponTumble(object target, int strength)
{
	if (!this || !target) return;
	
	strength = strength ?? 100;
	if (strength <= 0) return;
	
	if (target->GetAlive())
	{
		target->SetAction("Tumble");
		// Constrained by conservation of linear momentum, unrealism != 1 for unrealistic behaviour.
		var unrealism = 3;
		var mass = strength * GetMass() / 100;
		var obj_mass = target->GetMass();
		target->SetXDir((target->GetXDir() * obj_mass + GetXDir() * mass * unrealism) / (mass + obj_mass));		
		target->SetYDir((target->GetYDir() * obj_mass + GetYDir() * mass * unrealism) / (mass + obj_mass));
	}
}
