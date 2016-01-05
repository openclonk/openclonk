//Airship Hitbox

public func IsProjectileTarget() { return true; }

public func Damage(int change, int cause, int by_player)
{
	//forward the damage to airship parent (losing the damage type)
	var parent = GetActionTarget();
	if (parent) 
		parent->DoDamage(change, cause, by_player); 
	else 
		RemoveObject();
}

// remove if airship is lost
public func AttachTargetLost() { RemoveObject(); }

// Only save main airship object
func SaveScenarioObject() { return false; }

local ActMap = {
		Attach = {
			Prototype = Action,
			Name = "Attach",
			Procedure = DFA_ATTACH,
			Directions = 1,
			X = 0,
			Y = 0,
			Wdt = 64,
			Hgt = 54,
			NextAction = "Attach",
		},
};
