// Scorches which can placed on buildings to indicate the an attack has taken place.
// Add the scorch by object->AddScorch() with local coordinates.

// Adds a scorch to an object, x and y are local coordinates and r is the rotation.
// Strength determines the amount of smoke and duration if not nil the length of the effect.
global func AddScorch(int x, int y, int r, int strength, int duration)
{
	// Safety and create the scorch.
	if (!this)
		return;
	var scorch = CreateObject(Wood, x, y, NO_OWNER);
	if (!scorch) 
		return;
	if (strength == nil)
		strength = 50;
	// Change wood properties such that it acts as a scorch.
	scorch->SetObjectLayer(scorch);
	scorch->SetR(r);
	scorch->SetClrModulation(0x80804000);
	scorch->SetCategory(C4D_StaticBack);
	scorch.Collectible = false;
	scorch.Plane = this.Plane + 1;
	// Add an effect for creating smoke.
	var fx = AddEffect("FireScorching", scorch, 1, 2, scorch);
	fx.strength = strength;
	fx.duration = duration;
	return scorch;
}

global func FxFireScorchingTimer(object target, proplist effect, int time)
{
	if (effect.duration != nil && time >= effect.duration) 
	{ 
		RemoveObject(); 
		return FX_Execute_Kill;
	}
	// Create smoke particles which move with the wind.
	var wind = BoundBy(GetWind(), -5, 5);
	CreateParticle("SmokeDirty", PV_Random(-5, 5), PV_Random(-5, 5), wind, -effect.strength/8, PV_Random(20, 40), Particles_SmokeTrail(), 2);
	CreateParticle("Smoke", PV_Random(-2, 2), PV_Random(-2, 2), 0, 0, PV_Random(20, 40), Particles_Smoke(), 1);
	return FX_OK;
}
