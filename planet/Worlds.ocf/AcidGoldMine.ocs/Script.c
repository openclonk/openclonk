/**
	Acid Gold Mine
	An acid lake with a cliff leading to a volcanic gold mine.
	
	@author Sven2
*/


protected func Initialize()
{
	// Show wealth in HUD.
	GUI_Controller->ShowWealth();
	
	// Rules: team account and buying at flagpole.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	
	// Goal: gain wealth dependent on difficulty.
	var goal = CreateObject(Goal_Wealth);
	goal->SetWealthGoal(100 + 150 * SCENPAR_Difficulty);

	// Initialize different parts of the scenario.
	InitEnvironment(SCENPAR_Difficulty);
	InitVegetation(SCENPAR_MapSize);
	InitAnimals(SCENPAR_MapSize);
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

	// Position crew and give them materials.
	var index = 0, crew;
	while (crew = GetCrew(plr, index))
	{
		var pos = FindTopSpot();
		crew->SetPosition(pos.x, pos.y - 10);
		while(crew->Stuck())
			crew->SetPosition(pos.x, crew->GetY()-1);
		crew->CreateContents(Shovel);
		// First clonk can construct, others can chop.
		if (index == 0)
			crew->CreateContents(Hammer);
		else
			crew->CreateContents(Axe);
		index++;
	}
	
	// Give the player basic plus pumping knowledge.
	GivePlayerBasicKnowledge(plr);
	GivePlayerPumpingKnowledge(plr);
	GivePlayerSpecificKnowledge(plr, [WallKit]);
	
	// Give the player the elementary base materials.
	GivePlayerElementaryBaseMaterial(plr);
	return;
}


/*-- Scenario Initialization --*/

private func InitEnvironment(int difficulty)
{
	// Acid rain.
	Cloud->Place(8 + 2 * difficulty);
	Cloud->SetPrecipitation("Acid", 100);
	// Sky.
	SetSkyParallax(1, 20, 20, 0,0, nil, nil);
	// Some natural disasters.
	Earthquake->SetChance(100);
	Volcano->SetChance(1 + difficulty);
	Volcano->SetMaterial("DuroLava");
	Meteor->SetChance(16 + 4 * difficulty);
	// Bottom item killer.
	var fx = AddEffect("KeepAreaClear", nil, 1, 5);
	fx.search_criterion=Find_And(Find_AtRect(0, LandscapeHeight() - 10, LandscapeWidth(), 10), Find_Not(Find_Category(C4D_StaticBack)));
	return;
}

private func InitVegetation(int map_size)
{
	// Mushrooms before any earth materials, because they create their own caves.
	LargeCaveMushroom->Place(15, Shape->Rectangle(LandscapeWidth() / 4, 172 * 6, LandscapeWidth() / 2, 60 * 6));
	// Create earth materials
	// Create them in big clusters so the whole object arrangement looks a bit less uniform and more interesting.
	PlaceObjectBatches([Firestone], 3, 100, 5);
	PlaceObjectBatches([Rock, Loam, Loam], 10, 200, 10);
	// Misc vegetation
	SproutBerryBush->Place(5, Shape->Rectangle(0, LandscapeHeight() / 4, LandscapeWidth(), LandscapeHeight() * 3 / 4));
	Mushroom->Place(5, Shape->Rectangle(0, LandscapeHeight() / 4, LandscapeWidth(), LandscapeHeight() * 3 / 4));
	Tree_Coniferous_Burned->Place(2, Shape->Rectangle(0, 0, LandscapeWidth(), LandscapeHeight() / 4));
	return;
}

private func InitAnimals(int map_size)
{
	return;
}

private func InitMaterial(int amount)
{
	// Starting materials in lorry.
	var pos = FindTopSpot();
	var lorry = CreateObjectAbove(Lorry, pos.x, pos.y);
	if (lorry)
	{
		lorry->CreateContents(WallKit, 5);
		lorry->CreateContents(Wood, 12);
		lorry->CreateContents(Metal, 5);
		lorry->CreateContents(Bread, 8);
		lorry->CreateContents(Firestone, 5);
		lorry->CreateContents(Dynamite, 3);
		lorry->CreateContents(DynamiteBox, 2);
		while(lorry->Stuck())
			lorry->SetPosition(lorry->GetX(), lorry->GetY()-1);
	}
	// Create some chests in caves.
	var chest_sets = [[[DynamiteBox,2], [Dynamite,5], [Bread,5]], [[Loam,5], [WallKit,3], [Wood,8]], [[Bread,10],[Firestone,5],[Wood,8]]];
	for (var i = 0; i < 3; ++i)
	{
		var chest_pos = FindLocation(Loc_Material("Tunnel"), Loc_Wall(CNAT_Bottom));
		if (chest_pos)
		{
			var chest = CreateObjectAbove(Chest, chest_pos.x, chest_pos.y);
			if (chest)
				for (var chest_fill in chest_sets[i])
					chest->CreateContents(chest_fill[0], chest_fill[1]);
		}
	}
	// A barrel somewhere in a cave.
	var chest_pos = FindLocation(Loc_Material("Tunnel"), Loc_Wall(CNAT_Bottom));
	if (chest_pos)
		CreateObjectAbove(Barrel, chest_pos.x, chest_pos.y);

	return;
}


/*-- Helper Functions --*/

private func FindTopSpot()
{
	return FindLocation(Loc_InRect(LandscapeWidth() / 3, 0, LandscapeWidth() * 2 / 3, LandscapeHeight() / 9), Loc_Wall(CNAT_Bottom), Loc_Space(10, CNAT_Top)) ?? { x = LandscapeWidth() / 3 + Random(30), y = LandscapeHeight() / 12 };
}

global func FxKeepAreaClearTimer(object q, proplist fx, int time)
{
	for (var obj in FindObjects(fx.search_criterion))
		if (obj) 
			obj->RemoveObject();
	return FX_OK;
}
