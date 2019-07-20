/* Hot ice */

func InitializeRound() // called by Goal_MultiRoundMelee
{
	g_player_spawn_index = 0;
	if (GetType(g_player_spawn_positions) == C4V_Array)
		ShuffleArray(g_player_spawn_positions);

	// Materials: Chests
	var i, pos;
	var ls_wdt = LandscapeWidth(), ls_hgt = LandscapeHeight();
	var chest_area_y = ls_hgt*[0, 30][SCENPAR_MapType]/100;
	var chest_area_hgt = ls_hgt/2;
	// Chests in regular mode. Boom packs in grenade launcher mode.
	var num_extras = [6, 12][SCENPAR_Weapons];
	for (i = 0; i<num_extras; ++i)
	{
		var pos = FindLocation(Loc_InRect(0, chest_area_y, ls_wdt, chest_area_hgt-100), Loc_Wall(CNAT_Bottom)); // Loc_Wall adds us 100 pixels...
		if (pos)
		{
			if (SCENPAR_Weapons == 0)
			{
				var chest = CreateObjectAbove(Chest, pos.x, pos.y);
				if (chest)
				{
					chest->CreateContents(Firestone, 5);
					chest->CreateContents(Bread, 1);
					chest->CreateContents(Bow, 1);
					chest->CreateContents(FireArrow, 1)->SetStackCount(5);
					chest->CreateContents(BombArrow, 1)->SetStackCount(5);
					chest->CreateContents(Shield, 1);
					chest->CreateContents(IronBomb, 3);
				}
			}
			else
			{
				var boompack = CreateObjectAbove(Boompack, pos.x, pos.y);
			}
		}
	}
	// Materials: Firestones
	for (i = 0; i<30; ++i)
	{
		var pos = FindLocation(Loc_InRect(0, chest_area_y, ls_wdt, chest_area_hgt), Loc_Solid());
		if (pos)
			if (IsFirestoneSpot(pos.x, pos.y))
				CreateObjectAbove(Firestone, pos.x, pos.y-1);
	}
	// Some firestones and bombs in lower half. For ap type 1, more firestones in lower than upper half.
	for (i = 0; i<30; ++i)
	{
		var pos = FindLocation(Loc_InRect(0, ls_hgt/2, ls_wdt, ls_hgt/3), Loc_Solid());
		if (pos)
			if (IsFirestoneSpot(pos.x, pos.y))
				CreateObjectAbove([Firestone, IronBomb][Random(Random(3))],pos.x, pos.y-1);
	}

	SetSky(g_theme.Sky);
	g_theme->InitializeRound();
	g_theme->InitializeMusic();
}

static g_player_spawn_positions, g_map_width, g_player_spawn_index;

func InitPlayerRound(int plr, object crew) // called by Goal_MultiRoundMelee
{
	// everything visible
	SetFoW(false, plr);
	// Player positioning. 
	var ls_wdt = LandscapeWidth(), ls_hgt = LandscapeHeight();
	var start_pos;
	// Position by map type?
	if (SCENPAR_SpawnType == 0)
	{
		if (g_player_spawn_positions && g_player_spawn_index < GetLength(g_player_spawn_positions))
		{
			start_pos = g_player_spawn_positions[g_player_spawn_index++];
			var map_zoom = ls_wdt / g_map_width;
			start_pos = {x = start_pos[0]*map_zoom + map_zoom/2, y = start_pos[1]*map_zoom};
		}
		else
		{
			// Start positions not defined or exhausted: Spawn in lower area for both maps becuase starting high is an an advantage.
			start_pos = FindLocation(Loc_InRect(ls_wdt/5, ls_hgt/2, ls_wdt*3/5, ls_hgt/3), Loc_Wall(CNAT_Bottom), Loc_Func(Scenario.IsStartSpot));
			if (!start_pos) start_pos = FindLocation(Loc_InRect(ls_wdt/10, 0, ls_wdt*8/10, ls_hgt*4/5), Loc_Wall(CNAT_Bottom), Loc_Func(Scenario.IsStartSpot));
			if (!start_pos) start_pos = {x = Random(ls_wdt*6/10)+ls_wdt*2/10, y = ls_hgt*58/100};
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
		balloon->SetSpeed(0, 0);
		crew->SetSpeed(0, 0);
		balloon->CreateEffect(IntNoGravity, 1, 1);
	}
	// initial material
	if (SCENPAR_Weapons == 0)
	{
		crew->CreateContents(Shovel);
		crew->CreateContents(Club);
		crew->CreateContents(WindBag);
		crew->CreateContents(Firestone, 2);
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
			if (!Goal_MultiRoundMelee->IsHandicapped(plr))
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
}

func StartRound() // called by Goal_MultiRoundMelee
{
	for (var clonk in FindObjects(Find_OCF(OCF_CrewMember)))
		if (SCENPAR_SpawnType == 1 && clonk->GetActionTarget())
			RemoveEffect("IntNoGravity", clonk->GetActionTarget());
}

local IntNoGravity = new Effect {
	Timer = func() {
		Target->SetSpeed(0, 0);
	}
};


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
	if (!GBackSolid(x-3, y + 2)) return false;
	if (!GBackSolid(x + 3, y + 2)) return false;
	// Spawn with some space.
	return PathFree(x-5, y, x + 5, y) && PathFree(x, y-21, x, y-1);
}

func IsFirestoneSpot(int x, int y)
{
// Very thorough ice surrounding check so they don't explode right away or when the first layer of ice melts
	return GBackSolid(x, y-1) && GBackSolid(x, y + 4) && GBackSolid(x-2, y) && GBackSolid(x + 2, y);
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
