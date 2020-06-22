/**
	Lead Bullet
	Ammunition for various weapons.	
	
	@author Ringwall, Maikel
*/

#include Library_Stackable

public func MaxStackCount() { return 40; }

public func IsBullet() { return true; }
public func ProjectileDamage() { return 5; }
public func TumbleStrength() { return 100; }
public func FlightTime() { return 30; }

protected func Hit()
{
	if (GetEffect("HitCheck", this))
	{
		RemoveEffect("HitCheck", this);
		Sound("Objects::Weapons::Blunderbuss::BulletHitGround?");
		CreateParticle("StarSpark", 0, 0, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(10, 20), Particles_Glimmer(), 3);
		RemoveObject();
	}
	return;
}

public func Launch(object shooter, int angle, int dist, int speed, int offset_x, int offset_y, int prec_angle)
{
	SetController(shooter->GetController());
	AddEffect("HitCheck", this, 1, 1, nil, nil, shooter);

	LaunchProjectile(angle, dist, speed, offset_x, offset_y, prec_angle);
	
	// Remove after some time.
	SetAction("Travel");
	SetComDir(COMD_None);

	// Smush vertexes into one point.
	SquishVertices(true);
	
	// Neat trail.
	CreateObject(BulletTrail)->Set(this, 2, 200);
			
	// Sound.
	Sound("Objects::Weapons::Blunderbuss::BulletShot?");
}

public func HitObject(object obj)
{
	if (WeaponCanHit(obj))
	{
		if (obj->GetAlive())
			Sound("Hits::ProjectileHitLiving?");
		else
			Sound("Objects::Weapons::Blunderbuss::BulletHitGround?");
		
		obj->~OnProjectileHit(this);
		WeaponDamage(obj, this->ProjectileDamage(), FX_Call_EngObjHit, false);
		WeaponTumble(obj, this->TumbleStrength());
		if (!this)
			return;
	}
	RemoveObject();
	return;
}


public func UpdatePicture()
{
	var nr_bullets = GetStackCount();
	if (nr_bullets >= MaxStackCount())
		SetGraphics(nil);
	if (nr_bullets < MaxStackCount())
		 SetGraphics(Format("%d", nr_bullets));
	// Realigns vertex points if more than one bullet.
	var do_squish = nr_bullets == 1;
	SquishVertices(do_squish);
	_inherited(...);
}

private func SquishVertices(bool squish)
{
	if (squish)
	{
		SetVertex(1, VTX_X, 0, 2);
		SetVertex(1, VTX_Y, 0, 2);
		SetVertex(2, VTX_X, 0, 2);
		SetVertex(2, VTX_Y, 0, 2);
	}
	else
	{
		SetVertex(1, VTX_X, -3, 2);
		SetVertex(1, VTX_Y, 1, 2);
		SetVertex(2, VTX_X, 3, 2);
		SetVertex(2, VTX_Y, 1, 2);
	}
	return;
}

protected func Traveling()
{
	if (GetActTime() > FlightTime())
		RemoveObject();
}

public func TrailColor(int time)
{
	return RGBa(255, 255, 255, 240 * Max(0, FlightTime() - time) / FlightTime());
}

public func IsArmoryProduct() { return true; }

local ActMap = {
	Travel = {
		Prototype = Action,
		Name = "Travel",
		Procedure = DFA_FLOAT,
		NextAction = "Travel",
		Length = 1,
		Delay = 1,
		FacetBase = 1,
		StartCall = "Traveling"
	},
};
local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Components = {Metal = 1};