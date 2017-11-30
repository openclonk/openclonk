/**
	Horrid Highway
	Players need to completely excavate the sky island far away.
		
	@author Maikel
*/


// Whether the intro has been initialized.
static intro_init;

protected func Initialize()
{
	// Show wealth in HUD.
	GUI_Controller->ShowWealth();
	
	// Goal: locomotive highway.
	var goal = CreateObject(Goal_LocomotiveHighway);
	goal.LocomotiveGoal = 8 * SCENPAR_Difficulty;
	
	// Rules: team account and buying at flagpole.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	
	// Allow for base respawns.
	var relaunch_rule = GetRelaunchRule();
	relaunch_rule->SetInventoryTransfer(false);
	relaunch_rule->SetLastClonkRespawn(true);
	relaunch_rule->SetFreeCrew(false);
	relaunch_rule->SetAllowPlayerRestart(true);
	relaunch_rule->SetBaseRespawn(true);
	relaunch_rule->SetRespawnDelay(0);
	
	// Initialize different parts of the scenario.
	InitEnvironment(SCENPAR_Difficulty);
	InitVegetation();
	InitAnimals(SCENPAR_Difficulty);
	InitBridges();
	InitLeftIsland();
	InitMiddleIsland();
	InitRightIsland();
	InitDisasters(SCENPAR_Difficulty);
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
	// Zoom range.
	SetPlayerZoomByViewRange(plr, 1200, nil, PLRZOOM_LimitMax);
	SetPlayerZoomByViewRange(plr, 500, nil, PLRZOOM_Direct | PLRZOOM_Set);
	SetPlayerViewLock(plr, true);
	
	// Position and materials.
	var i, crew;
	for (i = 0; crew = GetCrew(plr, i); ++i)
	{
		crew->SetPosition(100, LandscapeHeight() / 2 - 10);
		crew->CreateContents(Shovel);
	}
	
	// Give the player its knowledge.
	GivePlayerBasicKnowledge(plr);
	GivePlayerPumpingKnowledge(plr);
	GivePlayerFarmingKnowledge(plr);
	GivePlayerWeaponryKnowledge(plr);
	GivePlayerArtilleryKnowledge(plr);
	GivePlayerAdvancedKnowledge(plr);
	GivePlayerAirKnowledge(plr);
	
	// Give the player its base materials.
	GivePlayerElementaryBaseMaterial(plr);
	GivePlayerToolsBaseMaterial(plr);
	GivePlayerSpecificBaseMaterial(plr, [[Dynamite, 20, 10]]);
	
	// Claim ownership of structures, last player who joins owns all the flags.
	for (var structure in FindObjects(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole"))))
		structure->SetOwner(plr);
		
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


/*-- Scenario Initialization --*/

// Initializes environment and disasters.
private func InitEnvironment(int difficulty)
{
	// Init time and have normal cycle.
	Time->Init();

	// Set a certain parallax.
	SetSkyParallax(0, 20, 20);
	
	// Adjust sky dependent on difficulty setting.
	if (difficulty == 3)
		SetSkyAdjust(RGBa(225, 255, 205, 191), RGB(63, 200, 0));
	
	// Clouds and rain.
	Cloud->Place(15);
	Cloud->SetPrecipitation("Water", 60);
	if (difficulty == 3)
		Cloud->SetPrecipitation("Acid", 60);
	return;
}

// Initializes grass, trees and in-earth objects.
private func InitVegetation()
{
	// Grass on all islands.
	Grass->Place(100);
	
	// Vegetation around all islands.
	Flower->Place(20);
	Mushroom->Place(20);
	Fern->Place(12);
	Branch->Place(30);
	Trunk->Place(8);
	Vine->Place(18, nil, {attach_material = Loc_Or(Loc_Material("Granite"), Loc_Material("Rock"), Loc_Material("Everrock"), Loc_Material("Gold"))});
	Cotton->Place(8);
	Wheat->Place(8);
	
	// Some objects in the earth.	
	PlaceObjects(Rock, 40, "Earth");
	PlaceObjects(Firestone, 40, "Earth");
	PlaceObjects(Loam, 30, "Earth");

	// Place some trees.
	Tree_Deciduous->Place(20, Rectangle(0, 0, 600, LandscapeHeight() / 3));
	Tree_Deciduous->Place(20, Rectangle(LandscapeWidth() - 600, 0, 600, LandscapeHeight() / 3));
	Tree_Coniferous2->Place(12, Rectangle(0, 0, 600, LandscapeHeight() / 3));
	Tree_Coniferous2->Place(12, Rectangle(LandscapeWidth() - 600, 0, 600, LandscapeHeight() / 3));
	return;
}

// Initializes animals.
private func InitAnimals(int difficulty)
{
	// Some fireflies attracted to trees.
	Firefly->Place(6);
	// Place some bats depending on difficulties.
	var bat_region = Rectangle(LandscapeWidth() - 600, 0, 600, LandscapeHeight());
	if (difficulty >= 2)
		bat_region = Rectangle(LandscapeWidth() / 2 - 300, 0, LandscapeWidth() / 2 + 300, LandscapeHeight());
	Bat->Place(10 * difficulty + 5 * difficulty**2, bat_region, {tunnel_only = true});
	// Place zaps on higher difficulties.
	if (difficulty >= 2)
		Zaphive->Place(2 * difficulty);
	return;
}

private func InitBridges()
{
	// Create four indestructible bridges to connect.
	var height = LandscapeHeight() / 2 + 6;
	var x4 = 4;
	while (GetMaterial(x4, height) == Material("Brick"))
		x4 += 8;
	var x1 = x4;
	while (GetMaterial(x4, height) != Material("Brick") || x4 < LandscapeWidth() / 4)
		x4 += 8;
	var x2 = x4;
	while (GetMaterial(x4, height) == Material("Brick") || x4 < LandscapeWidth() / 2)
		x4 += 8;
	var x3 = x4;
	while (GetMaterial(x4, height) != Material("Brick") || 3 * x4 < LandscapeWidth() / 4)
		x4 += 8;
	var bridge;
	bridge = CreateObject(WoodenBridge, x1 + 20, height);
	bridge->MakeInvincible();
	bridge->SetClrModulation(RGB(80, 120, 200));
	bridge = CreateObject(WoodenBridge, x2 - 20, height);
	bridge->MakeInvincible();
	bridge->SetClrModulation(RGB(80, 120, 200));
	bridge = CreateObject(WoodenBridge, x3 + 20, height);
	bridge->MakeInvincible();
	bridge->SetClrModulation(RGB(80, 120, 200));
	bridge = CreateObject(WoodenBridge, x4 - 20, height);
	bridge->MakeInvincible();
	bridge->SetClrModulation(RGB(80, 120, 200));
	return;
}

private func InitLeftIsland()
{
	var switch = CreateObjectAbove(Switch, 20, LandscapeHeight() / 2);
	var goal = FindObject(Find_ID(Goal_LocomotiveHighway));
	goal->SetPlrViewOnSignalChange(false);
	switch->SetSwitchTarget(goal);
	switch->SetSwitchDir(-1);
	
	var guidepost = CreateObjectAbove(EnvPack_Guidepost2, 40, LandscapeHeight() / 2);
	guidepost->SetInscription("$MsgHorridHighwayEast$");
	guidepost.MeshTransformation = EnvPack_Guidepost2.MeshTransformation;
	
	var lorry = CreateObjectAbove(Lorry, 80, LandscapeHeight() / 2 - 2);
	lorry->CreateContents(Shovel, 2);
	lorry->CreateContents(Hammer, 2);
	lorry->CreateContents(Axe, 2);
	lorry->CreateContents(Barrel, 2);
	
	var elevator = CreateObjectAbove(Elevator, 180, 384);
	elevator->CreateShaft(196);
	CreateObjectAbove(Compensator, 230, 384);
	
	var workshop = CreateObjectAbove(ToolsWorkshop, 156, 480);
	workshop->CreateContents(Wood, 8);
	workshop->CreateContents(Metal, 8);
	
	var chemical_lab = CreateObjectAbove(ChemicalLab, 160, 584);
	chemical_lab->CreateContents(Firestone, 8);
	chemical_lab->CreateContents(Coal, 8);
	CreateObjectAbove(Flagpole, 120, 584);
	
	var bridge = FindObject(Find_ID(WoodenBridge), Sort_Distance(0, LandscapeHeight() / 2));
	bridge->CreateObjectAbove(WindGenerator, 0, -4);
	bridge->CreateObjectAbove(Flagpole, -36, -4);
	bridge->CreateObjectAbove(Catapult, 6, -6);
	return;
}

private func InitMiddleIsland()
{
	var lorry = CreateObjectAbove(Lorry, LandscapeWidth() / 2 + RandomX(-20, 20), LandscapeHeight() / 2 - 2);
	lorry->CreateContents(Firestone, 12);
	lorry->CreateContents(Dynamite, 12);
	lorry->CreateContents(DynamiteBox, 8);
	lorry->CreateContents(PowderKeg, 8);
	lorry->CreateContents(Pickaxe, 2);
	
	var guidepost = CreateObjectAbove(EnvPack_Guidepost2, LandscapeWidth() / 2 + 40, LandscapeHeight() / 2);
	guidepost->SetInscription("$MsgResourcesWest$");
	guidepost.MeshTransformation = EnvPack_Guidepost2.MeshTransformation;
	
	var diamond_cnt = 12;
	for (var cnt = 0; cnt < diamond_cnt; cnt++)
	{
		var pos = FindLocation(Loc_Material("Gold"));
		if (pos)
		{
			CreateObject(Diamond_Socket, pos.x, pos.y);
		}
	}
	return;
}

private func InitRightIsland()
{
	var guidepost = CreateObjectAbove(EnvPack_Guidepost2, LandscapeWidth() - 40, LandscapeHeight() / 2);
	guidepost->SetInscription("$MsgHorridHighwayWest$");
	guidepost.MeshTransformation = EnvPack_Guidepost2.MeshTransformation;
	
	var elevator = CreateObjectAbove(Elevator, LandscapeWidth() - 180, 384);
	elevator->CreateShaft(196);
	
	var foundry = CreateObjectAbove(Foundry, LandscapeWidth() - 100, 384);
	foundry->CreateContents(Coal, 8);
	
	CreateObjectAbove(WoodenCabin, LandscapeWidth() - 100, 480);
	return;
}

private func InitDisasters(int difficulty)
{
	// Lightning: clouds are already inited.
	Cloud->SetLightning(3 * difficulty**2 + difficulty + 2);
	// Rockfall: appears around the central sky island.
	if (difficulty >= 2)
	{
		Rockfall->SetChance(10 * (difficulty - 1));
		Rockfall->SetArea(Rectangle(LandscapeWidth() / 2 - 200, 0, 400, 20));
		if (difficulty >= 3)
			Rockfall->SetExplosiveness(100);
	}
	// Meteors: controlled by effect to happen between the main islands.
	CreateEffect(FxControlMeteors, 100, 1, difficulty);
	return;
}

static const FxControlMeteors = new Effect
{
	Construction = func(int difficulty)
	{
		this.difficulty = difficulty;
		this.chance = 9 * difficulty + 3 * difficulty**2;
		this.Interval = 10;
		// Find spawn range according to outer bridges.
		var bridge_left = FindObject(Find_ID(WoodenBridge), Sort_Distance(0, LandscapeHeight() / 2));
		var bridge_right = FindObject(Find_ID(WoodenBridge), Sort_Distance(LandscapeWidth(), LandscapeHeight() / 2));
		this.spawn_range = [bridge_left->GetX() + 134, bridge_right->GetX() - 134];
		return FX_OK;
	},
	Timer = func(int time)
	{
		if (Random(100) >= 100 - this.chance)
		{ 
			var x = RandomX(this.spawn_range[0], this.spawn_range[1]);
			var spawn_id = nil;
			if (this.difficulty >= 3 && !Random(3))
				spawn_id = Chippie_Egg;
			LaunchMeteor(x, -12, RandomX(40, 60), RandomX(-15, 15), RandomX(40, 50), spawn_id);
		}
		return FX_OK;
	}
};

