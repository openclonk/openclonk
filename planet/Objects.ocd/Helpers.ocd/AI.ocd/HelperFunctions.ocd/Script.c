/**
	AI Helper Functions
	Functionality that helps the AI.
	
	@author Maikel
*/


// Returns the object that determines the movement of the target.
// For example the object the target is attached to or contained in.
private func GetTargetMovementObject(object target)
{
	// If attached: return the object the target is attached to.
	if (target->GetProcedure() == DFA_ATTACH)
		if (target->GetActionTarget())
			return target->GetActionTarget();
	// Default: return the target itself.
	return target;
}

// Returns the x-speed of the target or the object that causes its movement.
private func GetTargetXDir(object target, int prec)
{
	var moving_target = GetTargetMovementObject(target);
	return moving_target->GetXDir(prec);
}

// Returns the y-speed of the target or the object that causes its movement.
private func GetTargetYDir(object target, int prec)
{
	var moving_target = GetTargetMovementObject(target);
	return moving_target->GetYDir(prec);
}