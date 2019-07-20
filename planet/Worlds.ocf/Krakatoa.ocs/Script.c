/**
	Krakatoa's Krach
	Players are challenged to build up a settlement on top of an active volcano
	after their airplane crashed. They need to gather gold from the core of the
	volcano to show your skills of entering a volcano. The gold bars need to be
	transported out by a newly constructed airplane.
	
	@author Maikel
*/


// Whether the intro has been initialized.
static intro_init;

protected func Initialize()
{
	// Show wealth in HUD.
	GUI_Controller->ShowWealth();
	
	// Goal: construct an airplane and fill it with gold bars.
	var goal = CreateObject(Goal_Script);
	// Add an effect to check whether the goal is fulfilled.
	var effect = AddEffect("GoalCheck", nil, 100, 2, nil);
	effect.goal = goal;
	effect.barcnt = 8 * SCENPAR_Difficulty;
	// Set goal name and description.	
	goal.Name = "$GoalName$";
	goal.Description = Format("$GoalDesc$", effect.barcnt);
	goal.Picture = Krakatoa_GoalIcon;
	goal.PictureName = Format("%d", SCENPAR_Difficulty);

	// Some rules.
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_BuyAtFlagpole);
	
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
	// Set zoom range.
	SetPlayerZoomByViewRange(plr, 500, nil, PLRZOOM_Direct | PLRZOOM_LimitMax);
	SetPlayerViewLock(plr, true);
	
	// Give the player its knowledge and base materials.
	GivePlayerBasicKnowledge(plr);
	GivePlayerPumpingKnowledge(plr);
	GivePlayerAdvancedKnowledge(plr);
	GivePlayerArtilleryKnowledge(plr);
	GivePlayerAirKnowledge(plr);

	// Give the player the elementary base materials.
	GivePlayerElementaryBaseMaterial(plr);

	// When rejoining, try joining at a flag. The default position may
	// otherwise be on the wrong side of the volcano.
	var flag = FindObject(Find_ID(Flagpole));
	
	// Give crew some equipment.
	var index = 0, crew;
	while (crew = GetCrew(plr, index++))
	{
		if (index == 1)
			crew->CreateContents(Hammer);
		if (index == 2)
			crew->CreateContents(Axe);
		crew->CreateContents(Shovel);
		if (flag)
			crew->SetPosition(flag->GetX(), flag->GetY());
	}

	// Initialize the intro sequence if not yet started.
	if (!intro_init)
	{
		StartSequence("Intro", 0, SCENPAR_Difficulty);
		intro_init = true;
		// Give only the first joined player some wealth.
		SetWealth(plr, 75 - 25 * SCENPAR_Difficulty);
	}
	return;
}


/*-- Goal Check --*/

global func FxGoalCheckTimer(object target, proplist effect)
{
	// Complete goal if there is an airplane with the required amount of gold bars.
	for (var plane in FindObjects(Find_ID(Airplane), Find_Not(Find_Func("IsBroken"))))
	{
		if (plane->ContentsCount(GoldBar) >= effect.barcnt)
		{
			if (effect.goal)
				effect.goal->Fulfill();
			return -1;
		}
	}
	return 1;
}


/*-- Scenario Initialization --*/

private func InitEnvironment(int difficulty)
{
	// Adjust the mood, orange sky, darker feeling in general.
	var dark = 10;
	SetSkyAdjust(RGB(150, 42, 0));
	SetGamma(100 - dark, 100 - dark, 100 - dark);
	
	// Time of days and celestials.
	var time = CreateObject(Time);
	time->SetTime(60 * 20);
	time->SetCycleSpeed(20);
		
	// Some dark clouds which rain few ashes.
	Cloud->Place(15);
	Cloud->SetPrecipitation("Ashes", 10 * difficulty);
	
	// Some natural disasters, earthquakes, volcanos, meteorites.
	Meteor->SetChance(2 + 4 * difficulty);
	if (difficulty >= 2)
		Earthquake->SetChance(6 * difficulty);

	// Initialize the effect for controlling the big volcano.
	var effect = AddEffect("BigVolcano", nil, 100, 5, nil);
	effect.difficulty = difficulty;
	return;
}

private func InitVegetation(int map_size)
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	
	// Place some trees, rather with leaves.
	var veg;
	for (var i = 0; i < 20 + Random(4); i++)
		PlaceVegetation(Tree_Coconut, 0, 0, wdt, hgt, 1000 * (61 + Random(40)));
	// Create an effect to make sure there will always grow some new trees.	
	AddEffect("EnsureTrees", nil, 100, 20, nil);
	// Some large cave mushrooms, equals amounts on both sides.
	LargeCaveMushroom->Place(12 + 4 * map_size, Shape->Rectangle(0, hgt / 2, wdt / 2, hgt / 2), { terraform = false });
	LargeCaveMushroom->Place(12 + 4 * map_size, Shape->Rectangle(wdt / 2, hgt / 2, wdt / 2, hgt / 2), { terraform = false });
	// Some dead tree trunks.
	for (var i = 0; i < 16 + Random(4); i++)
	{
		veg = PlaceVegetation(Trunk, 0, 0, LandscapeWidth(), hgt, 1000 * (61 + Random(20)));
		if (veg)
			veg->SetR(RandomX(-20, 20));
	}
	// Some mushrooms as source of food.
	Mushroom->Place(22 + Random(8));
	// Some ferns, to be burned soon.
	Fern->Place(25 + Random(5));
	// Branches as a nice additional source of wood.
	Branch->Place(30 + Random(8));
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
	return;
}

// Ensures that there will always grow some trees.
global func FxEnsureTreesTimer()
{
	// Place a tree if there are less than eight trees, with increasing likelihood for lower amounts of trees.
	var nr_trees = ObjectCount(Find_Func("IsTree"), Find_ID(Tree_Coconut));
	if (Random(9) >= nr_trees)
		if (!Random(20))
			PlaceVegetation(Tree_Coconut, 0, 0, LandscapeWidth(), LandscapeHeight(), 3);
	return FX_OK;
}

protected func OnGoalsFulfilled()
{
	// Give the remaining players their achievement.
	GainScenarioAchievement("Done", BoundBy(SCENPAR_Difficulty, 1, 3));
	return false;
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
		if (lava) InsertMaterial(Material("DuroLava"), lava.x, lava.y);
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
	Sound("Environment::Disasters::Earthquake", true, 100, nil, 1);
	// Shake also the viewport a bit on a big eruption.
	ShakeViewport(3200, x, y);
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
		var obj = CreateObjectAbove(LavaChunk, effect.X + RandomX(-5, 5), effect.Y, NO_OWNER);
		var ang = - 90 + RandomX(-angw / 2, angw / 2);
		var xdir = Cos(ang, lev) + RandomX(-3, 3);
		obj->SetR(Random(360));
		obj->SetXDir(xdir);
		obj->SetYDir(Sin(ang, lev) + RandomX(-3, 3));
		obj->SetRDir(-10 + Random(21));
		obj->DoCon(RandomX(-20, 40));
	}
	// Some lava glow and smoke through particles.
	return FX_OK;
}

global func FxBigEruptionStop(object target, proplist effect, int reason, bool temporary)
{
	if (temporary)
		return FX_OK;
	// Stop eruption sound.
	Sound("Environment::Disasters::Earthquake", true, 100, nil, -1);
	Sound("Environment::Disasters::EarthquakeEnd",true);
	return FX_OK;
}


/*-- Helper functions --*/

global func TestGoldCount()
{
	var pos;
	while (pos = FindLocation(Loc_Material("Gold")))
	{
		var pos = CreateObjectAbove(Rock, pos.x, pos.y)->Explode(100);
	}
	var gold_count = ObjectCount(Find_ID(Nugget));
	return gold_count;
}
