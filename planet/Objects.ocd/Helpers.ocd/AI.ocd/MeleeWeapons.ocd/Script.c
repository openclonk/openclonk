/**
	AI Melee Weapons
	Functionality that helps the AI use melee weapons. Handles:
	 * Sword
	
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
			// OK, slash!
			this->SelectItem(fx, fx.weapon);
			return fx.weapon->ControlUse(fx.Target, tx, ty);
		}
		// Clonk is above us - jump there.
		this->ExecuteJump(fx);
		if (dx<-5)
			fx.Target->SetComDir(COMD_Left);
		else if (dx > 5)
			fx.Target->SetComDir(COMD_Right);
		else
			fx.Target->SetComDir(COMD_None);
	}
	// Not in range. Walk there.
	if (!fx.Target->GetCommand() || !Random(10))
		fx.Target->SetCommand("MoveTo", fx.target);
	return true;
}
