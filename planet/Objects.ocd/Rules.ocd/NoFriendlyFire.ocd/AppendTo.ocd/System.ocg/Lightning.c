// Disable friendly fire for lightning strikes if rule is active.

#appendto Lightning

private func PerformLightningStrike(object target, int damage)
{
	var w_controller = this->GetController();
	var t_controller = target->GetController();
	if (FindObject(Find_ID(Rule_NoFriendlyFire)))
		if (w_controller != NO_OWNER && t_controller != NO_OWNER && !Hostile(w_controller, t_controller))
			return;
	return _inherited(target, damage, ...);
}