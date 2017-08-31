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
#include Scoreboard_Death

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
	
	Scoreboard->Init(
		[{key = "koth", title = Goal_KingOfTheHill, sorted = true, desc = true, default = 0, priority = 80}]
		);
	Scoreboard->SetTitle("King of the Hill");
	//CalculatePosition();
	ScheduleCall(this, "PostInitialize", 3);
	return _inherited(...);
}

func PostInitialize()
{
	Init();
}

func Init()
{
	location=CreateObjectAbove(KingOfTheHill_Location, 0, 5, NO_OWNER);
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
	Scoreboard->SetPlayerData(player, "koth", player_points[player]);
}

protected func InitializePlayer(int plr, int x, int y, object base, int team)
{
	Scoreboard->NewPlayerEntry(plr);
	player_suicides[plr]=0;
	
	Goal_Melee->MakeHostileToAll(plr, team);
	return inherited(plr, x, y, base, team, ...);
}

public func IsFulfilled()
{
	return Goal_Melee->IsFulfilled(); // the same condition as a normal melee
}

func OnClonkDeath(object clonk, int killer)
{	
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
			
			// for the kill logs
			AddEffect("NewKing", GetCursor(killer), 1, 3, nil);
		}
	}
	CheckForWinner();
	return;
}

public func GetAdditionalPlayerRelaunchString(object clonk, int plr, int killed_by)
{
	if (!Hostile(killed_by, plr)) return;
	if (!location->GetKing()) return;
	if (location->GetKing()->GetOwner() != killed_by) return;
	if (!GetEffect("NewKing", GetCursor(killed_by))) return;
	var msg = Format("$IsNowKing$", GetTaggedPlayerName(killed_by));
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

public func GetDescription(int plr)
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
	return msg;
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

// returns a list of the teams ingame
private func GetTeamList()
{
	var teams=[];
	for(var i = 0; i < GetPlayerCount(); i++)
	{
		var p=GetPlayerByIndex(i);
		var t=GetPlayerTeam(p);
		
		var found = false;
		for(var x=0;x<GetLength(teams);++x)
			if(teams[x] == t) {found = true; break;}
		if(found) continue;
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
		var t = teams[i];
		var p = 0;
		var names = "";
		for(var d=0;d<GetPlayerCount();++d)
		{
			var p=GetPlayerByIndex(d);
			if(GetPlayerTeam(p) != t) continue;
			
			p += player_points[p];

			var comma = ", ";
			if(GetLength(names) == 0) comma = "";
			names = Format("%s%s%s", names, comma, GetTaggedPlayerName(p));
		}
		

		ret[GetLength(ret)]={nr=t, points=p, player_names=names};
	}
	return ret;
}


public func GetShortDescription(int plr)
{
	return ""; // TODO
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (GetPointLimit() != 10) props->AddCall("Goal", this, "SetPointLimit", GetPointLimit());
	if (GetRadius() != 300) props->AddCall("Goal", this, "SetRadius", GetRadius());
	return true;
}

local Name = "$Name$";
