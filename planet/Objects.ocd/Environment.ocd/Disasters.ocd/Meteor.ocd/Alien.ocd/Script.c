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
		Alpha = PV_Linear(255,0),
		Size = PV_Linear(30,150),
		Stretch = 1000,
		Phase = 0,
		Rotation = PV_Random(-GetR() - 15, -GetR() + 15),
		DampingX = 1000,
		DampingY = 1000,
		BlitMode = GFX_BLIT_Additive,
		CollisionVertex = 0,
		OnCollision = PC_Die(),
		Attach = nil
	};
	fx.sparkright = 
	{
		R = 255,
		B = 255,
		G = 255,
		Alpha = PV_KeyFrames(0, 0, 0, 500, 255, 1000, 0),
		Size = PV_Linear(20,100),
		Stretch = 1000,
		Phase = 0,
		Rotation = 30,
		ForceX = 0,
		ForceY = 0,
		DampingX = 1000,
		DampingY = 1000,
		BlitMode = GFX_BLIT_Additive,
		CollisionVertex = 0,
		OnCollision = PC_Die(),
		Attach = ATTACH_Back | ATTACH_MoveRelative
	};
	fx.sparkleft = 
	{
		Prototype = fx.sparkright,
		Size = PV_Linear(30,100),
		Rotation = -30,
	};
	fx.trail = 
	{
		R = 255,
		B = 255,
		G = 255,
		Alpha = PV_Linear(255,0),
		Size = GetCon()/3,
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
	
	// Fire trail.
	fx.fire.ForceX = -GetXDir()/2 + RandomX(-5,5);
	fx.fire.ForceY = -GetYDir()/2;
	CreateParticle("BlueFireTrail", PV_Random(-1, 1), -15, 0, -GetYDir(), 7, fx.trail, 1);
	
	CreateParticle("BlueSpark", 10, 15, 100 + GetXDir(), -GetYDir(), 20, fx.sparkright, 1);
	CreateParticle("BlueSpark", -10, 15, -100 + GetXDir(), -GetYDir(), 20, fx.sparkleft, 1);
	CreateParticle("BlueFire", PV_Random(-8, 8), PV_Random(-15, -35), 0, 0, 30, fx.fire, 4);
	return 1;
}

/*-- Proplist --*/

local Name = "$Name$";
