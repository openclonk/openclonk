/*--
		Modular scoreboard: Kill streaks
		Author: Maikel

		This script can be included to create a kill streak count column in the scoreboard.
		Make sure that the following functions return _inherited(...);
			* Initialize();
			* InitializePlayer(int plr);
			* RelaunchPlayer(int plr, int killer);
			* RemovePlayer(int plr);
--*/


local score_killstreak_list; // Here the kill streak count of all players is stored, access through plrid.

/*-- Callbacks --*/

protected func Initialize()
{
	// Make sure it is a list.
	score_killstreak_list = [];
	// Set scoreboard kill streak count caption.
	SetScoreboardData(SBRD_Caption, GetKillStreakCol(), "{{Scoreboard_KillStreak}}", SBRD_Caption);
	return _inherited(...);
}

protected func InitializePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// Create scoreboard kill streak count entry for this player.
	score_killstreak_list[plrid] = 0;
	SetScoreboardData(plrid, GetKillStreakCol(), Format("%d", score_killstreak_list[plrid]), score_killstreak_list[plrid]);
	return _inherited(plr, ...);
}

protected func RelaunchPlayer(int plr, int killer)
{
	var plrid = GetPlayerID(plr);
	var killerid = GetPlayerID(killer);
	// Modify scoreboard kill streak count entry for killed player.
	score_killstreak_list[plrid] = 0;
	SetScoreboardData(plrid, GetKillStreakCol(), Format("%d", score_killstreak_list[plrid]), score_killstreak_list[plrid]);
	// Only if killer exists and has not committed suicide.
	if (plr == killer || !GetPlayerName(killer))
		return _inherited(plr, killer, ...);
	// Only if killer and victim are on different teams.
	if (GetPlayerTeam(killer) && GetPlayerTeam(killer) == GetPlayerTeam(plr))
		return _inherited(plr, killer, ...);
	// Modify scoreboard kill streak count entry for killer.
	score_killstreak_list[killerid]++;
	SetScoreboardData(killerid, GetKillStreakCol(), Format("%d", score_killstreak_list[killerid]), score_killstreak_list[killerid]);
	return _inherited(plr, killer, ...);
}

protected func RemovePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// Clear scoreboard kill streak count entry for this player.
	SetScoreboardData(plrid, GetKillStreakCol(), nil, nil);
	return _inherited(plr, ...);
}

/*-- Misc --*/

public func SetKillStreakCount(int plr, int value)
{
	var plrid = GetPlayerID(plr);
	score_killstreak_list[plrid] = value;
	return;
}

public func GetKillStreakCount(int plr)
{
	var plrid = GetPlayerID(plr);
	return score_killstreak_list[plrid];
}

public func DoKillStreakCount(int plr, int value)
{
	var plrid = GetPlayerID(plr);
	score_killstreak_list[plrid] += value;
	return;
}

public func GetKillStreakCol()
{
	//return ScoreboardCol(Scoreboard_KillStreak);
	return 109;
}
	
protected func Definition(def)
{
	SetProperty("Name", "Scoreboard Kill streaks", def);
}
