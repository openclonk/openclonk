/*
	Arrow
	The arrow is the standard ammo for the bow and base object for all other
	types of arrows. This object is stackable (up to 15) as it is required
	from the bow.
	The arrow employs the HitCheck effect (in System.ocg/HitChecks.c) originally
	from Hazard to search for targets during flight.
	
	@author Newton
*/

#include Library_Stackable
#include Library_Flammable


protected func Construction()
{
	SetR(90);
	return _inherited(...);
}

public func Launch(int angle, int str, object shooter, object weapon)
{
	SetShape(-2, -2, 4, 11);
	SetVertex(0, VTX_Y, 3, 1);
	SetVertex(1, VTX_Y, 4, 1);
	SetVertex(2, VTX_Y, -2, 1);
	SetPosition(GetX(), GetY() - 2);
	var xdir = Sin(angle,str);
	var ydir = Cos(angle,-str);
	SetXDir(xdir);
	SetYDir(ydir);
	SetR(angle);
	Sound("Objects::Arrow::Shoot?");
	// Shooter controls the arrow for correct kill tracing.
	SetController(shooter->GetController());
	
	AddEffect("HitCheck", this, 1,1, nil, nil, shooter);
	AddEffect("InFlight", this, 1,1, this);
	return;
}

private func Stick()
{
	if (GetEffect("InFlight",this))
	{
		RemoveEffect("HitCheck",this);
		RemoveEffect("InFlight",this);
	
		SetXDir(0);
		SetYDir(0);
		SetRDir(0);
	
		var x = Sin(GetR(), 9);
		var y = -Cos(GetR(), 9);
		var mat = GetMaterial(x, y);
		// Stick in any material.
		if (mat != -1)
			SetVertex(2, VTX_Y, -10, 1);
	}
	return;
}

public func HitObject(object obj)
{
	// Determine damage to obj from speed and arrow strength.
	var relx = GetXDir() - obj->GetXDir();
	var rely = GetYDir() - obj->GetYDir();
	var speed = Sqrt(relx * relx + rely * rely);
	var dmg = ArrowStrength() * speed * 1000 / 100;
	
	if (WeaponCanHit(obj))
	{
		if (obj->GetAlive())
			Sound("Hits::ProjectileHitLiving?");
		else
			Sound("Objects::Arrow::HitGround");
		
		obj->~OnProjectileHit(this);
		WeaponDamage(obj, dmg, FX_Call_EngObjHit, true);
		WeaponTumble(obj, this->TumbleStrength());
	}
	
	// Stick does something unwanted to controller.
	if (this) 
		Stick();
	return;
}

public func Hit()
{
	if (GetEffect("InFlight",this))
		Sound("Objects::Arrow::HitGround");
	Stick();
}

// The flight effect rotates the arrow according to its speed.
public func FxInFlightStart(object target, proplist effect, int temp)
{
	if (temp) 
		return 1;
	effect.x = target->GetX();
	effect.y = target->GetY();
	return 1;
}

public func FxInFlightTimer(object target, proplist effect, int time)
{
	var oldx = effect.x;
	var oldy = effect.y;
	var newx = GetX();
	var newy = GetY();
	
	// And additionally, we need one check: If the arrow has no speed
	// anymore but is still in flight, we'll remove the hit check.
	if (oldx == newx && oldy == newy)
	{
		// But we give the arrow 10 frames to speed up again.
		effect.var2++;
		if (effect.var2 >= 10)
			return Hit();
	}
	else
		effect.var2 = 0;

	// Rotate arrow according to speed.
	var anglediff = Normalize(Angle(oldx, oldy, newx, newy) - GetR(), -180);
	SetRDir(anglediff / 2);
	effect.x = newx;
	effect.y = newy;
	return 1;
}

protected func UpdatePicture()
{
	var arrow_count = GetStackCount();
	if (arrow_count >= 9) 
		SetGraphics(nil);
	else 
		SetGraphics(Format("%d", arrow_count));
	_inherited(...);
}

protected func Entrance()
{
	// Reset the sticky-vertex.
	SetVertex(2, VTX_Y, 0, 1);
}

protected func RejectEntrance()
{
	// Can't be collected while flying.
	if (GetEffect("InFlight",this)) 
		return true;
	return _inherited(...);
}

public func IsArrow() { return true; }
public func MaxStackCount() { return 15; }
public func ArrowStrength() { return 10; }
public func TumbleStrength() { return 100; }
public func IsArmoryProduct() { return true; }


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
local Components = {Wood = 3};
local BlastIncinerate = 5;
local ContactIncinerate = 1;
