/*
	Fire Arrow
	Same as an arrow but is ignited when being fired. Overloads from the arrow script
	and implements additional features.
	
	@author Maikel
*/

#include Arrow


// Callback from the bow: add burning effect to the arrow here.
public func Launch(int angle, int str, object shooter, object weapon)
{
	AddEffect("IntBurning", this, 1, 1, this);
	// Forward to the arrow for other functionality.
	return _inherited(angle, str, shooter, weapon, ...);
}


// Burning effect: takes care of the particles and light radius.
public func FxIntBurningStart(object target, proplist effect, int temp)
{
	if (temp) 
		return 1;
	// The arrow burns for 8 seconds.	
	effect.burn_time = 36 * 8;
	// Set interval to every frame.
	effect.Interval = 1;
	return 1;
}

public func FxIntBurningTimer(object target, proplist effect, int time)
{
	// Check if burn time already has been exceeded.
	if (time >= effect.burn_time)
	{
		// If the fire arrow is burned up it changes to a normal arrow.
		ChangeDef(Arrow);
		// Update picture and set light range to zero in the new arrow.
		this->UpdatePicture();
		SetLightRange(0);
		return -1;
	}
	
	// In the last second the burning reduces a bit.
	var burn_level = BoundBy(3 * (effect.burn_time - time), 10, 100);
	// The rotation of the arrow determines the offset for the particles.
	var x = Sin(GetR(), 2);
	var y = -Cos(GetR(), 2);
	// Fire effects.
	var particle_fire = Particles_Fire();
	particle_fire.Size = PV_KeyFrames(0, 0, PV_Random(2, 4), 500, 2, 1000, 0);
	CreateParticle("Fire", PV_Random(x - 2, x + 2), PV_Random(y - 2, y + 2), PV_Random(-1, 1), PV_Random(-1, 1), 20 + Random(10), particle_fire, burn_level / 30);
	// Smoke effects.
	var particle_smoketrail = Particles_SmokeTrail();
	particle_smoketrail.Size = PV_KeyFrames(0, 0, PV_Random(2, 3), 200, PV_Random(4, 6), 1000, PV_Random(4, 6));
	particle_smoketrail.ForceY = nil;
	CreateParticle("Smoke", PV_Random(x - 1, x + 1), PV_Random(y - 1, y + 1), PV_Random(-1, 1), PV_Random(-1, 1), 40 + Random(20), particle_smoketrail, burn_level / 30);
	var particle_smoke = Particles_Smoke();
	particle_smoke.Size = PV_Linear(PV_Random(1, 3), PV_Random(2, 4));
	CreateParticle("Smoke", PV_Random(x - 1, x + 1), PV_Random(y - 1, y + 1), PV_Random(-2, 2), PV_Random(-2, 2), 40 + Random(20), particle_smoke, burn_level / 30);
	// Light level.
	SetLightRange(burn_level / 3, burn_level / 3);
	SetLightColor(FIRE_LIGHT_COLOR);
	return 1;

}

public func FxIntBurningStop(object target, proplist effect, int reason, bool temp)
{
	if (temp) 
		return 1;
	// Set light range to zero.	
	SetLightRange(0);
	return 1;
}

// Callback from the engine on entering another object.
protected func Entrance()
{
	// Remove the burning effect when this object is collected, light range is set to zero automatically.
	// Collecting the fire arrow fast enough means you can reuse it again.
	RemoveEffect("IntBurning", this);
	return _inherited(...);
}

// Callback from the hitcheck effect: incinerate target on impact.
public func HitObject(object obj)
{
	// Incinerate object to the amount where a clonk (ContactIncinerate = 10) won't fully burn.
	// ContactIncinerate = 1 implies 100% incinaration.
	// ContactIncinerate = 10 implies 37-43% incinaration.
	// Hitting the same clonk twice with a fire arrow usually means it while burn indefinitely.
	// Check before incinerating if the hit was blocked by the clonk, its shield or an effect.
	if (obj.ContactIncinerate && !obj->~QueryCatchBlow(this))
		obj->Incinerate(BoundBy(100 - 7 * (obj.ContactIncinerate - 1) + Random(7), 0, 100), GetController());
	// Additional damage from normal arrow hit, however, reduced.
	return _inherited(obj, ...);
}

// Determines the arrow strength: only 30% that of the normal arrow.
public func ArrowStrength() { return 3; }


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Components = {Wood = 3, Firestone = 1, Coal = 1};
