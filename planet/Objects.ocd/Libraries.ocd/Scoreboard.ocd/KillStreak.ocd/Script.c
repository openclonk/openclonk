/*--
		Modular scoreboard: Kill streaks
		Author: Maikel

		This script can be included to create a kill streak count column in the scoreboard.
		Make sure that the following functions return _inherited(...);
			* Initialize();
			* InitializePlayer(proplist plr);
			* OnClonkDeath(object clonk, proplist killer);
			* RemovePlayer(proplist plr);
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

protected func InitializePlayer(proplist plr)
{
	// make scoreboard entry for player
	score_killstreak_list[plr] = 0;
	Scoreboard->NewPlayerEntry(plr);
	return _inherited(plr, ...);
}

protected func OnClonkDeath(object clonk, proplist killer)
{
	var plr = clonk->GetOwner();
	// reset scoreboard kill streak count entry for killed player.
	score_killstreak_list[plr] = 0;
	Scoreboard->SetPlayerData(plr, "killstreaks", nil);
	// Only if killer exists and has not committed suicide.
	if (plr == killer || !killer->GetName())
		return _inherited(clonk, killer, ...);
	// Only if killer and victim are on different teams.
	if (killer->GetTeam() && killer->GetTeam() == plr->GetTeam())
		return _inherited(clonk, killer, ...);
	// Modify scoreboard kill streak count entry for killer.
	score_killstreak_list[killer]++;
	Scoreboard->SetPlayerData(killer, "killstreaks", score_killstreak_list[killer]);
	return _inherited(clonk, killer, ...);
}

protected func RemovePlayer(proplist plr)
{
	return _inherited(plr, ...);
}

/*-- Misc --*/

public func SetKillStreakCount(proplist plr, int value)
{
	score_killstreak_list[plr] = value;
	Scoreboard->SetPlayerData(plr, "killstreaks", score_killstreak_list[plr]);
	return;
}

public func GetKillStreakCount(proplist plr)
{
	return score_killstreak_list[plr];
}

public func DoKillStreakCount(proplist plr, int value)
{
	score_killstreak_list[plr] += value;
	Scoreboard->SetPlayerData(plr, "killstreaks", score_killstreak_list[plr]);
	return;
}

local Name = "Scoreboard Kill streaks";
