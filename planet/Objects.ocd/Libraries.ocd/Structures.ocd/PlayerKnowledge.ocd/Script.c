/**
	Player Knowledge

	Library to control the players knowledge/plans:
     * GetKnowledge(proplist player, id plan, int index, int category)
     * HasKnowledge(proplist player, id plan)
     * GiveKnowledge(proplist player, id plan)
     * RemoveKnowledge(proplist player, id plan)


	@author Marky, Maikel
*/

/* --- Properties --- */

protected func Definition(def type)
{
	AddProperties(Player, {
		GetKnowledgeData = type.GetKnowledgeData,
		GetKnowledge = type.GetKnowledge,
		GiveKnowledge = type.GiveKnowledge,
		HasKnowledge = type.HasKnowledge,
		RemoveKnowledge = type.RemoveKnowledge,
		// Specifics
		GiveAllKnowledge = type.GiveAllKnowledge,
		GiveBasicKnowledge = type.GiveBasicKnowledge,
		GivePumpingKnowledge = type.GivePumpingKnowledge,
		GiveFarmingKnowledge = type.GiveFarmingKnowledge,
		GiveWeaponryKnowledge = type.GiveWeaponryKnowledge,
		GiveArtilleryKnowledge = type.GiveArtilleryKnowledge,
		GiveAdvancedKnowledge = type.GiveAdvancedKnowledge,
		GiveAirKnowledge = type.GiveAirKnowledge
		});
}

/* --- Specific Knowledge --- */

// Provides global functions to give players knowledge for scenarios.

 
// Gives the player plans for all constructible or producible objects.
public func GiveAllKnowledge()
{
	GiveBasicKnowledge();
	GivePumpingKnowledge();
	GiveFarmingKnowledge();
	GiveWeaponryKnowledge();
	GiveArtilleryKnowledge();
	GiveAdvancedKnowledge();
	GiveAirKnowledge();
	return;
}

// Gives the player plans according to basic knowledge.
public func GiveBasicKnowledge()
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
	GiveKnowledge(knowledge);
	return;
}

public func GivePumpingKnowledge()
{
	GiveKnowledge([
		// Stuff needed for pumping.
		Pump, Pipe /*,LiquidTank TODO: add when graphics are done*/
	]);
	return;
}

public func GiveFarmingKnowledge()
{
	GiveKnowledge([
		// Structures needed to process farming materials.
		Kitchen, Loom, Windmill,
		// Basic tools for farming.
		Sickle,
		// Processed goods.
		Cloth, Flour, Bread
	]);
	return;
}

public func GiveWeaponryKnowledge()
{
	GiveKnowledge([
		// Armory to construct the weapons.
		Armory,
		// Weapons and explosives.
		Bow, Arrow, FireArrow, BombArrow, Club, Sword, Javelin, Shield, Blunderbuss, LeadBullet, IronBomb, GrenadeLauncher, PowderKeg, Helmet, SmokeBomb,
		// Artillery vehicles.
		Catapult, Cannon
	]);
	return;
}

public func GiveArtilleryKnowledge()
{
	GiveKnowledge([
		// Stuff to set up artillery.
		Armory, PowderKeg, Catapult, Cannon
	]);
	return;
}

public func GiveAdvancedKnowledge()
{
	GiveKnowledge([
		// Inventors lab to construct the items.
		InventorsLab, Loom,
		// Advanced items in tools workshop and needed materials.
		Ropeladder, MetalBarrel, PowderKeg, WallKit, Cloth, DivingHelmet,
		// Advanced items in inventors lab.
		TeleGlove, WindBag, GrappleBow, Boompack, Balloon
	]);
	return;
}

public func GiveAirKnowledge()
{
	GiveKnowledge([
		// Shipyard to construct the vehicles.
		Shipyard, Loom,
		// Materials needed.
		Cloth,
		// Airship and plane.
		Airship, Airplane
	]);
	return;
}


/* --- Object Interface --- */

public func GetKnowledgeData()
{
	if (this.Data.Knowledge == nil)
	{
		this.Data.Knowledge = [];
	}
	return this.Data.Knowledge;
}

public func GetKnowledge(int index, int category)
{
	var knowledge = GetKnowledgeData();
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
	return IsValueInArray(GetKnowledgeData(), plan);
}

public func GiveKnowledge(any plan)
{
	if (plan == nil)
	{
		return false;
	}
	// Do it for several plans?
	if (GetType(plan) == C4V_Array)
	{
		var success = true;
		for (var type in plan)
		{
			success &= GiveKnowledge(type);
		}
		return success;
	}
	else
	{
		AssertTypeOneOf(C4V_Def, plan, "plan");
	}
	if (!HasKnowledge(plan))
	{
		PushBack(GetKnowledgeData(), plan);
	}
	return true;
}

public func RemoveKnowledge(any plan)
{
	if (plan == nil)
	{
		return false;
	}
	// Do it for several plans?
	if (GetType(plan) == C4V_Array)
	{
		var success = true;
		for (var type in plan)
		{
			success &= RemoveKnowledge(type);
		}
		return success;
	}
	else
	{
		AssertTypeOneOf(C4V_Def, plan, "plan");
	}
	return RemoveArrayValue(GetKnowledgeData(), plan);
}

// Do not save object, but save the callbacks
// Not sure if it works, but whatever
func SaveScenarioObject(proplist props)
{
	var player = GetOwner();
	for (var plan in player->GetKnowledgeData())
	{
		props->Add("Knowledge", "GetPlayer(%d)->GiveKnowledge(%i)", player.ID, plan);
	}
	return false;
}
