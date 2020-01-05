/*--
		Modular scoreboard: Kill streaks
		Author: Maikel

		This script can be included to create a kill streak count column in the scoreboard.
		Make sure that the following functions return _inherited(...);
			* Initialize();
			* InitializePlayer(int plr);
			* OnClonkDeath(object clonk, int killer);
			* RemovePlayer(int plr);
--*/


local score_killstreak_list; // Here the kill streak count of all players is stored, access through plr.

/*-- Callbacks --*/

protected func Initialize()
{
	// Make sure it is a list.
	score_killstreak_list = [];
	// init scoreboard
	Scoreboard->Init(
		[{key = "killstreaks", title = Scoreboard_KillStreak, sorted = true, desc = true, default = "", priority = 20}]
		);
	return _inherited(...);
}

protected func InitializePlayer(int plr)
{
	// make scoreboard entry for player
	score_killstreak_list[plr] = 0;
	Scoreboard->NewPlayerEntry(plr);
	return _inherited(plr, ...);
}

protected func OnClonkDeath(object clonk, int killer)
{
	var plr = clonk->GetOwner();
	// reset scoreboard kill streak count entry for killed player.
	score_killstreak_list[plr] = 0;
	Scoreboard->SetPlayerData(plr, "killstreaks", nil);
	// Only if killer exists and has not committed suicide.
	if (plr == killer || !GetPlayerName(killer))
		return _inherited(clonk, killer, ...);
	// Only if killer and victim are on different teams.
	if (GetPlayerTeam(killer) && GetPlayerTeam(killer) == GetPlayerTeam(plr))
		return _inherited(clonk, killer, ...);
	// Modify scoreboard kill streak count entry for killer.
	score_killstreak_list[killer]++;
	Scoreboard->SetPlayerData(killer, "killstreaks", score_killstreak_list[killer]);
	return _inherited(clonk, killer, ...);
}

protected func RemovePlayer(int plr)
{
	return _inherited(plr, ...);
}

/*-- Misc --*/

public func SetKillStreakCount(int plr, int value)
{
	score_killstreak_list[plr] = value;
	Scoreboard->SetPlayerData(plr, "killstreaks", score_killstreak_list[plr]);
	return;
}

public func GetKillStreakCount(int plr)
{
	return score_killstreak_list[plr];
}

public func DoKillStreakCount(int plr, int value)
{
	score_killstreak_list[plr] += value;
	Scoreboard->SetPlayerData(plr, "killstreaks", score_killstreak_list[plr]);
	return;
}

local Name = "Scoreboard Kill streaks";
