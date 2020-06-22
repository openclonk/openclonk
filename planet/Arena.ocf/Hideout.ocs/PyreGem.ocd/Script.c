/*--- Pyre Gem ---*/

local has_graphics_e;
local thrower;

public func Initialize()
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

public func Departure(object from)
{
	SetRDir(RandomX(-15, 15));
	thrower = from;
}

func Hit()
{
	var thrower_owner = NO_OWNER;
	if (thrower)
	{
		thrower_owner = thrower->GetOwner();
	}
	AddEffect("GemPyre", nil, 100, 1, nil, nil, [GetX(), GetY()], has_graphics_e, this->GetOwner(), thrower_owner);
	RemoveObject();
}


global func FxGemPyreStart(object target, effect, int temporary, array coordinates, bool e, int owner, int thrower_owner)
{
	if (temporary) 
		return 1;

	effect.x = coordinates[0];
	effect.y = coordinates[1];
	effect.thrower = thrower_owner;
	effect.owner = owner;
	effect.objects = [];
	
	effect.particles =
	{
		Prototype = Particles_Air(),
		Size = PV_Linear(2, 0),
		R = PV_Random(120, 140),
		G = PV_Random(20, 30),
		B = PV_Random(90, 110),
		BlitMode = GFX_BLIT_Additive
	};
	
	if (e)
	{
		effect.particles.R = PV_Random(190, 200);
		effect.particles.G = 0;
		effect.particles.B = PV_Random(20, 40);
	}
}


global func FxGemPyreTimer(object target, effect, int time)
{
	var x = effect.x;
	var y = effect.y;
	
	var radius_max = ((time / 2) + 1) * 6;
	var radius_min = ((time / 2) + 1) * 4;
	
	if (time > 32) return -1;
	
	for (var i = 0; i < (20 + time); i++)
	{
		var r = Random(360);
		var d = Random(radius_max - radius_min) + radius_min + RandomX(-2, 2);
		var xoff = +Sin(r, d);
		var yoff = -Cos(r, d);

		if (!PathFree(x, y, x + xoff, y + yoff)) continue;

		CreateParticle("Air", x + xoff, y + yoff, PV_Random(xoff - 3, xoff + 3), PV_Random(yoff - 3, yoff + 3), PV_Random(5, 10), effect.particles, 2);
	}
	
	var potential = 30 - time;
	for (var obj in FindObjects(Find_NoContainer(), Find_OCF(OCF_Alive), Find_Distance(radius_max, x, y), Find_Not(Find_Distance(radius_min, x, y)), Find_ID(Clonk)))
	{
		if (IsValueInArray(effect.objects, obj))
		{
			continue;
		}

		if (PathFree(x, y, obj->GetX(), obj->GetY()))
		{
			obj->DoEnergy((-BoundBy(potential, 1, 26) * 3) / 5, 0, 0, effect.thrower);
			obj->CreateParticle("MagicFire", 0, 0, PV_Random(-15, 15), PV_Random(-15, 15), PV_Random(5, 10), effect.particles, 20);
			obj->Fling(RandomX(-2, 2), -2 -(BoundBy(potential, 10, 30) / 10));
			effect.objects[GetLength(effect.objects)] = obj;
		}	
	}
	return 1;
}

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
