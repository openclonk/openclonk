/*--- Flint ---*/

protected func Construction()
{
	var graphic = Random(3);
	if(graphic)
		SetGraphics(Format("%d",graphic));
}

func Hit()
{
	Sound("Fuse");
	CreateParticle("Fire", 0, 0, PV_Random(-5, 5), PV_Random(-15, 5), PV_Random(10, 40), Particles_Glimmer(), 5);
}

func Hit2()
{
	Explode(18);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local Plane = 530; // cause it's explosive, players should see it in a pile of stuff