/*--
		Modular scoreboard: Kills
		Author: Maikel

		This script can be included to create a kill count column in the scoreboard.
		Make sure that the following functions return _inherited(...);
			* Initialize();
			* InitializePlayer(int plr);
			* OnClonkDeath(object clonk, int killer);
			* RemovePlayer(int plr);
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

protected func InitializePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// init scoreboard for player
	score_kill_list[plrid] = 0;
	Scoreboard->NewPlayerEntry(plr);
	return _inherited(plr, ...);
}

protected func OnClonkDeath(object clonk, int killer)
{
	var plr = clonk->GetOwner();
	var plrid = GetPlayerID(killer);
	// Only if killer exists and has not committed suicide.
	if (killer == plr || killer == NO_OWNER)
		return _inherited(clonk, killer, ...);
	// Only if killer and victim are on different teams.
	if (GetPlayerTeam(killer) && GetPlayerTeam(killer) == GetPlayerTeam(plr))
		return _inherited(clonk, killer, ...);
	// Modify scoreboard kill count entry for killer.
	score_kill_list[plrid]++;
	Scoreboard->SetPlayerData(killer, "kills", score_kill_list[plrid]);
	return _inherited(clonk, killer, ...);
}

protected func RemovePlayer(int plr)
{
	return _inherited(plr, ...);
}

/*-- Misc --*/

public func SetKillCount(int plr, int value)
{
	var plrid = GetPlayerID(plr);
	score_kill_list[plrid] = value;
	Scoreboard->SetPlayerData(plr, "kills", score_kill_list[plrid]);
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
	Scoreboard->SetPlayerData(plr, "kills", score_kill_list[plrid]);
	return;
}

local Name = "Scoreboard Kills";
