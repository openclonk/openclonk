/*--- Flint ---*/

local has_graphics_e;

protected func Initialize()
{
	if (Random(2))
	{
		SetGraphics("E");
		has_graphics_e = true;	
	}
	else
	{
		SetGraphics("");
		has_graphics_e = false;
	}
	
	if (this->GetX() < 920)
	{
		SetGraphics("E");
		has_graphics_e = true;
	}
	else if (this->GetX() > 1280)
	{
		SetGraphics("");
		has_graphics_e = false;
	}
	 
	SetR(Random(360));
}

protected func Departure()
{
	SetRDir(RandomX(-15, 15));
}

func Hit()
{
	AddEffect("GemSlowField", nil, 100, 1, nil, nil, GetX(), GetY(), has_graphics_e);
	RemoveObject();
}

global func FxGemSlowFieldStart(object target, proplist effect, int temporary, int x, int y, bool e)
{
	if (temporary) 
		return 1;

	effect.x = x;
	effect.y = y;

	effect.particles =
	{
		Prototype = Particles_Spark(),
		Size = PV_Linear(3, 0),
		ForceY = PV_Gravity(-40),
		R = PV_Random(120, 140),
		G = PV_Random(20, 30),
		B = PV_Random(90, 110),
		OnCollision = PC_Die(),
		CollisionVertex = 1000
	};

	if (e)
	{
		effect.particles.R = PV_Random(190, 210);
		effect.particles.G = 0;
		effect.particles.B = PV_Random(20, 40);
	}
}

global func FxGemSlowFieldTimer(object target, proplist effect, int time)
{
	var x = effect.x;
	var y = effect.y;

	if (time > 150) return FX_Execute_Kill;
	
	var radius = 62;
	
	for (var i = 0; i < 40; i++)
	{
		var r = Random(360);
		var d = Min(Random(20) + Random(130), radius);
		var xoff = +Sin(r, d);
		var yoff = -Cos(r, d); 
		
		if (!PathFree(x, y, x + xoff, y + yoff)) continue;
		CreateParticle("MagicFire", x + xoff, y + yoff, PV_Random(-2, 2), PV_Random(0, 4), PV_Random(10, 40), effect.particles, 2);
	}

	for (var obj in FindObjects(Find_Distance(radius, x, y)))
	{
		if (!PathFree(x, y, obj->GetX(), obj->GetY())) continue;
		
		var speed = Distance(0, 0, obj->GetXDir(), obj->GetYDir());
		if (speed < 16 ) continue;

		var angle = Angle(0, 0, obj->GetXDir(), obj->GetYDir());
		obj->SetXDir(obj->GetXDir(100) + Sin(-angle, speed * 3), 100);
		obj->SetYDir(obj->GetYDir(100) + Cos(-angle, speed * 3) - 10, 100);
		obj->SetYDir(obj->GetYDir() - 5);
	}

	return 1;
}

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
