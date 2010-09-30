/*-- 
	Sword target 
	Author: Maikel
	
	Straw man that bursts if struck by a sword.
--*/

protected func Initialize()
{
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(225+Random(91),0,1,0), Trans_Scale(800)));
}

public func Burst()
{
	CastParticles("Straw", 100, 25, 0, -3, 30, 40, RGB(255,255,255), RGB(255,255,255));
	RemoveObject();
	return;
}

public func OnSwordHit()
{
	Burst();
	return;
}

protected func Definition(def) 
{
	SetProperty("Name", "$Name$", def);
}