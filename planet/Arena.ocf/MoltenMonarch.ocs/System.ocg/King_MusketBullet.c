#appendto LeadBullet

local lowering;

public func LessDamage()
{
	lowering = 6;
	SetClrModulation(RGB(255, 128, 0));
}

public func ProjectileDamage() { return 10 - lowering; }

public func TrailColor(int time)
{
	var r = 255, g = 255, b = 255;
	if (lowering)
	{
		r = 255 - Min(time * 5);
		g = 255 - Min(time * 18, 255);
		b = 0;
	}
	return RGBa(255, 255, 255, 240 * Max(0, FlightTime() - time) / FlightTime());
}

public func Launch(object shooter, int angle, int dist, int speed, int offset_x, int offset_y, int prec_angle)
{
	if (speed > Blunderbuss.BulletSpeed[1])
	{
		LessDamage();
	}

	return _inherited(shooter, angle, dist, speed, offset_x, offset_y, prec_angle);
}