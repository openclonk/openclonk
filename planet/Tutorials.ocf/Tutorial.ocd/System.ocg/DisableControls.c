// Functionality to disable the clonk's controls.

static g_controls_disabled;

global func DisablePlrControls(int plr, bool disable_crew)
{
	// Set controls to disables for this player.
	if (g_controls_disabled == nil)
		g_controls_disabled = [];
	g_controls_disabled[plr] = true;
	// Disable crew if specified.
	if (disable_crew) 
	{
		for (var crew in FindObjects(Find_OCF(OCF_CrewMember), Find_Owner(plr)))
		{
			crew->SetCategory(C4D_StaticBack);
		}
	}
	return;
}


global func EnablePlrControls(int plr, bool enable_crew)
{
	// Set controls to enabled for this player.
	if (g_controls_disabled == nil)
		g_controls_disabled = [];	
	g_controls_disabled[plr] = false;
	// Enable crew if specified.
	if (enable_crew) 
	{
		for (var crew in FindObjects(Find_OCF(OCF_CrewMember), Find_Owner(plr)))
		{
			crew->SetCategory(C4D_Living);
		}
	}
	return;
}

global func ObjectControl(int plr)
{
	// Check whether controls are enable for this player.
	if (g_controls_disabled && g_controls_disabled[plr])
	{
		return true;
	}
	return _inherited(plr, ...);
}