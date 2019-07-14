/*--- Flint ---*/

protected func Construction()
{
	var graphic = Random(3);
	if (graphic)
		SetGraphics(Format("%d",graphic));
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
	return Explode(18);
}

public func HasExplosionOnImpact() { return true; }

public func IsExplosive() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Plane = 530; // cause it's explosive, players should see it in a pile of stuff