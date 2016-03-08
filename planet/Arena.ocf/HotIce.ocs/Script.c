/* Hot ice */

static g_remaining_rounds, g_winners, g_check_victory_effect;

func Initialize()
{
	g_remaining_rounds = SCENPAR_Rounds;
	g_winners = [];
	InitializeRound();
}

// Resets the scenario, redrawing the map.
func ResetRound()
{
	// Retrieve all Clonks.
	var clonks = FindObjects(Find_OCF(OCF_CrewMember));
	for (var clonk in clonks)
	{
		var container = clonk->Contained();
		if (container)
		{
			clonk->Exit();
			container->RemoveObject();
		}
		clonk->SetObjectStatus(C4OS_INACTIVE);
	}
	// Clear and redraw the map.
	LoadScenarioSection("Empty");
	LoadScenarioSection("main");
	InitializeRound();
	// Re-enable the players.
	for (var clonk in clonks)
	{
		clonk->SetObjectStatus(C4OS_NORMAL);
		SetCursor(clonk->GetOwner(), clonk);
		// Select the first item. This fixes item ordering.
		clonk->SetHandItemPos(0, 0);
		InitializePlayer(clonk->GetOwner());
	}
}

func InitializeRound()
{
	// Checking for victory: Only active after a Clonk dies.
	g_check_victory_effect = AddEffect("CheckVictory", nil, 1, 0);
	g_player_spawn_index = 0;
	ShuffleArray(g_player_spawn_positions);

	// Materials: Chests
	var i,pos;
	var ls_wdt = LandscapeWidth(), ls_hgt = LandscapeHeight();
	var chest_area_y = ls_hgt*[0,30][SCENPAR_MapType]/100;
	var chest_area_hgt = ls_hgt/2;
	// Chests in regular mode. Boom packs in grenade launcher mode.
	var num_extras = [6,12][SCENPAR_Weapons];
	for (i=0; i<num_extras; ++i)
		if (pos=FindLocation(Loc_InRect(0,chest_area_y,ls_wdt,chest_area_hgt-100), Loc_Wall(CNAT_Bottom))) // Loc_Wall adds us 100 pixels...
		{
			if (SCENPAR_Weapons == 0)
			{
				var chest = CreateObjectAbove(Chest,pos.x,pos.y);
				if (chest)
				{
					chest->CreateContents(Firestone,5);
					chest->CreateContents(Bread,1);
					chest->CreateContents(Bow,1);
					chest->CreateContents(FireArrow,1)->SetStackCount(5);
					chest->CreateContents(BombArrow,1)->SetStackCount(5);
					chest->CreateContents(Shield,1);
					chest->CreateContents(IronBomb,3);
				}
			}
			else
			{
				var boompack= CreateObjectAbove(Boompack,pos.x,pos.y);
			}
		}
	// Materials: Firestones
	for (i=0; i<30; ++i)
		if (pos=FindLocation(Loc_InRect(0,chest_area_y,ls_wdt,chest_area_hgt), Loc_Solid()))
			if (IsFirestoneSpot(pos.x,pos.y))
				CreateObjectAbove(Firestone,pos.x,pos.y-1);
	// Some firestones and bombs in lower half. For ap type 1, more firestones in lower than upper half.
	for (i=0; i<30; ++i)
		if (pos=FindLocation(Loc_InRect(0,ls_hgt/2,ls_wdt,ls_hgt/3), Loc_Solid()))
			if (IsFirestoneSpot(pos.x,pos.y))
				CreateObjectAbove([Firestone,IronBomb][Random(Random(3))],pos.x,pos.y-1);
	return true;
}

static g_player_spawn_positions, g_map_width, g_player_spawn_index;

func InitializePlayer(int plr)
{
	// everything visible
	SetFoW(false, plr);
	// Player positioning. 
	var ls_wdt = LandscapeWidth(), ls_hgt = LandscapeHeight();
	var crew = GetCrew(plr), start_pos;
	// Position by map type?
	if (g_player_spawn_positions && g_player_spawn_index < GetLength(g_player_spawn_positions))
	{
		start_pos = g_player_spawn_positions[g_player_spawn_index++];
		var map_zoom = ls_wdt / g_map_width;
		start_pos = {x=start_pos[0]*map_zoom+map_zoom/2, y=start_pos[1]*map_zoom};
	}
	else
	{
		// Start positions not defined or exhausted: Spawn in lower area for both maps becuase starting high is an an advantage.
		start_pos = FindLocation(Loc_InRect(ls_wdt/5,ls_hgt/2,ls_wdt*3/5,ls_hgt/3), Loc_Wall(CNAT_Bottom), Loc_Func(Scenario.IsStartSpot));
		if (!start_pos) start_pos = FindLocation(Loc_InRect(ls_wdt/10,0,ls_wdt*8/10,ls_hgt*4/5), Loc_Wall(CNAT_Bottom), Loc_Func(Scenario.IsStartSpot));
		if (!start_pos) start_pos = {x=Random(ls_wdt*6/10)+ls_wdt*2/10, y=ls_hgt*58/100};
	}
	crew->SetPosition(start_pos.x, start_pos.y-10);
	// initial material
	if (SCENPAR_Weapons == 0)
	{
		crew->CreateContents(Shovel);
		crew->CreateContents(Club);
		crew->CreateContents(WindBag);
		crew->CreateContents(Firestone,2);
	}
	else
	{
		// Grenade launcher mode
		crew.MaxContentsCount = 2;
		crew->CreateContents(WindBag);
		var launcher = crew->CreateContents(GrenadeLauncher);
		if (launcher)
		{
			var ammo = launcher->CreateContents(IronBomb);
			launcher->AddTimer(Scenario.ReplenishLauncherAmmo, 10);
		}
	}
	crew.MaxEnergy = 100000;
	crew->DoEnergy(1000);
	return true;
}

func OnClonkDeath(object clonk)
{
	var plr = clonk->GetOwner();
	// Skip eliminated players, NO_OWNER, etc.
	if (!GetPlayerName(plr)) 
		return true; 

	var crew = CreateObject(Clonk, 0, 0, plr);
	crew->MakeCrewMember(plr);
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, plr);
	// We just use the relaunch object as a dumb container.
	crew->Enter(relaunch);

	// Check for victory after three seconds to allow stalemates.
	g_check_victory_effect.Interval = 36 * 3;
}

// Returns a list of colored player names, for example "Sven2, Maikel, Luchs"
global func GetTeamPlayerNames(int team)
{
	var str = "";
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		if (GetPlayerTeam(plr) == team)
		{
			var comma = "";
			if (str != "") comma = ", ";
			str = Format("%s%s<c %x>%s</c>", str, comma, GetPlayerColor(plr), GetPlayerName(plr));
		}
	}
	return str;
}

global func FxCheckVictoryTimer(_, proplist effect)
{
	var find_living = Find_And(Find_OCF(OCF_CrewMember), Find_NoContainer());
	var clonk = FindObject(find_living);
	if (!clonk)
	{
		// Stalemate!
		Log("$Stalemate$");
		GameCall("ResetRound");
	}
	else if (!FindObject(find_living, Find_Hostile(clonk->GetOwner())))
	{
		// We have a winner!
		var team = GetPlayerTeam(clonk->GetOwner());
		PushBack(g_winners, team);
		// Announce the winning team.
		Log("$WinningTeam$", GetTeamPlayerNames(team));
		if (--g_remaining_rounds > 0 || GetLeadingTeam() == nil)
		{
			if (g_remaining_rounds == 1)
				Log("$LastRound$");
			else if (g_remaining_rounds > 1)
				Log("$RemainingRounds$", g_remaining_rounds);
			else
				Log("$Tiebreak$");
			GameCall("ResetRound");
		}
		else
		{
			GameCall("EliminateLosers");
		}
	}
	// Go to sleep again.
	effect.Interval = 0;
	return FX_OK;
}

// Returns the team which won the most rounds, or nil if there is a tie.
global func GetLeadingTeam()
{
	var teams = [], winning_team = g_winners[0];
	for (var w in g_winners)
	{
		teams[w] += 1;
		if (teams[w] > teams[winning_team])
			winning_team = w;
	}
	// Detect a tie.
	for (var i = 0; i < GetLength(teams); i++)
	{
		if (i != winning_team && teams[i] == teams[winning_team])
			return nil;
	}
	return winning_team;
}

func EliminateLosers()
{
	// Determine the winning team.
	var winning_team = GetLeadingTeam();
	// Eliminate everybody who isn't on the winning team.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		if (GetPlayerTeam(plr) != winning_team)
			EliminatePlayer(plr);
	}
	// The scenario goal will end the scenario.
}

/* Called periodically in grenade launcher */
func ReplenishLauncherAmmo()
{
	if (!ContentsCount()) CreateContents(IronBomb);
	return true;
}

// Horizontal Loc_Space doesn't work with Loc_Wall because it checks inside the ground.
func IsStartSpot(int x, int y)
{
	// Don't spawn just at the border of an island.
	if (!GBackSolid(x-3,y+2)) return false;
	if (!GBackSolid(x+3,y+2)) return false;
	// Spawn with some space.
	return PathFree(x-5, y, x+5, y) && PathFree(x, y-21, x, y-1);
}

func IsFirestoneSpot(int x, int y)
{
// Very thorough ice surrounding check so they don't explode right away or when the first layer of ice melts
	return GBackSolid(x,y-1) && GBackSolid(x,y+4) && GBackSolid(x-2,y) && GBackSolid(x+2,y);
}
