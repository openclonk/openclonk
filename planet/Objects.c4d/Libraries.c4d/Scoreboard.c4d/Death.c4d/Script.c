/*--
		Modular scoreboard: Deaths
		Author: Maikel

		This script can be included to create a death count column in the scoreboard.
		Make sure that the following functions return _inherited(...);
			* Initialize();
			* InitializePlayer(int plr);
			* RelaunchPlayer(int plr, int killer);
			* RemovePlayer(int plr);
--*/


local score_death_list; // Here the death count of all players is stored, access through plrid.

/*-- Callbacks --*/

protected func Initialize()
{
	// Make sure it is a list.
	score_death_list = [];
	// Set scoreboard death count caption.
	SetScoreboardData(SBRD_Caption, GetDeathCol(), "{{Scoreboard_Death}}", SBRD_Caption);
	return _inherited(...);
}

protected func InitializePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// Create scoreboard death count entry for this player.
	score_death_list[plrid] = 0;
	SetScoreboardData(plrid, GetDeathCol(), Format("%d", score_death_list[plrid]), score_death_list[plrid]);
	return _inherited(plr, ...);
}

protected func RelaunchPlayer(int plr, int killer)
{
	var plrid = GetPlayerID(plr);
	// Modify scoreboard death count entry for this player.
	score_death_list[plrid]++;
	SetScoreboardData(plrid, GetDeathCol(), Format("%d", score_death_list[plrid]), score_death_list[plrid]);
	return _inherited(plr, killer, ...);
}

protected func RemovePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// Clear scoreboard death count entry for this player.
	SetScoreboardData(plrid, GetDeathCol(), nil, nil);
	return _inherited(plr, ...);
}

/*-- Misc --*/

public func SetDeathCount(int plr)
{
	var plrid = GetPlayerID(plr);
	score_death_list[plrid] = 0;
	return;
}

public func GetDeathCount(int plr)
{
	var plrid = GetPlayerID(plr);
	return score_death_list[plrid];
}

public func GetDeathCol()
{
	//return ScoreboardCol(Scoreboard_Death);
	return 101;
}

protected func Definition(def)
{
	SetProperty("Name", "Scoreboard Deaths", def);
}
