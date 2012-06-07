/**
	Structure Library
	Basic library for structures, handles:
	* Damage
	
	@author Maikel
*/

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