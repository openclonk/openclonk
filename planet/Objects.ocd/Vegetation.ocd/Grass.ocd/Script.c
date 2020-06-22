/**
	Grass 
	Is placed just behind the landscape.
	
	@authors
*/

protected func Initialize()
{
	DoCon(Random(50));
	if (Random(2))
		SetGraphics("1");
	return;
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
	CreateParticle("Grass", 0, 0, PV_Random(-20, 20), PV_Random(-20, 10), PV_Random(30, 100), Particles_Straw(), 30);
	RemoveObject();
}

// Place some grass at the surface in the given area. The amount determines the density.
public func Place(int amount, proplist area)
{
	// Definition call.
	if (this != Grass)
		return;
	var rectangle = Shape->LandscapeRectangle();
	if (area) 
		rectangle = area->GetBoundingRectangle(); 
		
	var grass_list = [];
	for (var x = rectangle.x; x <= rectangle.x + rectangle.wdt; x += 9)
	{
		for (var y = rectangle.y; y <= rectangle.y + rectangle.hgt; y += 3)
		{
			if (GetMaterial(AbsX(x), AbsY(y)) == Material("Sky") && GetMaterial(AbsX(x), AbsY(y + 3)) == Material("Earth"))
			{
				if (Random(100) < amount)
				{
					var grass =	CreateObjectAbove(Grass, AbsX(x), AbsY(y + 1));
					PushBack(grass_list, grass);
				}
			}
		}
	}
	return grass_list;
}

// OUTDATED: use Grass->Place() instead, this function will be removed at some point.
global func PlaceGrass(int amount, int start, int end, int height, int bottom)
{
	if (!start)
		start = 0;
	if (!end)
		end = LandscapeWidth();
	if (!height)
		height = 0;
	if (!bottom)
		bottom = LandscapeHeight();
		
	var x = start, y; 
	while (x < end)
	{
		y = height;
		while (y < bottom)
		{
			if (GetMaterial(AbsX(x), AbsY(y)) == Material("Sky"))
				if (GetMaterial(AbsX(x), AbsY(y + 3)) == Material("Earth"))
					if (Random(100) < amount)
						CreateObjectAbove(Grass, AbsX(x), AbsY(y + 1), NO_OWNER);
			y += 3;
		}
		x += 9;
	}
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Con");
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local Plane = -1;
local Placement = 0;
local BlastIncinerate = 1;
