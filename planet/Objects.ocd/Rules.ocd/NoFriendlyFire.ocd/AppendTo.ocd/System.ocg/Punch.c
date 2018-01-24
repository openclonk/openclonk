// Disable friendly fire for damage done through the Punch function.
// This needs be done by overloading because the callback QueryCatchBlow
// only halves the damage and prevents the tumbling.

global func Punch(object obj, int strength)
{
	var w_controller = NO_OWNER;
	if (this)
		w_controller = this->GetController();
	var t_controller = obj->GetController();
	if (FindObject(Find_ID(Rule_NoFriendlyFire)))
		if (w_controller != NO_OWNER && t_controller != NO_OWNER && !Hostile(w_controller, t_controller))
			return false;
	return _inherited(obj, strength, ...);
}
