//Airship Hitbox

local health;
local parent;

protected func Initialize()
{
	health = 30;
}

public func IsProjectileTarget(target,shooter) { return true; }

public func Damage(int change, int byplayer)
{
	if(GetDamage() > health)
	{
		parent->AirshipDeath();
	}
}

public func SetAirshipParent(object airship)
{
	parent = airship;
}

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
