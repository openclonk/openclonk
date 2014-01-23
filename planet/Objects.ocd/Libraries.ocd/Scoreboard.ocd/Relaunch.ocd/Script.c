/*--
		Modular scoreboard: Relaunches
		Author: Maikel

		This script can be included to create a relaunch count column in the scoreboard.
		Make sure that the following functions return _inherited(...);
			* Initialize();
			* InitializePlayer(int plr);
			* RelaunchPlayer(int plr, int killer);
			* RemovePlayer(int plr);
--*/


local score_relaunch_list; // Here the relaunch count of all players is stored, access through plrid.

// Overload this.
public func RelaunchCount()
{
	return 5;
}

/*-- Callbacks --*/

protected func Initialize()
{
	// Make sure it is a list.
	score_relaunch_list = [];
	// init scoreboard
	// init scoreboard, uses the condition of Scoreboard_Deaths too
	Scoreboard->Init(
		[{key = "relaunches", title = Scoreboard_Relaunch, sorted = true, desc = true, default = "", priority = 75, conditional = Scoreboard_Death.ScoreboardCondition}]
		);
	return _inherited(...);
}

protected func InitializePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// create scoreboard entry
	score_relaunch_list[plrid] = RelaunchCount();
	Scoreboard->NewPlayerEntry(plr);
	Scoreboard->SetPlayerData(plr, "relaunches", score_relaunch_list[plrid]);
	return _inherited(plr, ...);
}

protected func RelaunchPlayer(int plr, int killer)
{
	var plrid = GetPlayerID(plr);
	// Modify scoreboard relaunch count entry for this player.
	score_relaunch_list[plrid]--;
	Scoreboard->SetPlayerData(plr, "relaunches", score_relaunch_list[plrid]);
	return _inherited(plr, killer, ...);
}

protected func RemovePlayer(int plr)
{
	return _inherited(plr, ...);
}

/*-- Misc --*/

public func SetRelaunchCount(int plr, int value)
{
	var plrid = GetPlayerID(plr);
	score_relaunch_list[plrid] = value;
	Scoreboard->SetPlayerData(plr, "relaunches", score_relaunch_list[plrid]);
	return;
}

public func GetRelaunchCount(int plr)
{
	var plrid = GetPlayerID(plr);
	return score_relaunch_list[plrid];
}

public func DoRelaunchCount(int plr, int value)
{
	var plrid = GetPlayerID(plr);
	score_relaunch_list[plrid] += value;
	Scoreboard->SetPlayerData(plr, "relaunches", score_relaunch_list[plrid]);
	return;
}

local Name = "Scoreboard Relaunches";
