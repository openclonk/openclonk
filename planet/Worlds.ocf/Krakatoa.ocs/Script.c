/**
	Krakatoa
	Players are challenged to build up a settlement on top of an active volcano.
	The goal is to expand your reign by building flags to cover the landscape.
	Also you need to gather some gold to show your skills of entering a volcano.
	
	@author Maikel
*/


static volcano_location;
static plr_init;

protected func Initialize()
{
	// Create expansion and wealth goal.
	var goal = CreateObject(Goal_Wealth);
	goal->SetWealthGoal(100 + 100 * SCENPAR_Difficulty);
	goal = CreateObject(Goal_Expansion);
	goal->SetExpansionGoal(200 + 50 * SCENPAR_Difficulty);

	// Some rules.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	CreateObject(Rule_StructureHPBars);
	
	// Rescale chasm exits.
	var map_zoom = GetScenarioVal("MapZoom", "Landscape");
	for (var i = 0; i < 5; i++)
		for (var j = 0; j < 2; j++)
			chasm_exits[i][j] *= map_zoom;
		
	// Initialize different parts of the scenario.
	InitEnvironment(SCENPAR_Difficulty);
	InitVegetation(SCENPAR_MapSize);
	InitAnimals();
	InitMaterial(4 - SCENPAR_Difficulty);	
	return;
}


/*-- Player Initialization --*/

protected func InitializePlayer(int plr)
{
	// Move all crew to start position.
	var index = 0, crew;
	while (crew = GetCrew(plr, index++))
		crew->SetPosition(volcano_location[0] + RandomX(-5, 5), volcano_location[1]);
	// Give only the first joined player some wealth.
	if (!plr_init)
	{
		SetWealth(plr, 50);
		plr_init = true;
	}
	// Give the player its knowledge and base materials.
	GivePlayerBasicKnowledge(plr);
	GivePlayerPumpingKnowledge(plr);
	GivePlayerAdvancedKnowledge(plr);
	GivePlayerArtilleryKnowledge(plr);
	GivePlayerAirKnowledge(plr);

	// Give the player the elementary base materials.
	GivePlayerElementaryBaseMaterial(plr);
	
	// Give crew some equipment.
	var index = 0;
	while (crew = GetCrew(plr, index++))
	{
		if (index == 1)
			crew->CreateContents(Hammer);
		if (index == 2)
			crew->CreateContents(Axe);
		crew->CreateContents(Shovel);
	}
	// Harsh zoom range.
	SetPlayerZoomByViewRange(plr, 5000, nil, PLRZOOM_Direct | PLRZOOM_LimitMax);
	SetPlayerViewLock(plr, true);
	return;
}

// Returns a suitable location to start the conquest.
private func FindVolcanoLocation()
{
	// Default to the middle of the map.
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	volcano_location = [wdt / 2, hgt / 2];
	var x, y, cnt = 1000;
	for (var i = cnt; i > 0; i--)
	{
		// Random x coordinate, biased to the middle of the map.
		var var_wdt = wdt * (400 - 200 * i / cnt) / 400;
		var x = wdt / 2 + RandomX(-var_wdt, var_wdt);
		var y = 0;
		// Find corresponding y coordinate.
		while (!GBackSolid(x, y) && y < 9 * hgt / 10)
			y += 2;
		// Check if surface is relatively flat (check for flatter surfaces first).
		var d = i / 40 + 1;
		if (!GBackSolid(x+d, y-4) && !GBackSolid(x-d, y-4) && GBackSolid(x+d, y+4) && GBackSolid(x-d, y+4))
		{
			volcano_location = [x, y - 10];
			break;
		}
	}
	return;
}


/*-- Scenario Initialization --*/

private func InitEnvironment(int difficulty)
{
	// Adjust the mood, orange sky, darker feeling in general.
	var dark = 10;
	SetSkyAdjust(RGB(150, 42, 0));
	SetGamma(RGB(0, 0, 0), RGB(128 - dark, 128 - dark, 128 - dark), RGB(255 - 2 * dark, 255 - 2 * dark, 255 - 2 * dark));
	
	// Time of days and celestials.
	CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(60 * 20);
	time->SetCycleSpeed(20);
		
	// Some dark clouds which rain few ashes.
	Cloud->Place(15);
	Cloud->SetPrecipitation("Ashes", 10);
	
	// Some natural disasters, earthquakes, volcanos, meteorites.
	Meteor->SetChance(2 + 4 * difficulty);
	if (difficulty >= 2)
		Earthquake->SetChance(3 * difficulty);

	// Initialize the effect for controlling the big volcano.
	var effect = AddEffect("BigVolcano", nil, 100, 5, nil);
	effect.difficulty = difficulty;
	return;
}

private func InitVegetation(int map_size)
{
	// Place some trees, rather with leaves.
	var veg;
	for (var i = 0; i < 20 + Random(4); i++)
		PlaceVegetation(Tree_Coconut, 0, 0, LandscapeWidth(), LandscapeHeight(), 1000 * (61 + Random(40)));
	// Create an effect to make sure there will always grow some new trees.	
	AddEffect("EnsureTrees", nil, 100, 20, nil);
	// Some large cave mushrooms.
	LargeCaveMushroom->Place(20 + 4 * map_size, Rectangle(0, LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight() / 2), { terraform = false });
	// Some dead tree trunks.
	for (var i = 0; i < 16 + Random(4); i++)
	{
		veg = PlaceVegetation(Trunk, 0, 0, LandscapeWidth(), LandscapeHeight(), 1000 * (61 + Random(20)));
		if (veg)
			veg->SetR(RandomX(-20, 20));
	}
	// Some mushrooms as source of food.
	for (var i = 0; i < 30 + Random(5); i++)
		PlaceVegetation(Mushroom, 0, 0, LandscapeWidth(), LandscapeHeight());
	// Some ferns, to be burned soon.
	for (var i = 0; i < 25 + Random(5); i++)
		PlaceVegetation(Fern, 0, 0, LandscapeWidth(), LandscapeHeight());
	// Ranks as a nice additional source of wood.
	for (var i = 0; i < 16 + Random(4); i++)
	{
		veg = PlaceVegetation(Rank, 0, 0, LandscapeWidth(), LandscapeHeight());
		if (veg)
			veg->SetR(RandomX(-20, 20));
	}
	// Some objects in the earth.	
	PlaceObjects(Rock, 30 + 15 * map_size + Random(10),"Earth");
	PlaceObjects(Firestone, 40 + 15 * map_size + Random(5), "Earth");
	PlaceObjects(Loam, 30 + 15 * map_size + Random(5), "Earth");
	return;
}

private func InitAnimals()
{
	return;
}

private func InitMaterial(int amount)
{
	// Find start location and place lorry plus extras there.
	FindVolcanoLocation();
	var lorry = CreateObject(Lorry);
	lorry->SetPosition(volcano_location[0], volcano_location[1]);
	lorry->CreateContents(Loam, 5);
	lorry->CreateContents(Bread, 5);
	lorry->CreateContents(Wood, 8);
	lorry->CreateContents(Rock, 4);
	lorry->CreateContents(Metal, 4);
	for (var i = 0; i < 5; i++)
		lorry->CreateContents(Barrel)->PutLiquid("Water", 300);
	return;
}

// Ensures that there will always grow some trees.
global func FxEnsureTreesTimer()
{
	// Place a tree if there are less than eight trees, with increasing likelihood for lower amounts of trees.
	var nr_trees = ObjectCount(Find_Func("IsTree"));
	if (Random(9) >= nr_trees)
		if (!Random(20))
			PlaceVegetation(Tree_Coconut, 0, 0, LandscapeWidth(), LandscapeHeight(), 3);
	return FX_OK;
}


/*-- Volcano Effect --*/

// The volcano will do two types of eruptions:
// Smaller ones where just some lava is flowing out of the chasms.
// Bigger ones with chunks, ashes, rocks, explosions, etc.
global func FxBigVolcanoStart(object target, proplist effect, int temporary)
{
	if (temporary)
		return FX_OK;
	// Ensure right effect interval.
	effect.Interval = 5;
	
	for (var pos in chasm_exits)
	 	CreateObject(Flagpole, pos[0], pos[1]);
	
	return FX_OK;
}

global func FxBigVolcanoTimer(object target, proplist effect)
{
	// Insert some lava in the big body of lava in the core of the volcano.
	// Find a surface on which we can release some lava pixels and bubbles.
	/*for (var x = Random(250); x < LandscapeWidth(); x += RandomX(200, 300))
	{
		// Find first tunnel from the bottom.
		var y = LandscapeHeight();
		while (GBackSemiSolid(x, y) && y > 0)
			y -= 2;
		// Check if there is liquid below the tunnel.
		if (GBackLiquid(x, y + 4))
		{
			InsertMaterial(Material("DuroLava"), x, y + 4);
		}	
	}*/
	
	// Some bubles in the lava.
	// TODO
	
	// Some small fountains at random locations in the different chasms.
	for (var i = 0; i < 5; i++)
	{
		var pos = chasm_exits[i];
		var lava = FindLocation(Loc_Material("DuroLava"), Loc_InRect(pos[0] - 100, pos[1] - 100, 200, 200));
		InsertMaterial(Material("DuroLava"), lava.x, lava.y);
	}
	
	// At more rare occasions there will be a bigger eruption with chunks.
	if (!Random(1200 - 200 * effect.difficulty) && false)
	{
		// Find the location to erupt from the chasm_exit.
		var pos = chasm_exits[0];
		var x = pos[0];
		var y = pos[1];
		var at_lava = GBackLiquid(x, y);
		if (at_lava)
			while (GBackLiquid(x, y) && y > 0)
				y--;
		else
			while (!GBackLiquid(x, y) && y < LandscapeHeight())
				y++;

		// Check if there is enough room for an eruption.
		if (GBackLiquid(x - 5, y + 8) && GBackLiquid(x + 5, y + 8) && !GBackSemiSolid(x - 5, y - 8) && !GBackSemiSolid(x + 5, y - 8))
		{
			// Launch a big eruption from this location.
			AddEffect("BigEruption", nil, 100, 1, nil, nil, x, y - 2);
		}
	}
	return FX_OK;
}

global func FxBigEruptionStart(object target, proplist effect, int temporary, int x, int y)
{
	if (temporary)
		return FX_OK;
	// Ensure right effect interval.
	effect.Interval = 1;
	// Take over launch coordinates.
	effect.X = x;
	effect.Y = y;
	// Duration of 6-9 seconds.
	effect.Duration = (6 + Random(4)) * 36;
	// Use earthquake sound for this eruption.
	Sound("Earthquake", true, 100, nil, 1);
	// Shake also the viewport a bit on a big eruption.
	ShakeViewPort(3200, x, y);
	return FX_OK;
}

global func FxBigEruptionTimer(object target, proplist effect, int time)
{
	// Eruption lasts for some time.
	if (time > effect.Duration)
		return FX_Execute_Kill;
	// Cast some lava and ashes pixels at surface.
	for (var i = 0; i < 3; i++)
	{
		var x = effect.X + RandomX(-80, 80);
		var y = effect.Y + 5;
		while (GBackLiquid(x, y))
			y--;
		CastPXS("DuroLava", 2, 120 + Random(40), x, y - 2, 0, 40);
		if (!Random(4))
		{
			CastPXS("Ashes", 2, 120 + Random(40), x, y - 2, 0, 40);
			Smoke(x, y, 8 + Random(4));	
		}
	}
	// Throw around some lava chunks.
	if (!Random(6))
	{
		var angw = 40, lev = 120;
		var obj = CreateObject(LavaChunk, effect.X + RandomX(-5, 5), effect.Y, NO_OWNER);
		var ang = - 90 + RandomX(-angw / 2, angw / 2);
		var xdir = Cos(ang, lev) + RandomX(-3, 3);
		obj->SetR(Random(360));
		obj->SetXDir(xdir);
		obj->SetYDir(Sin(ang, lev) + RandomX(-3, 3));
		obj->SetRDir(-10 + Random(21));
		obj->DoCon(RandomX(-20,40));
	}
	// Some lava glow and smoke through particles.
	return FX_OK;
}

global func FxBigEruptionStop(object target, proplist effect, int reason, bool temporary)
{
	if (temporary)
		return FX_OK;
	// Stop eruption sound.
	Sound("Earthquake", true, 100, nil, -1);
	Sound("EarthquakeEnd",true);
	return FX_OK;
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
