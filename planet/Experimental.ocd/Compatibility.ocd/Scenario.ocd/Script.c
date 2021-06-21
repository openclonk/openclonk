/**
	Compatibility scenario script

	Recreates the behaviour of old scenarios in script.

	Process:
	1) Include the script in the scenario
	2) Copy the deleted properties from Scenario.txt to Script.c.
	   - lists of digits have to be defined as an array
	   - lists of IDs have to be defined as a proplist
	   The removed properties are:
	   -
	
	Motivation:
	Why would you remove the properties from Scenario.txt if you offer a
	script that does the exact same thing?
	- You can convert old scenarios easily
	- You get an example how you could implement the scenario in the current version
	- You do not have to carry outdated properties and non-customizable rules/behavior
	  in the engine, which makes the engine code easier to maintain and edit
	- Scenarios can be customized more easily by not behaving according to the old
	  rules by default.

	@author Marky
 */
 
/* --- Properties --- */

static const PlayerProfile = new Global
{
	Wealth = [0, 0, 0, 250],
	Crew = {Clonk = 1},
	BaseMaterial = {},
	BaseProduction = {},
	Material = {},
	Vehicles = {},
};

local Player1 = nil;
local Player2 = nil;
local Player3 = nil;
local Player4 = nil;

local Vegetation = {};               // Vegetation types and ratio.
local VegetationLevel = [50, 30, 0, 100];
local InEarth = {};
local InEarthLevel = [50, 0, 0, 100];
local Animals = {};
local Nest = {};
local Objects = {}; // Environment control objects that are placed at game start.

/* --- Functions --- */

func Initialize()
{
	_inherited(...);
	
	InitVegetation();
	InitInEarth();
	InitAnimals();
	InitEnvironment();
	InitRules();
	InitGoals();
}

func InitVegetation()
{
	// Get an array with the correct ratios
	var vegetation = GetAsList(Vegetation);
	
	for (var amount = CalcVegetationAmount(RandomRange(VegetationLevel[0], VegetationLevel[1], VegetationLevel[2], VegetationLevel[3])); amount > 0; --amount)
	{
		PlaceVegetation(RandomElement(vegetation));
	}
}

func InitInEarth()
{
	// Get an array with the correct ratios
	var in_earth = GetAsList(InEarth);

	for (var amount = CalcInEarthAmount(RandomRange(InEarthLevel[0], InEarthLevel[1], InEarthLevel[2], InEarthLevel[3])); amount > 0; --amount)
	{
		PlaceObjects(RandomElement(in_earth), 1);
	}
}

func InitAnimals()
{
	// Place animals
	for (var prop in GetProperties(Animals))
	{
		var type = GetDefinition(prop);
		var amount = Animals[prop];
		if (type)
		{
			type->~Place(amount);
		}
	}
	
	// Place nests
	for (var prop in GetProperties(Nest))
	{
		var type = GetDefinition(prop);
		var amount = Nest[prop];
		if (type)
		{
			PlaceObjects(type, amount);
		}
	}
}

func InitEnvironment()
{
	// Place environment objects
	for (var type in GetAsList(Objects))
	{
		CreateObject(type, 0, 0, NO_OWNER);
	}
}

func InitRules()
{
}

func InitGoals()
{
}

func GetAsList(proplist props)
{
	var list = [];
	for (var prop in GetProperties(props))
	{
		var type = GetDefinition(prop);
		if (!type)
		{
			continue;
		}
		for (var amount = props[prop]; amount > 0; --amount)
		{
			PushBack(list, type);
		}
	}
	return list;
}

func InitializePlayer(int player_nr, int x, int y, object base, int team, id extra_data)
{
	var profiles = [Player1, Player2, Player3, Player4];
	RemoveHoles(profiles);
	if (!GetLength(profiles))
	{
		profiles = [new PlayerProfile {}];
	}
	
	var settings;
	var profiles_num = GetLength(profiles);
	if (profiles_num > 1)
	{
		settings = profiles[player_nr % profiles_num];
	}
	else
	{
		settings = profiles[0];
	}

	// Wealth, home base materials, abilities
	player_nr->SetWealth(RandomRange(settings.Wealth[0], settings.Wealth[1], settings.Wealth[2], settings.Wealth[3]));
	for (var material in GetAsList(settings.BaseMaterial))
	{
		DoBaseMaterial(player_nr, material, 1);
	}
	for (var material in GetAsList(settings.BaseProduction))
	{
		DoBaseProduction(player_nr, material, 1);
	}
	
	// Material
	for (var item in GetAsList(settings.Material))
	{
		if (base)
		{
			base->CreateContents(item);
		}
		else
		{
			PlaceOnSurface(item, x, y, 5, 20);
		}
	}
	
	// Vehicles
	for (var vehicle in GetAsList(settings.Vehicles))
	{
		if (base)
		{
			base->CreateContents(vehicle)->SetCommand("Exit");
		}
		else
		{
			PlaceOnSurface(player_nr, vehicle, x, y, 60, 200);
		}
	}
	return true;
}

func PlaceOnSurface(int player, id type, int x, int y, int wdt, int hgt)
{
	var area = Loc_InRect(x - wdt/2, y - hgt/2, wdt, hgt);
	var left = type->GetDefOffset(0);
	var space_top = Loc_Space(type->GetDefHeight(), CNAT_Top);
	var space_l = Loc_Space(Abs(left), CNAT_Left);
	var space_r = Loc_Space(left + type->GetDefWidth(), CNAT_Right);
	var on_surface = Loc_Wall(CNAT_Bottom, Loc_Not(Loc_Liquid()));
	// Find object near the desired area first, with less restrictions
	var spot = FindLocation(on_surface, space_l, space_r, space_top, area);
	spot = spot ?? FindLocation(on_surface, space_top, area);
	spot = spot ?? FindLocation(on_surface, area);
	// Same again, but on the whole landscape
	if (area != nil)
	{
		spot = FindLocation(on_surface, space_l, space_r, space_top);
		spot = spot ?? FindLocation(on_surface, space_top);
		spot = spot ?? FindLocation(on_surface);
	}

	if (spot)
	{
		CreateObject(type, spot.x, spot.y, player);
	}
}
