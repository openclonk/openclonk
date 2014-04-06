/**
	Straw man
	Clonk sized puppet that bursts if:
	 * struck by a weapon (sword, axe)
	 * hit by a projectile
	 * hit by a shockwave
	Also incinerates on contact and blasts.
	
	@author Ringwall, Maikel
*/


protected func Initialize()
{
	// Position straw man facing screen plus deviations around y-axis.
	SetProperty("MeshTransformation", Trans_Rotate(30 + Random(76), 0, 1, 0));
	return;
}

// Create straw particles and remove object when burst.
public func Burst()
{
	CreateParticle("Straw", 0, 0, PV_Random(-30, 30), PV_Random(-30, 30), PV_Random(30, 120), Particles_Straw(), 200);
	return RemoveObject();
}

// Receptive to projectiles.
public func IsProjectileTarget() { return true; }

// Burst on projectile hits.
public func OnProjectileHit()
{
	Burst();
}

// Burst on punching damage: sword or axe.
// Remove object after con < 30 from fire damage.
public func Damage(int change, int type)
{
	if (type == FX_Call_EngGetPunched)
		return Burst();
	if (type == FX_Call_DmgFire && GetCon() < 30)
	{
		CastPXS("Ashes", 20, 15);
		return RemoveObject();
	}
}

// Receptive to shockwaves.
public func CanBeHitByShockwaves() { return true; }

// Burst on shockwaves.
public func OnShockwaveHit(int level, int x, int y)
{
	Burst();
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Plane = 300;
local BlastIncinerate = 1;
local ContactIncinerate = 2;
