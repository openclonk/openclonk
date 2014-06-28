/**
	Deep Sea Mining
	Mine gems buried deeply below the ocean.
	
	@authors Sven2
*/

static main_island_x, main_island_y; // set in Map.c

protected func Initialize()
{
	// Goal: Sell Gems, amount depends on availability.
	var gem_goal = FindObject(Find_ID(Goal_SellGems));
	if (gem_goal)
	{
		var gems = (GetMaterialCount(Material("Ruby")) + GetMaterialCount(Material("Amethyst"))) / 125;
		var percentage = 60;
		gem_goal->SetTargetAmount((gems * percentage) / 100);
	}
	
	// Initialize different parts of the scenario.
	InitEnvironment();
	InitVegetation();
	InitAnimals();
	InitMainIsland();
		
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
	
	// Claim ownership of unowned structures
	for (var structure in FindObjects(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole")), Find_Owner(NO_OWNER)))
	{
		structure->SetOwner(plr);
		structure->~RefreshOwnershipOfSurrounding();
	}
		
	// Should be done in OnOwnerChanged? It doesn't happen ATM.
	RedrawAllFlagRadiuses();
	

	return;
}

// Initializes environment and disasters.
private func InitEnvironment()
{
	// Water refill from sides
	var initial_water_level = 0;
	while (GetMaterial(0,initial_water_level) != Material("Water")) ++initial_water_level;
	ScheduleCall(nil, this.EnsureWaterLevel, 20, 999999999, initial_water_level);

	// Set a certain parallax.
	SetSkyParallax(0, 20, 20);
	
	// No disasters for now
	//Meteor->SetChance(5); Cloud->SetLightning(16);
	
	return;
}

// Ensures that the sea doesn't disappear
func EnsureWaterLevel(int level, bool no_recursion)
{
	var water_mat = Material("Water");
	if (GetMaterial(0,level) != water_mat) CastPXS("Water", 100, 20, 0,level, 90, 10);
	if (GetMaterial(LandscapeWidth()-1,level) != water_mat) CastPXS("Water", 100, 20, LandscapeWidth()-1,level, 270, 10);
	// Extra insertion at a lower level so it's not easy to block off
	if (!no_recursion && !Random(3)) EnsureWaterLevel(level + 50 + Random(450), true);
	return true;
}

private func InitVegetation()
{
	// Grass on starting island.
	PlaceGrass(85);
	
	// Place some cocont trees all around the main island
	for (var i = 0; i < 10 + Random(8); i++)
		PlaceVegetation(Tree_Coconut, 0, 0, LandscapeWidth(), LandscapeHeight(), 1000 * (61 + Random(40)));
		
	// Create an effect to make sure there will always grow some new trees.
	ScheduleCall(nil, this.EnsureTrees, 100, 999999999);
	
	// Some objects in the earth.	
	PlaceObjects(Rock, 10 + Random(10),"Earth");
	PlaceObjects(Firestone, 35 + Random(5), "Earth");
	PlaceObjects(Loam, 25 + Random(5), "Earth");

	// Underwater vegetation
	Seaweed->Place(20);
	Coral->Place(30);
	
	return;
}

// Ensures that there will always grow some trees on the main island.
func EnsureTrees()
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	// Place a tree if there are less than eight trees, with increasing likelihood for lower amounts of trees.
	var nr_trees = ObjectCount(Find_Func("IsTree"), Find_Func("IsStanding"));
	if (Random(9) >= nr_trees)
		if (!Random(20))
			PlaceVegetation(Tree_Coconut, main_island_x - 300, main_island_y - 200, 600, 400, 3);
	return true;
}

private func InitAnimals()
{
	// Place fish in upper ocean area (there tend to be small basins below, where lots of fish could get stuck)
	var fish_area = GetFishArea();
	Fish->Place(50, fish_area);
	Piranha->Place(25, fish_area);
	ScheduleCall(nil, this.EnsureAnimals, 237, 999999999);
	return true;
}

private func GetFishArea() { return Rectangle(50, main_island_y, LandscapeWidth() - 100, LandscapeHeight()/2 - main_island_y); }

private func EnsureAnimals()
{
	if (ObjectCount(Find_ID(Fish), Find_Not(Find_Action("Dead"))) < 50) Fish->Place(1, GetFishArea());
	if (ObjectCount(Find_ID(Piranha), Find_Not(Find_Action("Dead"))) < 25) Piranha->Place(1, GetFishArea());
	return true;
}

// Initializes the main island according to the material specification.
private func InitMainIsland()
{
	var amount = 3;
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
		lorry->CreateContents(WallKit, 4);
		//lorry->CreateContents(Boompack, 1);	
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

	for (var i = 0; i < 100; i++)
	{
		var x = main_island_x + xpos + Random(sep*2+1)-sep;
		var y = main_island_y / 2 - 220;
		
		while (!GBackSolid(x, y) && y < LandscapeHeight()*3/4)
			y++;	
			
		if (!no_struct || !FindObject(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole")), Find_Distance(60, x, y)))
			break;
	}

	return [x, y];
}
