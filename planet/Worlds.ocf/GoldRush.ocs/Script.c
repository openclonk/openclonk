/**
	Gold Rush
	A simple landscape with some gold, the goal is to mine some gold to gain wealth.
	
	@author Maikel
*/


// Scenario properties which can be set later by the lobby options.
static const SCENOPT_Material = 2; // Amount of material available from start.
static const SCENOPT_MapSize = 1; // Size of the map.
static const SCENOPT_Difficulty = 1; // Difficulty settings.

// Whether the intro has been initialized.
static intro_init;

protected func Initialize()
{
	// Rules: team account and buying at flagpole.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	
	// Goal: Gain Wealth, amount depends on difficulty, though bounded by availability.
	var gold = GetMaterialCount(Material("Gold")) / GetMaterialVal("Blast2ObjectRatio", "Material", Material("Gold"));
	var percentage = 70 + 10 * SCENOPT_Difficulty;
	var wealth_goal = Min(200 + 200 * SCENOPT_Difficulty, gold * 5 * percentage / 100);
	var goal = CreateObject(Goal_Wealth);
	goal->SetWealthGoal(wealth_goal);
	
	// Second goal: Construct golden statue, amount depends on difficulty.
	var statue_cnt = SCENOPT_Difficulty;
	goal = CreateObject(Goal_Construction);
	goal->AddConstruction(Idol, statue_cnt);
	
	// Initialize different parts of the scenario.
	InitEnvironment();
	InitVegetation();
	InitAnimals();
	InitMaterial(SCENOPT_Material);
	
	return;
}


/*-- Player Initialization --*/

protected func InitializePlayer(int plr)
{ 
	// Move clonks to location and give them a shovel.
	var index = 0, crew;
	while (crew = GetCrew(plr, index))
	{
		var x = 80 + Random(40);
		crew->SetPosition(x, FindHeight(x) - 20);
		crew->CreateContents(Shovel);
		// First clonk can construct, others can chop.
		if (index == 0)
			crew->CreateContents(Hammer);
		else
			crew->CreateContents(Axe);
		index++;
	}
	
	// Harsh zoom range.
	SetPlayerZoomByViewRange(plr, 500, nil, PLRZOOM_Direct | PLRZOOM_LimitMax);
	SetPlayerViewLock(plr, true);
	
	// Give the player basic knowledge.
	GivePlayerBasicKnowledge(plr);
	GivePlayerSpecificKnowledge(plr, [Idol]);
	
	// Give the player the elementary base materials and some tools.
	GivePlayerElementaryBaseMaterial(plr);
	GivePlayerToolsBaseMaterial(plr);
	
	// Initialize the intro sequence if not yet started.
	if (!intro_init)
	{
		StartSequence("Intro", 0);
		intro_init = true;
	}
	return;
}


/*-- Scenario Initialization --*/

private func InitEnvironment()
{
	SetSkyParallax(0, 20, 20);
	
	CreateEnvironmentObjects("Temperate");
	
	// Set time of day to evening and create some clouds and celestials.
	Cloud->Place(10);
	Cloud->SetPrecipitation("Water", 8);
	CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(60 * 12);
	time->SetCycleSpeed(20);
	return;
}

private func InitVegetation()
{
	// Place some trees in a forest shape.
	PlaceForest([Tree_Coniferous], 0, LandscapeHeight() / 2 + 50, nil, true);

	SproutBerryBush->Place();
	PlaceGrass(100);
	
	// Some objects in the earth.	
	PlaceObjects(Rock, 25 + 10 * SCENOPT_MapSize + Random(10),"Earth");
	PlaceObjects(Firestone, 20 + 10 * SCENOPT_MapSize + Random(5), "Earth");
	PlaceObjects(Loam, 20 + 10 * SCENOPT_MapSize + Random(5), "Earth");

	return;
}

private func InitAnimals()
{
	// Some butterflies as atmosphere.
	for (var i = 0; i < 10 + 5 * SCENOPT_MapSize; i++)
		PlaceAnimal(Butterfly);
	return;
}

private func InitMaterial(int amount)
{
	// No extra materials for little materials.
	if (amount <= 1)
		return;
		
	// For medium amount of materials provide a lorry with resources.	
	if (amount >= 2)
	{
		var x = 160 + Random(40);
		var lorry = CreateObject(Lorry, x, FindHeight(x) - 20);
		lorry->CreateContents(Wood, 6);
		lorry->CreateContents(Metal, 4);
		lorry->CreateContents(Rock, 4);
		lorry->CreateContents(Dynamite, 4);
		lorry->CreateContents(Pickaxe);

		// For large amount of materials add more explosives into the lorry
		if (amount >= 3)
		{
			lorry->CreateContents(Wood, 6);
			lorry->CreateContents(Metal, 4);
			lorry->CreateContents(Rock, 4);
			lorry->CreateContents(DynamiteBox, 2);
		}
	}
	return;
}


/*-- Helper functions --*/

global func TestGoldCount()
{
	var pos;
	while (pos = FindLocation(Loc_Material("Gold")))
	{
		var pos = CreateObject(Rock, pos.x, pos.y)->Explode(100);
	}
	var gold_count = ObjectCount(Find_ID(Nugget));
	return gold_count;
}

