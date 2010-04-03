/*--
		Modular scoreboard: Kills
		Author: Maikel

		This script can be included to create a kill count column in the scoreboard.
		Make sure that the following functions return _inherited(...);
			* Initialize();
			* InitializePlayer(int plr);
			* RelaunchPlayer(int plr, int killer);
			* RemovePlayer(int plr);
--*/


local score_kill_list; // Here the kill count of all players is stored, access through plrid.

/*-- Callbacks --*/

protected func Initialize()
{
	// Make sure it is a list.
	score_kill_list = [];
	// Set scoreboard kill count caption.
	SetScoreboardData(SBRD_Caption, GetKillCol(), "{{Scoreboard_Kill}}", SBRD_Caption);
	return _inherited(...);
}

protected func InitializePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// Create scoreboard kill count entry for this player.
	score_kill_list[plrid] = 0;
	SetScoreboardData(plrid, GetKillCol(), Format("%d", score_kill_list[plrid]), score_kill_list[plrid]);
	return _inherited(plr, ...);
}

protected func RelaunchPlayer(int plr, int killer)
{
	var plrid = GetPlayerID(killer);
	// Only if killer exists and has not committed suicide.
	if (killer == plr || killer == NO_OWNER)
		return _inherited(plr, killer, ...);
	// Only if killer and victim are on different teams.
	if (GetPlayerTeam(killer) && GetPlayerTeam(killer) == GetPlayerTeam(plr))
		return _inherited(plr, killer, ...);
	// Modify scoreboard kill count entry for killer.
	score_kill_list[plrid]++;
	SetScoreboardData(plrid, GetKillCol(), Format("%d", score_kill_list[plrid]), score_kill_list[plrid]);
	return _inherited(plr, killer, ...);
}

protected func RemovePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// Clear scoreboard kill count entry for this player.
	SetScoreboardData(plrid, GetKillCol(), nil, nil);
	return _inherited(plr, ...);
}

/*-- Misc --*/

public func SetKillCount(int plr, int value)
{
	var plrid = GetPlayerID(plr);
	score_kill_list[plrid] = value;
	return;
}

public func GetKillCount(int plr)
{
	var plrid = GetPlayerID(plr);
	return score_kill_list[plrid];
}

public func DoKillCount(int plr, int value)
{
	var plrid = GetPlayerID(plr);
	score_kill_list[plrid] += value;
	return;
}

public func GetKillCol()
{
	//return ScoreboardCol(Scoreboard_Kill);
	return 107;
}
	
protected func Definition(def)
{
	SetProperty("Name", "Scoreboard Kills", def);
}
