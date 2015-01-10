/*
	Deathmatch
	Author: JCaesar
	
	Provides the standard Deathmatch goal.
*/

#include Goal_Melee
#include Scoreboard_KillStreak
#include Scoreboard_Death
#include Scoreboard_Kill

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
	var clonk = CreateObjectAbove(Clonk, 0, 0, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	JoinPlayer(plr);
	// Scenario script callback.
	GameCall("OnPlayerRelaunch", plr);
	// Show scoreboard for a while
	DoScoreboardShow(1, plr + 1);
	Schedule(this,Format("DoScoreboardShow(-1, %d)", plr + 1), 35 * MIME_ShowBoardTime);
	NotifyHUD();
	return; // _inherited(plr, killer, ...);
}

protected func JoinPlayer(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
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
	// Check whether someone has reached the limit.
	var winner = nil;
	for (var i = 0; i < GetPlayerCount(); i++) 
		if(GetKillCount(GetPlayerByIndex(i)) >= maxkills)
			winner = GetPlayerByIndex(i);
	if (winner == nil)
		// Otherwise just check if there are no enemies
		return Goal_Melee->IsFulfilled();
	// Eliminate all players, that are not in a team with one of the winners
	for (var i = 0; i < GetPlayerCount(); i++)  
	{
		var plr = GetPlayerByIndex(i);
		if (winner == plr)
			continue;
		if (!CheckTeamHostile(winner, plr))
			continue;
		EliminatePlayer(plr);
	}
	return true;
}

public func GetDescription(int plr)
{
	if(IsFulfilled()) 
	{
		if (GetKillCount(plr) >= maxkills) 
			return "$MsgVictory$";
	} 
	else 
	{
		var score = GetRelativeScore(plr);
		if (score.kills > 0)
			return Format("$MsgAhead$",  score.kills,  GetPlayerName(score.best));
		else if (score.kills < 0)
			return Format("$MsgBehind$", -score.kills, GetPlayerName(score.best));
		else if (score.best == plr) 
			return Format("$MsgYouAreBest$", score.kills);
		else 
			return Format("$MsgEqual$", GetPlayerName(score.best));
	}
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
		else if(score.best == byplr) MessageWindow(Format("$MsgYouAreBest$", score.kills), byplr);
		else MessageWindow(Format("$MsgEqual$", GetPlayerName(score.best)), byplr);
	}
}

private func GetRelativeScore(int player)
{
	var bestplayer = -1, bestscore = -1;
	for(var i = 0; i < GetPlayerCount(); ++i)
	{
		var plr = GetPlayerByIndex(i);
		if(plr != player && ((GetKillCount(plr) > bestscore) || (bestplayer == -1))) {
			bestplayer = plr;
			bestscore = GetKillCount(plr);
		}
	}
	
	// special case if there is only one player in the game
	if(bestplayer == -1)
	{
		bestplayer = player;
		bestscore = GetKillCount(player);
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
