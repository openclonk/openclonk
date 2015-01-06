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
	InitVegetation();
	InitMaterials();
	InitEnvironment();
	return;
}

protected func InitializePlayer(int plr)
{
	// Get the only clonk of the player.
	var clonk = GetCrew(plr);
	
	// Players start in a random small cave.
	var cave = FindStartCave();
	clonk->SetPosition(cave[0], cave[1]);

	// Players start with a shovel, a pickaxe and two firestones.
	clonk->CreateContents(Shovel);
	clonk->CreateContents(Pickaxe);
	clonk->CreateContents(Torch);
	clonk->CreateContents(Firestone, 2);
	
	// Set the zoom range to be standard low, but allow for zooming out
	// such that light sources a bit further away can be spotted.
	SetPlayerZoomByViewRange(plr, 300, nil, PLRZOOM_Direct);
	SetPlayerZoomByViewRange(plr, 600, nil, PLRZOOM_LimitMax);
	SetPlayerViewLock(plr, true);
	SetFoW(false, plr);
	return;
}

// Finds a start cave which is furthest away from the center and from other already used start caves.
private func FindStartCave()
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
		var dist = Distance(cave[0], cave[1], wdt, hgt);
		for (var comp_cave in used_caves)
			dist = Min(dist, Distance(cave[0], cave[1], comp_cave[0], comp_cave[1]));
		if (dist > max_dist)
		{
			best_index = index;
			max_dist = dist;
		}
	}
	// If no cave has found, spawn in the large cave.
	if (best_index == nil)
		return [wdt, hgt];
	// Move the found cave in front of the separator.
	var found_cave = cave_list[best_index];
	RemoveArrayIndex(cave_list, best_index);
	for (var i = GetLength(cave_list); i >= 1; i--)
		cave_list[i] = cave_list[i - 1];
	cave_list[0] = found_cave;
	// PushFront(cave_list, found_cave);
	// Return the location of the found cave.
	return found_cave;
}

/*-- Scenario Initiliaztion --*/

private func InitVegetation()
{
	// Cave mushrooms provide wood, extra place them in the large caves.
	LargeCaveMushroom->Place(100 + Random(30), nil, { terraform = false });
	
	// Some mushrooms to regain health.
	Mushroom->Place(80);
	Fern->Place(60);
	
	// Some objects in the earth.	
	PlaceObjects(Loam, 50 + Random(30), "Earth");
	PlaceObjects(Firestone, 50 + Random(30), "Earth");
	PlaceObjects(Dynamite, 20 + Random(10), "Earth");
	PlaceObjects(DynamiteBox, 10 + Random(5), "Rock");
	
	// Place some branches and trunks around the map.
	Branch->Place(100);
	
	return;
}

private func InitMaterials()
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	
	// Create lorries at random small caves, but only for half of them.
	for (var index = GetLength(cave_list) - 1; index >= GetLength(cave_list) / 2; index--)
	{
		var cave = cave_list[index];
		var lorry = CreateObject(Lorry, cave[0], cave[1]);
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
		// Both sides of the cave are lighted for all players.
		var torch = CreateObject(Torch, wdt / 2 + side * 50, hgt / 2 + 32);
		torch->AttachToWall(true);
		// Create lorry with useful tools and weapons.
		var lorry = CreateObject(Lorry, wdt / 2 + side * 50, hgt / 2 + 44);
		lorry->CreateContents(Bow, 2);
		lorry->CreateContents(BombArrow, 4);
		lorry->CreateContents(Boompack, 2);
		lorry->CreateContents(PowderKeg, 2);
		lorry->CreateContents(TeleGlove, 1);
		lorry->CreateContents(WindBag, 1);
	}

	return;
}

private func InitEnvironment()
{

	return;
}


