/**
	AI Target Finding
	Functionality that helps the AI to find targets to attack.
	
	@author Sven2, Maikel
*/


public func FindTarget(effect fx)
{
	// Consider hostile clonks, or all clonks if the AI does not have an owner.
	var hostile_criteria = Find_Hostile(fx.Target->GetOwner());
	if (fx.Target->GetOwner() == NO_OWNER)
		hostile_criteria = Find_Not(Find_Owner(fx.Target->GetOwner()));
	for (var target in fx.Target->FindObjects(Find_InRect(fx.guard_range.x - fx.Target->GetX(), fx.guard_range.y - fx.Target->GetY(), fx.guard_range.wdt, fx.guard_range.hgt), Find_OCF(OCF_CrewMember), hostile_criteria, Find_NoContainer(), Sort_Random()))
		if (fx.ranged || PathFree(fx.Target->GetX(), fx.Target->GetY(), target->GetX(), target->GetY()))
			if (this->HasWeaponForTarget(fx, target))
				return target;
	// Nothing found.
	return;
}

public func FindEmergencyTarget(effect fx)
{
	// Consider hostile clonks, or all clonks if the AI does not have an owner.
	var hostile_criteria = Find_Hostile(fx.Target->GetOwner());
	if (fx.Target->GetOwner() == NO_OWNER)
		hostile_criteria = Find_Not(Find_Owner(fx.Target->GetOwner()));
	// Search nearest enemy clonk in area even if not in guard range, used e.g. when outside guard range (AI fell down) and being attacked.
	for (var target in fx.Target->FindObjects(Find_Distance(200), Find_OCF(OCF_CrewMember), hostile_criteria, Find_NoContainer(), Sort_Distance()))
		if (PathFree(fx.Target->GetX(), fx.Target->GetY(), target->GetX(), target->GetY()))
			if (this->HasWeaponForTarget(fx, target))
				return target;
	// Nothing found.
	return;
}

public func CheckTargetInGuardRange(effect fx)
{
	// If target is not in guard range, reset it and return false.
	if (!Inside(fx.target->GetX() - fx.guard_range.x, -10, fx.guard_range.wdt + 9) || !Inside(fx.target->GetY() - fx.guard_range.y, -10, fx.guard_range.hgt + 9)) 
	{
		if (ObjectDistance(fx.target) > 250)
		{
			fx.target = nil;
			return false;
		}
	}
	return true;
}

// Checks whether the AI has a weapon for the target.
public func HasWeaponForTarget(effect fx, object target)
{
	target = target ?? fx.target;
	// Already has a weapon, or a vehicle
	if (fx.weapon && this->IsWeaponForTarget(fx, fx.weapon, target))
		return true;
	// Look for weapons in the inventory
	for (var weapon in FindObjects(Find_Container(fx.Target)))
		if (this->IsWeaponForTarget(fx, weapon, target))
			return true;
	return false;
}

// Returns whether a weapon can be used for a certain target.
public func IsWeaponForTarget(effect fx, object weapon, object target)
{
	weapon = weapon ?? fx.weapon;
	target = target ?? fx.target;
	if (!weapon || !target)
		return false;	
	// If on a vehicle forward behavior.
	if (fx.vehicle)
		return this->IsVehicleForTarget(fx, fx.vehicle, target);
	
	// Assume that all weapons can attack living targets.
	if (target->GetAlive())
		return true;
	
	// Structures and vehicles can only be attacked by explosives.
	var cat_target = target->GetCategory();
	if ((cat_target & C4D_Structure) || (cat_target & C4D_Vehicle))
	{
		if (weapon->~IsExplosive())
			return true;	
	}  
	
	// Weapon is unable to attack current target.
	return false;
}

// Returns whether a weapon can be used for a certain target.
public func IsVehicleForTarget(effect fx, object vehicle, object target)
{
	vehicle = vehicle ?? fx.vehicle;
	target = target ?? fx.target;
	if (!vehicle || !target)
		return false;
	// Airships may board everywhere.
	if (vehicle->GetID() == Airship)
		return true;
	// Catapult may fire at everything.
	if (vehicle->GetID() == Catapult)
		return true;
	// Airplanes can attack anything a priori.
	if (vehicle->GetID() == Airplane)
		return true;
	return false;
}
