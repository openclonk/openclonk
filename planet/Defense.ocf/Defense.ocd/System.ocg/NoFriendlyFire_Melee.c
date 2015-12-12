/* Disable friendly fire */
// This could also be a general rule

#appendto Library_MeleeWeapon

public func ApplyShieldFactor(from, target, ...)
{
	// 100% shield if allied
	var w_controller = from->GetController();
	var t_controller = target->GetController();
	if (w_controller >= 0) // NO_OWNER is probably lost controller management (e.g. chain reactions). Always hit.
		if ((t_controller == ENEMY) == (w_controller == ENEMY)) // ENEMY can't hit ENEMY and others can't hit others.
			return 100;
	// Otherwise regular check
	return inherited(from, target, ...);
}