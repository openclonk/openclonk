// Stone door destructible.

#appendto StoneDoor

protected func Damage()
{
	if (GetDamage() > 180)
		Split2Components();
	return;
}