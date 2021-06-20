/*--
		Modular scoreboard: Deaths
		Author: Maikel

		This script can be included to create a death count column in the scoreboard.
		Make sure that the following functions return _inherited(...);
			* Initialize();
			* InitializePlayer(int plr);
			* OnClonkDeath(object clonk, int killer);
			* RemovePlayer(int plr);
--*/


local score_death_list; // Here the death count of all players is stored, access through plrid.

/*-- Callbacks --*/

// called by the scoreboard, assigns a symbol to the scoreboard field
// used by Scoreboard_Relaunch too
public func ScoreboardCondition(x)
{
	if (GetType(x) != C4V_Int) return x;
	
	if (x == -1) return Rule_KillLogs;
	return x;
}

protected func Initialize()
{
	// Make sure it is a list.
	score_death_list = [];
	// init scoreboard
	Scoreboard->Init(
		[{key = "deaths", title = Scoreboard_Death, sorted = true, desc = true, default = 0, priority = 75, conditional = Scoreboard_Death.ScoreboardCondition}]
		);
	return _inherited(...);
}

protected func InitializePlayer(int plr)
{
	var plrid = GetPlayerID(plr);
	// Create scoreboard entry for this player, will only do it once
	score_death_list[plrid] = 0;
	Scoreboard->NewPlayerEntry(plr);
	return _inherited(plr, ...);
}

protected func OnClonkDeath(object clonk, int killer)
{
	var plr = clonk->GetOwner();
	var plrid = GetPlayerID(plr);
	// Modify scoreboard death count entry for this player.
	score_death_list[plrid]++;
	Scoreboard->SetPlayerData(plr, "deaths", score_death_list[plrid]);
	return _inherited(clonk, killer, ...);
}

protected func RemovePlayer(int plr)
{
	return _inherited(plr, ...);
}

/*-- Misc --*/

public func SetDeathCount(int plr)
{
	var plrid = GetPlayerID(plr);
	score_death_list[plrid] = 0;
	Scoreboard->SetPlayerData(plr, "deaths", score_death_list[plrid]);
	return;
}

public func GetDeathCount(int plr)
{
	var plrid = GetPlayerID(plr);
	return score_death_list[plrid];
}

local Name = "Scoreboard Deaths";
