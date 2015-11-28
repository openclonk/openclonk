/**
	Base Material & Production
	Library to control the players base material and production. The initial values are read
	from the Scenario.txt entries and per script one can modify these by:
     * GetBaseMaterial(int plr, id def, int index, int category)
     * SetBaseMaterial(int plr, id def, int cnt)
     * DoBaseMaterial(int plr, id def, int change)
     * GetBaseProduction(int plr, id def, int index, int category)
     * SetBaseProduction(int plr, id def, int cnt)
     * DoBaseProduction(int plr, id def, int change)
    Performs also two callbacks to a base of the player:
     * OnBaseMaterialChange(id def, int change);
     * OnBaseProductionChange(id def, int change);
	
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

global func GetBaseMaterial(int plr, id def, int index, int category)
{
	var base = FindObject(Find_ID(BaseMaterial), Find_Owner(plr));
	if (!base) 
		base = CreateObjectAbove(BaseMaterial, AbsX(10), AbsY(10), plr);
	if (base) 
		return base->GetBaseMat(def, index, category);
}

global func SetBaseMaterial(int plr, id def, int cnt)
{
	var base = FindObject(Find_ID(BaseMaterial), Find_Owner(plr));
	if (!base) 
		base = CreateObjectAbove(BaseMaterial, AbsX(10), AbsY(10), plr);
	if (base)
		return base->SetBaseMat(def, cnt);
}

global func DoBaseMaterial(int plr, id def, int change)
{
	var base = FindObject(Find_ID(BaseMaterial), Find_Owner(plr));
	if (!base) 
		base = CreateObjectAbove(BaseMaterial, AbsX(10), AbsY(10), plr);
	if (base)
		return base->DoBaseMat(def, change);
}

global func GetBaseProduction(int plr, id def, int index, int category)
{
	var base = FindObject(Find_ID(BaseMaterial), Find_Owner(plr));
	if (!base)
		base = CreateObjectAbove(BaseMaterial, AbsX(10), AbsY(10), plr);
	if (base) 
		return base->GetBaseProd(def, index, category);
}

global func SetBaseProduction(int plr, id def, int cnt)
{
	var base = FindObject(Find_ID(BaseMaterial), Find_Owner(plr));
	if (!base) 
		base = CreateObjectAbove(BaseMaterial, AbsX(10), AbsY(10), plr);
	if (base)
		return base->SetBaseProd(def, cnt);
}

global func DoBaseProduction(int plr, id def, int change)
{
	var base = FindObject(Find_ID(BaseMaterial), Find_Owner(plr));
	if (!base) 
		base = CreateObjectAbove(BaseMaterial, AbsX(10), AbsY(10), plr);
	if (base) 
		return base->DoBaseProd(def, change);
}


/*-- Object Interface --*/

protected func Initialize()
{
	// Gather base materials based on Scenario.txt player entries.
	// TODO: Check teams and get the fitting player section
	var plr = GetOwner() % 4 + 1;
	var section = Format("Player%d", plr); 
	
	// Initialize arrays for material and production.
	base_material = [];
	base_production = [];
	
	// Load materials from Scenario.txt
	var index;
	var def, count;	
	while (true)
	{
		def = GetScenarioVal("BaseMaterial", section, index * 2);
		count = GetScenarioVal("BaseMaterial", section, index * 2 + 1);
		if (!def && !count) break;
		if (def)
			PushBack(base_material, [def, count]);
		index++;
	}
	
	// Load production from Scenario.txt
	index = 0;
	while (true)
	{
		def = GetScenarioVal("BaseProduction", section, index * 2);
		count = GetScenarioVal("BaseProduction", section, index * 2 + 1);
		if (!def && !count) break;
		if (def)
			PushBack(base_production, [def, count]);
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

public func GetBaseMat(id def, int index, int category)
{
	// Get the count if the id is given.
	if (def)
	{
		for (var combo in base_material)
			if (combo[0] == def)
				return combo[1];
		return nil;
	}
	// If an index is given look for the id.
	if (!category) 
		category = 0xffffff;
	var count = 0;
	for (var combo in base_material)
	{
		if (combo[0]->GetCategory() & category)
		{
			if (count == index)
				return combo[0];
			count++;
		}
	}
	return;
}

public func SetBaseMat(id def, int cnt)
{
	if (cnt == nil)
		return;
	cnt = Max(0, cnt);
	var change = 0;
	// Scan through current list of id's and set material if available.
	var found = false;
	for (var index = 0; index < GetLength(base_material); ++index)
	{
		if (base_material[index][0] == def)
		{
			change = cnt - base_material[index][1];
			base_material[index][1] = cnt;
			found = true;
		}
	}
	// If material is not available add it to the existing list.
	if (!found)
	{
		change = cnt;
		PushBack(base_material, [def, cnt]);
	}
	// Callback to the bases of the player.
	var i = 0, base;
	while (base = FindBase(GetOwner(), i++))
		base->~OnBaseMaterialChange(def, change);
	return;
}

public func DoBaseMat(id def, int change)
{
	if (change == 0) 
		return;
	// Scan through current list of id's and increase material if available. 
	var found = false;
	for (var index = 0; index < GetLength(base_material); ++index)
	{
		if (base_material[index][0] == def)
		{
			// Change must at least be minus the original value.
			change = Max(change, -base_material[index][1]);
			base_material[index][1] += change;
			found = true;
		}
	}
	// If material is not available add it to the existing list.
	if (!found)
	{
		// Change must at least be zero.
		change = Max(change, 0);
		PushBack(base_material, [def, Max(change, 0)]);
	}
	// Callback to the bases of the player.
	var i = 0, base;
	while (base = FindBase(GetOwner(), i++))
		base->~OnBaseMaterialChange(def, change);
	return;
}

public func GetBaseProd(id def, int index, int category)
{
	// Get the count if the id is given.
	if (def)
	{
		for (var combo in base_production)
			if (combo[0] == def)
				return combo[1];
		return;
	}
	// If an index is given look for the id.
	if (!category) 
		category = 0xffffff;
	var count = 0;
	for (var combo in base_production)
	{
		if (combo[0]->GetCategory() & category)
		{
			if (count == index) 
				return combo[0];
			count++;
		}
	}
	return;
}

public func SetBaseProd(id def, int cnt)
{
	if (cnt == nil)
		return;
	cnt = Max(0, cnt);
	var change = 0;
	// Scan through current list of id's and set production if available.
	var found = false;
	for (var index = 0; index < GetLength(base_production); ++index)
	{
		if (base_production[index][0] == def)
		{
			change = cnt - base_production[index][1];
			base_production[index][1] = cnt;
			found = true;
		}
	}
	// If material is not available add it to the existing list.
	if (!found)
	{
		change = cnt;
		PushBack(base_production, [def, cnt]);
	}
	// Callback to the bases of the player.
	var i = 0, base;
	while (base = FindBase(GetOwner(), i++))
		base->~OnBaseProductionChange(def, change);
	return;
}

public func DoBaseProd(id def, int change)
{
	if (change == 0)
		return;
	// Scan through current list of id's and increase production if available. 
	var found = false;
	for (var index = 0; index < GetLength(base_production); ++index)
	{
		if (base_production[index][0] == def)
		{
			// Change must at least be minus the original value.
			change = Max(change, -base_production[index][1]);
			base_production[index][1] += change;
			found = true;
		}
	}
	// If production is not available add it to the existing list.
	if (!found)
	{
		// Change must at least be zero.
		change = Max(change, 0);
		PushBack(base_production, [def, Max(change, 0)]);
	}
	// Callback to the bases of the player.
	var i = 0, base;
	while (base = FindBase(GetOwner(), i++))
		base->~OnBaseProductionChange(def, change);
	return;
}


/*-- Miscellaneous --*/

// Internal management object not saved. Use Scenario.txt or script 
// to adjust base materials and production.
func SaveScenarioObject() { return false; }

local Name = "$Name$";
