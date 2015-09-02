#appendto RelaunchContainer

public func OnWeaponSelected(id weapon)
{
	// Two items for special objects.
	if (weapon == Firestone || weapon == Dynamite) GiveWeapon(weapon);
	return inherited(weapon, ...);
}
