/*-- Restart --*/

public func Activate(int plr)
{
	// Notify scenario.
	if (GameCall("OnPlayerRestart", plr))
		return;
	// Remove the player's clonk, including contents.
	var clonk = GetCrew(plr);
	if (clonk && clonk->GetCrewEnabled())
	{
		// Remove contents only if the Base Respawn Rule isn't there otherwise it will handle inventory
		if (!ObjectCount(Find_ID(Rule_BaseRespawn)))
			while (clonk->Contents())
				clonk->Contents()->RemoveObject();
		clonk->Kill(clonk, true);
		clonk->RemoveObject();
	}
}

local Name = "$Name$";
local Description = "$Description$";
