/**
	Shrapnel
	Fragment of the iron bomb.
	
	@author Ringwaul
*/

public func ProjectileDamage() { return 3; }
public func TumbleStrength() { return 100; }
public func FlightTime() { return 4; }

protected func Initialize()
{
	SetAction("Flight");
	AddEffect("Fade", this, 1, 1, this);
}

public func Launch(int shooter)
{
	SetController(shooter);
	AddEffect("HitCheck", this, 1, 1, nil, nil);
}

protected func FxFadeTimer(object target, proplist effect, int timer)
{
	if (timer > FlightTime()) 
		RemoveObject();
	return FX_OK;
}

protected func Hit()
{
	ShakeFree(6);
	RemoveEffect("HitCheck", this);
	Sound("Objects::Weapons::Blunderbuss::BulletHitGround?");
	CreateParticle("StarSpark", 0, 0, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(10, 20), Particles_Glimmer(), 3);
	return RemoveObject();
}

public func HitObject(object obj)
{
	Sound("Hits::ProjectileHitLiving?");
	
	if (WeaponCanHit(obj))
	{
		obj->~OnProjectileHit(this);
		WeaponDamage(obj, this->ProjectileDamage(), FX_Call_EngObjHit);
		WeaponTumble(obj, this->TumbleStrength());
	}
	return RemoveObject();
}

public func TrailColor(int time)
{
	return RGBa(100, 100, 100, 240 * Max(0, FlightTime() - time) / FlightTime());
}

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLIGHT,
		NextAction = "Fly",
		Delay = 1,
		Length = 1,
	}
};