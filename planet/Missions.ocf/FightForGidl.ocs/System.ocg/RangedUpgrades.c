/* Apply upgrades to ranged weapons */

#appendto Bow
#appendto Musket
#appendto GrenadeLauncher
#appendto Javelin

public func Initialize(...)
{
	var r = _inherited(...);
	this.gidl_base_animation_set = this.animation_set;
	Gidl_UpdateLoadTimes();
	return r;
}

public func Entrance(...)
{
	Gidl_UpdateLoadTimes();
	return _inherited(...);
}

private func Gidl_UpdateLoadTimes()
{
	if (!Contained()) return false;
	var base = g_homebases[Contained()->GetOwner()];
	if (base)
	{
		var base_set = this.gidl_base_animation_set, new_set;
		this.animation_set = new_set = new base_set {};
		if (GetType(base_set.LoadTime)) new_set.LoadTime = Max(1, base_set.LoadTime * base.tech_load_speed_multiplier / 100);
		if (GetType(base_set.LoadTime2)) new_set.LoadTime2 = Max(1, base_set.LoadTime2 * base.tech_load_speed_multiplier / 100); // Bow: add arrow
		if (GetType(base_set.ShootTime)) new_set.ShootTime = Max(1, base_set.ShootTime * base.tech_load_speed_multiplier / 100);
	}
	return true;
}

public func Gidl_IsRangedWeapon() { return true; }
