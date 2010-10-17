// Castle door destructible.

#appendto CastleDoor

protected func Damage()
{
	if (GetDamage() > 180)
		RemoveObject();
	return;
}