// Stone door destructible, and auto control for the base.

#appendto StoneDoor

private func GetStrength() { return 300; }

protected func Damage()
{
	// Destroy if damage above strength.
	if (GetDamage() > GetStrength())
	{
		CastObjects(Ice, 5, 20);
		return RemoveObject();
	}
	// Change appearance.
	DoGraphics();
	return;
}