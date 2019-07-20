/**
	AI Melee Weapons
	Functionality that helps the AI use melee weapons. Handles:
	 * Sword
	 * Club
	 * Axe
	
	@author Sven2, Maikel
*/


/*-- General Melee Weapon --*/

private func ExecuteMelee(effect fx)
{
	// Still carrying the melee weapon?
	if (fx.weapon->Contained() != fx.Target)
	{
		fx.weapon = nil;
		return false;
	}
	// Are we in range?
	var x = fx.Target->GetX(), y = fx.Target->GetY(), tx = fx.target->GetX(), ty = fx.target->GetY();
	var dx = tx - x, dy = ty - y;
	if (Abs(dx) <= 10 && PathFree(x, y, tx, ty))
	{
		if (dy >= -15)
		{
			// Target is under us - sword slash downwards!
			if (!this->CheckHandsAction(fx))
				return true;
			// Stop here.
			fx.Target->SetCommand("None");
			fx.Target->SetComDir(COMD_None);
			// Cooldown?
			if (!fx.weapon->CanStrikeWithWeapon(fx.Target))
			{
				// While waiting for the cooldown, we try to evade...
				this->ExecuteEvade(fx, dx, dy);
				return true;
			}
			// OK, do a strike.
			this->SelectItem(fx, fx.weapon);
			// First try to do a single control use call.
			if (!fx.weapon->~HoldingEnabled() && fx.weapon->~ControlUse(fx.Target, tx, ty))
				return true;
			// Assume the weapon must be held longer for a strike.
			if (fx.weapon->~ControlUseStart(fx.Target, tx, ty))
			{
				fx.weapon->~ControlUseHolding(fx.Target, tx, ty);
				fx.weapon->~ControlUseStop(fx.Target, tx, ty);
			}
		}
		// Clonk is above us - jump there, but only if not commanded.
		if (!fx.commander)
		{
			this->ExecuteJump(fx);
			if (dx < -5)
				fx.Target->SetComDir(COMD_Left);
			else if (dx > 5)
				fx.Target->SetComDir(COMD_Right);
			else
				fx.Target->SetComDir(COMD_None);
		}
	}
	// Not in range. Walk there, but only if not being commanded.
	if (!fx.commander && (!fx.Target->GetCommand() || !Random(10)))
		fx.Target->SetCommand("MoveTo", fx.target);
	return true;
}

private func ExecuteClub(effect fx)
{
	// Still carrying the melee weapon?
	if (fx.weapon->Contained() != fx.Target)
	{
		fx.weapon = nil;
		return false;
	}
	// Are we in range?
	var x = fx.Target->GetX(), y = fx.Target->GetY(), tx = fx.target->GetX(), ty = fx.target->GetY();
	var dx = tx-x, dy = ty-y;
	if (Abs(dx) <= 10 && PathFree(x, y, tx, ty))
	{
		if (Abs(dy) >= 15)
		{
			// Clonk is above or below us - wait
			if (dx<-5) fx.Target->SetComDir(COMD_Left); else if (dx>5) fx.Target->SetComDir(COMD_Right); else fx.Target->SetComDir(COMD_None);
			return true;
		}
		if (!this->CheckHandsAction(fx)) return true;
		// Stop here
		fx.Target->SetCommand("None"); fx.Target->SetComDir(COMD_None);
		// cooldown?
		if (!fx.weapon->CanStrikeWithWeapon(fx.Target))
		{
			//Message("MeleeWAIT %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
			// While waiting for the cooldown, we try to evade...
			this->ExecuteEvade(fx, dx, dy);
			return true;
		}
		// OK, attack! Prefer upwards strike
		dy -= 16;
		fx.weapon->ControlUseStart(fx.Target, dx, dy);
		fx.weapon->ControlUseHolding(fx.Target, dx, dy);
		fx.weapon->ControlUseStop(fx.Target, dx, dy);
		return true;
	}
	// Not in range. Walk there.
	if (!fx.Target->GetCommand() || !Random(10)) fx.Target->SetCommand("MoveTo", fx.target);
	//Message("Melee %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
	return true;
}

private func ExecuteBomber(effect fx)
{
	// Still carrying the bomb?
	if (fx.weapon->Contained() != fx.Target)
	{
		fx.weapon = nil;
		return false;
	}
	// Are we in range?
	if (fx.Target->ObjectDistance(fx.target) < 20 && !fx.weapon->OnFire())
	{
		// make sure it kills the AI clonk, because it would be useless after the explosion
		fx.Target->DoEnergy(5 - fx.Target->GetEnergy());
		// Boom!
		fx.weapon.TimeToExplode = 10;
		fx.weapon->Incinerate(100, fx.Target->GetController());
	}
	else
	{
		// Not in range. Walk there.
		if (!fx.Target->GetCommand() || !Random(10))
			fx.Target->SetCommand("MoveTo", fx.target);
	}
	return true;
}
