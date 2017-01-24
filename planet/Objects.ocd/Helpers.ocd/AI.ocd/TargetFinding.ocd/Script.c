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

// Returns whether a weapon can be used for a certain target.
public func IsWeaponForTarget(effect fx, object weapon, object target)
{
	// TODO: Implement shooting at different targets, e.g. alive vs. structure.
	return false;
}