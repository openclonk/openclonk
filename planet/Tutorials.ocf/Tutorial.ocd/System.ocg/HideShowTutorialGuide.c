// Shows and hides the tutorial guide if the [H] button is pressed.

global func PlayerControl(int plr, int ctrl, id spec_id, int x, int y, int strength, bool repeat, int status)
{
	if (ctrl != CON_TutorialGuide)
		return _inherited(plr, ctrl, spec_id, x, y, strength, repeat, status, ...);
	// Don't do anything if the player is a sequence.
	if (GetActiveSequence())	
		return;
	// Find the guide object for this player.
	var guide = FindObject(Find_ID(TutorialGuide), Find_Owner(plr));
	if (!guide)
		return;
	// If hidden, show the guide. If shown, hide the guide.
	if (guide->IsHidden())
		guide->ShowGuide();
	else
		guide->HideGuide();
	return;
}
