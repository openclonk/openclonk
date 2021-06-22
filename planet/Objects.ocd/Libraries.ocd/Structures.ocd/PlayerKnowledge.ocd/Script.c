/**
	Player Knowledge

	Library to control the players knowledge/plans:
     * GetPlrKnowledge(proplist player, id plan, int index, int category)
     * HasPlrKnowledge(proplist player, id plan)
     * GivePlrKnowledge(proplist player, id plan)
     * RemovePlrKnowledge(proplist player, id plan)

    Should be incorporated into a player management system later

	@author Marky, Maikel
*/

/* --- Properties --- */

// Local variables to store the player's knowledge.
local knowledge = [];



/* --- Global interface --- */

global func GetPlrKnowledge(proplist player, id plan, int index, int category)
{
	var manager = Library_PlayerKnowledge->GetKnowledgeManager(player);
	if (manager)
	{
		if (plan)
		{
			LogLegacyWarning("GetPlrKnowledge() using 'id' parameter", "HasPlrKnowledge()", VERSION_10_0_OC);
			return manager->HasKnowledge(plan);
		}
		else
		{
			return manager->GetKnowledge(index, category);
		}
	}
}

global func HasPlrKnowledge(proplist player, id plan)
{
	var manager = Library_PlayerKnowledge->GetKnowledgeManager(player);
	if (manager)
	{
		return manager->HasKnowledge(plan);
	}
}

global func GivePlrKnowledge(proplist player, any plan)
{
	// Do it for all players?
	if (player == nil)
	{
		var success = true;
		for (var i = 0; i < GetPlayerCount(); ++i)
		{
			success &= GivePlrKnowledge(GetPlayerByIndex(i), plan);
		}
		return success;
	}

	// Do it for several plans?
	if (GetType(plan) == C4V_Array)
	{
		var success = true;
		for (var type in plan)
		{
			success &= GivePlrKnowledge(player, type);
		}
		return success;
	}
	else
	{
		AssertTypeOneOf(C4V_Def, plan, "plan");
	}

	// Default handling for one player
	var manager = Library_PlayerKnowledge->GetKnowledgeManager(player);
	if (manager)
	{
		return manager->GiveKnowledge(plan);
	}
	return false;
}

global func RemovePlrKnowledge(proplist player, any plan)
{
	// Do it for all players?
	if (player == nil)
	{
		var success = true;
		for (var i = 0; i < GetPlayerCount(); ++i)
		{
			success &= RemovePlrKnowledge(GetPlayerByIndex(i), plan);
		}
		return success;
	}
	
	// Do it for several plans?
	if (GetType(plan) == C4V_Array)
	{
		var success = true;
		for (var type in plan)
		{
			success &= RemovePlrKnowledge(player, type);
		}
		return success;
	}
	else
	{
		AssertTypeOneOf(C4V_Def, plan, "plan");
	}

	// Default handling for one player
	var manager = Library_PlayerKnowledge->GetKnowledgeManager(player);
	if (manager)
	{
		return manager->RemoveKnowledge(plan);
	}
	return false;
}

/* --- Specific Knowledge --- */

// Provides global functions to give players knowledge for scenarios.

 
// Gives the player plans for all constructible or producible objects.
global func GivePlayerAllKnowledge(proplist plr)
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

// Gives the player plans according to basic knowledge.
global func GivePlayerBasicKnowledge(proplist plr)
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

global func GivePlayerPumpingKnowledge(proplist plr)
{
	var knowledge = [
		// Stuff needed for pumping.
		Pump, Pipe /*,LiquidTank TODO: add when graphics are done*/
	];
	GivePlrKnowledge(plr, knowledge);
	return;
}

global func GivePlayerFarmingKnowledge(proplist plr)
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

global func GivePlayerWeaponryKnowledge(proplist plr)
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

global func GivePlayerArtilleryKnowledge(proplist plr)
{
	var knowledge = [
		// Stuff to set up artillery.
		Armory, PowderKeg, Catapult, Cannon
	];
	GivePlrKnowledge(plr, knowledge);
	return;
}

global func GivePlayerAdvancedKnowledge(proplist plr)
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

global func GivePlayerAirKnowledge(proplist plr)
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


/* --- Definition Interface --- */

func GetKnowledgeManager(proplist player)
{
	var manager = FindObject(Find_ID(Library_PlayerKnowledge), Find_AnyLayer(),  Find_Owner(player));
	if (!manager)
	{
		manager = CreateObject(Library_PlayerKnowledge, 0, 0, player);
	}
	return manager;
}

/* --- Object Interface --- */


public func GetKnowledge(int index, int category)
{
	if (category)
	{
		var nr = -1;
		for (var plan in knowledge)
		{
			if (plan->GetCategory() & category)
			{
				nr += 1;
			}
			if (nr == index)
			{
				return plan;
			}
		}
		return nil;
	}
	else
	{
		return knowledge[index];
	}
}


public func HasKnowledge(id plan)
{
	return IsValueInArray(knowledge, plan);
}

public func GiveKnowledge(any plan)
{
	if (plan == nil)
	{
		return false;
	}
	if (!HasKnowledge(plan))
	{
		PushBack(knowledge, plan);
	}
	return true;
}

public func RemoveKnowledge(id plan)
{
	if (plan == nil)
	{
		return false;
	}
	return RemoveArrayValue(knowledge, plan);
}

// Do not save object, but save the callbacks
// Not sure if it works, but whatever
func SaveScenarioObject(proplist props)
{
	var player = GetOwner();
	for (var plan in knowledge)
	{
		props->Add("Knowledge", "GivePlrKnowledge(%d, %i)", player, plan);
	}
	return false;
}
