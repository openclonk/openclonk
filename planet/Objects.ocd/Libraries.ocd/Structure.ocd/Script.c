/**
	Structure Library
	Basic library for structures, handles:
	* Damage
	* Energy bar if rule active
	
	@author Maikel
*/

func Initialize()
{
	if (ObjectCount(Find_ID(Rule_EnergyBarsAboveStructures)) > 0)
	{
		if (this.HitPoints != nil)
			AddEnergyBar();
	}
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
	return _inherited(change, cause, cause_plr);
}