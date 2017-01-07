// Disable friendly fire for ranged weapons if rule is active.

#appendto Arrow
#appendto Javelin
#appendto LeadBullet
#appendto Boompack
#appendto Shrapnel

public func HitObject(object target, ...)
{
	var w_controller = this->GetController();
	var t_controller = target->GetController();
	if (FindObject(Find_ID(Rule_NoFriendlyFire)))
		if (w_controller != NO_OWNER && t_controller != NO_OWNER && !Hostile(w_controller, t_controller))
			return false;
	return inherited(target, ...);
}
