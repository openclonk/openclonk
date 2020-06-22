/*
	Bomb Arrow
	Same as an arrow but explodes on impact. Overloads from the arrow script
	and implements additional features.
	
	@author Maikel
*/

#include Arrow


// Callback from the hitcheck effect: explode on impact with target.
public func HitObject(object obj)
{
	// First let the normal arrow hit with reduced damage take place, then explode for additonal damage.
	_inherited(obj, ...);	
	// Explosion strength a little random with only a small radius.
	return Explode(14 + Random(3));
}

// Callback on hit: explode on impact with landscape.
public func Hit()
{
	// Only explode on impact with the landscape if the object was in flight.
	// Explosion strength a little random with only a small radius.
	if (GetEffect("InFlight", this))
		return Explode(12 + Random(3));
	return;
}

// Determines the arrow strength: only 30% that of the normal arrow.
public func ArrowStrength() { return 3; }

public func HasExplosionOnImpact() { return !!GetEffect("InFlight", this); }

public func IsExplosive() { return true; }

private func OnBurnDown()
{
	// Got burned? Explode based on stack size.
	Explode(10 + Random(3) + 23 * GetStackCount() / MaxStackCount());
	_inherited(...);
	return true; // Do not create burned object
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Components = {Wood = 3, Metal = 1, Firestone = 2};