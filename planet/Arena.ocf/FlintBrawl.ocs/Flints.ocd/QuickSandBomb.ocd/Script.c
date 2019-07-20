/*--- Flint ---*/

local fused_fx;

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

local FusedEffect = new Effect
{
	Timer = func()
	{
		this.Target->Explode(30);
	}
};

func Hit(int xdir, int ydir)
{
	if (!fused_fx)
		fused_fx = CreateEffect(FusedEffect, 1, 30);
	ShakeFree(GetX(), GetY(), 10);
	SetSpeed(xdir, ydir, 100);
	Sound("Environment::Disasters::EarthquakeEnd");
}

public func HasExplosionOnImpact() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Plane = 530; // cause it's explosive, players should see it in a pile of stuff