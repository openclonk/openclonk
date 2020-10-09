// Takes care of all settings for creating a map and initializing a player.

static Settings_MapWdt;
static Settings_MapHgt;
static Settings_MapType;
static Settings_MapClimate;
static Settings_Goal;
static Settings_GodMode;

static const CSETTING_MapSize_Min = 50;
static const CSETTING_MapSize_Max = 800;

static const CSETTING_MapType_Empty = 0;
static const CSETTING_MapType_MapTypeFlatland = 1;
static const CSETTING_MapType_MapTypeHills = 2;
static const CSETTING_MapType_MapTypeMountains = 3;

static const CSETTING_MapClimate_Temperate = 0;
static const CSETTING_MapClimate_Cold = 1;

static const CSETTING_Goal_Tutorial = 0;
static const CSETTING_Goal_Mining = 1;
static const CSETTING_Goal_Expansion = 2;

static const CSETTING_GodMode_Off = 0;
static const CSETTING_GodMode_Host = 1;
static const CSETTING_GodMode_All = 2;

global func InitMapSettings()
{
	if (Settings_MapWdt == nil || Settings_MapHgt == nil)
	{
		// Load map size from scenario parameters.
		// The shape of the map can be narrow[1:3], basic[4:3] or wide[3:1].
		// The size of the map can be small, normal, large.
		var size = 25 * ((SCENPAR_MapSize % 3) + 2);
		var shape = [[2, 6], [4, 3], [6, 2]][SCENPAR_MapSize / 3];
		Settings_MapWdt = size * shape[0];
		Settings_MapHgt = size * shape[1];
	}
	if (Settings_MapType == nil)
	{
		Settings_MapType = SCENPAR_MapType;	
	}
	if (Settings_MapClimate == nil)
	{
		Settings_MapClimate = SCENPAR_MapClimate;	
	}
	return;
}

global func InitGameSettings()
{
	InitGameGoals();
	InitGameRules();
	InitGameVegetation();
	InitGameEnvironment();
	InitGameAnimals();
	return;
}

global func InitGameGoals()
{
	if (Settings_Goal == nil)
		Settings_Goal = SCENPAR_Goal;
	// Create goal according to settings.
	if (Settings_Goal == CSETTING_Goal_Tutorial)
	{
		var goal = CreateObject(Goal_Tutorial);
		goal.Name = "$MsgGoalName$";
		goal.Description = "$MsgGoalDescription$";
	}
	else if (Settings_Goal == CSETTING_Goal_Mining)
	{
		var goal = CreateObject(Goal_ResourceExtraction);
		goal->SetResource("Gold");
		goal->SetResource("Ruby");
		goal->SetResource("Amethyst");
	}
	else if (Settings_Goal == CSETTING_Goal_Expansion)
	{
		var goal = CreateObject(Goal_Expansion);
		goal->SetExpansionGoal(600);
	}
	return;
}

global func InitGameRules()
{
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
	
	// Show wealth in HUD.
	GUI_Controller->ShowWealth();
	return;
}

global func InitGameVegetation()
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	var map_area_size = wdt * hgt / 20000;
	var map_surface_size = wdt / 50;
	
	// Place some vegetation.
	Grass->Place(100);
	Flower->Place(map_surface_size / 3);
	Mushroom->Place(map_surface_size / 3);
	Fern->Place(map_surface_size / 3);
	Cotton->Place(map_surface_size / 2);
	Wheat->Place(map_surface_size / 2);
	Vine->Place(map_surface_size / 4);
	Branch->Place(map_surface_size / 4);
	Trunk->Place(map_surface_size / 6);
	
	// Place some trees.
	Tree_Deciduous->Place(map_surface_size);
	Tree_Coniferous2->Place(map_surface_size);	
	LargeCaveMushroom->Place(map_surface_size / 2, nil, { terraform = false });
	
	// Some objects in the earth.	
	PlaceObjects(Rock, map_area_size, "Earth");
	PlaceObjects(Firestone, map_area_size, "Earth");
	PlaceObjects(Loam, map_area_size, "Earth");
	Diamond->Place(map_surface_size / 3, Rectangle(0, 0, wdt, hgt / 3), {cluster_size = 1});
	return;
}

global func InitGameEnvironment()
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	var map_area_size = wdt * hgt / 20000;
	var map_surface_size = wdt / 50;
	
	SetSkyParallax(0, 20, 20);
		
	// Time of days and celestials.
	var time = CreateObject(Time);
	time->SetTime(60 * 12);
	time->SetCycleSpeed(20);
		
	// Some dark clouds which rain few ashes.
	Cloud->Place(map_surface_size / 4);
	Cloud->SetPrecipitation("Water", 10);
	return;
}

global func InitGameAnimals()
{
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
	var map_area_size = wdt * hgt / 20000;
	var map_surface_size = wdt / 50;
	
	Wipf->Place(map_surface_size / 5, Rectangle(0, 0, wdt, 2 * hgt / 3));
	Bat->Place(map_area_size / 5, Rectangle(0, 2 * hgt / 3, wdt, hgt / 3));
	Mosquito->Place(map_surface_size / 10);
	Zaphive->Place(map_surface_size / 10);
	Butterfly->Place(map_surface_size / 5);
	return;
}

global func InitPlayerSettings(int plr)
{
	if (Settings_GodMode == nil)
		Settings_GodMode = SCENPAR_GodMode;
	if (Settings_GodMode == CSETTING_GodMode_All || (Settings_GodMode == CSETTING_GodMode_Host && IsFirstPlayer(plr)))
		GiveGodMode(plr);
	return;
}

global func IsFirstPlayer(int plr)
{
	var lowest_plr = 10**6;
	for (var check_plr in GetPlayers(C4PT_User))
		lowest_plr = Min(lowest_plr, check_plr);
	return plr == lowest_plr;
}
