#appendto IronBomb

public func FxFuseBurnTimer(object target, effect fx, int time)
{
	if (g_theme != MiamiIce) return inherited(target, fx, time, ...);

	if (!fx.clr)
		fx.clr = HSL(Random(255), 255, 100);

	// Emit some smoke from the fuse hole.
	var i = 3;
	var x = +Sin(GetR(), i);
	var y = -Cos(GetR(), i);
	CreateParticle("Smoke", x, y, x, y, PV_Random(18, 36), Particles_Colored(Particles_Smoke(), fx.clr), 2);
	// Explode if time is up.
	if (time >= FuseTime())
	{
		DoExplode();
		return FX_Execute_Kill;
	}
	return FX_OK;
}
