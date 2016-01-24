/*-- Restart --*/

local remove_contents;

protected func Initialize()
{
	// Contents are removed 
	remove_contents = true;
	return;
}

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
		if (!ObjectCount(Find_ID(Rule_BaseRespawn)) && remove_contents)
			while (clonk->Contents())
				clonk->Contents()->RemoveObject();
		clonk->Kill(clonk, true);
		clonk->RemoveObject();
	}
}

public func SetRemoveContents(bool do_removal)
{
	remove_contents = do_removal;
	return;
}

local Name = "$Name$";
local Description = "$Description$";
