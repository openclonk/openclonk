/** 
	Flooded Veins
	A cave landscape where the player starts at the top and needs to retrieve some gems from
	caves filled with water.

	@author Sven2, Maikel
*/


// Whether the intro has been initialized.
static intro_init;

// Whether the first players has been initialized.
static first_plr_init;

protected func Initialize()
{
	// Rules: team account and buying at flagpole.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	
	// Goal: Sell a certain amount of gems dependent on difficulty.
	var goal = CreateObject(Goal_SellGems);
	goal->SetTargetAmount(10 * SCENPAR_Difficulty);

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
	SetPlayerZoomByViewRange(plr, 300, nil, PLRZOOM_Direct | PLRZOOM_LimitMax);
	SetPlayerViewLock(plr, true);
	
	// First player inits the base.
	if (!first_plr_init)
	{
		InitBase(plr, 4 - SCENPAR_Difficulty);
		first_plr_init = true;
	}
	
	// Position and materials for the crew.
	var crew;
	for (var i = 0; crew = GetCrew(plr, i); ++i)
	{
		crew->SetPosition(20 + Random(32), 160 - 10);
		crew->CreateContents(Shovel);
		if (i == 0)
			crew->CreateContents(Hammer);
		else
			crew->CreateContents(Axe);
	}
	
	// Give the player basic and pumping knowledge.
	GivePlayerBasicKnowledge(plr);
	GivePlayerPumpingKnowledge(plr);
	GivePlayerSpecificKnowledge(plr, [WallKit, WindBag, TeleGlove]);
	
	// Give the player the elementary base materials.
	GivePlayerElementaryBaseMaterial(plr);
	
	// Initialize the intro sequence if not yet started.
	if (!intro_init)
	{
		StartSequence("Intro", 0);
		intro_init = true;
	}	
	return;
}

private func InitBase(int owner, int amount)
{
	var y = 160;
	
	// The basic settlement consists of a flagpole and a wind generator.
	var x = 232;
	while ((!GBackSky(x, y - 62) || !GBackSky(x, y - 66)) && x < 400)
		x++;
	CreateObject(WindGenerator, x + 4, y, owner);
	CreateObject(Flagpole, x - 24, y, owner);
	
	// Additional material includes a foundry and tools workshop.
	if (amount >= 2)
	{
		var foundry = CreateObject(Foundry, x + 38, y, owner);
		foundry->CreateContents(Coal, 4);
		foundry->CreateContents(Metal, 2);
		var workshop = CreateObject(ToolsWorkshop, x - 56, y, owner);
		workshop->CreateContents(Wood, 4);
		workshop->CreateContents(Metal, 2);
		var lorry = CreateObject(Lorry, x - 56, 160);
		
		// And even more material includes explosives and food.
		if (amount >= 3)
		{
			var chemicallab = CreateObject(ChemicalLab, x + 84, y, owner);
			chemicallab->CreateContents(Dynamite, 4);
			chemicallab->CreateContents(DynamiteBox, 4);
			workshop->CreateContents(Wood, 4);
			workshop->CreateContents(Metal, 2);
			workshop->CreateContents(Pickaxe, 1);
			lorry->CreateContents(Bread, 4);
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
		
	// On insane difficulty the water level keeps rising.
	if (difficulty >= 3)
		AddEffect("RisingWater", nil, 100, 1);
	return;
}

global func FxRisingWaterTimer(object target, proplist effect)
{
	// Insert pixel on the water surface if there is tunnel above.
	for (var x = Random(120); x < LandscapeWidth(); x += RandomX(80, 120))
	{
		// Find first tunnel from the bottom.
		var y = LandscapeHeight();
		while (GBackSemiSolid(x, y) && y > 0)
			y -= 1;
		// Check if there is liquid below the tunnel.
		if (GBackLiquid(x, y + 2))
			InsertMaterial(Material("Water"), x, y + 4);
	}	
	return FX_OK;
}

private func InitVegetation(int map_size)
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	
	// Cave mushrooms scattered around the top and middle sections.
	LargeCaveMushroom->Place(10, Rectangle(0, 0, wdt, hgt / 8), { terraform = false });
	LargeCaveMushroom->Place(15, Rectangle(0, hgt / 8, wdt, hgt / 8), { terraform = false });
	LargeCaveMushroom->Place(15, Rectangle(0, 2 * hgt / 8, wdt, hgt / 8), { terraform = false });
	
	// Cave entrance covered with mushrooms and bushes.
	SproutBerryBush->Place(4, Rectangle(0, 120, 100, 40));
	Fern->Place(4, Rectangle(0, 120, 100, 40));
	Mushroom->Place(8, Rectangle(0, 120, 100, 40));
	
	// The cavern has some grass wherever possible.
	PlaceGrass(100);
	
	// Entrance also location for a small cemetary.
	CreateObject(Column, 64, 160)->SetObjDrawTransform(400, 0, 0, 0, 400, 0);
	for (var x = 72; x < 104; x += RandomX(6, 14))
		CreateObject(Clonk_Grave, x, 160)->SetInscriptionMessage("R.I.P.");
	CreateObject(Column, 112, 160)->SetObjDrawTransform(400, 0, 0, 0, 400, 0);
	
	// Some ferns and mushrooms scattered around the top and middle sections.
	Fern->Place(12, Rectangle(0, 0, wdt, 3 * hgt / 8));
	Mushroom->Place(20, Rectangle(0, 0, wdt, 3 * hgt / 8));
	
	// Create earth materials in big clusters so the whole object arrangement looks a bit less uniform and more interesting.
	PlaceBatches([Firestone], 3, 100, 5);
	PlaceBatches([Rock, Loam, Loam], 10, 200, 10);
	
	// Place some underwater vegetation in the flooded caves.
	var place_rect = Rectangle(50, hgt / 2, wdt - 100, hgt / 2);
	Seaweed->Place(16 + 4 * map_size, place_rect);
	Coral->Place(16 + 8 * map_size, place_rect);
	return;
}

private func InitAnimals(int map_size, int difficulty)
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	
	// Place some fishes and piranhas as difficulty prescribes it.
	var place_rect = Rectangle(50, hgt / 2, wdt - 100, hgt / 2);
	var fish_count = 10 + 10 * map_size;
	Fish->Place(fish_count * (3 - difficulty), place_rect);
	Piranha->Place(fish_count * (difficulty - 1), place_rect);
	return;
}


/*-- Some helper functions --*/

private func PlaceBatches(array item_ids, int n_per_batch, int batch_radius, int n_batches)
{
	// place a number (n_batches) of batches of objects of types item_ids. Each batch has n_per_batch objects.
	// fewer batches and/or objects may be placed if no space is found
	var loc, loc2, n_item_ids = GetLength(item_ids), n_created = 0, obj;
	for (var i = 0; i < n_batches; ++i)
		if (loc = FindLocation(Loc_Material("Earth")))
			for (var j = 0; j < n_per_batch; ++j)
				if (loc2 = FindLocation(Loc_InRect(loc.x - batch_radius,loc.y - batch_radius, batch_radius * 2, batch_radius * 2), Loc_Material("Earth")))
					if (obj = CreateObject(item_ids[Random(n_item_ids)], loc2.x, loc2.y))
					{
						obj->SetPosition(loc2.x, loc2.y);
						++n_created;
					}
	return n_created;
}

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
