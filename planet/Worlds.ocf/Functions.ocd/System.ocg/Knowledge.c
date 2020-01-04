/**
	Knowdlege
	Provides global functions to give players knowledge for scenarios.
	
	@author Maikel
*/

 
// Gives the player plans for all constructible or producible objects.
global func GivePlayerAllKnowledge(int plr)
{
	GivePlayerBasicKnowledge(plr);
	GivePlayerPumpingKnowledge(plr);
	GivePlayerFarmingKnowledge(plr);
	GivePlayerWeaponryKnowledge(plr);
	GivePlayerArtilleryKnowledge(plr);
	GivePlayerAdvancedKnowledge(plr);
	GivePlayerAirKnowledge(plr);
	return;
}

// Gives the player specific knowledge as given in the knowledge array.
global func GivePlayerSpecificKnowledge(int plr, array knowledge)
{
	GivePlrKnowledge(plr, knowledge);
	return;
}

// Gives the player specific knowledge as given in the knowledge array.
global func RemovePlayerSpecificKnowledge(int plr, array knowledge)
{
	RemovePlrKnowledge(plr, knowledge);
	return;
}

// Gives the player plans according to basic knowledge.
global func GivePlayerBasicKnowledge(int plr)
{
	var knowledge = [
		// Basic structures for a settlement and production of tools and explosives.
		Flagpole, Basement, WindGenerator, SteamEngine, Compensator, Sawmill, Foundry, Elevator, ToolsWorkshop, ChemicalLab, Chest, WoodenBridge,
		// Basic tools for mining and tree chopping and loam production.
		Shovel, Hammer, Axe, Pickaxe, Barrel, Bucket, Torch, Lantern,
		// The basic resources.
		Metal, Loam, GoldBar,
		// Some of the basic explosives.
		Dynamite, DynamiteBox,
		// Some basic vehicles which aid in the settlement construction.
		Lorry
	];
	GivePlrKnowledge(plr, knowledge);
	return;
}

global func GivePlayerPumpingKnowledge(int plr)
{
	var knowledge = [
		// Stuff needed for pumping.
		Pump, Pipe /*,LiquidTank TODO: add when graphics are done*/
	];
	GivePlrKnowledge(plr, knowledge);
	return;
}

global func GivePlayerFarmingKnowledge(int plr)
{
	var knowledge = [
		// Structures needed to process farming materials.
		Kitchen, Loom, Windmill,
		// Basic tools for farming.
		Sickle,
		// Processed goods.
		Cloth, Flour, Bread
	];
	GivePlrKnowledge(plr, knowledge);
	return;
}

global func GivePlayerWeaponryKnowledge(int plr)
{
	var knowledge = [
		// Armory to construct the weapons.
		Armory,
		// Weapons and explosives.
		Bow, Arrow, FireArrow, BombArrow, Club, Sword, Javelin, Shield, Blunderbuss, LeadBullet, IronBomb, GrenadeLauncher, PowderKeg, Helmet, SmokeBomb,
		// Artillery vehicles.
		Catapult, Cannon
	];
	GivePlrKnowledge(plr, knowledge);
	return;
}

global func GivePlayerArtilleryKnowledge(int plr)
{
	var knowledge = [
		// Stuff to set up artillery.
		Armory, PowderKeg, Catapult, Cannon
	];
	GivePlrKnowledge(plr, knowledge);
	return;
}

global func GivePlayerAdvancedKnowledge(int plr)
{
	var knowledge = [
		// Inventors lab to construct the items.
		InventorsLab, Loom,
		// Advanced items in tools workshop and needed materials.
		Ropeladder, MetalBarrel, PowderKeg, WallKit, Cloth, DivingHelmet,
		// Advanced items in inventors lab.
		TeleGlove, WindBag, GrappleBow, Boompack, Balloon
	];
	GivePlrKnowledge(plr, knowledge);
	return;
}

global func GivePlayerAirKnowledge(int plr)
{
	var knowledge = [
		// Shipyard to construct the vehicles.
		Shipyard, Loom,
		// Materials needed.
		Cloth,
		// Airship and plane.
		Airship, Airplane
	];
	GivePlrKnowledge(plr, knowledge);
	return;
}
