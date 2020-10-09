// Stone door taking damage from projectiles
// Just reroute energy loss to damage

#appendto StoneDoor

func Initialize()
{
	// smaller SolidMask and larger shape so doors can take damage more easily
	SetSolidMask(2, 0, 4, 40, 2, 0);
	SetShape(-10,-20, 20, 40);
	// effect to reroute life loss to damage
	AddEffect("IntLife2Damage", this, 1, 0, this);
	return _inherited(...);
}

func Death()
{
	Damage();
	return _inherited(...);
}

func FxIntLife2DamageDamage(target, fx, dmg, cause)
{
	if (cause & 32) if (dmg<0) Damage();
	return dmg;
}

func GetDamage() { return GetStrength() - GetEnergy() + 1; }
func GetStrength() { return this.MaxEnergy/1000; }



// Smaller search radius
func Find_InRect(x, y, wdt, hgt) { return inherited(x/2, y + 10, wdt/2, hgt-20); }

// Open for all players
func Find_Allied(plr) { return Find_Not(Find_Owner(ENEMY)); }
