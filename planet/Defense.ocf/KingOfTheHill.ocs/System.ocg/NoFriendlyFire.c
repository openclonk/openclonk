// Add no friendly fire to these objects.

#appendto Flagpole

public func Construction()
{
	// Notify friendly fire rule.
	GameCallEx("OnCreationRuleNoFF", this);
	return _inherited(...);
}

public func Destruction()
{
	// Notify friendly fire rule.
	GameCallEx("OnDestructionRuleNoFF", this);
	return _inherited(...);
}

local HasNoFriendlyFire = true;