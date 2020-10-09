/**
	Acid Rift
	Mine the rubies before it's too late
	
	@author Sven2
*/


// Whether the intro has been initialized.
static intro_init;

// Set in Map.c
//static g_start_map_x, g_start_map_y;

static g_start_x, g_start_y;

protected func Initialize()
{
	// Show wealth in HUD.
	GUI_Controller->ShowWealth();
	
	// Goal: Ruby mine
	var goal = CreateObject(Goal_SellGems);
	goal->SetTargetAmount(BoundBy(SCENPAR_Difficulty*10, 10, 20));
	// Rules: team account and buying at flagpole.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	
	// Start position from map generation
	var map_zoom = 6;
	g_start_x = g_start_map_x * map_zoom;
	g_start_y = g_start_map_y * map_zoom;
	
	// Initialize different parts of the scenario.
	InitEnvironment(SCENPAR_MapSize, SCENPAR_Difficulty);
	InitVegetation(SCENPAR_MapSize, SCENPAR_Difficulty);
	InitAnimals(SCENPAR_Difficulty);
	InitMaterial(4 - SCENPAR_Difficulty);	
	return;
}

protected func OnGoalsFulfilled()
{
	// Give the remaining players their achievement.
	GainScenarioAchievement("Done", BoundBy(SCENPAR_Difficulty, 1, 3));
	return false;
}


/*-- Player Initialization --*/

protected func InitializePlayer(int plr)
{ 
	// Harsh zoom range.
	SetPlayerZoomByViewRange(plr, 500, nil, PLRZOOM_Direct | PLRZOOM_LimitMax);
	SetPlayerViewLock(plr, true);

	// Move clonks to location and give them a shovel.
	var index = 0, crew;
	while (crew = GetCrew(plr, index))
	{
		crew->SetPosition(g_start_x + RandomX(-12, 12), g_start_y);

		// First clonk can construct, others can chop.
		if (index == 0)
		{
			crew->CreateContents(Shovel);
			crew->CreateContents(Pickaxe);
		}
		else
		{
			crew->CreateContents(Shovel);
			crew->CreateContents(Axe);
			crew->CreateContents(Hammer);
		}
		index++;
	}
	
	// Give the player basic knowledge.
	GivePlayerBasicKnowledge(plr);
	GivePlayerSpecificKnowledge(plr, [InventorsLab, Ropeladder, MetalBarrel, PowderKeg, GrappleBow, WallKit, Pipe, Pump, TeleGlove, WindBag]);
	
	// Give the player the elementary base materials and some tools.
	GivePlayerElementaryBaseMaterial(plr);
	GivePlayerToolsBaseMaterial(plr);
	SetBaseMaterial(plr, Wood, 100);
	SetBaseProduction(plr, Wood, 10);
	SetBaseMaterial(plr, Cloth, 10);
	SetBaseProduction(plr, Cloth, 5);
	
	// Ensure mimimum player wealth.
	var add_wealth = Max(0, 75 - 25 * SCENPAR_Difficulty - GetWealth(plr));
	DoWealth(plr, add_wealth);
	
	// Initialize the intro sequence if not yet started.
	if (!intro_init)
	{
		//StartSequence("Intro", 0);
		intro_init = true;
	}
	return;
}


/*-- Scenario Initialization --*/

private func InitEnvironment(int map_size, int difficulty)
{
	// Adjust the sky a bit.
	SetSkyParallax(0, 20, 20);
	SetSkyAdjust(RGBa(225, 255, 205, 191), RGB(63, 200, 0));
	
	var map_size_factor = [90, 120, 140][map_size-1];

	// Disasters
	Meteor->SetChance((difficulty * 11) * map_size_factor / 120);
	if (difficulty >= 2) Rockfall->SetChance((difficulty * 50 - 80) * map_size_factor / 120);
	Rockfall->SetArea(Shape->Rectangle(200, 0, LandscapeWidth() - 400, 1));
	if (difficulty >= 2) Rockfall->SetExplosiveness(BoundBy(difficulty * 25, 50, 60));
	
	// Acid rain!
	Cloud->Place(BoundBy(40 * difficulty - 30, 10, 70) * map_size_factor / 120);
	Cloud->SetPrecipitation("Acid", 100);
	
	return;
}

private func InitVegetation(int map_size, int difficulty)
{
	// Define parts of the map for even distribution.
	var top = Shape->Rectangle(0, 0, LandscapeWidth(), LandscapeHeight() / 3);
	var middle = Shape->Rectangle(0, LandscapeHeight() / 3, LandscapeWidth(), LandscapeHeight() / 3);
	var bottom = Shape->Rectangle(0, 2 * LandscapeHeight() / 3, LandscapeWidth(), LandscapeHeight() / 3);
	
	// Place some cave mushrooms for wood.
	LargeCaveMushroom->Place(8, middle, { terraform = false });
	LargeCaveMushroom->Place(8, bottom, { terraform = false });
		
	// Place some bushes, ferns and mushrooms.
	SproutBerryBush->Place(2, top);
	SproutBerryBush->Place(2, middle);
	SproutBerryBush->Place(2, bottom);
	Fern->Place(20, top);
	Fern->Place(20, middle);
	Fern->Place(20, bottom);
	Mushroom->Place(10, top);
	Mushroom->Place(10, middle);
	Mushroom->Place(10, bottom);
	
	// Some branches and trunks.
	Branch->Place(24 + Random(12));
	Trunk->Place(12 + Random(8));
	
	// Some objects in the earth.	
	PlaceObjects(Rock, 25 + 10 * map_size + Random(10),"Earth");
	PlaceObjects(Firestone, 20 + 10 * map_size + Random(5), "Earth");
	PlaceObjects(Loam, (5 + 2 * map_size) * (4 - difficulty) + Random(5), "Earth");
	return;
}

private func InitAnimals(int difficulty)
{
	// Place some fish or piranhas on the basin.
	var fish = Fish;
	if (difficulty >= 3)
		fish = Piranha;
	fish->Place(4);	
	return;
}

private func InitMaterial(int amount)
{
	// Always material for a starting flagpole
	var lorry = CreateObjectAbove(Lorry, g_start_x + RandomX(-12, 12), g_start_y);
	lorry->CreateContents(Wood, 3);
	lorry->CreateContents(Metal, 1);
	
	// For medium amount of materials provide a lorry with resources.	
	if (amount >= 2)
	{
		lorry->CreateContents(Wood, 4);
		lorry->CreateContents(Metal, 3);
		lorry->CreateContents(Rock, 4);
		lorry->CreateContents(Dynamite, 4);
		lorry->CreateContents(Loam, 4);
		
		// For large amount of materials provide some buildings as well.
		if (amount >= 3)
		{
			lorry->CreateContents(Wood, 6);
			lorry->CreateContents(Metal, 4);
			lorry->CreateContents(Rock, 4);
			lorry->CreateContents(DynamiteBox, 4);
			lorry->CreateContents(Ropeladder, 4);	
		}		
	}
	return;
}
