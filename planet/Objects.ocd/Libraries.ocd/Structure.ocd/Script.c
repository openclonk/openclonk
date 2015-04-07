/**
	Structure Library
	Basic library for structures, handles:
	* Damage
	* Energy bar if rule active
	* Basements
	
	@author Maikel
*/

// All structure related local variables are stored in a single proplist.
// This reduces the chances of clashing local variables. See Initialize
// for which variables are being used.
local lib_structure;

protected func Initialize()
{
	// Initialize the single proplist for the structure library.
	if (lib_structure == nil)
		lib_structure = {};
	lib_structure.basement = nil;
	// Add energy bars if the rule is active.
	if (FindObject(Find_ID(Rule_StructureHPBars)))
		if (this.HitPoints != nil)
			AddEnergyBar();
	return _inherited(...);
}

public func Damage(int change, int cause, int cause_plr)
{
	// Only do stuff if the object has the HitPoints property.
	if (this && this.HitPoints != nil)
		if (GetDamage() >= this.HitPoints)
		{		
			// Destruction callback is made by the engine.
			return RemoveObject();
		}
	return _inherited(change, cause, cause_plr, ...);
}

// This object is a structure.
public func IsStructure() { return true; }


/*-- Basement Handling --*/

public func SetBasement(object to_basement)
{
	lib_structure.basement = to_basement;
	return;
}

public func GetBasement() { return lib_structure.basement; }

public func IsStructureWithoutBasement() { return IsStructure() && !lib_structure.basement; }
