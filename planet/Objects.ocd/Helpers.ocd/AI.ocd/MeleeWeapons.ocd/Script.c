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
	if (fx.weapon->Contained() != this)
	{
		fx.weapon = nil;
		return false;
	}
	// Are we in range?
	var x = GetX(), y = GetY(), tx = fx.target->GetX(), ty = fx.target->GetY();
	var dx = tx - x, dy = ty - y;
	if (Abs(dx) <= 10 && PathFree(x, y, tx, ty))
	{
		if (dy >= -15)
		{
			// Target is under us - sword slash downwards!
			if (!this->CheckHandsAction(fx))
				return true;
			// Stop here.
			SetCommand("None");
			SetComDir(COMD_None);
			// Cooldown?
			if (!fx.weapon->CanStrikeWithWeapon(this))
			{
				// While waiting for the cooldown, we try to evade...
				this->ExecuteEvade(fx, dx, dy);
				return true;
			}
			// OK, slash!
			this->SelectItem(fx.weapon);
			return fx.weapon->ControlUse(this, tx,ty);
		}
		// Clonk is above us - jump there.
		this->ExecuteJump();
		if (dx<-5)
			SetComDir(COMD_Left);
		else if (dx > 5)
			SetComDir(COMD_Right);
		else
			SetComDir(COMD_None);
	}
	// Not in range. Walk there.
	if (!GetCommand() || !Random(10))
		SetCommand("MoveTo", fx.target);
	return true;
}


