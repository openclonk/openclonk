/* Hot ice */

static g_remaining_rounds, g_winners, g_check_victory_effect;
static g_gameover;

func Initialize()
{
	g_remaining_rounds = SCENPAR_Rounds;
	g_winners = [];
	InitializeRound();

	Scoreboard->Init([
		// Invisible team column for sorting players under their teams.
		{key = "team", title = "", sorted = true, desc = false, default = "", priority = 90},
		{key = "wins", title = "Wins", sorted = true, desc = true, default = 0, priority = 100},
		{key = "death", title = "", sorted = false, default = "", priority = 0},
	]);

}

// Resets the scenario, redrawing the map.
func ResetRound()
{
	// Retrieve all Clonks.
	var clonks = [];
	for (var clonk in FindObjects(Find_OCF(OCF_CrewMember)))
	{
		var container = clonk->Contained();
		if (container)
		{
			clonk->Exit();
			container->RemoveObject();
		}
		else
		{
			// Players not waiting for a relaunch get a new Clonk to prevent
			// status effects from carrying over to the next round.
			var new_clonk = CreateObject(clonk->GetID(), 0, 0, clonk->GetOwner());
			new_clonk->GrabObjectInfo(clonk);
			clonk = new_clonk;
		}
		PushBack(clonks, clonk);
		clonk->SetObjectStatus(C4OS_INACTIVE);
	}
	// Clear and redraw the map.
	LoadScenarioSection("main");
	InitializeRound();
	AssignHandicaps();
	// Re-enable the players.
	for (var clonk in clonks)
	{
		clonk->SetObjectStatus(C4OS_NORMAL);
		SetCursor(clonk->GetOwner(), clonk);
		// Select the first item. This fixes item ordering.
		clonk->SetHandItemPos(0, 0);
		InitPlayerRound(clonk->GetOwner());
	}
}

func InitializeRound()
{
	// Checking for victory: Only active after a Clonk dies.
	g_check_victory_effect = AddEffect("CheckVictory", nil, 1, 0);
	g_player_spawn_index = 0;
	if (GetType(g_player_spawn_positions) == C4V_Array)
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

	// The game starts after a delay to ensure that everyone is ready.
	GUI_Clock->CreateCountdown(3);

	SetSky(g_theme.Sky);
	g_theme->InitializeRound();
	g_theme->InitializeMusic();

	return true;
}

static g_player_spawn_positions, g_map_width, g_player_spawn_index;

global func ScoreboardTeam(int team) { return team * 100; }

func InitializePlayer(int plr)
{
	// Add the player and their team to the scoreboard.
	Scoreboard->NewPlayerEntry(plr);
	Scoreboard->SetPlayerData(plr, "wins", "");
	var team = GetPlayerTeam(plr);
	Scoreboard->NewEntry(ScoreboardTeam(team), GetTeamName(team));
	Scoreboard->SetData(ScoreboardTeam(team), "team", "", ScoreboardTeam(team));
	Scoreboard->SetPlayerData(plr, "team", "", ScoreboardTeam(team) + 1);

	// Players joining at runtime will participate in the following round.
	// Should only happen if it's not game start, else Clonks would start stuck in a RelaunchContainer.
	if (FrameCounter() > 1) PutInRelaunchContainer(GetCrew(plr));
}

func InitializePlayers()
{
	AssignHandicaps();
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		InitPlayerRound(plr);
	}
}

func InitPlayerRound(int plr)
{
	// Unmark death on scoreboard.
	Scoreboard->SetPlayerData(plr, "death", "");
	// everything visible
	SetFoW(false, plr);
	SetPlayerViewLock(plr, true);
	// Player positioning. 
	var ls_wdt = LandscapeWidth(), ls_hgt = LandscapeHeight();
	var crew = GetCrew(plr), start_pos;
	// Position by map type?
	if (SCENPAR_SpawnType == 0)
	{
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
	}
	else // Balloon spawn
	{
		var spawn_x = ls_wdt/3, spawn_y = 10;
		spawn_x += Random(spawn_x);
		var balloon = CreateObject(BalloonDeployed, spawn_x, spawn_y - 16, plr);
		crew->SetPosition(spawn_x, spawn_y);
		balloon->SetRider(crew);
		crew->SetAction("Ride", balloon);
		balloon->SetSpeed(0,0);
		crew->SetSpeed(0,0);
	}
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
			// Start reloading the launcher during the countdown.
			if (!IsHandicapped(plr))
			{
				crew->SetHandItemPos(0, crew->GetItemPos(launcher));
				// This doesn't play the animation properly - simulate a click instead.
				/* crew->StartLoad(launcher); */
				crew->StartUseControl(CON_Use, 0, 0, launcher);
				crew->StopUseControl(0, 0, launcher);
			}
		}
	}
	crew.MaxEnergy = 100000;
	crew->DoEnergy(1000);
	// Disable the Clonk during the countdown.
	crew->SetCrewEnabled(false);
	crew->SetComDir(COMD_Stop);

	if (SCENPAR_SpawnType == 1 && balloon)
		balloon->CreateEffect(IntNoGravity, 1, 1);

	return true;
}

local IntNoGravity = new Effect {
	Timer = func() {
		Target->SetSpeed(0,0);
	}
};

// Called by the round start countdown.
func OnCountdownFinished()
{
	// Re-enable all Clonks.
	for (var clonk in FindObjects(Find_OCF(OCF_CrewMember)))
	{
		clonk->SetCrewEnabled(true);
		SetCursor(clonk->GetOwner(), clonk);
		if (SCENPAR_SpawnType == 1 && clonk->GetActionTarget())
			RemoveEffect("IntNoGravity", clonk->GetActionTarget());
	}
}

func PutInRelaunchContainer(object clonk)
{
	var plr = clonk->GetOwner();
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, plr);
	// We just use the relaunch object as a dumb container.
	clonk->Enter(relaunch);
	// Allow scrolling around the landscape.
	SetPlayerViewLock(plr, false);
}

func OnClonkDeath(object clonk)
{
	var plr = clonk->GetOwner();
	// Mark death on scoreboard.
	Scoreboard->SetPlayerData(plr, "death", "{{Scoreboard_Death}}");
	// Skip eliminated players, NO_OWNER, etc.
	if (GetPlayerName(plr)) 
	{
		var crew = CreateObject(Clonk, 0, 0, plr);
		crew->MakeCrewMember(plr);
		PutInRelaunchContainer(crew);
	}

	// Check for victory after three seconds to allow stalemates.
	if (!g_gameover)
		g_check_victory_effect.Interval = g_check_victory_effect.Time + 36 * 3;
}

// Returns an array of team -> number of players in team.
func GetTeamPlayers()
{
	var result = CreateArray(GetTeamCount() + 1);
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i), team = GetPlayerTeam(plr);
		SetLength(result, Max(team + 1, GetLength(result)));
		result[team] = result[team] ?? [];
		PushBack(result[team], plr);
	}
	return result;
}

static g_handicapped_players;

func _MinSize(int a, array b) { if (b == nil) return a; else return Min(a, GetLength(b)); }

// Assigns handicaps so that the number of not-handicapped players is the same for all teams.
func AssignHandicaps()
{
	g_handicapped_players = CreateArray(GetPlayerCount());
	var teams = GetTeamPlayers();
	var smallest_size = Reduce(teams, Scenario._MinSize, ~(1<<31));
	for (var team in teams) if (team != nil)
	{
		var to_handicap = GetLength(team) - smallest_size;
		while (GetLength(team) > to_handicap)
			RemoveArrayIndexUnstable(team, Random(GetLength(team)));
		for (var plr in team)
		{
			SetLength(g_handicapped_players, Max(plr + 1, GetLength(g_handicapped_players)));
			g_handicapped_players[plr] = true;
		}
	}
}

func IsHandicapped(int plr)
{
	return !!g_handicapped_players[plr];
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
	var msg;
	if (!clonk)
	{
		// Stalemate!
		msg = "$Stalemate$";
		Log(msg);
		GameCall("ResetRound");
	}
	else if (!FindObject(find_living, Find_Hostile(clonk->GetOwner())))
	{
		// We have a winner!
		var team = GetPlayerTeam(clonk->GetOwner());
		PushBack(g_winners, team);
		// Announce the winning team.
		msg = Format("$WinningTeam$", GetTeamPlayerNames(team));
		Log(msg);

		// Update the scoreboard.
		UpdateScoreboardWins(team);

		// The leading team has to win the last round.
		if (--g_remaining_rounds > 0 || GetLeadingTeam() != team)
		{
			var msg2 = CurrentRoundStr();
			Log(msg2);
			msg = Format("%s|%s", msg, msg2);
			GameCall("ResetRound");
		}
		else
		{
			GameCall("EliminateLosers");
		}
	}
	// Switching scenario sections makes the Log() messages hard to see, so announce them using a message as well.
	CustomMessage(msg);
	// Go to sleep again.
	effect.Interval = 0;
	return FX_OK;
}

global func CurrentRoundStr()
{
	if (g_remaining_rounds == 1)
		return "$LastRound$";
	else if (g_remaining_rounds > 1)
		return Format("$RemainingRounds$", g_remaining_rounds);
	else if (GetLeadingTeam() == nil)
		return "$Tiebreak$";
	else
		return "$BonusRound$";
}

global func UpdateScoreboardWins(int team)
{
	var wins = GetTeamWins(team);
	Scoreboard->SetData(ScoreboardTeam(team), "wins", wins, wins);
	// We have to update each player as well to make the sorting work.
	for (var i = 0; i < GetPlayerCount(); i++)
	{
		var plr = GetPlayerByIndex(i);
		if (GetPlayerTeam(plr) == team)
		{
			Scoreboard->SetPlayerData(plr, "wins", "", wins);
		}
	}
}

global func GetTeamWins(int team)
{
	var wins = 0;
	for (var w in g_winners)
		if (w == team)
			wins++;
	return wins;
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
	g_gameover = true;
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

// ============= Themes =============
static const DefaultTheme = new Global
{
	InitializeRound = func() { },
	LavaMat = "^DuroLava",
	IceMats = ["^Ice-ice", "^Ice-ice2"],
	AltMatRatio = 50,
	BackgroundMat = nil,
	Sky = "Default",
	PlayList = nil,
	InitializeMusic = func()
	{
		// No special play list => music by Ambience
		if (this.PlayList == nil)
			InitializeAmbience();
		else
		{
			// Remove Ambience to avoid interference.
			RemoveAll(Find_ID(Ambience));
			SetPlayList(this.PlayList, NO_OWNER, true);
			SetGlobalSoundModifier(nil);
		}
	}
};

static const HotIce = new DefaultTheme
{
	InitializeRound = func() 
	{
		Stalactite->Place(10 + Random(3));
	}
};

static const EciToh = new DefaultTheme
{
	LavaMat = "DuroLava",
	IceMats = ["Coal", "Rock-rock"],
	AltMatRatio = 8,
	BackgroundMat = "Tunnel",
	InitializeRound = func() 
	{
		Stalactite->Place(10 + Random(3));
	}
};

static const MiamiIce = new DefaultTheme
{
	IceMats = ["^BlackIce-black", "^BlackIce-black"],
	Sky = "SkyMiami",
	PlayList =
	{
		PlayList = "beach",
		MusicBreakChance = 0,
	},

	InitializeRound = func()
	{
		// Colors
		Scenario->CreateEffect(MiamiObjects, 1, 1);

		Tree_Coconut->Place(RandomX(7, 13));
	},

	MiamiObjects = new Effect {
		Timer = func(int time)
		{
			for (var o in FindObjects(Find_NoContainer()))
			{
				if (o->GetID() == Tree_Coconut)
					continue;
				o->SetClrModulation(HSL(time, 255, 100));
			}
		},
	}
};
