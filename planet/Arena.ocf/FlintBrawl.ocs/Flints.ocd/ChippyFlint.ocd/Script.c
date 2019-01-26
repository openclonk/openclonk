/*--- Flint ---*/

public func IsGrenadeLauncherAmmo() { return true; }

protected func Construction()
{
	return true;
}

func Hit()
{
	ScheduleCall(this, this.Fuse, 1, 1);
	return true;
}

func Fuse()
{
	Sound("Fire::Spark*");
	CreateParticle("Fire", 0, 0, PV_Random(-5, 5), PV_Random(-15, 5), PV_Random(10, 40), Particles_Glimmer(), 5);
	return true;
}

func Hit2()
{
	for (var i = 0; i < 10; ++i)
	{
		var egg = CreateObject(Chippie_Egg, 0, 0, GetController());
		egg.age = -5000;
		egg->SetVelocity(Random(360), RandomX(20, 40));
	}
	
	var particles = 
	{
		Prototype = Particles_Material(RGB(100, 255, 50)),
		DampingX = 800, DampingY = 800,
		ForceY = -GetGravity() / 10,
	};
	CreateParticle("SmokeDirty", PV_Random(-5, 5), PV_Random(-5, 5),
					PV_Random(-10, 10), PV_Random(-10, 10),
					PV_Random(10, 20), particles, 60);
	RemoveObject();
}

public func HasExplosionOnImpact() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Plane = 530; // cause it's explosive, players should see it in a pile of stuff