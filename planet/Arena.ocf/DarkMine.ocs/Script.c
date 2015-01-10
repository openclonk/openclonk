/**
	Dark Mine
	Dark caves with narrow connections set the stage for this battle. Every player
	only has a single clonk and no relaunches, so caution is needed.
	
	@author Maikel
*/


// List for storing the different large caves.
static cave_list;

protected func Initialize()
{
	// Goals and rules.
	CreateObject(Goal_LastManStanding);
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_Gravestones);
	
	// Rescale cave coordinates with map zoom and shuffle them.
	var mapzoom = GetScenarioVal("MapZoom", "Landscape");
	for (var cave in cave_list)
	{
		cave[0] *= mapzoom;
		cave[1] *= mapzoom;	
	}
	ShuffleArray(cave_list);
	// Then add a nil entry at position one as a separator.
	PushFront(cave_list, nil);
	
	// Initialize different parts of the scenario.
	// Amount of things depends on the map size.
	var plr_cnt = GetStartupPlayerCount();
	var map_size = BoundBy(120 + plr_cnt * 10, 140, 240);
	InitVegetation(map_size);
	InitMaterials(map_size);
	InitEnvironment(map_size);
	InitLorries();
	return;
}

// Callback from the last man standing goal.
protected func RelaunchCount() 
{ 
	// Relaunch count depends on scenario setting.
	return BoundBy(SCENPAR_NrRelaunches, 0, 3); 
}

// Callback from the last man standing goal.
protected func KillsToRelaunch() 
{
	// No relaunches awarded for kills.
	return 0; 
}

// Callback from the last man standing goal.
// Takes over the role of initializing the player.
protected func OnPlayerRelaunch(int plr, int relaunch_cnt)
{
	var is_relaunch = relaunch_cnt != RelaunchCount();
	// Get the only clonk of the player.
	var clonk = GetCrew(plr);
	
	// Players start in a random small cave, the cave depends on whether it is a relaunch.
	var cave = FindStartCave(plr, is_relaunch);
	clonk->SetPosition(cave[0], cave[1]);

	// Players start with a shovel, a pickaxe and two firestones.
	clonk->CreateContents(Shovel);
	clonk->CreateContents(Pickaxe);
	clonk->CreateContents(Torch);
	// Better weapons after relaunching.
	if (!is_relaunch)
		clonk->CreateContents(Firestone, 2);
	else
		clonk->CreateContents(Bow)->CreateContents(BombArrow);
	
	// Set the zoom range to be standard low, but allow for zooming out
	// such that light sources a bit further away can be spotted.
	SetPlayerZoomByViewRange(plr, 300, nil, PLRZOOM_Direct);
	SetPlayerZoomByViewRange(plr, 600, nil, PLRZOOM_LimitMax);
	SetPlayerViewLock(plr, true);
	return;
}

// Finds a start cave which is furthest away from the center and from other already used start caves.
private func FindStartCave(int plr, bool is_relaunch)
{
	var wdt = LandscapeWidth() / 2;
	var hgt = LandscapeHeight() / 2;
	// Find the already used caves.
	var used_caves = [];
	var index_av;
	for (index_av = 0; index_av < GetLength(cave_list); index_av++)
	{
		if (cave_list[index_av] == nil)
			break;
		PushBack(used_caves, cave_list[index_av]);
	}
	// Then iterate over all still available caves and find the one furthest away from all other caves.
	var best_index;
	var max_dist = 0;
	for (var index = index_av + 1; index < GetLength(cave_list); index++)
	{
		var cave = cave_list[index];
		// Also furthest away from center cave.
		var dist = Distance(cave[0], cave[1], wdt, hgt);
		// For a normal cave take distance to used caves, for a relaunch to alive clonks.
		if (!is_relaunch)
		{
			for (var comp_cave in used_caves)
				dist = Min(dist, Distance(cave[0], cave[1], comp_cave[0], comp_cave[1]));
		}
		else
		{
			for (var clonk in FindObjects(Find_OCF(OCF_CrewMember), Find_Not(Find_Owner(plr))))
				dist = Min(dist, Distance(cave[0], cave[1], clonk->GetX(), clonk->GetY()));
		}
		if (dist > max_dist)
		{
			best_index = index;
			max_dist = dist;
		}
	}
	// If no cave has found, spawn in the large cave.
	if (best_index == nil)
		return [wdt, hgt];
	// Determine found cave and move it in front of the separator if it is not a relaunch.
	var found_cave = cave_list[best_index];
	if (!is_relaunch)
	{
		RemoveArrayIndex(cave_list, best_index);
		PushFront(cave_list, found_cave);
	}
	// Return the location of the found cave.
	return found_cave;
}


/*-- Scenario Initiliaztion --*/

private func InitVegetation(int map_size)
{
	// Place some cave mushrooms for cover.
	LargeCaveMushroom->Place(map_size - 20, nil, { terraform = false });
	
	// Some mushrooms to regain health.
	Mushroom->Place(map_size / 2);
	Fern->Place(map_size / 3);
	
	// Place some branches and trunks around the map.
	Branch->Place(map_size / 2);
	Trunk->Place(map_size / 4, nil, { size = [60, 80] });
	return;
}

private func InitMaterials(int map_size)
{
	// Some objects in the earth or rock material.	
	PlaceObjects(Loam, map_size / 3, "Earth");
	PlaceObjects(Firestone, map_size / 3, "Earth");
	PlaceObjects(Dynamite, map_size / 5, "Earth");
	PlaceObjects(DynamiteBox, map_size / 6, "Rock");
	PlaceObjects(PowderKeg, map_size / 10, "Rock");
	
	// Some pickaxes, shovels in the tunnels.
	for (var i = 0; i < map_size / 6; i++)
	{
		var loc = FindLocation(Loc_Tunnel(), Loc_Wall(CNAT_Bottom));
		if (!loc)
			continue;
		CreateObjectAbove([Shovel, Pickaxe][Random(2)], loc.x, loc.y)->SetR(Random(360));	
	}
	return;
}

private func InitEnvironment(int map_size)
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	// Some lights in the main cave as a strategic element.
	for (var side = -1; side <= 1; side += 2)
	{
		// Both sides of the cave are lighted for all players.
		var torch = CreateObjectAbove(Torch, wdt / 2 + side * 50, hgt / 2 + 32);
		torch->AttachToWall(true);
	}
	return;
}

private func InitLorries()
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	// Create lorries at random small caves, but only for half of them.
	for (var index = GetLength(cave_list) - 1; index >= GetLength(cave_list) / 2; index--)
	{
		var cave = cave_list[index];
		var lorry = CreateObjectAbove(Lorry, cave[0], cave[1]);
		// Basic objects which are in every lorry.
		lorry->CreateContents(Dynamite, RandomX(2, 4));
		lorry->CreateContents(Loam, RandomX(2, 4));
		// Objects which are only in half of the lorries.
		if (!Random(2)) 
			lorry->CreateContents(DynamiteBox, RandomX(1, 2));
		if (!Random(2)) 
			lorry->CreateContents(Shield);
		if (!Random(2)) 
			lorry->CreateContents(Torch, 2);
		// Objects which are only in one third of the lorries.
		if (!Random(3)) 
			lorry->CreateContents(GrappleBow, RandomX(1, 2));
		if (!Random(3)) 
			lorry->CreateContents(Bread, RandomX(1, 2));
		if (!Random(3))
		{
			lorry->CreateContents(Bow);
			lorry->CreateContents([Arrow, FireArrow, FireArrow][Random(3)], 2);
		}
		// Objects which are only in one fifth of the lorries.
		if (!Random(5)) 
			lorry->CreateContents(Javelin, RandomX(1, 2));
		if (!Random(5)) 
			lorry->CreateContents(Club, RandomX(1, 2));
		if (!Random(5)) 
		{
			var barrel = lorry->CreateContents(Barrel);
			barrel->SetFilled("Water", Barrel->BarrelMaxFillLevel());
		}
		// Objects which are only in one eighth of the lorries.
		if (!Random(8))
			lorry->CreateContents(IronBomb, RandomX(1, 2));
		if (!Random(8))
			lorry->CreateContents(WallKit, 1);
		if (!Random(8))
		{
			lorry->CreateContents(Musket);
			lorry->CreateContents(LeadShot);
		}
	}
	// Create two lorries at the main cave.
	for (var side = -1; side <= 1; side += 2)
	{
		// Create lorry with useful tools and weapons.
		var lorry = CreateObjectAbove(Lorry, wdt / 2 + side * 50, hgt / 2 + 44);
		lorry->CreateContents(Bow, 2);
		lorry->CreateContents(BombArrow, 4);
		lorry->CreateContents(Boompack, 2);
		lorry->CreateContents(PowderKeg, 2);
		lorry->CreateContents(TeleGlove, 1);
		lorry->CreateContents(WindBag, 1);
		lorry->CreateContents(GrenadeLauncher);
		lorry->CreateContents(IronBomb, 4);
	}
	return;
}
