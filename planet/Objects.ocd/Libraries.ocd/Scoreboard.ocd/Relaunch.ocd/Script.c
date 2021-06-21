/*--
		Modular scoreboard: Relaunches
		Author: Maikel

		This script can be included to create a relaunch count column in the scoreboard.
		Make sure that the following functions return _inherited(...);
			* Initialize();
			* InitializePlayer(proplist plr);
			* OnClonkDeath(object clonk, proplist killer);
			* RemovePlayer(proplist plr);
--*/

/*-- Callbacks --*/

protected func Initialize()
{
	// init scoreboard
	// init scoreboard, uses the condition of Scoreboard_Deaths too
	if (GetRelaunchRule()->HasUnlimitedRelaunches()) return;
    Scoreboard->Init(
		[{key = "relaunches", title = Scoreboard_Relaunch, sorted = true, desc = true, default = "", priority = 75, conditional = Scoreboard_Death.ScoreboardCondition}]
		);
	return _inherited(...);
}

protected func InitializePlayer(proplist plr)
{
	if (GetRelaunchRule()->HasUnlimitedRelaunches())
		return;
	Scoreboard->NewPlayerEntry(plr);
	Scoreboard->SetPlayerData(plr, "relaunches", GetRelaunchRule()->GetPlayerRelaunchCount(plr));
	return _inherited(plr, ...);
}

protected func OnClonkDeath(object clonk, proplist killer)
{
	var plr = clonk->GetOwner();
	if (GetRelaunchRule()->HasUnlimitedRelaunches())
		return;
	Scoreboard->SetPlayerData(plr, "relaunches", GetRelaunchRule()->GetPlayerRelaunchCount(plr));
	return _inherited(clonk, killer, ...);
}

protected func OnPlayerRelaunchCountChanged(proplist plr)
{
    if (GetRelaunchRule()->HasUnlimitedRelaunches()) return;
	Scoreboard->SetPlayerData(plr, "relaunches", GetRelaunchRule()->GetPlayerRelaunchCount(plr));
	return _inherited(plr, ...);
}


protected func RemovePlayer(proplist plr)
{
	return _inherited(plr, ...);
}

/*-- Misc --*/



local Name = "Scoreboard Relaunches";
