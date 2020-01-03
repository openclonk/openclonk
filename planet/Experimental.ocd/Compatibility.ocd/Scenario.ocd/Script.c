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
	for (var prop in GetProperties(Objects))
	{
		var type = GetDefinition(prop);
		for (var amount = Objects[prop]; amount > 0; --amount)
		{
			CreateObject(type, 0, 0, NO_OWNER);
		}
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
