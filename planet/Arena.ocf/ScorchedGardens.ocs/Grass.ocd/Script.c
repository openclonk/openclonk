/*-- Grass --*/

protected func Initialize()
{
	DoCon(Random(50));
	if (Random(2))
		SetGraphics("1");
}


public func CanBeHitByShockwaves() { return true; }

protected func Damage()
{
	if (GetDamage() > 80 && !Random(5))
	{
		Destroy();
	}
}

private func Destroy()
{
	var particles = 
	{
		Prototype = Particles_Straw(),
		R = 200,
		G = 50,
		B = 50
	};
	CreateParticle("Grass", 0, 0, PV_Random(-20, 20), PV_Random(-20, 10), PV_Random(30, 100), particles, 30);
	RemoveObject();
}

global func PlaceGrass(int amount, int start, int end)
{
	if (!start)
		start = 0;
	if (!end)
		end = LandscapeWidth();
		
	var x = start, y; 
	while (x < end)
	{
		y = 0;
		while (y < LandscapeHeight())
		{
			if (GetMaterial(AbsX(x), AbsY(y)) == Material("Sky"))
				if (GetMaterial(AbsX(x), AbsY(y + 3)) == Material("Earth"))
					if (Random(100) < amount)
						CreateObjectAbove(Grass, AbsX(x), AbsY(y + 4), NO_OWNER);
			y += 3;
		}
		x += 9;
	}
}

local Name = "Grass";
