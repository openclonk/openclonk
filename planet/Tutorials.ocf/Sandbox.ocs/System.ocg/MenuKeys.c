// Menu keys for the god mode menus.

global func PlayerControl(proplist plr, int ctrl, id spec_id, int x, int y, int strength, bool repeat, int status)
{
	var cursor = plr->GetCursor();
	if (ctrl == CON_TutorialGuide)
	{
		if (cursor->GetMenu() && cursor->GetMenu().ID == cursor.idHudOS)
			cursor->HideObjectSpawnUI();
		else	
			cursor->ShowObjectSpawnUI();
		return;
	}
	return _inherited(plr, ctrl, spec_id, x, y, strength, repeat, status, ...);
}
