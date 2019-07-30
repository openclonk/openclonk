/*--- Flint ---*/

local counter = 0;

public func IsGrenadeLauncherAmmo() { return true; }

protected func Construction()
{
	return true;
}

func HitEffect()
{
	var smoke =
	{
		Alpha = PV_Linear(255, 0),
		Size = 15,
		DampingX = 900, DampingY = 900,
		R = 100, G = 100, B = 100,
		Phase = PV_Random(0, 15)
	};
	CreateParticle("Smoke", 0, 0, PV_Random(-5, 5), PV_Random(-5, 5), 20, smoke, 25);
}

func Hit(int xdir, int ydir)
{
	if (counter < 2 + Random(10))
	{
		Bounce(xdir, ydir);
		Sound("Hits::Materials::Glass::GlassHit[34]", {pitch = 100 + 10 * counter});
		counter++;
		HitEffect();
		return;
	}
	
	// Cast lots of shrapnel.
	var shrapnel_count = 40;
	var offset = GetSurfaceVector(0, 0);
	for (var cnt = 0; cnt < shrapnel_count; cnt++)
	{
		var shrapnel = CreateObject(Shrapnel, offset[0], offset[1]);
		shrapnel->SetVelocity(Random(359), RandomX(100, 140));
		shrapnel->SetRDir(-30 + Random(61));
		shrapnel->Launch(GetController());
		CreateObjectAbove(BulletTrail)->Set(shrapnel, 2, 30);
	}
	Sound("Hits::Materials::Glass::GlassShatter", {volume = 20});
	Fireworks(RGB(150, 115, 200));
	RemoveObject();
}

func Bounce(int xdir, int ydir)
{
	var angle = Angle(0, 0, xdir, ydir);
	
	var surface = GetSurfaceVector(0, 0);
	var surface_angle = Angle(0, 0, surface[0], surface[1]);
	var angle_diff = GetTurnDirection(angle - 180, surface_angle);
	var new_angle = surface_angle + angle_diff + RandomX(-10, 10);
	
	var speed = Distance(0, 0, xdir, ydir);
	speed = 4 * speed / 4;
	SetXDir(Sin(new_angle, speed), 100);
	SetYDir(-Cos(new_angle, speed), 100);
}


public func HasExplosionOnImpact() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Plane = 530; // cause it's explosive, players should see it in a pile of stuff
