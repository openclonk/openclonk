/**
	AI Ranged Weapons
	Functionality that helps the AI use ranged weapons. Handles:
	 * Bow
	 * Blunderbuss
	 * Grenade launcher
	
	@author Sven2, Maikel
*/


/*-- General Ranged Weapon --*/

private func ExecuteRanged(effect fx)
{
	// Still carrying the bow?
	if (fx.weapon->Contained() != fx.Target)
	{
		fx.weapon = fx.post_aim_weapon = nil;
		return false;
	}
	// Finish shooting process.
	if (fx.post_aim_weapon)
	{
		// Wait max one second after shot (otherwise may be locked in wait animation forever if something goes wrong during shot).
		if (FrameCounter() - fx.post_aim_weapon_time <= (fx.post_aim_weapon->~GetShootTime() ?? 35))
			if (this->IsAimingOrLoading(fx))
				return true;
		fx.post_aim_weapon = nil;
	}
	// Target still in guard range?
	if (!this->CheckTargetInGuardRange(fx))
		return false;
	// Look at target.
	this->ExecuteLookAtTarget(fx);
	// Make sure we can shoot.
	if (!this->IsAimingOrLoading(fx) || !fx.aim_weapon || !fx.aim_weapon->~IsRangedWeapon())
	{
		this->CancelAiming(fx);
		if (!this->CheckHandsAction(fx)) return true;
		// Start aiming.
		this->SelectItem(fx, fx.weapon);
		if (!fx.weapon->ControlUseStart(fx.Target, fx.target->GetX() - fx.Target->GetX(), fx.target->GetY() - fx.Target->GetY()))
			return false;
		fx.aim_weapon = fx.weapon;
		fx.aim_time = fx.time;
		fx.post_aim_weapon = nil;
		// Enough for now.
		return;
	}
	// Stuck in aim procedure check?
	if (GetEffect("IntAimCheckProcedure", fx.Target) && !fx.Target->ReadyToAction()) 
		return this->ExecuteStand(fx);
	// Calculate offset to target. Take movement into account.
	var x = fx.Target->GetX(), y = fx.Target->GetY(), tx = fx.target->GetX(), ty = fx.target->GetY();
	// Aim for the head of crew members (y-4) so it's harder to evade by jumping.
	if (fx.target->GetOCF() & OCF_CrewMember)
		ty = ty - 4;
	// Aim just above the base of structures, this makes the changes higher for explosives to detonate on the basement or solid material.
	if (fx.target->GetCategory() & C4D_Structure)
		ty = ty + fx.target->GetBottom() - 2;
	var d = Distance(x, y, tx, ty);
	// Projected travel time of the arrow.
	var dt = d * 10 / fx.projectile_speed; 
	tx += this->GetTargetXDir(fx.target, dt);
	ty += this->GetTargetYDir(fx.target, dt);
	if (!fx.target->GetContact(-1))
		if (!fx.target->GetCategory() & C4D_StaticBack)
			ty += GetGravity() * dt * dt / 200;
	// Get shooting angle.
	var shooting_angle;
	if (fx.ranged_direct)
	{
		// For straight shots it is just the angle to the target if the path is free.
		if (PathFree(x, y, tx, ty))
			shooting_angle = Angle(x, y, tx, ty, 10);
	}
	// For ballistic shots get the angle (path free check is done inside).
	// The lower of the two angles is preferentially returned.
	else
		shooting_angle = this->GetBallisticAngle(x, y, tx, ty, fx.projectile_speed, 160);
	// If we have a valid shooting angle we can proceed.
	if (shooting_angle != nil)
	{
		// No ally on path? Also search for allied animals, just in case.
		// Ignore this if requested or if no friendly fire rules is active.
		var ally;
		if (!fx.ignore_allies || FindObject(Find_ID(Rule_NoFriendlyFire)))
			ally = FindObject(Find_OnLine(0, 0, tx - x, ty - y), Find_Exclude(fx.Target), Find_OCF(OCF_Alive), Find_Owner(fx.Target->GetOwner()));
		if (ally)
		{
			// Try to jump, if not possible just wait.
			if (this->ExecuteJump(fx)) 
				return true;
		}
		else
		{
			// Aim / shoot there.
			x = Sin(shooting_angle, 1000, 10);
			y = -Cos(shooting_angle, 1000, 10);
			fx.aim_weapon->ControlUseHolding(fx.Target, x, y);
			if (fx.Target->IsAiming() && fx.time >= fx.aim_time + fx.aim_weapon->GetReloadTime())
			{
				fx.aim_weapon->ControlUseStop(fx.Target, x, y);
				 // Assign post-aim status to allow slower shoot animations to pass.
				fx.post_aim_weapon = fx.aim_weapon;
				fx.post_aim_weapon_time = FrameCounter();
				fx.aim_weapon = nil;
			}
			return true;
		}
	}
	// Path not free or out of range. Just wait for enemy to come or move to it if in agressive mode.
	fx.aim_weapon->ControlUseHolding(fx.Target, tx - x, ty - y);
	if (fx.is_aggressive && d > 40)
		fx.Target->SetCommand("MoveTo", nil, fx.target->GetX(), fx.target->GetY());
	// Might also change target if current is unreachable.
	var new_target;
	if (!Random(3))
		if (new_target = this->FindTarget(fx))
			fx.target = new_target;
	return true;
}

private func IsAimingOrLoading(effect fx)
{
	return !!GetEffect("IntAim*", fx.Target);
}


/*-- Bow --*/

private func HasArrows(effect fx, object weapon)
{
	if (weapon->Contents(0))
		return true;
	if (FindObject(Find_Container(fx.Target), Find_Func("IsArrow")))
		return true;
	return false;
}


/*-- Blunderbuss --*/

private func HasAmmo(effect fx, object weapon)
{
	if (weapon->Contents(0))
		return true;
	if (FindObject(Find_Container(fx.Target), Find_Func("IsBullet")))
		return true;
	return false;
}


/*-- Grenade Launcher --*/

private func HasBombs(effect fx, object weapon)
{
	if (weapon->Contents(0))
		return true;
	if (FindObject(Find_Container(fx.Target), Find_Func("IsGrenadeLauncherAmmo")))
		return true;
	return false;
}
