/* Effect for hit check */

// EffectVars:
// 0 - old X-Position
// 1 - old Y-Position
// 2 - shooter (Objekt that shot the projectile)
// 4 - live? If true, the shooter can be hit by the projectile
// 5 - never hit the shooter. True or false

global func FxHitCheckStart(object target, int effect, int temp, object byObj, bool neverShooter)
{
	if(temp) return;
	EffectVar(0, target, effect) = target->GetX();
	EffectVar(1, target, effect) = target->GetY();
	if(!byObj)
		byObj = target;
	if(byObj->Contained())
		byObj = (byObj->Contained());
	EffectVar(2, target, effect) = byObj;
	EffectVar(4, target, effect) = false;
	EffectVar(5, target, effect) = neverShooter;
}	

global func FxHitCheckTimer(object target, int effect, int time)
{
	var obj;
	// Oh man. :O
	var oldx = EffectVar(0, target, effect);
	var oldy = EffectVar(1, target, effect);
	var newx = target->GetX();
	var newy = target->GetY();
	var dist = Distance(oldx, oldy, newx, newy);
	
	var shooter = EffectVar(2,target,effect);
	var live = EffectVar(4,target,effect);
	var nevershooter = EffectVar(5,target,effect);
	
	if(live) shooter = target;
	
	if(Distance(oldx,oldy,newx,newy) <= Max(1,Max(Abs(target->GetXDir()),Abs(target->GetYDir())))*2)
	{
		// we search for objects along the line on which we moved since the last check
		// ans sort by distance (closer first)
		for(obj in FindObjects(Find_OnLine(oldx,oldy,newx,newy),
													 Find_NoContainer(),
													 Sort_Distance(oldx, oldy)))
		{
			// Excludes
			if(obj == target) continue;
			if(obj == shooter) continue;
			
			// unlike in hazard, there is no NOFF rule (yet)
			// CheckEnemy
			//if(!CheckEnemy(obj,target)) continue;

			// IsBulletTarget or Alive will be hit
			if(obj->~IsProjectileTarget(target->GetID(),target,shooter) || obj->GetOCF() & OCF_Alive)
			{
				//Log("%s IsBulletTarget: %i, %s, %s","HitCheck",GetName(obj),GetID(target),GetName(target),GetName(EffectVar(2, target, effect)));
				return(target->~HitObject(obj));
			}
		}
	}

	EffectVar(0, target, effect) = newx;
	EffectVar(1, target, effect) = newy;

	// the projectile will be only switched to "live", meaning that it can hit the
	// shooter himself when the shot exited the shape of the shooter one time
	if(!nevershooter) {
		if(!live) {
			var ready = true;
			// we search for all objects with the id of our shooter
			for(var foo in FindObjects(Find_AtPoint(target->GetX(),target->GetY()),Find_ID(shooter->GetID())))
			{
				// if its the shooter...
				if(foo == shooter)
					// we may not switch to "live" yet
					ready = false;
			}
			// otherwise, the shot will be live
			if(ready)
				EffectVar(4, target, effect) = true;
		}
	}
}
