/*-- 
	Sword target 
	Author: Maikel
	
	Straw man that bursts if struck by a sword.
--*/

protected func Initialize()
{
	SetAction("Float");
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
	SetProperty("ActMap", {
		Float = {
			Prototype = Action,
			Name = "Float",
			Procedure = DFA_FLOAT,
			Directions = 1,
			FlipDir = 0,
			Length = 1,
			Delay = 1,
			X = 0,
			Y = 0,
			Wdt = 25,
			Hgt = 25,
			NextAction = "Float",
		},
	}, def);
}