/*-- 
	Sword target 
	Author: Maikel
	
	Straw man that bursts if struck by a sword.
--*/

protected func Initialize()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(45+Random(46),0,1,0)));
}

public func Burst()
{
	CreateParticle("Straw", 0, 0, PV_Random(-30, 30), PV_Random(-30,30), PV_Random(30, 120), Particles_Straw(), 200);
	RemoveObject();
	return;
}

public func IsProjectileTarget() { return true; }

public func Damage()
{
	Burst();
	return;
}

protected func Definition(def) 
{
	SetProperty("Name", "$Name$", def);
}
