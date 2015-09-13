// Functionality to disable the clonk's controls.
// The clonk's ObjectControl function needs to be overloaded.

#appendto Clonk

public func ObjectControl(int plr)
{
	// Check whether controls are enable for this player.
	if (g_controls_disabled && g_controls_disabled[plr])
	{
		return true;
	}
	return _inherited(plr, ...);
}
