/** 
	Rapid Refining
	Use the oil from an underground well to power your settlement.
	
	@author Maikel
*/


// Whether the intro has been initialized.
static intro_init;

// Whether the first players has been initialized.
static first_plr_init;

protected func Initialize()
{
	// Show wealth in HUD.
	GUI_Controller->ShowWealth();
	
	// Rules: team account and buying at flagpole.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	
	// Goal: pump oil into refinery drain.
	var goal = CreateObject(Goal_Refinery);
	var amount = 400;
	if (SCENPAR_Difficulty == 2)
		amount = 800;
	else if (SCENPAR_Difficulty == 3)
		amount = 1600; 
	goal->SetGoalAmount(amount);

	// Initialize different parts of the scenario.
	InitEnvironment(SCENPAR_Difficulty);
	InitVegetation(SCENPAR_MapSize);
	InitAnimals(SCENPAR_MapSize, SCENPAR_Difficulty);
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
	SetPlayerZoomByViewRange(plr, 400, nil, PLRZOOM_Direct | PLRZOOM_LimitMax);
	SetPlayerViewLock(plr, true);
	SetFoW(false, plr);
	
	// First player inits the base.
	if (!first_plr_init)
	{
		InitBase(plr, 4 - SCENPAR_Difficulty);
		first_plr_init = true;
		// Give only the first joined player some wealth.
		SetWealth(plr, 150 - 50 * SCENPAR_Difficulty);
	}
	
	// Position and materials for the crew.
	var crew;
	for (var i = 0; crew = GetCrew(plr, i); ++i)
	{
		crew->SetPosition(20 + Random(32), 160 - 10);
		crew->CreateContents(Shovel);
	}
	
	// Give the player basic and pumping knowledge.
	GivePlayerBasicKnowledge(plr);
	GivePlayerPumpingKnowledge(plr);
	GivePlayerWeaponryKnowledge(plr);
	GivePlayerAdvancedKnowledge(plr);
	GivePlayerFarmingKnowledge(plr);
	RemovePlayerSpecificKnowledge(plr, [WindGenerator]);	
	
	// Give the player the elementary base materials.
	GivePlayerElementaryBaseMaterial(plr);
	GivePlayerToolsBaseMaterial(plr);
	
	// Initialize the intro sequence if not yet started.
	if (!intro_init)
	{
		//StartSequence("Intro", 0);
		intro_init = true;
	}	
	return;
}

private func InitBase(int owner, int amount)
{
	var y = 160;
	
	// The basic settlement: flagpole, wind generator, chest.
	CreateObjectAbove(Flagpole, 184, y, owner);
	var chest = CreateObjectAbove(Chest, 202, y, owner);
	chest->CreateContents(Hammer, 2);
	chest->CreateContents(Axe, 2);
	CreateObjectAbove(WindGenerator, 222, y, owner);
	
	// Two pumps connected to the refinery drain.
	var refinery_exit = CreateObjectAbove(RefineryDrain, 8, y, owner);
	var pump1 = CreateObjectAbove(Pump, 250, y, owner);
	var pump2 = CreateObjectAbove(Pump, 284, y, owner);
	var pipe1 = pump1->CreateObject(Pipe);
	pipe1->ConnectPipeTo(pump1);
	pipe1 = pump1->CreateObject(Pipe, 8);
	pipe1->ConnectPipeTo(pump1);
	pipe1->ConnectPipeTo(refinery_exit);
	var pipe2 = pump2->CreateObject(Pipe, 8);
	pipe2->ConnectPipeTo(pump2);
	pipe2 = pump2->CreateObject(Pipe, -8);
	pipe2->ConnectPipeTo(pump2);

	// Additional material in the chest.
	if (amount >= 2)
	{
		chest->CreateContents(Wood, 12);
		chest->CreateContents(Metal, 8);
		chest->CreateContents(Dynamite, 4);
		if (amount >= 3)
		{
			chest->CreateContents(Wood, 12);
			chest->CreateContents(Metal, 8);
			chest->CreateContents(Loam, 4);
			chest->CreateContents(Pickaxe, 2);
			chest->CreateContents(Bread, 4);
		}
	}
	return;
}


/*-- Scenario Initialization --*/

private func InitEnvironment(int difficulty)
{
	// Sky has some parallax.
	SetSkyParallax(1, 20, 20);
	
	// Some earthquakes if difficulty prescribes it.
	if (difficulty >= 2)
		Earthquake->SetChance(4 * (difficulty - 1));
		
	// A waterfall above the underground lake.
	var waterfall_x = 50;
	var waterfall_y = LandscapeHeight() - 600;
	var trunk = CreateObjectAbove(Trunk, waterfall_x, waterfall_y);
	trunk->DoCon(30); trunk->SetR(150); trunk.Plane = 510;
	trunk.MeshTransformation = [-70, 0, 998, 0, 0, 1000, 0, 0, -998, 0, -70, 0];
	trunk->MakeInvincible();
	
	var waterfall = CreateWaterfall(waterfall_x + 22, waterfall_y - 10, 10, "Water");
	waterfall->SetDirection(3, 3, 2, 3);
	waterfall->SetSoundLocation(waterfall_x + 40, waterfall_y + 240);
	
	CreateLiquidDrain(8, 1040, 10);
	CreateLiquidDrain(16, 1040, 10);
	CreateLiquidDrain(24, 1040, 10);
	return;
}

private func InitVegetation(int map_size)
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	
	// Some plants and trees on the outside.
	Tree_Deciduous->Place(16, Shape->Rectangle(0, 0, 240, 160));
	Tree_Coniferous2->Place(2, Shape->Rectangle(0, 0, 240, 160));
	Cotton->Place(4, Shape->Rectangle(0, 0, 240, 160));
	SproutBerryBush->Place(2, Shape->Rectangle(0, 0, 240, 160));
	Grass->Place(100);
	
	// Some plants and in the caves.
	LargeCaveMushroom->Place(40 + 6 * map_size, nil, {terraform = false});
	Fern->Place(40 + 6 * map_size);
	Mushroom->Place(40 + 6 * map_size);
	Branch->Place(20 + 3 * map_size);
	
	// Some objects in the earth.	
	PlaceObjects(Rock, 40 + 10 * map_size + Random(5),"Earth");
	PlaceObjects(Firestone, 40 + 10 * map_size + Random(5), "Earth");
	PlaceObjects(Loam, 40 + 10 * map_size + Random(5), "Earth");
	
	// Underwater plants.
	var place_rect = Shape->Rectangle(0, 2 * hgt / 3, wdt / 4, hgt / 3);
	Seaweed->Place(12, place_rect);
	Coral->Place(8, place_rect);
	
	// Place some diamonds in the hard to reach location.
	Diamond->Place(20, Shape->Rectangle(5 * wdt / 6, 0, wdt / 6, hgt / 4));
	return;
}

private func InitAnimals(int map_size, int difficulty)
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	
	// Some butterflies as atmosphere.
	for (var i = 0; i < 8; i++)
		PlaceAnimal(Butterfly);
		
	// Some wipfs underground.
	Wipf->Place(10);
	
	// Place some fishes and piranhas as difficulty prescribes it.
	var place_rect = Shape->Rectangle(0, 2 * hgt / 3, wdt / 4, hgt / 3);
	var fish_count = 15;
	Fish->Place(fish_count * (3 - difficulty), place_rect);
	Piranha->Place(fish_count * (difficulty - 1), place_rect);
	
	// Bats on higher difficulty.
	if (difficulty >= 2)
		Bat->Place((difficulty - 1) * 12, Shape->Rectangle(2 * wdt / 3, 0, wdt / 3, hgt));
	return;
}
