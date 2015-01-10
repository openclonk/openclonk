/**
	Gem Grabbers
	Players need to grab gems from non trivial locations in a landscape with many sky islands.
	There are many tools either available directly or for production on the other islands. 
	
	Todo's:
	* Rain and wind effects.
	* Attacking birds on the islands with new buildings.
	* Some other hurdles.
	* Decoration on all the islands.
		
	@author Maikel
*/


protected func Initialize()
{
	// Rules: team account and buying at flagpole.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	
	// Goal: Sell Gems, amount depends on difficulty and initial availability.
	var gems = (4 * GetMaterialCount(Material("Ruby"))) / (5 * GetMaterialVal("Blast2ObjectRatio", "Material", Material("Ruby")));
	gems += (4 * GetMaterialCount(Material("Amethyst"))) / (5 * GetMaterialVal("Blast2ObjectRatio", "Material", Material("Amethyst")));
	var percentage = 55 + 15 * SCENPAR_Difficulty;
	var goal = CreateObject(Goal_SellGems);
	goal->SetTargetAmount((gems * percentage) / 100);
	
	// Initialize different parts of the scenario.
	InitEnvironment(SCENPAR_Difficulty);
	InitVegetation();
	InitAnimals();
	InitResources(SCENPAR_Difficulty);
	InitMainIsland(4 - SCENPAR_Difficulty);
	InitIslands(4 - SCENPAR_Difficulty);
		
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
	
	// Position and materials.
	var i, crew;
	for (i = 0; crew = GetCrew(plr, i); ++i)
	{
		var pos = FindMainIslandPosition();
		crew->SetPosition(pos[0], pos[1] - 11);
		crew->CreateContents(Shovel);
	}
	
	// Give the player its knowledge.
	GivePlayerBasicKnowledge(plr);
	GivePlayerPumpingKnowledge(plr);
	GivePlayerWeaponryKnowledge(plr);
	GivePlayerAdvancedKnowledge(plr);
	GivePlayerAirKnowledge(plr);
	RemovePlayerSpecificKnowledge(plr, [InventorsLab, Shipyard, WallKit]);
	
	// Only clonks for sale at the homebase, depending on diffuculty: 3, 5, 10 available.
	var nr_clonks = Max(9 - 2 * SCENPAR_Difficulty, 1);
	if (SCENPAR_Difficulty == 1)
		nr_clonks += 3;
	SetBaseMaterial(plr, Clonk, nr_clonks);
	SetBaseProduction(plr, Clonk, nr_clonks);
	
	// Claim ownership of structures, last player who joins owns all the main island flags.
	for (var structure in FindObjects(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole"))))
		structure->SetOwner(plr);

	return;
}


/*-- Scenario Initialization --*/

// Initializes environment and disasters.
private func InitEnvironment(int difficulty)
{
	// Set time to almost night and have stars.	
	CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(20 * 60 + 15);
	time->SetCycleSpeed(0);
	
	// Clouds and rain.
	Cloud->Place(15);
	Cloud->SetPrecipitation("Water", 100 + 25 * difficulty);
	for (var cloud in FindObjects(Find_ID(Cloud)))
	{
		// Make some clouds appear in the foreground with high alpha.
		if (Random(3) == 0)
		{
			cloud.Plane = 600;
			cloud->SetCloudAlpha(40);
		}			
	}
	
	// Set a certain parallax.
	SetSkyParallax(0, 20, 20);
	
	// Disasters: meteors and lightning.
	Meteor->SetChance(2 * difficulty);
	Cloud->SetLightning(8 * difficulty);
	
	return;
}

// Initializes grass, trees and in-earth objects.
private func InitVegetation()
{
	// Grass on all islands.
	PlaceGrass(85);
	
	// Place some cocont trees all around the island and some extra trees on the main island.
	for (var i = 0; i < 40 + Random(8); i++)
		PlaceVegetation(Tree_Coconut, 0, 0, LandscapeWidth(), LandscapeHeight(), 1000 * (61 + Random(40)));
	for (var i = 0; i < 6 + Random(2); i++)
		PlaceVegetation(Tree_Coconut, LandscapeWidth() - 300, LandscapeHeight() - 200, 600, 400, 1000 * (71 + Random(30)));
		
	// Create an effect to make sure there will always grow some new trees.	
	AddEffect("EnsureTreesOnMainIsland", nil, 100, 20, nil);
	
	// Some objects in the earth.	
	PlaceObjects(Rock, 35 + Random(10), "Earth");
	PlaceObjects(Firestone, 25 + Random(5), "Earth");
	
	return;
}

// Ensures that there will always grow some trees on the main island.
global func FxEnsureTreesOnMainIslandTimer()
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	// Place a tree if there are less than eight trees, with increasing likelihood for lower amounts of trees.
	var nr_trees = ObjectCount(Find_Func("IsTree"), Find_InRect(wdt / 2 - 300, hgt / 2 - 200, 600, 400));
	if (Random(9) >= nr_trees)
		if (!Random(20))
			PlaceVegetation(Tree_Coconut, wdt / 2 - 300, hgt / 2 - 200, 600, 400, 3);
	return FX_OK;
}

// Initializes animals.
private func InitAnimals()
{

	return;
}

// Initializes the growth of gem stalactites.
private func InitResources(int difficulty)
{
	// Add an effect to ensure gem stalactites are grown when the player seems unable to complete the goal.
	var effect = AddEffect("GrowGemStalactites", nil, 100, 60 * 36, nil, nil, difficulty);
	effect.difficulty = difficulty;
	return;
}

// Ensures that gem stalactites grow if the player has too few gems available.
global func FxGrowGemStalactitesTimer(object target, proplist effect, int time)
{
	// Get the number of gems available and compare with the goal.
	var gems = (GetMaterialCount(Material("Ruby")) + GetMaterialCount(Material("Amethyst"))) / 125;
	gems += ObjectCount(Find_Or(Find_ID(Ruby), Find_ID(Amethyst)));
	var goal = FindObject(Find_ID(Goal_SellGems));
	if (!goal)
		return 1;
	// The comparison depends on the difficulty settings.	
	if (gems - goal->GetTargetAmount() > 5 * (4 - effect.difficulty))
		return 1;	
		
	// Find a location to grow a stalactite, possible away from others and clonks.
	var pos, good_pos;
	for (var attempts = 0; attempts < 30; attempts++)
	{
		good_pos = false;
		var dist = 64; // distance from border
		pos = FindLocation(Loc_Sky(), Loc_Wall(CNAT_Top), Loc_Space(8), Loc_InRect(dist, dist, LandscapeWidth() - 2 * dist, LandscapeHeight() - 2 * dist));
		if (!pos)
			continue;
			
		// Check for an even solid ceiling.
		var x = pos.x, y = pos.y;
		while ((!GBackSemiSolid(x - 7, y) || !GBackSemiSolid(x + 7, y)) && y > pos.y - 5)
			y--;
		if (y <= pos.y - 5)
			continue;
		
		// Check for other stalactites and crew members.
		var dist = 300;		
		if (!!FindLocation(Loc_Or(Loc_Material("Ruby"), Loc_Material("Amethyst")), Loc_InRect(pos.x - dist, pos.y - dist, 2 * dist, 2 * dist)))
			continue;		
		if (!!FindObject(Find_OCF(OCF_CrewMember), Find_Distance(dist, pos.x, pos.y)))
			continue;

		// Found a good location.
		good_pos = true;
		break;
	}
	
	if (!good_pos)
		return 1;
		
	// Add growing stalactite effect.
	effect = AddEffect("GrowStalactite", nil, 100, 8, nil);
	effect.Material = "Ruby";
	if (Random(2))
		effect.Material = "Amethyst";
	effect.X = pos.x;
	effect.Y = pos.y;
	effect.Size = 36;

	return 1;
}

// Grows a single stalactite.
global func FxGrowStalactiteTimer(object target, proplist effect, int time)
{
	if (effect.Size <= 1)
		return -1;

	var width = 4 * Min(effect.Size, 24) / 3 + RandomX(3, 5);
	
	for (var x = effect.X - width / 2; x <= effect.X + width / 2; x++)
	{
		var cnt = 0;
		var y = effect.Y;
		while (!GBackSemiSolid(x, y) && ++cnt < 10)
		{
			DrawMaterialQuad(effect.Material, x, y, x + 1, y, x + 1, y + 1, x, y + 1, true);
			y--;
		}
	}
	
	effect.Y += Random(2) + 1;
	effect.X += RandomX(-1, 1);
	effect.Size--;
}

// Initializes the main island according to the material specification.
private func InitMainIsland(int amount)
{
	amount = BoundBy(amount, 1, 3);
	var pos;
	
	// Always start with a lorry filled with: hammer(x2), axe(x2), wood(x6) and metal(x4) and.
	// The boompack is also always there to have the opportunity to get one clonk on another
	// sky island and start exploring.
	var lorry_pos = FindMainIslandPosition(0, 80);
	var lorry = CreateObjectAbove(Lorry, lorry_pos[0], lorry_pos[1] - 8);
	lorry->CreateContents(Hammer, 2);
	lorry->CreateContents(Axe, 2);
	lorry->CreateContents(Wood, 6);
	lorry->CreateContents(Metal, 4);
	lorry->CreateContents(Boompack, 1);
	
	// If more material is specified, create a small settlement: flag(x2) and windmill.
	// Also fill lorry a bit more with: pickaxe(x1), dynamite(x4), wood(x4), metal(x2).
	if (amount >= 2)
	{
		pos = FindMainIslandPosition(-120, 20);
		CreateObjectAbove(Flagpole, pos[0], pos[1]);
		pos = FindMainIslandPosition(120, 20);
		CreateObjectAbove(Flagpole, pos[0], pos[1]);
		pos = FindMainIslandPosition(nil, nil, true);
		CreateObjectAbove(WindGenerator, pos[0], pos[1]);
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
		CreateObjectAbove(Sawmill, pos[0], pos[1]);
		pos = FindMainIslandPosition(nil, nil, true);
		CreateObjectAbove(ChemicalLab, pos[0], pos[1]);
		pos = FindMainIslandPosition(nil, nil, true);
		CreateObjectAbove(ToolsWorkshop, pos[0], pos[1]);
	
		lorry->CreateContents(Barrel, 1);
		lorry->CreateContents(Bucket, 1);
		lorry->CreateContents(Loam, 4);
		lorry->CreateContents(DynamiteBox, 1);	
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
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();

	for (var i = 0; i < 100; i++)
	{
		var x = RandomX(wdt / 2 + xpos - sep, wdt / 2 + xpos + sep);
		var y = hgt / 2 - 220;
		
		while (!GBackSolid(x, y) && y < 3 * hgt / 4)
			y++;	
			
		if (!no_struct || !FindObject(Find_Or(Find_Category(C4D_Structure), Find_Func("IsFlagpole")), Find_Distance(60, x, y)))
			break;
	}

	return [x, y];
}

// Initializes the smaller islands according to the material specification.
private func InitIslands(int amount)
{
	amount = BoundBy(amount, 1, 3);
	
	// Locate all islands and store them in an array.
	var islands = FindIslands();
	
	// Debug: Draw the island rects.
	//for (var island in islands)
	//	DrawBoundingBox(island[0], island[1], island[2], island[3]);
		
	// Add some structures, materials and vegetation to each island depending on material specification.
	var island_nr = 1;
	for (var island in islands)
		island_nr += ProvideIsland(island, island_nr, amount);

	return;
}

// Returns an array with rectangles enclosing each of the islands.
private func FindIslands()
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	var main = [wdt / 2 - 500, hgt / 2 - 225, 1000, 600];
	var spot = FindLocation(Loc_Solid(), Loc_Not(Loc_InRect(main[0], main[1], main[2], main[3])));
	var islands = []; var island_cond;
	// Search for islands iteratively excluding the rectangle of the last found island.
	do 
	{
		var island = MakeIslandRect(spot.x, spot.y);
		islands[GetLength(islands)] = island;
		island_cond = Loc_Not(Loc_InRect(island[0], island[1], island[2], island[3]));
		for (var i = 0; i < GetLength(islands) - 1; i++)
		{
			var island = islands[i];
			island_cond = Loc_And(island_cond, Loc_Not(Loc_InRect(island[0], island[1], island[2], island[3])));
		}	
	}
	while (spot = FindLocation(Loc_Solid(), Loc_Not(Loc_InRect(main[0], main[1], main[2], main[3])), island_cond));
	
	return islands;
}

private func MakeIslandRect(int x, int y)
{
	var x1 = x, x2 = x;
	var y1 = y, y2 = y;
	var lwdt = LandscapeWidth();
	var lhgt = LandscapeHeight();
	
	// find some random spots until one finds the boundaries.
	for (var i = 0; i < 250; i++)
	{
		var range = 60;
		var spot = FindLocation(Loc_Solid(), Loc_InRect(Max(0, x - range), Max(0, y - range), Min(2 * range, lwdt - x + range), Min(2 * range, lhgt - y + range)));
		if (!spot)
			continue;
		x = spot.x; y = spot.y;
		if (x < x1) x1 = x;
		if (x > x2) x2 = x;
		if (y < y1) y1 = y;
		if (y > y2) y2 = y;		
	}
	
	// Make the rectangle somewhat bigger.
	var extra_size = 80;
	var x1 = Max(0, x1 - extra_size);
	var y1 = Max(0, y1 - extra_size);
	var wdt = x2 - x1 + extra_size; wdt = Min(wdt, lwdt - x1);
	var hgt = y2 - y1 + extra_size; hgt = Min(hgt, lhgt - y1);
		
	var rect = [x1, y1, wdt, hgt];
	return rect;
}

// Provides some stuff for each of the 6-9 other islands, dependent on the material settings.
private func ProvideIsland(array island, int number, int amount)
{
	// All of the islands have a few in-earth loam pieces.
	PlaceObjects(Loam, amount + RandomX(1, 3), "Earth", island[0], island[1], island[2], island[3]);
	
	var spot = FindLocation(Loc_InRect(island[0], island[1], island[2], island[3] / 2), Loc_Wall(CNAT_Bottom), Loc_Space(20), Loc_Sky());
	if (!spot)
		return 0;		
	
	// An inventors lab without power supply for the first island. With construction 
	// materials for Boompack, balloon, windbag and teleglove dependent on material settings.
	if (number == 1)
	{
		var lab = CreateObjectAbove(InventorsLab, spot.x, spot.y);
		lab->CreateContents(Wood, 2 * amount);	
		lab->CreateContents(Metal, 2 * amount);	
		lab->CreateContents(Cloth, amount);	
		lab->CreateContents(PowderKeg, amount - 1);
		lab->CreateContents(Firestone, amount - 1);
		lab->MakeInvincible();
	}
	
	// A shipyard with material for a airship, but without power supply for the second island.
	// If the player finds this island and manages to construct a windmill he can escape.
	if (number == 2)
	{
		var shipyard = CreateObjectAbove(Shipyard, spot.x, spot.y);
		shipyard->CreateContents(Wood, 4);
		shipyard->CreateContents(Metal, 2 * amount);
		shipyard->MakeInvincible();
	}
	
	// A cannon with a powder keg for the third island and place some metal & wood.
	if (number == 3)
	{
		var cannon = CreateObjectAbove(Cannon, spot.x, spot.y);
		cannon->CreateContents(PowderKeg);
		PlaceObjects(Wood, amount + Random(2), "Earth", island[0], island[1], island[2], island[3]);
		PlaceObjects(Metal, amount + Random(2), "Earth", island[0], island[1], island[2], island[3]);
	}
	
	// A catapult for the fourth island and place some metal & wood.
	if (number == 4)
	{
		SproutBerryBush->Place(Random(amount + 1), Rectangle(island[0], island[1] - 80, island[2], island[3] / 2));
		CreateObjectAbove(Catapult, spot.x, spot.y);
		PlaceObjects(Wood, amount + Random(2), "Earth", island[0], island[1], island[2], island[3]);
		PlaceObjects(Metal, amount + Random(2), "Earth", island[0], island[1], island[2], island[3]);
	}
	
	// For the other islands a lorry with explosives, gold bars and loam.
	// Place some sproutberries as well.
	if (number >= 5)
	{
		var lorry = CreateObjectAbove(Lorry, spot.x, spot.y);
		lorry->CreateContents(Loam, 2 + amount);
		lorry->CreateContents(DynamiteBox, amount);
		lorry->CreateContents(Dynamite, 4);
		lorry->CreateContents(GoldBar, Random(amount + 1));
		SproutBerryBush->Place(amount + Random(2), Rectangle(island[0], island[1] - 80, island[2], island[3] / 2));
	}
	
	// For all the islands some decoration.
	if (!Random(3))
	{
		var spot = FindLocation(Loc_InRect(island[0], island[1], island[2], island[3] / 2), Loc_Wall(CNAT_Bottom), Loc_Space(20), Loc_Sky());
		if (spot)
			CreateObjectAbove(Column, spot.x, spot.y);
	}	

	return 1;
}


/*-- Some helper functions --*/

global func TestGemCount()
{
	var pos;
	while (pos = FindLocation(Loc_Or(Loc_Material("Ruby"), Loc_Material("Amethyst"))))
	{
		var pos = CreateObjectAbove(Rock, pos.x, pos.y)->Explode(100);
	}
	var gem_count = ObjectCount(Find_Or(Find_ID(Ruby), Find_ID(Amethyst)));
	return gem_count;
}

global func DrawBoundingBox(int x, int y, int wdt, int hgt)
{
	// Draw top and bottom lines.
	DrawMaterialQuad("Brick", x, y + 1, x + wdt, y + 1, x + wdt, y - 1, x, y - 1);
	DrawMaterialQuad("Brick", x, y + hgt + 1, x + wdt, y + hgt + 1, x + wdt, y + hgt - 1, x, y + hgt - 1);
	
	// Draw right and left lines.
	DrawMaterialQuad("Brick", x - 1, y, x + 1, y, x + 1, y + hgt, x - 1, y + hgt);
	DrawMaterialQuad("Brick", x + wdt - 1, y, x + wdt + 1, y, x + wdt + 1, y + hgt, x + wdt - 1, y + hgt);	
	
	return;
}
