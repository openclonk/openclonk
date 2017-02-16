// Create the menu to spawn objects.

#appendto Library_ClonkControl

public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, int status)
{
	if (!this) 
		return false;

	// Spawn menu
	if (ctrl == CON_SpawnMenu && status == CONS_Down)
	{
		// Close any menu if open.
		if (GetMenu())
		{
			var is_spawn = GetMenu()->~IsSpawnMenu();
			// unclosable menu? bad luck.
			if (!TryCancelMenu()) return true;
			// If contents menu, don't open new one and return.
			if (is_spawn)
				return true;
		}
		// Open contents menu.
		CancelUse();
		GUI_SpawnMenu->CreateFor(this);
		// The interaction menu calls SetMenu(this) in the clonk,
		// so after this call menu = the created menu.
		if (GetMenu())
			GetMenu()->~Show();		
		return true;
	}
	
	// Unhandled control will be handled by the library itself.
	return _inherited(plr, ctrl, x, y, strength, repeat, status, ...);
}
