/**
	Gold Rush
	A simple landscape with some gold, the goal is to mine some gold to gain wealth.
	
	@author Maikel
*/


// Scenario properties which can be set later by the lobby options.
static const SCENOPT_Material = 1; // Amount of material available from start.
static const SCENOPT_MapSize = 1; // Size of the map.
static const SCENOPT_Difficulty = 1; // Difficulty settings.

protected func Initialize()
{
	// Rules: team account and buying at flagpole.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	
	// Goal: Gain Wealth, amount depends on difficulty, though bounded by availability.
	var gold = GetMaterialCount(Material("Gold")) / GetMaterialVal("Blast2ObjectRatio", "Material", Material("Gold"));
	var percentage = 60 + 10 * SCENOPT_Difficulty;
	var wealth_goal = gold * 5 * percentage / 1000 * 10;
	var goal = CreateObject(Goal_Wealth);
	goal->SetWealthGoal(wealth_goal);
	
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
		crew->SetPosition(LandscapeWidth() / 2 + RandomX(-12, 12), LandscapeHeight() - 92);

		// First clonk can construct, others can chop.
		if (index == 0)
		{
			crew->CreateContents(Shovel);
			crew->CreateContents(Pickaxe);
		}
		else
		{
			crew->CreateContents(Axe);
			crew->CreateContents(Hammer);
		}
		index++;
	}
	
	// Give the player basic knowledge.
	GivePlayerBasicKnowledge(plr);
	GivePlayerSpecificKnowledge(plr, [InventorsLab, Ropeladder, MetalBarrel, PowderKeg, GrappleBow]);
	
	// Give the player the elementary base materials and some tools.
	GivePlayerElementaryBaseMaterial(plr);
	GivePlayerToolsBaseMaterial(plr);
	return;
}


/*-- Scenario Initialization --*/

private func InitEnvironment()
{
	// Adjust the sky a bit.
	SetSkyParallax(0, 20, 20);
	SetSkyAdjust(RGBa(255, 255, 255, 191), RGB(63, 255, 0));
	
	// Waterfalls dominate the landscape.
	for (var i = 0; i < 20; i++)
	{
		var x = Random(LandscapeWidth());
		if (!GBackSky(x, 0))
			continue;
		CreateWaterfall(x, 0, RandomX(20, 40), "Water");
	}	
	return;
}

private func InitVegetation()
{
	// Place some cocont trees.
	for (var i = 0; i < 40 + Random(8); i++)
		PlaceVegetation(Tree_Coconut, 0, 0, LandscapeWidth(), LandscapeHeight(), 1000 * (61 + Random(40)));
	SproutBerryBush->Place(4);
	PlaceGrass(100);
	LargeCaveMushroom->Place(10, { terraform = false });
	Fern->Place(60);
	Mushroom->Place(40);
	
	// Some objects in the earth.	
	PlaceObjects(Rock, 25 + 10 * SCENOPT_MapSize + Random(10),"Earth");
	PlaceObjects(Firestone, 20 + 10 * SCENOPT_MapSize + Random(5), "Earth");
	PlaceObjects(Loam, 20 + 10 * SCENOPT_MapSize + Random(5), "Earth");

	return;
}

private func InitAnimals()
{
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
		var lorry = CreateObject(Lorry, LandscapeWidth() / 2 + RandomX(-12, 12), LandscapeHeight() - 92);
		lorry->CreateContents(Wood, 6);
		lorry->CreateContents(Metal, 4);
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


/*-- Some helper functions --*/

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

