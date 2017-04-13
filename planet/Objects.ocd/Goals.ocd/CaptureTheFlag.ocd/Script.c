/*--
	Capture The Flag
	Author: Maikel
	
	Capture the flag of the opposing team and bring it to your base to gain points.
--*/


#include Library_Goal

local score_list;

func ScoreboardTeamID(int team)
{
	return team + 1000;
}

protected func Initialize()
{
	score_list = [];
	GetRelaunchRule()->SetRespawnDelay(0);
	GetRelaunchRule()->SetDefaultRelaunches(nil);
	
	// init scoreboard
	Scoreboard->Init(
		[
		{key = "title", title = "Teams"},
		{key = "ctf", title = Goal_CaptureTheFlag, sorted = true, desc = true, default = 0, priority = 80}
		]
		);
	Scoreboard->SetTitle("Capture the Flag");
	
	return _inherited(...);
}

private func GetScoreGoal()
{
	var flag_cnt = GameCall("CaptureFlagCount");
	if (flag_cnt != nil)
		return Max(1, flag_cnt);
	return 1;
}

public func SetFlagBase(int team, int x, int y)
{
	var base = CreateObject(Goal_FlagBase, x, y, NO_OWNER);
	base->SetTeam(team);
	var flag = CreateObject(Goal_Flag, x, y, NO_OWNER);
	flag->SetAction("AttachBase", base);
	flag->SetTeam(team);
	return;
}

public func AddTeamScore(int team)
{
	score_list[team]++;
	
	Scoreboard->SetData(ScoreboardTeamID(team), "ctf", score_list[team]);
	
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

protected func InitializePlayer(int plr, int x, int y, object base, int team)
{
	// Join new clonk.
	GetRelaunchRule()->DoRelaunch(plr, nil, RelaunchPosition(team), true);
	
	// make scoreboard entry for team
	Scoreboard->NewEntry(ScoreboardTeamID(team), GetTaggedTeamName(team));
	return _inherited(plr, x, y, base, team, ...);
}

public func RelaunchPosition(int iTeam)
{
	var base = FindObject(Find_ID(Goal_FlagBase), Find_Func("FindTeam", iTeam));
	if (base) return [base->GetX(), base->GetY() - 10];
	return nil;
}

protected func RemovePlayer(int plr)
{

	return _inherited(plr, ...);
}

protected func RejectTeamSwitch(int player, int new_team)
{
	// Prevent team switching in any case.
	return true;
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

public func GetDescription(int plr)
{
	var flags = GetScoreGoal() - score_list[GetPlayerTeam(plr)];
	if (IsFulfilled())
		return "$MsgGoalFulfilled$";
	else
	{
		var msg = "$MsgGoalCaptureTheFlag$";
		if (flags == 1)
			return Format("%s %s", msg, "$MsgGoalUnfulfilled1$");
		else
			return Format("%s %s", msg, Format("$MsgGoalUnfulfilledX$", flags));
	}
}

public func Activate(int byplr)
{
	var flags = GetScoreGoal() - score_list[GetPlayerTeam(byplr)];
	if (IsFulfilled())
		MessageWindow("$MsgGoalFulfilled$", byplr);
	else
	{
		var msg = "$MsgGoalCaptureTheFlag$";
		if (flags == 1)
			msg = Format("%s %s", msg, "$MsgGoalUnfulfilled1$");
		else
			msg = Format("%s %s", msg, Format("$MsgGoalUnfulfilledX$", flags));

		MessageWindow(msg, byplr);
	}
	return;
}

public func GetShortDescription(int plr)
{
	var team = GetPlayerTeam(plr);
	var flag = FindObject(Find_ID(Goal_Flag), Find_Func("FindTeam", team));
	var at_base = flag->IsAtBase();
	if (!at_base)
		return "$MsgShortCaptured$";
	
	var cursor = GetCursor(plr);
	flag = FindObject(Find_ID(Goal_Flag), Find_Not(Find_Func("FindTeam", team)), Find_Container(cursor));
	if (flag)
		return "$MsgShortReturn$";
	
	return "$MsgShortSteal$";
}

local Name = "$Name$";
local Visibility = VIS_Editor;
