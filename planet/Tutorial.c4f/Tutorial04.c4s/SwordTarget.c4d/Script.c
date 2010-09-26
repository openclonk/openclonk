/*-- 
	Sword target 
	Author: Maikel
	
	Straw man that bursts if struck by a sword.
--*/

protected func Initialize()
{
	SetAction("Float");
	SetProperty("MeshTransformation", Trans_Rotate(45+Random(91),0,1,0));
}

public func Burst()
{
	CastParticles("Straw", 130, 30, 0, -3, 30, 40, RGB(255,255,255), RGB(255,255,255));
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