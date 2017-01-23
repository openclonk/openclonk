/**
	AI Protection
	Functionality that helps the AI to protect itself and evade danger. Also 
	handles healing.
	
	@author Sven2, Maikel
*/


// AI Settings.
local HealingHitPointsThreshold = 30; // Number of hitpoints below which the AI will try to heal itself even in a dangerous situation.


public func ExecuteProtection(effect fx)
{
	// Search for nearby projectiles. Ranged AI also searches for enemy clonks to evade.
	var enemy_search;
	if (fx.ranged)
		enemy_search = Find_And(Find_OCF(OCF_CrewMember), Find_Not(Find_Owner(fx.Target->GetOwner())));
	var projectiles = fx.Target->FindObjects(Find_InRect(-150, -50, 300, 80), Find_Or(Find_Category(C4D_Object), Find_Func("IsDangerous4AI"), Find_Func("IsArrow"), enemy_search), Find_OCF(OCF_HitSpeed2), Find_NoContainer(), Sort_Distance());
	for (var obj in projectiles)
	{
		var dx = obj->GetX() - fx.Target->GetX(), dy = obj->GetY() - fx.Target->GetY();
		var vx = obj->GetXDir(), vy = obj->GetYDir();
		if (Abs(dx) > 40 && vx)
			dy += (Abs(10 * dx / vx)**2) * GetGravity() / 200;
		var v2 = Max(vx * vx + vy * vy, 1);
		var d2 = dx * dx + dy * dy;
		if (d2 / v2 > 4)
		{
			// Won't hit within the next 20 frames.
			continue;
		}
		// Distance at which projectile will pass clonk should be larger than clonk size (erroneously assumes clonk is a sphere).
		var l = dx * vx + dy * vy;
		if (l < 0 && Sqrt(d2 - l * l / v2) <= fx.Target->GetCon() / 8)
		{
			// Not if there's a wall between.
			if (!PathFree(fx.Target->GetX(), fx.Target->GetY(), obj->GetX(), obj->GetY()))
				continue;
			// This might hit.
			fx.alert=fx.time;
			// Use a shield if the object is not explosive.
			if (fx.shield && !obj->~HasExplosionOnImpact())
			{
				// Use it!
				this->SelectItem(fx, fx.shield);
				if (fx.aim_weapon == fx.shield)
				{
					// Continue to hold shield.
					fx.shield->ControlUseHolding(fx.Target, dx,dy);
				}
				else
				{
					// Start holding shield.
					if (fx.aim_weapon)
						this->CancelAiming(fx);
					if (!this->CheckHandsAction(fx))
						return true;
					if (!fx.shield->ControlUseStart(fx.Target, dx, dy))
						return false; // Something's broken :(
					fx.shield->ControlUseHolding(fx.Target, dx, dy);
					fx.aim_weapon = fx.shield;
				}
				return true;
			}
			// No shield. try to jump away.
			if (dx < 0)
				fx.Target->SetComDir(COMD_Right);
			else
				fx.Target->SetComDir(COMD_Left);
			if (this->ExecuteJump(fx))
				return true;
			// Can only try to evade one projectile.
			break;
		}
	}
	// Stay alert if there's a target. Otherwise alert state may wear off.
	if (!fx.target)
		fx.target = this->FindEmergencyTarget(fx);
	if (fx.target)
		fx.alert = fx.time;
	else if (fx.time - fx.alert > AI_AlertTime)
		fx.alert = nil;
	// If not evading the AI may try to heal.
	if (ExecuteHealing(fx))
		return true;
	// Nothing to do.
	return false;
}

public func ExecuteHealing(effect fx)
{
	var hp = fx.Target->GetEnergy();
	var hp_needed = fx.Target->GetMaxEnergy() - hp;
	// Only heal when alert if health drops below the healing threshold.
	// If not alert also heal if more than 40 hitpoints of health are lost.
	if (hp >= fx.control.HealingHitPointsThreshold && (fx.alert || hp_needed <= 40))
		return false;
	// Don't heal if already healing. One can speed up healing by healing multiple times, but we don't.
	if (GetEffect("HealingOverTime", fx.Target))
		return false;
	// Find food to heal with, find closest to but more than needed hp.
	var food;
	for (food in fx.Target->FindObjects(Find_Container(fx.Target), Find_Func("NutritionalValue"), Sort_Func("NutritionalValue")))
		if (food->NutritionalValue() >= hp_needed)
			break;
	if (!food)
		return false;
	// Eat the food.
	this->SelectItem(fx, food);
	if (food->~ControlUse(fx.Target))
		return true;
	return false;
}
