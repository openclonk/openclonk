/**
	IronBomb
	Explodes after a short fuse. Explodes on contact if shot by the grenade launcher.
	
	@author Ringwaul, Clonkonaut
*/

// If true, explodes on contact.
local armed; 

public func ControlUse(object clonk, int x, int y)
{
	// If already activated, nothing (so, throw).
	if (GetEffect("FuseBurn", this))
	{
		clonk->ControlThrow(this, x, y);
		return true;
	}
	Fuse();
	return true;
}

public func Fuse(bool explode_on_hit)
{
	armed = explode_on_hit;
	AddEffect("FuseBurn", this, 1, 1, this);
	return;
}

public func FuseTime() { return 90; }

public func IsFusing() { return !!GetEffect("FuseBurn", this); }

public func OnCannonShot(object cannon)
{
	return Fuse(true);
}

public func FxFuseBurnStart(object target, effect fx, int temp)
{
	if (temp)
		return FX_OK;
	Sound("Fire::FuseLoop", {loop_count = +1});
	return FX_OK;
}


public func FxFuseBurnTimer(object target, effect fx, int time)
{
	// Emit some smoke from the fuse hole.
	var i = 3;
	var x = +Sin(GetR(), i);
	var y = -Cos(GetR(), i);
	CreateParticle("Smoke", x, y, x, y, PV_Random(18, 36), Particles_Smoke(), 2);
	// Explode if time is up.
	if (time >= FuseTime())
	{
		DoExplode();
		return FX_Execute_Kill;
	}
	return FX_OK;
}

public func FxFuseBurnStop(object target, effect fx, int reason, bool temp)
{
	if (temp)
		return FX_OK;
	Sound("Fire::FuseLoop", {loop_count = -1});
	return FX_OK;
}

public func DoExplode()
{
	// Cast lots of shrapnel.
	var shrapnel_count = 20;
	for (var cnt = 0; cnt < shrapnel_count; cnt++)
	{
		var shrapnel = CreateObjectAbove(Shrapnel);
		shrapnel->SetVelocity(Random(359), RandomX(100, 140));
		shrapnel->SetRDir(-30 + Random(61));
		shrapnel->Launch(GetController());
		CreateObject(BulletTrail)->Set(shrapnel, 2, 30);
	}
	if (GBackLiquid())
		Sound("Fire::BlastLiquid2");
	else
		Sound("Fire::BlastMetal");
	CreateParticle("Smoke", PV_Random(-30, 30), PV_Random(-30, 30), 0, 0, PV_Random(40, 60), Particles_Smoke(), 60);
	Explode(28);
	return;
}

protected func Hit(int x, int y)
{
	if (armed) 
		return DoExplode();
	return StonyObjectHit(x, y);
}

protected func Incineration(int caused_by)
{
	Extinguish(); 
	Fuse();
	SetController(caused_by);
	return;
}

// Drop fusing bomb on death to prevent explosion directly after respawn
public func IsDroppedOnDeath(object clonk)
{
	return !!GetEffect("FuseBurn", this);
}

public func HasExplosionOnImpact() { return armed; }

public func IsWeapon() { return true; }
public func IsArmoryProduct() { return true; }
public func IsGrenadeLauncherAmmo() { return true; }
public func IsExplosive() { return true; }

/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = true;
local BlastIncinerate = 1;
local ContactIncinerate = 1;
local Components = {Firestone = 1, Metal = 1};