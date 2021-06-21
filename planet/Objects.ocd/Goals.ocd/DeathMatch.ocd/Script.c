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

// Duration in seconds the scoreboard will be shown to a player on an event.
local ShowBoardTime = 5;

func Initialize()
{
	maxkills = GameCall("WinKillCount");
	if (maxkills == nil || maxkills < 1)
		maxkills = 4;
	// Assure relaunching is enabled and infinite.
	GetRelaunchRule()->SetDefaultRelaunchCount(nil);
	return _inherited(...);
}

protected func OnClonkDeath(object clonk, proplist killer)
{
	var plr = clonk->GetOwner();
	_inherited(clonk, killer, ...);
	// Show scoreboard for a while
	DoScoreboardShow(1, plr);
	Schedule(this, Format("DoScoreboardShow(-1, %v)", plr), 35 * ShowBoardTime);
	NotifyHUD();
	return;
}

public func IsFulfilled()
{
	// Check whether someone has reached the limit.
	var winner = nil;
	for (var i = 0; i < GetPlayerCount(); i++) 
		if (GetKillCount(GetPlayerByIndex(i)) >= maxkills)
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
		plr->Eliminate();
	}
	return true;
}

public func GetDescription(proplist plr)
{
	if (IsFulfilled()) 
	{
		if (GetKillCount(plr) >= maxkills) 
			return "$MsgVictory$";
	} 
	else 
	{
		var score = GetRelativeScore(plr);
		if (score.kills > 0)
			return Format("$MsgAhead$",	 score.kills,  score.best->GetName());
		else if (score.kills < 0)
			return Format("$MsgBehind$", -score.kills, score.best->GetName());
		else if (score.best == plr) 
			return Format("$MsgYouAreBest$", score.kills);
		else 
			return Format("$MsgEqual$", score.best->GetName());
	}
}

public func Activate(proplist byplr)
{
	if (IsFulfilled()) 
	{
		if (GetKillCount(byplr) >= maxkills) MessageWindow("$MsgVictory$", byplr);
	} 
	else 
	{
		var score = GetRelativeScore(byplr);
		if (score.kills > 0)		 MessageWindow(Format("$MsgAhead$",	 score.kills,  score.best->GetName()), byplr);
		else if (score.kills < 0) MessageWindow(Format("$MsgBehind$", -score.kills, score.best->GetName()), byplr);
		else if (score.best == byplr) MessageWindow(Format("$MsgYouAreBest$", score.kills), byplr);
		else MessageWindow(Format("$MsgEqual$", score.best->GetName()), byplr);
	}
}

private func GetRelativeScore(proplist player)
{
	var bestplayer = -1, bestscore = -1;
	for (var i = 0; i < GetPlayerCount(); ++i)
	{
		var plr = GetPlayerByIndex(i);
		if (plr != player && ((GetKillCount(plr) > bestscore) || (bestplayer == -1))) {
			bestplayer = plr;
			bestscore = GetKillCount(plr);
		}
	}
	
	// special case if there is only one player in the game
	if (bestplayer == -1)
	{
		bestplayer = player;
		bestscore = GetKillCount(player);
	}
	
	return {best: bestplayer, kills: GetKillCount(player)-bestscore};
}

private func GetPlayerTeamScore(proplist player)
{
	if (player->GetTeam() < 1) return GetKillCount(player);
	return GetTeamScore(player->GetTeam());
}

private func GetTeamScore(int team) 
{
	if (team < 1) return 0;
	var score;
	for (var i = 0; i < GetPlayerCount(); ++i)
	{
		var plr = GetPlayerByIndex(i);
		var team2 = plr->GetTeam();
		if (team == team2)
			score += GetKillCount(plr);
	}
	return score;
}

public func GetShortDescription(proplist plr)
{
	return Format("Deathmatch: %d", GetRelativeScore(plr).kills);
}

local Name = "$Name$";
