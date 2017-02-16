// Disable friendly fire for melee weapons if rule is active.

#appendto Library_MeleeWeapon

public func ApplyShieldFactor(from, target, ...)
{
	var w_controller = from->GetController();
	var t_controller = target->GetController();
	if (FindObject(Find_ID(Rule_NoFriendlyFire)))
		if (w_controller != NO_OWNER && t_controller != NO_OWNER && !Hostile(w_controller, t_controller))
			return 100;
	return inherited(from, target, ...);
}