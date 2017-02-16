/**
	Clonkomotive
	Drive the train from left to right in tough terrain.

	@author Pyrit, Maikel
*/


// Whether the intro has been initialized.
static intro_init;

// Bat cave [x, y] coordinates. Set in Map.c
//static bat_cave;

public func Initialize()
{
	// Show wealth in HUD.
	GUI_Controller->ShowWealth();
	
	// Rules: team account and buying at flagpole.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	
	// Create goal: transport train to other village.
	var goal = CreateObject(Goal_LocomotiveTransport);
	goal->SetGoalRect(LandscapeWidth() - 180, FindHeight(LandscapeWidth() - 240) - 200, 180, 200);
	
	// Rescale map coordinates.
	var map_zoom = GetScenarioVal("MapZoom", "Landscape");
	bat_cave[0] = bat_cave[0] * map_zoom + map_zoom / 2;
	bat_cave[1] = bat_cave[1] * map_zoom + map_zoom / 2;
			
	// Initialize different parts of the scenario.
	InitEnvironment(SCENPAR_MapSize, SCENPAR_Difficulty);
	InitVegetation(SCENPAR_MapSize, SCENPAR_Difficulty);
	InitAnimals(SCENPAR_MapSize, SCENPAR_Difficulty);
	InitMaterial(4 - SCENPAR_Difficulty);
	InitCity();
	return;
}

protected func OnGoalsFulfilled()
{
	// Give the remaining players their achievement.
	GainScenarioAchievement("Done", BoundBy(SCENPAR_Difficulty, 1, 3));
	return false;
}


/*-- Player Initialization --*/

public func InitializePlayer(plr)
{
	// Move clonks to location and give them a shovel.
	var index = 0, crew;
	while (crew = GetCrew(plr, index))
	{
		var x = 60;
		var y = FindHeight(x);
		crew->SetPosition(x, y - 12);
		// First clonk can construct, others can chop.
		crew->CreateContents(Shovel);
		if (index == 0)
			crew->CreateContents(Hammer);
		else
			crew->CreateContents(Axe);
		index++;
	}
	
	// Give the player its knowledge.
	GivePlayerBasicKnowledge(plr);
	GivePlayerPumpingKnowledge(plr);
	GivePlayerWeaponryKnowledge(plr);
	GivePlayerAdvancedKnowledge(plr);
	GivePlayerFarmingKnowledge(plr);
	GivePlayerAirKnowledge(plr);
	GivePlayerSpecificKnowledge(plr, [WoodenBridge]);
	RemovePlayerSpecificKnowledge(plr, [WallKit]);

	// Give the player the elementary base materials and some tools.
	GivePlayerElementaryBaseMaterial(plr);
	GivePlayerToolsBaseMaterial(plr);
	
	// Take over small village at the start of the map.
	for (var building in FindObjects(Find_Or(Find_ID(Sawmill), Find_ID(WindGenerator), Find_ID(ToolsWorkshop))))
	{
		building->SetOwner(plr);
	}

	// Set zoom range.
	SetPlayerZoomByViewRange(plr, 600, nil, PLRZOOM_Direct | PLRZOOM_LimitMax);
	SetPlayerViewLock(plr, true);
	
	// Initialize the intro sequence if not yet started.
	if (!intro_init)
	{
		StartSequence("Intro", 0);
		intro_init = true;
	}
  	return;
}


/*-- Scenario Initialization --*/

private func InitEnvironment(int map_size, int difficulty)
{
	// Sky scrolling.
	SetSkyParallax(0, 75, 75, nil, nil, nil, -200);

	// Set time of day to evening and create some clouds and celestials.
	Cloud->Place(16);
	Cloud->SetPrecipitation("Water", 30 * difficulty);
	if (difficulty == 3)
	{
		Cloud->SetLightning(20);
		Meteor->SetChance(4);	
	}
	Time->Init();
	Time->SetTime(60 * 12);
	Time->SetCycleSpeed(20);
	// Place stars manually to not overlap with mountain background and fit map size.
	Time->PlaceStars(LandscapeWidth(), 180);
	
	// Add an effect which controls the rock fall in this round.
	AddEffect("ScenarioRockFall", nil, 100, 36, nil, nil, difficulty);
	return;
}

// Callback from time controller: this scenario has no celestials.
public func HasNoCelestials() { return true; }

private func InitVegetation(int map_size, int difficulty)
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	
	// Trees.
	Tree_Deciduous->Place(30 + 10 * map_size, Rectangle(wdt / 6, 0, 4 * wdt / 6, 4 * hgt / 9));
	Tree_Coniferous2->Place(4 + 2 * map_size, Rectangle(wdt / 6, 0, 4 * wdt / 6, 4 * hgt / 9));
	Tree_Coniferous3->Place(4 + 2 * map_size, Rectangle(wdt / 6, 0, 4 * wdt / 6, 4 * hgt / 9));
	LargeCaveMushroom->Place(12 + 2 * map_size, Rectangle(wdt / 6, 4 * hgt / 9, 4 * wdt / 6, 5 * hgt / 9));
		
	// Smaller vegetation.
	Grass->Place(100);
	Wheat->Place(10);
	SproutBerryBush->Place(3);
	Flower->Place(20 + 4 * map_size);
	Mushroom->Place(20 + 4 * map_size);
	Trunk->Place(6 + 2 * map_size);
	Fern->Place(20 + 4 * map_size);
	Branch->Place(20 + 4 * map_size);
	Cotton->Place(8, Rectangle(wdt - 600, 0, 600, hgt));
	
	// Under water vegetation.
	Seaweed->Place(15 + 5 * map_size);
	Coral->Place(10 + 2 * map_size);

	// Some objects in the earth.	
	PlaceObjects(Rock, 30 + 20 * map_size + Random(10),"Earth");
	PlaceObjects(Firestone, 30 + 10 * map_size + Random(5), "Earth");
	PlaceObjects(Loam, 4 + 2 * map_size, "Earth");
	return;
}

private func InitAnimals(int map_size, int difficulty)
{
	// Place some fish, sharks and piranhas.
	Fish->Place(20 + 4 * map_size);
	Piranha->Place((10 + 4 * map_size) * (difficulty - 1));
	Shark->Place(difficulty - 1);
	
	// Some insects: zaps, mosquitos, butterflies, fireflies.
	Zaphive->Place(2 + 4 * difficulty);
	Mosquito->Place(4 + 2 * map_size);
	Butterfly->Place(16 + 4 * map_size);
	Firefly->Place(4 + map_size);
	
	// Bats in the bat cave.
	Bat->Place(4 + 2 * difficulty, Rectangle(bat_cave[0] - 20, bat_cave[1] - 20, 40, 40));
	return;
}

private func InitMaterial(int amount)
{
	// Train on the start cliff.
	var train = CreateObjectAbove(Locomotive, 60, FindHeight(60) - 2);
	train->SetDir(DIR_Right);
	
	// Lorry with materials on normal and hard.
	if (amount >= 2)
	{
		var lorry = CreateObject(Lorry, 120, FindHeight(120) - 2);
		lorry->SetPosition(train->GetX()-25, train->GetY());
		lorry->CreateContents(Metal, 4);
		lorry->CreateContents(Wood, 4);
	}
	// Small settlement on normal.
	if (amount >= 3)
	{
		CreateObjectAbove(WindGenerator, 180, FindHeight(180));
		CreateObjectAbove(Sawmill, 210, FindHeight(210));
		CreateObjectAbove(ToolsWorkshop, 140, FindHeight(140));
	}
	
	// Always a catapult on the second cliff.
	CreateObjectAbove(Catapult, 880, FindHeight(880));
	
	// A lorry with rewards in the bat cave.
	var lorry = CreateObjectAbove(Lorry, bat_cave[0], bat_cave[1]);
	lorry->CreateContents(Pickaxe, amount);
	lorry->CreateContents(WallKit, 2 * amount);
	lorry->CreateContents(Wood, 2 * amount);
	return;
}

private func InitCity()
{
	var wdt = LandscapeWidth();
	CreateObjectAbove(WoodenCabin, wdt - 60, FindHeight(wdt - 60))->MakeInvincible();
	CreateObjectAbove(WoodenCabin, wdt - 190, FindHeight(wdt - 190))->MakeInvincible();
	CreateObjectAbove(Windmill, wdt - 120, FindHeight(wdt - 120))->MakeInvincible();
	return;
}


/*-- Rockfall --*/

global func FxScenarioRockFallStart(object target, proplist effect, int temp, int diff)
{
	if (temp)
		return FX_OK;
	effect.difficulty = diff;
	// Find the range for the rockfall to fall in (the last valley).
	var range_end;
	for (var x = LandscapeWidth(); x > 0; x -= 5)
	{
		var y = FindHeight(x) + 5;
		if (GetMaterial(x, y) == Material("Water"))
		{
			range_end = x;
			break;
		}
	}
	var end_y = FindHeight(range_end) + 5;
	var range_start = range_end;
	while (GetMaterial(range_start, end_y) == Material("Water") && range_start > 0)
		range_start--;
	while (GetMaterial(range_end, end_y) == Material("Water") && range_end < LandscapeWidth())
		range_end++;
	
	effect.range = [range_start + 60, range_end - 60];
	return FX_OK;
}

global func FxScenarioRockFallTimer(object target, proplist effect, int time)
{
	if (Random(600 / effect.difficulty**2) || GetEffect("LaunchRockFall"))
		return FX_OK;
	// Launch rock fall.
	var rockfall_effect = AddEffect("LaunchRockFall", nil, 100, 1, nil);
	rockfall_effect.range = effect.range;
	rockfall_effect.duration = 36 * (10 + 5 * effect.difficulty);
	return FX_OK;
}

global func FxLaunchRockFallStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	Sound("Environment::Disasters::Earthquake", true, 100, nil, 1);
	return FX_OK;
}

global func FxLaunchRockFallTimer(object target, proplist effect, int time)
{
	// Kill the effect once its duration is reached.
	if (time > effect.duration)
		return FX_Execute_Kill;

	var crumbs =
	{
		Size = PV_Random(1, 3),
		Phase = PV_Random(0, 2),
		Alpha = PV_KeyFrames(0, 0, 255, 900, 255, 1000, 0),
		CollisionVertex = 500,
		OnCollision = PC_Bounce(20),
		ForceY = PV_Gravity(50),
		ForceX = PV_Random(-5, 5, 15),
		Rotation = PV_Direction(PV_Random(750, 1250)),
		Attach = ATTACH_Front
	};

	if (!Random(3))
		CreateParticle("RockFragment", RandomX(effect.range[0], effect.range[1]), 5, PV_Random(-5, 5), 0, 36 * 5, crumbs, 1);

	if (time > 36 * 3 && !Random(10)) 
		CastObjects(RockFragment, 1, 10, RandomX(effect.range[0], effect.range[1]), 5, 180, 90);
	
	return FX_OK;
}

global func FxLaunchRockFallStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	// Stop sound.
	Sound("Environment::Disasters::Earthquake", true, 100, nil, -1);
	Sound("Environment::Disasters::EarthquakeEnd",true);
	return FX_OK;
}
