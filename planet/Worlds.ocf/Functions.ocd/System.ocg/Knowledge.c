/**
	Knowdlege
	Provides global functions to give players knowledge for scenarios.
	
	@author Maikel
*/

 
// Gives the player plans for all constructible or producible objects.
global func GivePlayerAllKnowledge(int plr)
{
	GivePlayerBasicKnowledge(plr);

	return;
}

// Gives the player specific knowledge as given in the knowledge array.
global func GivePlayerSpecificKnowledge(int plr, array knowledge)
{
	for (var plan in knowledge)
		SetPlrKnowledge(plr, plan);
	return;
}

// Gives the player specific knowledge as given in the knowledge array.
global func RemovePlayerSpecificKnowledge(int plr, array knowledge)
{
	for (var plan in knowledge)
		SetPlrKnowledge(plr, plan, true);
	return;
}

// Gives the player plans according to basic knowledge.
global func GivePlayerBasicKnowledge(int plr)
{
	var knowledge = [
		// Basic structures for a settlement and production of tools and explosives.
		Flagpole, Basement, WindGenerator, SteamEngine, Compensator, Sawmill, Foundry, Elevator, ToolsWorkshop, ChemicalLab, Chest,
		// Basic tools for mining and tree chopping and loam production.
		Shovel, Hammer, Axe, Pickaxe, Barrel, Bucket, Torch,
		// The basic resources.
		Metal, Loam, GoldBar,
		// Some of the basic explosives.
		Dynamite, DynamiteBox,
		// Some basic vehicles which aid in the settlement construction.
		Lorry
	];
	for (var plan in knowledge)
		SetPlrKnowledge(plr, plan);
	return;
}

global func GivePlayerPumpingKnowledge(int plr)
{
	var knowledge = [
		// Stuff needed for pumping.
		Pump, Pipe
	];
	for (var plan in knowledge)
		SetPlrKnowledge(plr, plan);
	return;
}

global func GivePlayerWeaponryKnowledge(int plr)
{
	var knowledge = [
		// Armory to construct the weapons.
		Armory,
		// Weapons and explosives.
		Bow, Arrow, Club, Sword, Javelin, Shield, Musket, LeadShot, IronBomb, GrenadeLauncher, PowderKeg,
		// Artillery vehicles.
		Catapult, Cannon
	];
	for (var plan in knowledge)
		SetPlrKnowledge(plr, plan);
	return;
}

global func GivePlayerArtilleryKnowledge(int plr)
{
	var knowledge = [
		// Stuff to set up artillery.
		Armory, PowderKeg, Catapult, Cannon
	];
	for (var plan in knowledge)
		SetPlrKnowledge(plr, plan);
	return;
}

global func GivePlayerAdvancedKnowledge(int plr)
{
	var knowledge = [
		// Inventors lab to construct the items.
		InventorsLab,
		// Advanced items in tools workshop.
		Ropeladder, MetalBarrel, PowderKeg, WallKit,
		// Advanced items in inventors lab.
		TeleGlove, WindBag, GrappleBow, Boompack, Balloon,
	];
	for (var plan in knowledge)
		SetPlrKnowledge(plr, plan);
	return;
}

global func GivePlayerAirKnowledge(int plr)
{
	var knowledge = [
		// Shipyard to construct the vehicles.
		Shipyard,
		// Airship and plane.
		Airship, Plane
	];
	for (var plan in knowledge)
		SetPlrKnowledge(plr, plan);
	return;
}
