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

/*-- Callbacks --*/

protected func Initialize()
{
	// Make sure it is a list.
	score_relaunch_list = [];
	// Set scoreboard relaunch count caption.
	SetScoreboardData(SBRD_Caption, GetRelaunchCol(), "{{Scoreboard_Relaunch}}", SBRD_Caption);
	return _inherited(...);
}

protected func InitializePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// Create scoreboard relaunch count entry for this player.
	score_relaunch_list[plrid] = RelaunchCount();
	SetScoreboardData(plrid, GetRelaunchCol(), Format("%d", score_relaunch_list[plrid]), score_relaunch_list[plrid]);
	return _inherited(plr, ...);
}

protected func RelaunchPlayer(int plr, int killer)
{
	var plrid = GetPlayerID(plr);
	// Modify scoreboard relaunch count entry for this player.
	score_relaunch_list[plrid]--;
	SetScoreboardData(plrid, GetRelaunchCol(), Format("%d", score_relaunch_list[plrid]), score_relaunch_list[plrid]);
	return _inherited(plr, killer, ...);
}

protected func RemovePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// Clear scoreboard relaunch count entry for this player.
	SetScoreboardData(plrid, GetRelaunchCol(), nil, nil);
	return _inherited(plr, ...);
}

/*-- Misc --*/

public func SetRelaunchCount(int plr, int value)
{
	var plrid = GetPlayerID(plr);
	score_relaunch_list[plrid] = value;
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
	return;
}

public func GetRelaunchCol()
{
	//return ScoreboardCol(Scoreboard_Relaunch);
	return 103;
}

// Overload this.
public func RelaunchCount()
{
	return 5;
}

local Name = "Scoreboard Relaunches";
