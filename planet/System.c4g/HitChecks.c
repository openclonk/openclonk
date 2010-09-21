/*--
		HitCheck.c
		Authors: Newton, Boni
	
		Effect for hit checking.
		Facilitates any hit check of a projectile. The Projectile hits anything
		which is either alive or returns for IsProjectileTarget(object projectile,
		object shooter) true. If the projectile hits something, it calls
		HitObject(object target) in the projectile.
--*/


// EffectVars:
// 0 - old X-Position
// 1 - old Y-Position
// 2 - shooter (Object that shot the projectile)
// 4 - live? If true, the shooter can be hit by the projectile
// 5 - never hit the shooter. True or false

global func FxHitCheckStart(object target, int effect, int temp, object by_obj, bool never_shooter)
{
	if (temp)
		return;
	EffectVar(0, target, effect) = target->GetX();
	EffectVar(1, target, effect) = target->GetY();
	if (!by_obj)
		by_obj = target;
	if (by_obj->Contained())
		by_obj = by_obj->Contained();
	EffectVar(2, target, effect) = by_obj;
	EffectVar(4, target, effect) = false;
	EffectVar(5, target, effect) = never_shooter;
	
	// C4D_Object has a hitcheck too -> change to vehicle to supress that.
	if (target->GetCategory() & C4D_Object)
		target->SetCategory((target->GetCategory() - C4D_Object) | C4D_Vehicle);
	return;
}

global func FxHitCheckStop(object target, int effect, int reason, bool temp)
{
	if (temp)
		return;
	
	target->SetCategory(target->GetID()->GetCategory());
	return;
}

global func FxHitCheckDoCheck(object target, int effect)
{
	var obj;
	// rather search in front of the projectile, since a hit might delete the effect,
	// and clonks can effectively hide in front of walls.
	var oldx = target->GetX();
	var oldy = target->GetY();
	var newx = target->GetX() + target->GetXDir() / 10;
	var newy = target->GetY() + target->GetYDir() / 10;
	var dist = Distance(oldx, oldy, newx, newy);
	
	var shooter = EffectVar(2, target, effect);
	var live = EffectVar(4, target, effect);
	
	if (live)
		shooter = target;
	
	if (Distance(oldx, oldy, newx, newy) <= Max(1, Max(Abs(target->GetXDir()), Abs(target->GetYDir()))) * 2)
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
			if(obj == target) continue;
			if(obj == shooter) continue;

			// Unlike in hazard, there is no NOFF rule (yet)
			// CheckEnemy
			//if(!CheckEnemy(obj,target)) continue;

			// IsProjectileTarget or Alive will be hit
			if (obj->~IsProjectileTarget(target, shooter) || obj->GetOCF() & OCF_Alive)
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

global func FxHitCheckAdd(object target, int effect, string neweffectname, int newtimer, by_obj, never_shooter)
{
	EffectVar(0, target, effect) = target->GetX();
	EffectVar(1, target, effect) = target->GetY();
	if (!by_obj)
		by_obj = target;
	if (by_obj->Contained())
		by_obj = by_obj->Contained();
	EffectVar(2, target, effect) = by_obj;
	EffectVar(4, target, effect) = false;
	EffectVar(5, target, effect) = never_shooter;
	return;
}

global func FxHitCheckTimer(object target, int effect, int time)
{
	EffectCall(target, effect, "DoCheck");
	// It could be that it hit something and removed itself. thus check if target is still there.
	// The effect will be deleted right after this.
	if (!target)
		return -1;
	
	EffectVar(0, target, effect) = target->GetX();
	EffectVar(1, target, effect) = target->GetY();
	var live = EffectVar(4, target, effect);
	var never_shooter = EffectVar(5, target, effect);
	var shooter = EffectVar(2, target, effect);

	// The projectile will be only switched to "live", meaning that it can hit the
	// shooter himself when the shot exited the shape of the shooter one time.
	if (!never_shooter)
	{
		if (!live)
		{
			var ready = true;
			// We search for all objects with the id of our shooter.
			for (var foo in FindObjects(Find_AtPoint(target->GetX(), target->GetY()), Find_ID(shooter->GetID())))
			{
				// If its the shooter...
				if(foo == shooter)
					// we may not switch to "live" yet.
					ready = false;
			}
			// Otherwise, the shot will be live.
			if (ready)
				EffectVar(4, target, effect) = true;
		}
	}
	return;
}

global func ProjectileHit(object obj, int dmg, bool tumble)
{
	if (!this || !obj)
		return;
	
	if (obj->GetAlive())
		if (obj->~QueryCatchBlow(this))
			return;
	if (!this || !obj)
		return;
		
	obj->~OnProjectileHit(this);
	if (!this || !obj)
		return;
	
	this->~OnStrike(obj);
	if (obj->GetAlive())
	{
		obj->DoEnergy(-dmg, false, FX_Call_EngObjHit, GetController());
		obj->~CatchBlow(-dmg, this);
	}
	else
	{
		obj->DoDamage(dmg, FX_Call_EngObjHit, GetController());
	}
	// Target could have done something with this projectile.
	if (!this || !obj)
		return;
	
	// Tumble target.
	if (obj->GetAlive() && tumble)
	{
		obj->SetAction("Tumble");
		// Constrained by conservation of linear momentum, unrealism != 1 for unrealistic behaviour.
		var unrealism = 3;
		var mass = GetMass();
		var obj_mass = obj->GetMass();
		obj->SetXDir((obj->GetXDir() * obj_mass + GetXDir() * mass * unrealism) / (mass + obj_mass));		
		obj->SetYDir((obj->GetYDir() * obj_mass + GetYDir() * mass * unrealism) / (mass + obj_mass));
	}
	return;
}
