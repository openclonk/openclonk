/* Disable friendly fire */
// This could also be a general rule

#appendto Arrow
#appendto Javelin
#appendto LeadShot
#appendto Boompack
#appendto Shrapnel

public func HitObject(object target, ...)
{
	// Go by controller
	var w_controller = GetController();
	var t_controller = target->GetController();
	if (w_controller >= 0) // NO_OWNER is probably lost controller management (e.g. chain reactions). Always hit.
		if ((t_controller == ENEMY) == (w_controller == ENEMY)) // ENEMY can't hit ENEMY and others can't hit others.
			return false;
	// OK, hit it
	return inherited(target, ...);
}
