/*
KOTH
Author: Zapper

This object keeps track of the points and deaths of the players.
Additionally it creates a KingOfTheHill_Location that does the rest.

Interface:

Radius of the area:
SetRadius(int x);
GetRadius();

points to achieve for the victory:
SetPointLimit(int x);
GetPointLimit();

automatically place the area on the map:
SearchPosition();
*/

#include Library_Goal

local player_points;
local player_deaths;
local location;
local point_limit;
local radius;
local player_suicides;

func Initialize()
{
	// standards
	player_points=[];
	player_deaths=[];
	player_suicides=[];
	SetRadius(300);
	SetPointLimit(10);
	
	//CalculatePosition();
	ScheduleCall(this, "PostInitialize", 3);
	return _inherited(...);
}

func PostInitialize()
{
	ScheduleCall(this, "RefreshScoreboard", 1);
	Init();
}

func Init()
{
	location=CreateObject(KingOfTheHill_Location, 0, 5, NO_OWNER);
	location->SetKotH(this);
}

func Destruction()
{
	if(location) location->RemoveObject();
}

func SearchPosition()
{
	var a=0, b=LandscapeHeight();

	var lastX;
	var lastY;
	
	while(Abs(a - b) > 10)
	{
		var m=(a + b) / 2;
		
		var block=PathFree2(0, m, LandscapeWidth(), m);
		
		if(!block)
		{
			a=m;
		}
		else
		{
			b=m;
			lastX=block[0];
			lastY=block[1];
		}
	}
	
	location->SetPosition(lastX + 10, lastY - 10);
	location->NewPosition();
}

public func GetPointLimit()
{
	return point_limit;
}

public func SetPointLimit(int x)
{
	point_limit=x;
}

public func GetRadius()
{
	return radius;
}

public func SetRadius(int to)
{
	radius=to;
}

func DoPoint(int player, int count)
{
	if (count == nil) 
		count = 1;
	player_points[player] = Max(player_points[player] + count, 0);
}

protected func InitializePlayer(plr)
{
	ScheduleCall(this, "RefreshScoreboard", 1);
	player_suicides[plr]=0;
	return Goal_Melee->InitializePlayer(plr, ...); // TODO
}

public func IsFulfilled()
{
	return Goal_Melee->IsFulfilled(); // TODO
}

func OnClonkDeath(object clonk, int killer)
{	
	ScheduleCall(this, "RefreshScoreboard", 1);
	if (clonk->GetAlive()) return;
		
	if (GetPlayerName(clonk->GetOwner()))
		++player_deaths[clonk->GetOwner()];
	 
	if(GetPlayerName(clonk->GetOwner()))
	if (killer == clonk->GetOwner() || killer == NO_OWNER)
	{
		// shame on the king who kills himself
		if (location->GetKing() == clonk)
		{
			DoPoint(clonk->GetOwner(),-1);
			return;
		}
		else
		{
			// non-king suicide
			player_suicides[clonk->GetOwner()]++;
			if(player_suicides[clonk->GetOwner()] % 2 == 0)
			{
				DoPoint(clonk->GetOwner(),-1);
			}
		}
		
	}
	
	if (location->GetKing() != nil) 
	{
		if (location->GetKing()->GetOwner() == killer)
			DoPoint(killer);
		else if (location->GetKing() == clonk)
		{
			DoPoint(killer);
			location->SetKing(GetCursor(killer));
		}
	}
	CheckForWinner();
	return;
}

func GetAdditionalPlayerRelaunchString(object clonk, int plr, int killed_by)
{
	if(!Hostile(killed_by, plr)) return;
	if(!location->GetKing()) return;
	if(location->GetKing()->GetOwner() != killed_by) return;
	var msg=Format("$IsNowKing$", GetTaggedPlayerName(killed_by));
	return msg;
}

private func CheckForWinner()
{
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		if (player_points[plr] >= GetPointLimit())
		{
			for (var j = 0; j < GetPlayerCount(); j++)
			{
				var check_plr = GetPlayerByIndex(j);
				if (check_plr == plr)
					continue;
				if (GetPlayerTeam(check_plr) != 0 && GetPlayerTeam(check_plr) == GetPlayerTeam(plr))
					continue;
				EliminatePlayer(check_plr);
			}
			break;
		}
	}
	return;
}


public func Activate(int byplr)
{
	var teams=GetTeamPoints();
	var lines=[];
	
	for(var i=0;i<GetLength(teams);++i)
	{
		lines[GetLength(lines)]=Format("%s: %d", teams[i]["player_names"], teams[i]["points"] );
	}
	
	var msg=Format("$MsgGoalDesc$", GetPointLimit());
	for(var i=0;i<GetLength(lines);++i)
		msg=Format("%s|%s", msg, lines[i]);
	return MessageWindow(msg, byplr);
}

private func GetTeamList()
{
	// Count enemy players.
	var teams=[];
	for(var i = 0; i < GetPlayerCount(); i++)
	{
		var p=GetPlayerByIndex(i);
		var t=GetPlayerTeam(p);
		var found=false;
		for(var x=0;x<GetLength(teams);++x)
			if(teams[x] == t) found=true;
		if(found)
			continue;
		teams[GetLength(teams)]=t;
	}
	return teams;
}

private func GetTeamPoints()
{
	
	var teams=GetTeamList();
	var ret=[];
	
	for(var i=0;i<GetLength(teams);++i)
	{
		var t=teams[i];
		var p=0;
		var names="";
		for(var d=0;d<GetPlayerCount();++d)
		{
			var p=GetPlayerByIndex(d);
			p+=player_points[p];
			names=Format("%s, ", names);
			names=Format("%s%s", names, GetTaggedPlayerName(p));
		}
		

		ret[GetLength(ret)]={nr=t, points=p, player_names=names};
	}
	return ret;
}

static const SBRD_Deaths=2;
static const SBRD_Points=1;

func RefreshScoreboard()
{
	SetScoreboardData(SBRD_Caption,SBRD_Caption,"King of the Hill",SBRD_Caption);
	SetScoreboardData(SBRD_Caption,SBRD_Points,Format("{{Sword}} / %d", GetPointLimit()),SBRD_Caption);
	SetScoreboardData(SBRD_Caption,SBRD_Deaths,"{{Clonk}}",SBRD_Caption);
	
	
	var points=GetTeamPoints();
	
	for(var cnt=0;cnt<GetPlayerCount();cnt++)
	{
		var plr=GetPlayerByIndex(cnt);
		SetScoreboardData(plr+2,SBRD_Caption,GetTaggedPlayerName(plr),SBRD_Caption);

		SetScoreboardData(plr+2,SBRD_Points,Format("%d", player_points[plr]),player_points[plr]);
		SetScoreboardData(plr+2,SBRD_Deaths,Format("%d", player_deaths[plr]),player_deaths[plr]);
	}
	SortScoreboard(SBRD_Deaths,1);
	SortScoreboard(SBRD_Points,1);
}


public func GetShortDescription(int plr)
{
	return ""; // TODO
}

local Name = "$Name$";
