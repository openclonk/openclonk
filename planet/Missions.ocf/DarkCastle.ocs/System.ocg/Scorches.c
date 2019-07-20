global func AddScorch(int x, int y, int r, int strength, int duration)
{
	var scorch = CreateObjectAbove(Wood, x, y, NO_OWNER);
	if (!scorch) return nil;
	scorch->SetObjectLayer(scorch);
	scorch->SetR(r);
	scorch->SetClrModulation(0x80804000);
	scorch->SetCategory(C4D_StaticBack);
	scorch.Collectible = false; // SetObjectLayer is not enough...
	scorch.Plane = this.Plane + 1;
	var fx = AddEffect("FireScorching", scorch, 1, 2, scorch);
	fx.strength = strength;
	fx.duration = duration;
	return scorch;
}

global func FxFireScorchingTimer(object target, proplist effect, int time)
{
	if (time >= effect.duration) { RemoveObject(); return FX_Execute_Kill; }
	// particles
	var wind = BoundBy(GetWind(), -5, 5);
	CreateParticle("SmokeDirty", PV_Random(-5, 5), PV_Random(-5, 5), wind, -effect.strength/8, PV_Random(20, 40), Particles_SmokeTrail(), 2);
	return FX_OK;
}
