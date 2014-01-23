/**
	Gem Grabbers
	Players need to grab gems from non trivial locations in a landscape with many sky islands.
	There are many tools either available directly or for production on the other islands. 
	
	Todo's:
	* Rain and wind effects.
	* Attacking birds on the islands with new buildings.
	* Some other hurdles.
	* Decoration on all the islands.
	* Flatten the main islands surface a bit in Map.c
		
	@authors Maikel
*/


// Scenario properties which can be set later by the lobby options.
static const SCENOPT_Material = 3; // Amount of material available from start.
static const SCENOPT_MapSize = 1; // Size of the map.
static const SCENOPT_Difficulty = 2; // Difficulty settings.

protected func Initialize()
{
	// Rules: team account and buying at flagpole.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	
	// Goal: Sell Gems, amount depends on difficulty and availability.
	var gems = (GetMaterialCount(Material("Ruby")) + GetMaterialCount(Material("Amethyst"))) / 125;
	var percentage = 55 + 15 * SCENOPT_Difficulty;
	var goal = CreateObject(Goal_SellGems);
	goal->SetTargetAmount((gems * percentage) / 100);
	
	// Initialize different parts of the scenario.
	InitEnvironment();
	InitVegetation();
	InitAnimals();
	InitMainIsland(SCENOPT_Material);
	InitIslands(SCENOPT_Material);
		
	return;
}

protected func InitializePlayer(int plr)
{
	// Harsh zoom range
	SetPlayerZoomByViewRange(plr, 500, 350, PLRZOOM_LimitMax);
	SetPlayerZoomByViewRange(plr, 500, 350, PLRZOOM_Direct);
	SetPlayerViewLock(plr, true);
	
	// Position and materials
	var i, crew;
	for (i = 0; crew = GetCrew(plr, i); ++i)
	{
		var pos = FindMainIslandPosition();
		crew->SetPosition(pos[0], pos[1] - 11);
		crew->CreateContents(Shovel);
	}
	
	// Give the player its knowledge.
	GivePlayerKnowledge(plr);
	
	// Only clonks for sale at the homebase.
	DoHomebaseMaterial(plr, Clonk, Max(4 - SCENOPT_Difficulty, 1));
	
	// Claim ownership of structures, last player who joins owns all the main island flags.
	for (var structure in FindObjects(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole"))))
		structure->SetOwner(plr);

	return;
}

// Give the relevant knowledge to each player.
private func GivePlayerKnowledge(int plr)
{
	var structures = [Flagpole, WindGenerator, SteamEngine, Compensator, Foundry, Sawmill, Elevator, Pump, ToolsWorkshop, ChemicalLab, Armory, Chest, Windmill, Kitchen];
	var items = [Loam, GoldBar, Metal, Shovel, Axe, Hammer, Pickaxe, Barrel, Bucket, Dynamite, DynamiteBox, PowderKeg, Pipe, TeleGlove, WindBag, GrappleBow, Boompack, Balloon];
	var weapons = [Bow, Arrow, Club, Sword, Javelin, Shield, Musket, LeadShot, IronBomb, GrenadeLauncher];
	var vehicles = [Lorry, Catapult, Cannon, Airship, Plane];
	for (var structure in structures)
		SetPlrKnowledge(plr, structure);
	for (var item in items)
		SetPlrKnowledge(plr, item);
	for (var weapon in weapons)
		SetPlrKnowledge(plr, weapon);	
	for (var vehicle in vehicles)
		SetPlrKnowledge(plr, vehicle);
	return;
}

// Initializes environment and disasters.
private func InitEnvironment()
{
	// Set time to almost night and have stars.	
	CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(20*60 + 15);
	time->SetCycleSpeed(0);
	
	// Clouds and rain.
	Cloud->Place(15);
	Cloud->SetPrecipitation("Water", 100 + 25 * SCENOPT_Difficulty);
	for (var cloud in FindObjects(Find_ID(Cloud)))
	{
		while (cloud->RemoveVertex())
			/* Empty */;
		// Make some clouds appear in the foreground with high alpha.
		if (Random(3) == 0)
		{
			cloud.Plane = 600;
			cloud->SetCloudAlpha(40);
		}			
	}
	
	// Set a certain parallax.
	SetSkyParallax(0, 20, 20);
	
	// Disasters: meteors and lightning.
	Meteor->SetChance(2 * SCENOPT_Difficulty);
	Cloud->SetLightning(8 * SCENOPT_Difficulty);
	
	return;
}

private func InitVegetation()
{
	// Grass on all islands.
	PlaceGrass(85);
	
	// Place some cocont trees all around the island and some extra trees on the main island.
	for (var i = 0; i < 40 + Random(8); i++)
		PlaceVegetation(Tree_Coconut, 0, 0, LandscapeWidth(), LandscapeHeight(), 1000 * (61 + Random(40)));
	for (var i = 0; i < 6 + Random(2); i++)
		PlaceVegetation(Tree_Coconut, LandscapeWidth() - 300, LandscapeHeight() - 200, 600, 400, 1000 * (71 + Random(30)));
		
	// Create an effect to make sure there will always grow some new trees.	
	AddEffect("EnsureTreesOnMainIsland", nil, 100, 20, nil);
	
	// Some objects in the earth.	
	PlaceObjects(Rock, 35 + Random(10),"Earth");
	PlaceObjects(Firestone, 25 + Random(5), "Earth");
	
	return;
}

// Ensures that there will always grow some trees on the main island.
global func FxEnsureTreesOnMainIslandTimer()
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	// Place a tree if there are less than eight trees, with increasing likelihood for lower amounts of trees.
	var nr_trees = ObjectCount(Find_Func("IsTree"), Find_InRect(wdt / 2 - 300, hgt / 2 - 200, 600, 400));
	if (Random(9) >= nr_trees)
		if (!Random(20))
			PlaceVegetation(Tree_Coconut, wdt / 2 - 300, hgt / 2 - 200, 600, 400, 3);
	return FX_OK;
}

private func InitAnimals()
{


	return;
}

// Initializes the main island according to the material specification.
private func InitMainIsland(int amount)
{
	amount = BoundBy(amount, 1, 3);
	var pos;
	
	// Always start with a lorry filled with: hammer(x2), axe(x2), wood(x6) and metal(x4).
	var lorry_pos = FindMainIslandPosition(0, 80);
	var lorry = CreateObject(Lorry, lorry_pos[0], lorry_pos[1] - 8);
	lorry->CreateContents(Hammer, 2);
	lorry->CreateContents(Axe, 2);
	lorry->CreateContents(Wood, 6);
	lorry->CreateContents(Metal, 4);
	
	// If more material is specified, create a small settlement: flag(x2) and windmill.
	// Also fill lorry a bit more with: pickaxe(x1), dynamite(x4), wood(x4), metal(x2).
	if (amount >= 2)
	{
		pos = FindMainIslandPosition(-120, 20);
		CreateObject(Flagpole, pos[0], pos[1]);
		pos = FindMainIslandPosition(120, 20);
		CreateObject(Flagpole, pos[0], pos[1]);
		pos = FindMainIslandPosition(nil, nil, true);
		CreateObject(WindGenerator, pos[0], pos[1]);
		lorry->CreateContents(Wood, 4);
		lorry->CreateContents(Metal, 2);
		lorry->CreateContents(Pickaxe, 1);
		lorry->CreateContents(Dynamite, 4);
	}
	
	// If still more material is specified, create a larger settlement: sawmill, chemical lab, tools workshop.
	// Also fill lorry a bit more with: Barrel (x1), Bucket(x1), Loam(x4), DynamiteBox(x2).
	if (amount >= 3)
	{
		pos = FindMainIslandPosition(nil, nil, true);
		CreateObject(Sawmill, pos[0], pos[1]);
		pos = FindMainIslandPosition(nil, nil, true);
		CreateObject(ChemicalLab, pos[0], pos[1]);
		pos = FindMainIslandPosition(nil, nil, true);
		CreateObject(ToolsWorkshop, pos[0], pos[1]);
	
		lorry->CreateContents(Barrel, 1);
		lorry->CreateContents(Bucket, 1);
		lorry->CreateContents(Loam, 4);
		lorry->CreateContents(DynamiteBox, 1);
		lorry->CreateContents(Boompack, 1);	
	}
	
	return;
}

// Tries to find a position on the main island.
private func FindMainIslandPosition(int xpos, int sep, bool no_struct)
{
	if (xpos == nil)
		xpos = 0;
	if (sep == nil) 
		sep = 250;
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();

	for (var i = 0; i < 100; i++)
	{
		var x = RandomX(wdt / 2 + xpos - sep, wdt / 2 + xpos + sep);
		var y = hgt / 2 - 220;
		
		while (!GBackSolid(x, y) && y < 3 * hgt / 4)
			y++;	
			
		if (!no_struct || !FindObject(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole")), Find_Distance(60, x, y)))
			break;
	}

	return [x, y];
}

// Initializes the smaller islands according to the material specification.
private func InitIslands(int amount)
{
	amount = BoundBy(amount, 1, 3);
	
	// Locate all islands and store them in an array.
	var islands = FindIslands();

	// Add some structures, materials and vegetation to each island depending on material specification.
	var island_nr = 1;
	for (var island in islands)
		island_nr += ProvideIsland(island, island_nr, amount);

	return;
}

// Returns an array with rectangles enclosing each of the islands.
private func FindIslands()
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	var main = [wdt / 2 - 500, hgt / 2 - 225, 1000, 600];
	var spot = FindLocation(Loc_Solid(), Loc_Not(Loc_InRect(main[0], main[1], main[2], main[3])));
	var islands = []; var island_cond;
	// Search for islands iteratively excluding the rectangle of the last found island.
	do 
	{
		var island = MakeIslandRect(spot.x, spot.y);
		islands[GetLength(islands)] = island;
		island_cond = Loc_Not(Loc_InRect(island[0], island[1], island[2], island[3]));
		for (var i = 0; i < GetLength(islands) - 1; i++)
		{
			var island = islands[i];
			island_cond = Loc_And(island_cond, Loc_Not(Loc_InRect(island[0], island[1], island[2], island[3])));
		}		
	
	}
	while (spot = FindLocation(Loc_Solid(), Loc_Not(Loc_InRect(main[0], main[1], main[2], main[3])), island_cond));
	
	return islands;
}

private func MakeIslandRect(int x, int y)
{
	var x1 = x, x2 = x;
	var y1 = y, y2 = y;
	var lwdt = LandscapeWidth();
	var lhgt = LandscapeHeight();
	
	// find some random spots until one finds the boundaries.
	for (var i = 0; i < 200; i++)
	{
		var spot = FindLocation(Loc_Solid(), Loc_InRect(Max(0, x - 60), Max(0, y - 60), Min(120, lwdt - x + 60), Min(120, lhgt - y + 60)));
		if (!spot)
			continue;
		x = spot.x; y = spot.y;
		if (x < x1) x1 = x;
		if (x > x2) x2 = x;
		if (y < y1) y1 = y;
		if (y > y2) y2 = y;		
	}
	
	// Make the rectangle somewhat bigger.
	var x1 = Max(0, x1 - 100);
	var y1 = Max(0, y1 - 100);
	var wdt = x2 - x1 + 200; wdt = Min(wdt, lwdt - x1);
	var hgt = y2 - y1 + 200; hgt = Min(hgt, lhgt - y1);
		
	var rect = [x1, y1, wdt, hgt];
	return rect;
}

// Provides some stuff for each of the 6-9 other islands, dependent on the material settings.
private func ProvideIsland(array island, int number, int amount)
{
	var spot = FindLocation(Loc_InRect(island[0], island[1], island[2], island[3] / 2), Loc_Wall(CNAT_Bottom), Loc_Space(20), Loc_Sky());
	if (!spot)
		return 0;
	
	// An inventors lab without power supply but with some rockets for the first island.
	if (number == 1)
	{
		var lab = CreateObject(InventorsLab, spot.x, spot.y);
		lab->MakeInvincible();
		lab->CreateContents(Boompack, amount);
	}
	
	// A shipyard without materials and power supply for the second island.
	if (number == 2)
	{
		var shipyard = CreateObject(Shipyard, spot.x, spot.y);
		shipyard->CreateContents(Wood, 4);
		shipyard->CreateContents(Metal, 2);
		shipyard->MakeInvincible();
	}
	
	// A cannon with a powder keg for the third island.
	if (number == 3)
	{
		var cannon = CreateObject(Cannon, spot.x, spot.y);
		cannon->CreateContents(PowderKeg);
	}
	
	// A catapult for the fourth island.
	if (number == 4)
	{
		SproutBerryBush->Place(Random(amount + 1), Rectangle(island[0], island[1] - 80, island[2], island[3] / 2));
		CreateObject(Catapult, spot.x, spot.y);
	}
	
	// A lorry with explosives, gold bars and loam.
	if (number == 5)
	{
		var lorry = CreateObject(Lorry, spot.x, spot.y);
		lorry->CreateContents(Loam, 2 + amount);
		lorry->CreateContents(DynamiteBox, amount);
		lorry->CreateContents(Dynamite, 4);
		lorry->CreateContents(GoldBar, Random(amount + 1));
	}
	
	// For the other islands a chest with a few grapplers, loam, gold bars and dynamite.
	if (number >= 6)
	{
		var chest = CreateObject(Chest, spot.x, spot.y);
		if (!Random(2))
			chest->CreateContents(GrappleBow, Random(amount + 1));
		else
			chest->CreateContents(Balloon, Random(amount + 1));
		chest->CreateContents(Loam, Random(amount + 1));
		chest->CreateContents(GoldBar, Random(amount + 1));
		chest->CreateContents(Dynamite, 2 + Random(amount + 1));
		SproutBerryBush->Place(amount + Random(2), Rectangle(island[0], island[1] - 80, island[2], island[3] / 2));
	}
	
	// For all the islands some decoration.
	if (!Random(3))
	{
		var spot = FindLocation(Loc_InRect(island[0], island[1], island[2], island[3] / 2), Loc_Wall(CNAT_Bottom), Loc_Space(20), Loc_Sky());
		if (spot)
			CreateObject(Column, spot.x, spot.y);
	}	

	return 1;
}

/*-- Some helper functions --*/

global func TestGemCount()
{
	var pos;
	while (pos = FindLocation(Loc_Or(Loc_Material("Ruby"), Loc_Material("Amethyst"))))
	{
		var pos = CreateObject(Rock, pos.x, pos.y)->Explode(100);
	}
	var gem_count = ObjectCount(Find_Or(Find_ID(Ruby), Find_ID(Amethyst)));
	return gem_count;
}
