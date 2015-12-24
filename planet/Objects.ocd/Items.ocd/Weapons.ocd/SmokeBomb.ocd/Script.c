/**
	Smoke bomb
	Spreads lots of smoke.
		
	@author Maikel
*/


protected func Initialize()
{

	return;
}

public func ControlUse(object clonk, int x, int y)
{
	// If already active don't do anything and let the clonk throw it.
	if (GetEffect("IntSmokeBomb", this))
		return false;
	Fuse();
	return true;
}

public func Fuse()
{
	// Add smoking effect.
	if (!GetEffect("IntSmokeBomb", this))
		AddEffect("IntSmokeBomb", this, 100, 2, this);
	return;
}

protected func Hit()
{
	Sound("Hits::Materials::Metal::DullMetalHit?");
	return;
}


/*-- Smoking --*/

protected func FxIntSmokeBombStart(object target, proplist effect, int temp)
{
	if (temp)
		return FX_OK;
	// Interval and length.
	effect.Interval = 4;
	effect.length = 18 * 36;
	// Store particles.
	effect.lifetime = 24 * 36;
	effect.smoke =
	{
		R = 255,
		G = 255,
		B = 255,
		Alpha = PV_KeyFrames(1, 0, 200, 500, PV_Random(200, 255), 1000, 0),
		CollisionVertex = 1000,
		OnCollision = PC_Stop(),
		ForceX = PV_Wind(50, PV_Random(-4, 4)),
		ForceY = PV_Random(-2, 1, 10),
		DampingX = 850,
		DampingY = 850,
		Size = PV_KeyFrames(0, 0, 1, 10, PV_Random(30, 40), 1000, PV_Random(25, 50)),
		Phase = PV_Random(0, 15)
	};
	// Sound.
	Sound("Fire::Smoke", false, 100, nil, +1);
	Sound("Liquids::SmokeSizzle", false, 100, nil, +1);
	// Make non-collectible.
	this.Collectible = false;
	return FX_OK;
}

protected func FxIntSmokeBombTimer(object target, proplist effect, int time)
{
	if (time > effect.length)
		return FX_Execute_Kill;
		
	// Increas interval to draw more particles.
	if (time > effect.length / 6)
		effect.Interval = 4;
	if (time > effect.length / 4)
		effect.Interval = 3;
	if (time > effect.length / 3)
		effect.Interval = 2;
	if (time > 2 * effect.length / 3)
		effect.Interval = 1;

	// Particles for smoke and a bit of fire.		
	var smoke_dx = GetVertex(0, VTX_X);
	var smoke_dy = GetVertex(0, VTX_Y);
	var fuse_dx = GetVertex(1, VTX_X);
	var fuse_dy = GetVertex(1, VTX_Y);
	var smoke_life = (effect.lifetime - time/2);
	CreateParticle("Smoke", smoke_dx, smoke_dy, PV_Random(smoke_dx - 100, smoke_dx + 100),  PV_Random(smoke_dy - 60, smoke_dy + 60), smoke_life, effect.smoke, 4);
	CreateParticle("Fire", fuse_dx, fuse_dy, PV_Random(2 * fuse_dx - 3, 2 * fuse_dx + 3),  PV_Random(2 * fuse_dy - 3, 2 * fuse_dy + 3), PV_Random(10, 20), Particles_Glimmer(), 2);
	
	return FX_OK;
}

protected func FxIntSmokeBombStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	// Sound.
	Sound("Fire::Smoke", false, 100, nil, -1);
	Sound("Liquids::SmokeSizzle", false, 100, nil, -1);
	RemoveObject();
	return FX_OK;
}

public func IsWeapon() { return true; }
public func IsArmoryProduct() { return true; }


/*-- Properties --*/

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";