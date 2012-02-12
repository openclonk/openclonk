// Restart will remove plane and contents instead of clonk.

#appendto Rule_Restart

public func Activate(int plr)
{
	// Notify scenario.
	if (GameCall("OnPlayerRestart", plr))
		return;
	// Remove the player's clonk, including contents.
	var clonk = GetCrew(plr);
	if (clonk)
	{
		var plane = clonk->Contained();
		if (plane)
			plane->RemoveObject();
		else
			clonk->RemoveObject();
	}
	return;
}