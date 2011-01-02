/*-- Grass --*/

protected func Initialize()
{
	DoCon(Random(50));
	if (Random(2))
		SetGraphics("1");
}

public func Incineration()
{
	Destroy();
	return;
}

public func CanBeHitByShockwaves() { return true; }

public func OnShockwaveHit()
{	
	Destroy();
	return true;
}

private func Destroy()
{
	CastParticles("Grass", 10, 35, 0, 0, 30, 50, RGB(255,255,255), RGB(255,255,255));
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
						CreateObject(Grass, AbsX(x), AbsY(y + 4), NO_OWNER);
			y += 3;
		}
		x += 9;
	}
}

local Name = "Grass";
