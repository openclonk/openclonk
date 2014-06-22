#appendto PowderKeg

local boom_strength = 40;

func SetStrength(new_strength)
{
	boom_strength = new_strength;
	return true;
}

func OnContainerDeath()
{
	GoBoom();
	return true;
}

public func Damage()
{
	return GoBoom();
}

func GoBoom()
{
	Exit();
	CastObjects(Firestone, boom_strength/10, boom_strength/3);
	Explode(boom_strength);
	return true;
}
