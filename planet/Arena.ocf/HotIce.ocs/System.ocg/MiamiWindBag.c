#appendto WindBag

public func FxIntBurstWindStart(object target, proplist effect, int temp, object clonk, int x, int y)
{
	if (g_theme != MiamiIce) return inherited(target, effect, temp, clonk, x, y, ...);

	if (temp)
		return FX_OK;
	effect.Interval = 1;
	effect.clonk = clonk;
	effect.x = clonk->GetX();
	effect.y = clonk->GetY();
	effect.angle = Angle(0, 0, x, y);
	// Sound effect.
	Sound("Objects::Windbag::Gust");
	// Particle effect.

	var clr = HSL(Random(255), 255, 100);

	for (var dr = 12; dr < 32; dr++)
	{
		var r = RandomX(-20, 20);
		var sx = Sin(effect.angle + r, dr / 2);
		var sy = -Cos(effect.angle + r, dr / 2);
		var vx = Sin(effect.angle + r, 2 * fill_amount / 3 + 12);
		var vy = -Cos(effect.angle + r, 2 * fill_amount / 3 + 12);
		if (!GBackSolid(sx, sy))
			CreateParticle("Air", sx, sy, vx, vy, 36, Particles_Colored(Particles_Air(), clr));
	}
	// Make a timer call for the instant movement effect.
	FxIntBurstWindTimer(target, effect, 0);
	return FX_OK;
}
