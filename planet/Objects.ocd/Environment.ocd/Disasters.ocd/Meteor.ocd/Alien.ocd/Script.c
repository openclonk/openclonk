/**
	Alien Meteor Skin
	A new meteorite skin that can f.e. be used when spawning aliens.
	
	@author Win	
*/

#include Meteor

public func OnAfterLaunch()
{
	// Adjust rotation to point into movement direction. 
	// You can't do this unless you also change the rotation of the trail -Win
	//SetR(Angle(0, 0, GetXDir(), GetYDir()));
	// Emits light
	SetLightRange(400, 100);
	SetLightColor(RGB(100, 254, 255));
	// Add particle effects.
	AddEffect("IntMeteor", this, 100, 1, this);
}

private func FxIntMeteorStart(object target, effect fx, bool temp)
{
	if (temp) return;
	fx.fire = 
	{
		R = 255,
		B = 255,
		G = 255,
		Alpha = PV_Linear(150, 0),
		Size = 30,
		Stretch = 1000,
		Phase = 0,
		Rotation = PV_Random(0, 359),
		DampingX = 1000,
		DampingY = 1000,
		BlitMode = GFX_BLIT_Additive,
		CollisionVertex = 0,
		OnCollision = PC_Die(),
		Attach = nil
	};
	fx.smoketrail = 
	{
		R = 255,
		B = 255,
		G = 255,
		
		Alpha = PV_KeyFrames(1000, 0, 0, 300, 255, 1000, 0),
		Size = PV_Linear(30, 60),
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
	fx.frontburn = 
	{
		R = 200,
		B = 255,
		G = 255,
		Alpha = PV_KeyFrames(0, 0, 0, PV_Random(200, 600), 0, 700, 255, 1000, 0),
		Size = PV_Random(30, 80),
		Stretch = 1000,
		Phase = 0,
		Rotation = 0,
		ForceX = 0,
		ForceY = 0,
		DampingX = 1000,
		DampingY = 1000,
		BlitMode = GFX_BLIT_Additive,
		CollisionVertex = 0,
		OnCollision = PC_Die(),
		Attach = ATTACH_Front | ATTACH_MoveRelative
	};
}

private func FxIntMeteorTimer(object target, effect fx)
{
	var size = GetCon();
	// Air drag.
	var ydir = GetYDir(100);
	ydir -= size * ydir ** 2 / 11552000; // Magic number.
	SetYDir(ydir, 100);
	
	// Smoke trail.
	CreateParticle("SmokeThick", 0, 0, PV_Random(-3, 3), PV_Random(-3, 3), 200, fx.smoketrail, 5);
	
	CreateParticle("FrontBurn", PV_Random(-2, 2), 15, 0, 0, 7, fx.frontburn, 2);
	
	CreateParticle("BlueFire", 0, -6, PV_Random(-3, 3), 0, 5, fx.fire, 2);
	return 1;
}

/*-- Proplist --*/

local Name = "$Name$";
