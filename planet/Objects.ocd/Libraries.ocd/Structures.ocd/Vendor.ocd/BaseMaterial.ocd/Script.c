/**
	Base Material & Production
	Library to control the players base material and production. The initial values are read
	from the Scenario.txt entries and per script one can modify these by:
     * GetBaseMaterial(int player, id material, int index, int category)
     * SetBaseMaterial(int player, id material, int amount)
     * DoBaseMaterial(int player, id material, int change)
     * GetBaseProduction(int player, id material, int index, int category)
     * SetBaseProduction(int player, id material, int amount)
     * DoBaseProduction(int player, id material, int change)
    Performs also two callbacks to a base of the player:
     * OnBaseMaterialChange(id material, int change);
     * OnBaseProductionChange(id material, int change);
	
	@author Randrian, Maikel
*/


// Local variables to store the player's material and production.
// Is an array filled with [id, count] arrays.
local base_material;
local base_production;
local production_unit = 0;

// Maximum number of material. 
static const BASEMATERIAL_MaxBaseMaterial = 25;
// Maximum number of production. 
static const BASEMATERIAL_MaxBaseProduction = 10;
// Produce every X frames (currently set to a minute).
static const BASEMATERIAL_ProductionRate = 2160;


/*-- Global interface --*/

global func GetBaseMaterial(int player, id material, int index, int category)
{
	var base = Library_BaseMaterial->GetBaseMaterialManager(player);
	if (base) 
		return base->GetBaseMat(material, index, category);
}

global func SetBaseMaterial(int player, id material, int amount)
{
	var base = Library_BaseMaterial->GetBaseMaterialManager(player);
	if (base)
		return base->SetBaseMat(material, amount);
}

global func DoBaseMaterial(int player, id material, int change)
{
	var base = Library_BaseMaterial->GetBaseMaterialManager(player);
	if (base)
		return base->DoBaseMat(material, change);
}

global func GetBaseProduction(int player, id material, int index, int category)
{
	var base = Library_BaseMaterial->GetBaseMaterialManager(player);
	if (base) 
		return base->GetBaseProd(material, index, category);
}

global func SetBaseProduction(int player, id material, int amount)
{
	var base = Library_BaseMaterial->GetBaseMaterialManager(player);
	if (base)
		return base->SetBaseProd(material, amount);
}

global func DoBaseProduction(int player, id material, int change)
{
	var base = Library_BaseMaterial->GetBaseMaterialManager(player);
	if (base) 
		return base->DoBaseProd(material, change);
}


/*-- Definition Interface --*/

protected func GetBaseMaterialManager(int player)
{
	var base = FindObject(Find_ID(Library_BaseMaterial), Find_AnyLayer(),  Find_Owner(player));
	if (!base)
	{
		base = CreateObject(Library_BaseMaterial, 0, 0, player);
	}
	return base;
}

/*-- Object Interface --*/

protected func Initialize()
{
	// Gather base materials based on Scenario.txt player entries.
	// TODO: Check teams and get the fitting player section
	var player = GetOwner() % 4 + 1;
	var section = Format("Player%d", player); 
	
	// Initialize arrays for material and production.
	base_material = [];
	base_production = [];
	
	// Load materials from Scenario.txt
	var index;
	var material, count;	
	while (true)
	{
		material = GetScenarioVal("BaseMaterial", section, index * 2);
		count = GetScenarioVal("BaseMaterial", section, index * 2 + 1);
		if (!material && !count) break;
		if (material)
		{
			PushBack(base_material, [material, count]);
		}
		index++;
	}
	
	// Load production from Scenario.txt
	index = 0;
	while (true)
	{
		material = GetScenarioVal("BaseProduction", section, index * 2);
		count = GetScenarioVal("BaseProduction", section, index * 2 + 1);
		if (!material && !count) break;
		if (material)
		{
			PushBack(base_production, [material, count]);
		}
		index++;
	}
	
	// Add a timer for executing base production.
	AddTimer("ExecBaseProduction", BASEMATERIAL_ProductionRate);
	return;
}

// Called every minute and updates the materials according to production.
public func ExecBaseProduction()
{
	production_unit++;
	// Look at all production.
	for (var combo in base_production)
	{
		// Check if this id is produced and check if it isn't already full.
		if (combo[1] > 0 && GetBaseMat(combo[0]) < BASEMATERIAL_MaxBaseMaterial)
		{
			// Produce the material every production value / BASEMATERIAL_MaxBaseProduction times.
			if (production_unit % BoundBy(BASEMATERIAL_MaxBaseProduction + 1 - combo[1], 1, BASEMATERIAL_MaxBaseProduction) == 0)
				DoBaseMat(combo[0], 1);
		}
	}
	return;
}

public func GetBaseMat(id material, int index, int category)
{
	// Get the count if the id is given.
	if (material)
	{
		for (var combo in base_material)
			if (combo[0] == material)
				return combo[1];
		return nil;
	}
	// If an index is given look for the id.
	category = category ?? 0xffffff;
	index = Max(0, index);
	var count = 0;
	for (var combo in base_material)
	{
		if (combo[0]->GetCategory() & category)
		{
			if (count == index)
			{
				return combo[0];
			}
			count++;
		}
	}
	return;
}

public func SetBaseMat(id material, int amount)
{
	if (amount == nil)
		return;
	amount = Max(0, amount);
	var change = 0;
	// Scan through current list of id's and set material if available.
	var found = false;
	for (var index = 0; index < GetLength(base_material); ++index)
	{
		if (base_material[index][0] == material)
		{
			change = amount - base_material[index][1];
			if (amount > 0)
			{
				base_material[index][1] = amount;
			}
			else
			{
				RemoveArrayIndex(base_material, index);
			}
			found = true;
			break;
		}
	}
	// If material is not available add it to the existing list.
	if (!found && amount > 0)
	{
		change = amount;
		PushBack(base_material, [material, amount]);
	}
	// Callback to the bases of the player.
	BroadcastBaseMaterialChange(material, change);
	return;
}

public func DoBaseMat(id material, int change)
{
	if (change == 0) 
		return;
	// Scan through current list of id's and increase material if available. 
	var found = false;
	for (var index = 0; index < GetLength(base_material); ++index)
	{
		if (base_material[index][0] == material)
		{
			// Change must at least be minus the original value.
			change = Max(change, -base_material[index][1]);
			base_material[index][1] += change;
			if (base_material[index][1] == 0)
			{
				RemoveArrayIndex(base_material, index);
			}
			found = true;
			break;
		}
	}
	// If material is not available add it to the existing list.
	if (!found)
	{
		// Change must at least be zero.
		change = Max(change, 0);
		if (change > 0)
		{
			PushBack(base_material, [material, change]);
		}
	}
	// Callback to the bases of the player.
	BroadcastBaseMaterialChange(material, change);
	return;
}

public func GetBaseProd(id material, int index, int category)
{
	// Get the count if the id is given.
	if (material)
	{
		for (var combo in base_production)
			if (combo[0] == material)
				return combo[1];
		return;
	}
	// If an index is given look for the id.
	category = category ?? 0xffffff;
	index = Max(0, index);
	var count = 0;
	for (var combo in base_production)
	{
		if (combo[0]->GetCategory() & category)
		{
			if (count == index) 
			{
				return combo[0];
			}
			count++;
		}
	}
	return;
}

public func SetBaseProd(id material, int amount)
{
	if (amount == nil)
		return;
	amount = Max(0, amount);
	var change = 0;
	// Scan through current list of id's and set production if available.
	var found = false;
	for (var index = 0; index < GetLength(base_production); ++index)
	{
		if (base_production[index][0] == material)
		{
			change = amount - base_production[index][1];
			if (amount > 0)
			{
				base_production[index][1] = amount;
			}
			else
			{
				RemoveArrayIndex(base_production, index);
			}
			found = true;
			break;
		}
	}
	// If material is not available add it to the existing list.
	if (!found && amount > 0)
	{
		change = amount;
		PushBack(base_production, [material, amount]);
	}
	// Callback to the bases of the player.
	BroadcastBaseProductionChange(material, change);
	return;
}

public func DoBaseProd(id material, int change)
{
	if (change == 0)
		return;
	// Scan through current list of id's and increase production if available. 
	var found = false;
	for (var index = 0; index < GetLength(base_production); ++index)
	{
		if (base_production[index][0] == material)
		{
			// Change must at least be minus the original value.
			change = Max(change, -base_production[index][1]);
			base_production[index][1] += change;
			if (base_production[index][1] == 0)
			{
				RemoveArrayIndex(base_production, index);
			}
			found = true;
			break;
		}
	}
	// If production is not available add it to the existing list.
	if (!found)
	{
		// Change must at least be zero.
		change = Max(change, 0);
		if (change > 0)
		{
			PushBack(base_production, [material, change]);
		}
	}
	// Callback to the bases of the player.
	BroadcastBaseProductionChange(material, change);
	return;
}


/*-- Miscellaneous --*/

protected func BroadcastBaseProductionChange(id material, int change)
{
	var i = 0, base;
	while (base = FindBase(GetOwner(), i++))
		base->~OnBaseProductionChange(material, change);
}

protected func BroadcastBaseMaterialChange(id material, int change)
{
	var i = 0, base;
	while (base = FindBase(GetOwner(), i++))
		base->~OnBaseMaterialChange(material, change);
}

// Internal management object not saved. Use Scenario.txt or script 
// to adjust base materials and production.
func SaveScenarioObject() { return false; }
