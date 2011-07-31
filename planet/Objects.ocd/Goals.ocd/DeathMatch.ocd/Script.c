/*
	Deathmatch
	Author: JCaesar
	
	Provides the standard Deathmatch goal.
*/

#include Goal_Melee
#include Scoreboard_KillStreak
#include Scoreboard_Death
#include Scoreboard_Kill
#include Scoreboard_Player

local maxkills;

static const MIME_ShowBoardTime = 5; // Duration in seconds the scoreboard will be shown to a player on an event.static const MIME_ShowBoardTime = 5; // Duration in seconds the scoreboard will be shown to a player on an event.

func Initialize()
{
	if(ObjectCount(Find_ID(GetID()), Find_Exclude(this)))
	{
		(FindObject(Find_ID(GetID()), Find_Exclude(this)).maxkills) += 2;
		return RemoveObject();
	}
	maxkills = GameCall("WinKillCount");
	if(maxkills == nil || maxkills < 1) maxkills = 4;
	return _inherited(...);
}

protected func InitializePlayer(int plr)
{
	// Join plr.
	JoinPlayer(plr);
	// Scenario script callback.
	GameCall("OnPlayerRelaunch", plr);
	return _inherited(plr, ...);
}

protected func RelaunchPlayer(int plr, int killer)
{
	_inherited(plr, killer, ...);
	var clonk = CreateObject(Clonk, 0, 0, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	JoinPlayer(plr);
	// Scenario script callback.
	GameCall("OnPlayerRelaunch", plr);
	// Show scoreboard for a while & sort.
	SortScoreboard(Scoreboard_KillStreak->GetKillStreakCol(), true);
	SortScoreboard(Scoreboard_Death->GetDeathCol(), true);
	SortScoreboard(Scoreboard_Kill->GetKillCol(), true);
	DoScoreboardShow(1, plr + 1);
	Schedule(this,Format("DoScoreboardShow(-1, %d)", plr + 1), 35 * MIME_ShowBoardTime);
	NotifyHUD();
	return; // _inherited(plr, killer, ...);
}

protected func JoinPlayer(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
	var x, y;
	var pos = FindRelaunchPos(plr);
	clonk->SetPosition(pos[0], pos[1]);
	return;
}

private func FindRelaunchPos(int plr)
{
	var tx, ty; // Test position.
	for (var i = 0; i < 500; i++)
	{
		tx = Random(LandscapeWidth());
		ty = Random(LandscapeHeight());
		if (GBackSemiSolid(AbsX(tx), AbsY(ty)))
			continue;
		if (GBackSemiSolid(AbsX(tx+5), AbsY(ty+10)))
			continue;
		if (GBackSemiSolid(AbsX(tx+5), AbsY(ty-10)))
			continue;
		if (GBackSemiSolid(AbsX(tx-5), AbsY(ty+10)))
			continue;
		if (GBackSemiSolid(AbsX(tx-5), AbsY(ty-10)))
			continue;
		// Succes.
		return [tx, ty];
	}
	return nil;
}

public func IsFulfilled()
{
	//Check whether someone has reached the limit
	var fulfilled = CreateArray();
	for(var i = GetPlayerCount() - 1; i > -1; --i) 
		if(GetKillCount(GetPlayerByIndex(i)) >= maxkills)
			fulfilled[GetLength(fulfilled)] = i;
	if(!GetLength(fulfilled)) return false;
	//Eliminate all players, that are not in a team with one of the winners
	for(var i = GetPlayerCount() - 1; i > -1; --i) 
	{
		var eliminate = true;
		for(var j = GetLength(fulfilled); j; --j) 
		{
			if(fulfilled[j] == i) {eliminate = false; break;}
			if(!CheckTeamHostile(GetPlayerByIndex(fulfilled[j]), GetPlayerByIndex(i))) {eliminate = false; break;}
		}
		if(eliminate) EliminatePlayer(GetPlayerByIndex(i));
	}
	return true;
}

public func Activate(int byplr)
{
	if(IsFulfilled()) 
	{
		if(GetKillCount(byplr) >= maxkills) MessageWindow("$MsgVictory$", byplr);
	} 
	else 
	{
		var score = GetRelativeScore(byplr);
		if(score.kills > 0)      MessageWindow(Format("$MsgAhead$",  score.kills,  GetPlayerName(score.best)), byplr);
		else if(score.kills < 0) MessageWindow(Format("$MsgBehind$", -score.kills,GetPlayerName(score.best)), byplr);
		else MessageWindow(Format("$MsgEqual$", GetPlayerName(score.best)), byplr);
	}
}

private func GetRelativeScore(int player)
{
	var bestplayer = -1, bestscore = 1<<31;
	for(var i = 0; i < GetPlayerCount(); ++i)
	{
		var plr = GetPlayerByIndex(i);
		if(plr != player && GetKillCount(plr) > bestscore) {
			bestplayer = plr;
			bestscore = GetKillCount(plr);
		}
	}
	return {best: bestplayer, kills: GetKillCount(player)-bestscore};
}

private func GetPlayerTeamScore(int player)
{
	if(GetPlayerTeam(player) < 1) return GetKillCount(player);
	return GetTeamScore(GetPlayerTeam(player));
}

private func GetTeamScore(int team) 
{
	if(team < 1) return 0;
	var score;
	for(var i = 0; i < GetPlayerCount(); ++i)
	{
		var plr = GetPlayerByIndex(i);
		var team2 = GetPlayerTeam(plr);
		if(team == team2)
			score += GetKillCount(plr);
	}
	return score;
}

public func GetShortDescription(int plr)
{
	return Format("Deathmatch: %d", GetRelativeScore(plr).kills);
}

local Name = "$Name$";