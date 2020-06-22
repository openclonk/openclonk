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

// Helper function: Convert target coordinates and projectile out speed to desired shooting angle. Because
// http://en.wikipedia.org/wiki/Trajectory_of_a_projectile says so. No SimFlight checks to check upper
// angle (as that is really easy to evade anyway) just always shoot the lower angle if sight is free.
private func GetBallisticAngle(int x, int y, int tx, int ty, int v, int max_angle)
{
	var dx = tx - x;
	var dy = ty - y;	
	// The variable v is in 1/10 pix/frame.
	// The variable gravity is in 1/100 pix/frame^2.
	var g = GetGravity();
	// Correct vertical distance to account for integration error. Engine adds gravity after movement, so
	// targets fly higher than they should. Thus, we aim lower. we don't know the travel time yet, so we
	// assume some 90% of v is horizontal (it's ~2px correction for 200px shooting distance).
	dy += Abs(dx) * g * 10 / (v * 180);
	// The variable q is in 1/10000 (pix/frame)^4 and dy is negative up.
	var q = v**4 - g * (g * dx * dx - 2 * dy * v * v);
	 // Check if target is out of range.
	if (q < 0)
		return nil;
	// Return lower angle if possible.
	var lower_angle = (Angle(0, 0, g * dx, Sqrt(q) - v * v, 10) + 1800) % 3600 - 1800;
	if (Inside(lower_angle, -10 * max_angle, 10 * max_angle) && this->CheckBallisticPath(x, y, tx, ty, v, lower_angle))
		return lower_angle;
	// Otherwise return upper angle if that one is possible.
	var upper_angle = (Angle(0, 0, g * dx, -Sqrt(q) - v * v, 10) + 1800) % 3600 - 1800;
	if (Inside(upper_angle, -10 * max_angle, 10 * max_angle) && this->CheckBallisticPath(x, y, tx, ty, v, upper_angle))
		return upper_angle;	
	// No possible shooting angle.
	return nil;
}

// Returns whether the ballistic path is free.
public func CheckBallisticPath(int x, int y, int tx, int ty, int v, int angle)
{
	// The projected flight time is now known.
	var vx = Sin(angle, v * 10, 10);
	var vy = -Cos(angle, v * 10, 10);
	var flight_time = 100 * Abs(tx - x) / Max(1, Abs(vx));
	// Simulate the flight and see if the flight time corresponds to the expected time.
	var flight = this->SimFlight(x, y, vx, vy, nil, nil, flight_time, 100);
	//Log("(%d, %d)->(%d, %d) with v = (%d, %d) and angle = %d in t = %d %v", x, y, tx, ty, vx, vy, angle / 10, flight_time, flight);
	// Both the projected and the simulated flight times should be the same for a free path.
	return -flight[4] == flight_time;
}
