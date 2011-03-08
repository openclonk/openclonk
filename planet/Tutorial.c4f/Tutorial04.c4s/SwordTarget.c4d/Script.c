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
	CastParticles("Straw", 100, 25, 0, -3, 30, 40, RGB(255,255,255), RGB(255,255,255));
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
