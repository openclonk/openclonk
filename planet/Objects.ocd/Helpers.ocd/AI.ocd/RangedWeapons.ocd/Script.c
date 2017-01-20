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
	if (fx.weapon->Contained() != this)
	{
		fx.weapon = fx.post_aim_weapon = nil;
		return false;
	}
	// Finish shooting process.
	if (fx.post_aim_weapon)
	{
		// Wait max one second after shot (otherwise may be locked in wait animation forever if something goes wrong during shot).
		if (FrameCounter() - fx.post_aim_weapon_time < 36)
			if (this->IsAimingOrLoading())
				return true;
		fx.post_aim_weapon = nil;
	}
	// Target still in guard range?
	if (!this->CheckTargetInGuardRange(fx))
		return false;
	// Look at target.
	this->ExecuteLookAtTarget(fx);
	// Make sure we can shoot.
	if (!this->IsAimingOrLoading() || !fx.aim_weapon)
	{
		this->CancelAiming(fx);
		if (!this->CheckHandsAction(fx)) return true;
		// Start aiming.
		this->SelectItem(fx.weapon);
		if (!fx.weapon->ControlUseStart(this, fx.target->GetX()-GetX(), fx.target->GetY()-GetY())) return false; // something's broken :(
		fx.aim_weapon = fx.weapon;
		fx.aim_time = fx.time;
		fx.post_aim_weapon = nil;
		// Enough for now.
		return;
	}
	// Stuck in aim procedure check?
	if (GetEffect("IntAimCheckProcedure", this) && !this->ReadyToAction()) 
		return this->ExecuteStand(fx);
	// Calculate offset to target. Take movement into account.
	// Also aim for the head (y-4) so it's harder to evade by jumping.
	var x = GetX(), y = GetY(), tx = fx.target->GetX(), ty = fx.target->GetY() - 4;
	var d = Distance(x, y, tx, ty);
	// Projected travel time of the arrow.
	var dt = d * 10 / fx.projectile_speed; 
	tx += this->GetTargetXDir(fx.target, dt);
	ty += this->GetTargetYDir(fx.target, dt);
	if (!fx.target->GetContact(-1))
		if (!fx.target->GetCategory() & C4D_StaticBack)
			ty += GetGravity() * dt * dt / 200;
	// Path to target free?
	if (PathFree(x, y, tx, ty))
	{
		// Get shooting angle.
		var shooting_angle;
		if (fx.ranged_direct)
			shooting_angle = Angle(x, y, tx, ty, 10);
		else
			shooting_angle = this->GetBallisticAngle(tx-x, ty-y, fx.projectile_speed, 160);
		if (GetType(shooting_angle) != C4V_Nil)
		{
			// No ally on path? Also search for allied animals, just in case.
			var ally;
			if (!fx.ignore_allies) ally = FindObject(Find_OnLine(0,0,tx-x,ty-y), Find_Exclude(this), Find_OCF(OCF_Alive), Find_Owner(GetOwner()));
			if (ally)
			{
				// Try to jump, if not possible just wait.
				if (this->ExecuteJump()) 
					return true;
			}
			else
			{
				// Aim / shoot there.
				x = Sin(shooting_angle, 1000, 10);
				y = -Cos(shooting_angle, 1000, 10);
				fx.aim_weapon->ControlUseHolding(this, x, y);
				if (this->IsAiming() && fx.time >= fx.aim_time + fx.aim_wait)
				{
					fx.aim_weapon->ControlUseStop(this, x, y);
					 // Assign post-aim status to allow slower shoot animations to pass.
					fx.post_aim_weapon = fx.aim_weapon;
					fx.post_aim_weapon_time = FrameCounter();
					fx.aim_weapon = nil;
				}
				return true;
			}
		}
	}
	// Path not free or out of range. Just wait for enemy to come...
	fx.aim_weapon->ControlUseHolding(this, tx - x, ty - y);
	// Might also change target if current is unreachable.
	var new_target;
	if (!Random(3))
		if (new_target = this->FindTarget(fx))
			fx.target = new_target;
	return true;
}

private func IsAimingOrLoading() { return !!GetEffect("IntAim*", this); }


/*-- Bow --*/

private func HasArrows(effect fx, object weapon)
{
	if (weapon->Contents(0))
		return true;
	if (FindObject(Find_Container(this), Find_Func("IsArrow")))
		return true;
	return false;
}


/*-- Blunderbuss --*/

private func HasAmmo(effect fx, object weapon)
{
	if (weapon->Contents(0))
		return true;
	if (FindObject(Find_Container(this), Find_Func("IsBullet")))
		return true;
	return false;
}


/*-- Grenade Launcher --*/

private func HasBombs(effect fx, object weapon)
{
	if (weapon->Contents(0))
		return true;
	if (FindObject(Find_Container(this), Find_Func("IsGrenadeLauncherAmmo")))
		return true;
	return false;
}
