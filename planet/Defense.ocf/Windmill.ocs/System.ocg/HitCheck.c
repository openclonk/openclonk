/* Variant of FxHitCheckDoCheck that does exclude the windmills if anything was shot by a player
*/

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
	var is_human = GetPlayerType(target->GetController()) == C4PT_User;

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
			if(obj == target) continue;
			if(obj == shooter) continue;
			if (is_human) {
				if (obj == g_windgen1) continue;
				if (obj == g_windgen2) continue;
				if (obj == g_windgen3) continue;
				if (obj == g_windmill) continue;
			}

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

// Do not reapply C4D_Object to arrows

global func FxHitCheckStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return;

	return;
}