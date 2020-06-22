/* Apply upgrades to ranged weapons */

#appendto Bow
#appendto Blunderbuss
#appendto GrenadeLauncher
#appendto Javelin

local orig_shooting_strength;

public func Initialize(...)
{
	var r = _inherited(...);
	this.gidl_base_animation_set = this.animation_set;
	Gidl_UpdateLoadTimes();
	orig_shooting_strength = this.shooting_strength;
	Guardians_UpdateShootingStrength();
	return r;
}

public func Entrance(...)
{
	Gidl_UpdateLoadTimes();
	Guardians_UpdateShootingStrength();
	return _inherited(...);
}

public func Gidl_UpdateLoadTimes()
{
	if (!Contained() || !g_homebases) return false;
	var base = g_homebases[Contained()->GetOwner()];
	if (base)
	{
		var base_set = this.gidl_base_animation_set, new_set;
		this.animation_set = new_set = new base_set {};
		if (GetType(base_set.LoadTime)) new_set.LoadTime = Max(1, base_set.LoadTime * base.tech_load_speed_multiplier / 100);
		if (GetType(base_set.LoadTime2)) new_set.LoadTime2 = Max(1, base_set.LoadTime2 * base.tech_load_speed_multiplier / 100); // Bow: add arrow
		if (GetType(base_set.ShootTime)) new_set.ShootTime = Max(1, base_set.ShootTime * base.tech_load_speed_multiplier / 100);
		if (GetType(base_set.ShootTime2)) new_set.ShootTime2 = Max(1, base_set.ShootTime2 * base.tech_load_speed_multiplier / 100);
		if (new_set.ShootTime <= new_set.ShootTime2)
			new_set.ShootTime = new_set.ShootTime2 + 1;
	}
	return true;
}

public func Guardians_UpdateShootingStrength()
{
	if (!Contained() || !g_homebases) return false;
	if (GetID() == Blunderbuss) return false; // Update not for blunderbusses
	var base = g_homebases[Contained()->GetOwner()];
	if (base)
	{
		this.shooting_strength = orig_shooting_strength + orig_shooting_strength * base.tech_shooting_strength_multiplier / 100;
	}
	return true;
}

public func Gidl_IsRangedWeapon() { return true; }
