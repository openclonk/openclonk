/* Hot ice */

func Initialize()
{
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
