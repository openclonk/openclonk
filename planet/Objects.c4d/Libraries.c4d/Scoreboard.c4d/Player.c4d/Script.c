/*--
		Modular scoreboard: Players
		Author: Maikel

		This script can be included to create a player column in the scoreboard.
		Make sure that the following functions return _inherited(...);
			* Initialize();
			* InitializePlayer(int plr);
			* RelaunchPlayer(int plr, int killer);
			* RemovePlayer(int plr);
--*/


/*-- Callbacks --*/

protected func Initialize()
{
	// Set general scoreboard caption.
	SetScoreboardData(SBRD_Caption, SBRD_Caption, "Scoreboard", SBRD_Caption);
	return _inherited(...);
}

protected func InitializePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// Create scoreboard player entry for this player (its name).
	SetScoreboardData(plrid, SBRD_Caption, GetTaggedPlayerName(plr), plrid);
	return _inherited(plr, ...);
}

protected func RemovePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// Clear scoreboard player entry for this player.
	SetScoreboardData(plrid, SBRD_Caption, nil, nil);
	return _inherited(plr, ...);
}

/*-- Misc --*/

protected func Definition(def)
{
	SetProperty("Name", "Scoreboard Players", def);
}
