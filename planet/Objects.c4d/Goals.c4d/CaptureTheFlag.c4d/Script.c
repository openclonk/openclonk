/*--
	Capture The Flag
	Author: Maikel
	
	Capture the flag of the opposing team and bring it to your base to gain points.
	TODO: Scoreboard.
--*/


#include Library_Goal

local score_list;

protected func Initialize()
{
	score_list = [];

	return _inherited(...);
}

private func GetScoreGoal()
{
	return 1;
}

public func SetFlagBase(int team, int x, int y)
{
	var base = CreateObject(Goal_FlagBase, x, y, NO_OWNER);
	base->SetTeam(team);
	var flag = CreateObject(Goal_Flag, x, y, NO_OWNER);
	flag->SetTeam(team);
	return;
}

public func AddTeamScore(int team)
{
	score_list[team]++;
	if (score_list[team] >= GetScoreGoal())
		EliminateOthers(team);
	return;
}

private func EliminateOthers(int win_team)
{
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		var team = GetPlayerTeam(plr);
		if (team != win_team)
			EliminatePlayer(plr);	
	}
	return;
}

protected func InitializePlayer(int plr)
{
	// Join new clonk.
	JoinPlayer(plr);
	// Broadcast to scenario.
	GameCall("OnPlayerRelaunch", plr);
	return _inherited(plr, ...);
}

protected func RelaunchPlayer(int plr)
{
	// New clonk.
	var clonk = CreateObject(Clonk, 0, 0, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	// Join new clonk.
	JoinPlayer(plr);
	// Broadcast to scenario.
	GameCall("OnPlayerRelaunch", plr);
	return _inherited(plr, ...);
}

private func JoinPlayer(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
	var team = GetPlayerTeam(plr);
	var base = FindObject(Find_ID(Goal_FlagBase), Find_Func("FindTeam", team));
	if (base)
		clonk->SetPosition(base->GetX(), base->GetY() - 10);
	return;
}

protected func RemovePlayer(int plr)
{

	return _inherited(plr, ...);
}

/*-- Goal interface --*/

public func IsFulfilled()
{
	// If Teams.txt-Teams still need to be chosen, the goal cannot be fulfilled.
	for (var i = 0; i < GetPlayerCount(); i++)
		if (GetPlayerTeam(GetPlayerByIndex(i)) == -1) 
			return false;
	// If only one team is left, the goal is fulfilled.
	var teamcnt = 0;
	for (var i = 0; i < GetTeamCount(); i++)
		if (GetPlayerInTeamCount(GetTeamByIndex(i)))
			teamcnt++;
	if (teamcnt < 2)
		return true;
	// If one team has reached the score limit, the goal is fulfilled.
	for (var i = 0; i < GetTeamCount(); i++)
	{
		var team = GetTeamByIndex(i);
		if (score_list[team] >= GetScoreGoal())
			return true;
	}
	return false;
}

public func Activate(int byplr)
{
	var flags = GetScoreGoal() - score_list[GetPlayerTeam(byplr)];
	if (IsFulfilled())
		MessageWindow("$MsgGoalFulfilled$", byplr);
	else
	{
		if (flags == 1)
			MessageWindow("$MsgGoalUnfulfilled1$", byplr);
		else
			MessageWindow(Format("$MsgGoalUnfulfilledX$", flags), byplr);
	}
	return;
}

public func GetShortDescription(int plr)
{
	return ""; // TODO
}

// Returns the number of players in a specific team.
private func GetPlayerInTeamCount(int team)
{
	var cnt = 0;
	for (var i = 0; i < GetPlayerCount(); i++)
		if (GetPlayerTeam(GetPlayerByIndex(i)) == team)
			cnt++;
	return cnt;
}

local Name = "$Name$";
