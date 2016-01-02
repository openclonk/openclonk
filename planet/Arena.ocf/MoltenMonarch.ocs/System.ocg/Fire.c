// Called if the object is submerged in incendiary material (for example in lava).
// The effect is made stronger for this scenario.
global func OnInIncendiaryMaterial()
{
	this->DoEnergy(-7, false, FX_Call_EngFire, NO_OWNER);
	return inherited(...);
}
