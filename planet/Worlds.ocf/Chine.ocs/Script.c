/**
	Chine
	A chine with a large waterfall and lots of vegetation.
	
	@author Maikel
*/


// Whether the intro has been initialized.
static intro_init;

protected func Initialize()
{
	// Show wealth in HUD.
	GUI_Controller->ShowWealth();
	
	// Rules: team account and buying at flagpole.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);

	// Goal: transport the cannon to the top of the chine.
	var cannon = CreateObjectAbove(Cannon, 96 + RandomX(-12, 12), LandscapeHeight() - 92);
	var keg = cannon->CreateContents(PowderKeg);
	// Infinite ammo for this cannon.
	keg->SetPowderCount(nil);
	var cannon_goal = CreateObject(Goal_Script);
	cannon_goal.Name = "$GoalName$";
	cannon_goal.Description = "$GoalDesc$";
	cannon_goal.Picture = Chine_GoalIcon;
	// Add an effect to check whether the goal is fulfilled.
	AddEffect("GoalCheck", nil, 100, 2, nil);

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
	SetPlayerZoomByViewRange(plr, LandscapeWidth(), nil, PLRZOOM_Direct | PLRZOOM_LimitMax);
	SetPlayerViewLock(plr, true);

	// Move clonks to location and give them a shovel.
	var index = 0, crew;
	while (crew = GetCrew(plr, index))
	{
		crew->SetPosition(96 + RandomX(-12, 12), LandscapeHeight() - 92);
		var u = 0;
		while(crew->Stuck())
		{
			crew->SetPosition(crew->GetX(), crew->GetY()-1);
			++u;
			if (u > 50) // This is bad, the clonk will most likely die
				break;
		}

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
	// Additional explosives: dynamite boxes.
	GivePlayerSpecificBaseMaterial(plr, [[DynamiteBox, 4, 2]]);
	
	// Ensure mimimum player wealth.
	var add_wealth = Max(0, 75 - 25 * SCENPAR_Difficulty - GetWealth(plr));
	DoWealth(plr, add_wealth);
	
	// Initialize the intro sequence if not yet started.
	if (!intro_init)
	{
		StartSequence("Intro", 0);
		intro_init = true;
	}
	return;
}


/*-- Goal Check --*/

global func FxGoalCheckTimer(object target, proplist effect)
{
	var cannon = FindObject(Find_ID(Cannon));
	if (!cannon)
	{
		// Start elimination sequence due to lost cannon.
		// TODO: determine clonk which was responsible and let him take the blame in the sequence.
		StartSequence("Failure", 0);
		return -1;
	}
	if (cannon->GetY() < 100)
	{
		var goal = FindObject(Find_ID(Goal_Script));
		if (goal)
			goal->Fulfill();
		return -1;	
	}
	return 1;
}


/*-- Scenario Initialization --*/

private func InitEnvironment(int map_size, int difficulty)
{
	// Adjust the sky a bit.
	SetSkyParallax(0, 20, 20);
	SetSkyAdjust(RGBa(225, 255, 205, 191), RGB(63, 200, 0));
	
	// Waterfalls dominate the landscape, they are place at the top left of the chine.
	var waterfall_x = 0;
	while (!GBackSky(waterfall_x, 0) && waterfall_x < LandscapeWidth() / 2)
		waterfall_x++;
	for (var i = 0; i < 16 + 4 * difficulty; i++)
	{
		var fall = CreateWaterfall(waterfall_x + 2, 0, RandomX(3, 4), "Water");
		fall->SetDirection(RandomX(14, 16), 12, 4, 4);
		fall->SetSoundLocation(LandscapeWidth() / 2, Random(LandscapeHeight()));
	}
	var trunk = CreateObjectAbove(Trunk, waterfall_x + 2, 20);
	trunk->SetR(-30); trunk.Plane = 550;
	trunk->MakeInvincible();
	
	// Cast some additional PXS at the start at random locations.
	for (var i = 0; i < 20000 + 10000 * map_size; i++)
		InsertMaterial(Material("Water"), Random(LandscapeWidth()), Random(5 * LandscapeHeight() / 6), RandomX(-5, 5), RandomX(3, 6));
	
	// Some natural disasters. 
	Earthquake->SetChance(2 + 2 * difficulty);
	if (difficulty >= 2)
		Rockfall->SetChance(20);
	if (difficulty >= 3)
	{
		Rockfall->SetChance(80);
		Rockfall->SetExplosiveness(15);
	}
	Rockfall->SetSpawnDistance(250);
	Rockfall->SetArea(Shape->Rectangle(128, 0, 128, LandscapeHeight() - 300));
	return;
}

private func InitVegetation(int map_size, int difficulty)
{
	// Define parts of the map for even distribution.
	var top = Shape->Rectangle(0, 0, LandscapeWidth(), LandscapeHeight() / 3);
	var middle = Shape->Rectangle(0, LandscapeHeight() / 3, LandscapeWidth(), LandscapeHeight() / 3);
	var bottom = Shape->Rectangle(0, 2 * LandscapeHeight() / 3, LandscapeWidth(), LandscapeHeight() / 3);
	
	// Place gras wherever possible.
	PlaceGrass(100);
	
	// Place some cocont trees and cave mushrooms for wood.
	for (var i = 0; i < 16 + Random(6); i++)
	{
		PlaceVegetation(Tree_Coconut, top.x, top.y, top.wdt, top.hgt, 1000 * (61 + Random(40)));
		PlaceVegetation(Tree_Coconut, middle.x, middle.y, middle.wdt, middle.hgt, 1000 * (61 + Random(40)));
		PlaceVegetation(Tree_Coconut, bottom.x, bottom.y, bottom.wdt, bottom.hgt, 1000 * (61 + Random(40)));
	}
	LargeCaveMushroom->Place(6, middle, { terraform = false });
	LargeCaveMushroom->Place(6, bottom, { terraform = false });
		
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
	Branch->Place(30 + 12 * map_size + Random(16));
	Trunk->Place(6 + 2 * map_size + Random(5));
	
	// Some objects in the earth.	
	PlaceObjects(Rock, 25 + 10 * map_size + Random(10),"Earth");
	PlaceObjects(Firestone, 25 + 10 * map_size + Random(10), "Earth");
	PlaceObjects(Loam, (6 + 2 * map_size) * (4 - difficulty) + Random(5), "Earth");
	if (difficulty == 1)
		PlaceObjects(Loam, 12, "Earth");
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
	// No extra materials for little materials.
	if (amount <= 1)
		return;
		
	// For medium amount of materials provide a lorry with resources.	
	if (amount >= 2)
	{
		var lorry = CreateObjectAbove(Lorry, 72 + RandomX(-12, 12), LandscapeHeight() - 92);
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
