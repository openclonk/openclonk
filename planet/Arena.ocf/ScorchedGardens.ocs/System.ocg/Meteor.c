#appendto Meteor

private func OnAfterLaunch()
{
	DoCon(100);
	return inherited(...);
}

private func FxIntMeteorStart(object target, effect fx, bool temp)
{
	if (temp) return;
	fx.smoketrail = 
	{
		R = 255,
		B = PV_KeyFrames(0,  0, 100,    30, 0,  100, 255, 1000, 255),
		G = PV_KeyFrames(0,  0, 150,  30, 0, 100, 255, 1000, 255),
		
		Alpha = PV_KeyFrames(1000, 0, 0, 30, 255, 1000, 0),
		Size = PV_Linear(10, 30),
		Stretch = 1000,
		Phase = PV_Random(0, 4),
		Rotation = PV_Random(-GetR() - 15, -GetR() + 15),
		DampingX = 1000,
		DampingY = 1000,
		BlitMode = 0,
		CollisionVertex = 0,
		OnCollision = PC_Stop(),
		Attach = nil
	};
	fx.brighttrail = 
	{
		Prototype = fx.smoketrail,
		Alpha = PV_Linear(180, 0),
		Size = PV_Linear(20, 30),
		BlitMode = GFX_BLIT_Additive,
	};
	fx.frontburn = 
	{
		R = 255,
		B = 50,
		G = 190,
		
		Alpha = PV_KeyFrames(0, 0, 0, 500, 25, 1000, 0),
		Size = PV_Linear(4, 5),
		Stretch = 1000,
		Phase = PV_Random(0, 4),
		Rotation = PV_Random(-GetR() - 15, -GetR() + 15),
		DampingX = 800,
		DampingY = 800,
		BlitMode = GFX_BLIT_Additive,
		CollisionVertex = 0,
		OnCollision = PC_Stop(),
		Attach = ATTACH_Front | ATTACH_MoveRelative,
		ForceX = PV_Random(-10, 10, 10),
		ForceY = PV_Random(-10, 10, 10),
	};
}

protected func FxIntMeteorTimer(object target, effect fx, bool temp)
{
	var size = GetCon();
	// Air drag.
	var ydir = GetYDir(100);
	ydir -= size * ydir ** 2 / 11552000; // Magic number.
	SetYDir(ydir, 100);
	// Smoke trail.
	CreateParticle("SmokeThick", PV_Random(-5, 5), PV_Random(-5, 5), PV_Random(-3, 3), PV_Random(-3, 3), 20, fx.smoketrail, 5);
	// Flash
	CreateParticle("SmokeThick", PV_Random(-2, 2), PV_Random(-2, 2), PV_Random(-3, 3), PV_Random(-3, 3), 3, fx.brighttrail, 2);
	// left and right burn
	CreateParticle("FireDense", PV_Random(-5, 5), PV_Random(-5, 5), PV_Random(-20, 20), PV_Random(-20, 20), 40, fx.frontburn, 20);
	return 1;
}
