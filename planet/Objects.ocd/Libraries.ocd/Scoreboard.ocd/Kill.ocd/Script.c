/*--
		Modular scoreboard: Kills
		Author: Maikel

		This script can be included to create a kill count column in the scoreboard.
		Make sure that the following functions return _inherited(...);
			* Initialize();
			* InitializePlayer(proplist plr);
			* OnClonkDeath(object clonk, proplist killer);
			* RemovePlayer(proplist plr);
--*/


local score_kill_list; // Here the kill count of all players is stored, access through plrid.

/*-- Callbacks --*/

protected func Initialize()
{
	// Make sure it is a list.
	score_kill_list = [];
	// init scoreboard
	Scoreboard->Init(
		[{key = "kills", title = Scoreboard_Kill, sorted = true, desc = true, default = 0, priority = 50}]
		);
	return _inherited(...);
}

protected func InitializePlayer(proplist plr)
{
	var plrid = plr.ID;
	// init scoreboard for player
	score_kill_list[plrid] = 0;
	Scoreboard->NewPlayerEntry(plr);
	return _inherited(plr, ...);
}

protected func OnClonkDeath(object clonk, proplist killer)
{
	var plr = clonk->GetOwner();
	// Only if killer exists and has not committed suicide.
	if (killer == plr || killer == NO_OWNER)
		return _inherited(clonk, killer, ...);
	// Only if killer and victim are on different teams.
	if (killer->GetTeam() && killer->GetTeam() == plr->GetTeam())
		return _inherited(clonk, killer, ...);
	// Modify scoreboard kill count entry for killer.
	var plrid = killer.ID;
	score_kill_list[plrid]++;
	Scoreboard->SetPlayerData(killer, "kills", score_kill_list[plrid]);
	return _inherited(clonk, killer, ...);
}

protected func RemovePlayer(proplist plr)
{
	return _inherited(plr, ...);
}

/*-- Misc --*/

public func SetKillCount(proplist plr, int value)
{
	var plrid = plr.ID;
	score_kill_list[plrid] = value;
	Scoreboard->SetPlayerData(plr, "kills", score_kill_list[plrid]);
	return;
}

public func GetKillCount(proplist plr)
{
	var plrid = plr.ID;
	return score_kill_list[plrid];
}

public func DoKillCount(proplist plr, int value)
{
	var plrid = plr.ID;
	score_kill_list[plrid] += value;
	Scoreboard->SetPlayerData(plr, "kills", score_kill_list[plrid]);
	return;
}

local Name = "Scoreboard Kills";
