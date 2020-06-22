/**
	Creation.c
	Creation of objects, particles or PSX.
	
	@author Ringwaul, Tyron		
*/

// Creates amount objects of type id inside the indicated rectangle(optional) in the indicated material.
// Returns the number of iterations needed, or -1 when the placement failed.
// documented in /docs/sdk/script/fn
global func PlaceObjects(id id, int amount, string mat_str, int x, int y, int wdt, int hgt, bool onsf, bool nostuck)
{
	var i, j;
	var rndx, rndy, obj;
	var mat;
	var objhgt = id->GetDefCoreVal("Height", "DefCore");
	
	mat = Material(mat_str);
	// Some failsavety.
	if (mat == -1)
		if (mat_str != "GBackSolid" && mat_str != "GBackSemiSolid" && mat_str != "GBackLiquid" && mat_str != "GBackSky")
			return -1;
	
	// Optional parameters wdt and hgt.
	if (!wdt)
		wdt = LandscapeWidth() - x - GetX();
	if (!hgt)
		hgt = LandscapeHeight() - y - GetY();

	// Cycle-saving method.
	if (mat != -1)
		while (i < amount)
		{
			// If there's isn't any or not enough of the given material, break before it gets an endless loop.
			if (j++ > 20000)
				return -1;
			// Destinated rectangle.
			rndx = x + Random(wdt);
			rndy = y + Random(hgt);
			// Positioning.
			if (GetMaterial(rndx, rndy) == mat)
			{
				// On-surface option.
				if (onsf)
					while (GBackSemiSolid(rndx, rndy) && rndy >= y)
						rndy--;
				if (rndy < y)
					continue;
				// Create and verify stuckness.
				obj = CreateObjectAbove(id, rndx, rndy + objhgt / 2, NO_OWNER);
				obj->SetR(Random(360));
				if (obj->Stuck() || nostuck)
					i++;
				else
					obj->RemoveObject();
			}
		}

	if (mat == -1)
		while (i < amount)
		{
			// If there's isn't any or not enough of the given material, break before it gets an endless loop.
			if (j++ > 20000)
				return -1;
			// Destinated rectangle.
			rndx = x + Random(wdt);
			rndy = y + Random(hgt);
			// Positioning.
			if (eval(Format("%s(%d,%d)", mat_str, rndx, rndy)))
			{
				// On-surface Option.
				if (onsf)
					while (GBackSemiSolid(rndx, rndy) && rndy >= y)
						rndy--;
				if (rndy < y)
					continue;
				// Create and verify stuckness.
				obj = CreateObjectAbove(id, rndx, rndy + objhgt / 2, NO_OWNER);
				obj->SetR(Random(360));
				if (obj->Stuck() || nostuck)
					i++;
				else
					obj->RemoveObject();
			}
		}

	return j;
}

// documented in /docs/sdk/script/fn
global func CastObjects(id def, int am, int lev, int x, int y, int angs, int angw)
{
	var objects = [];
	var objects_index = 0;
	if (!angw)
		angw = 360;
	for (var i = 0; i < am; i++)
	{
		var obj = CreateObjectAbove(def, x, y);
		// Some objects might directly remove themselves on creation.
		if (!obj) continue;
		var ang = angs - 90 + RandomX(-angw / 2, angw / 2);
		var xdir = Cos(ang, lev) + RandomX(-3, 3);
		obj->SetR(Random(360));
		obj->SetXDir(xdir);
		obj->SetYDir(Sin(ang, lev) + RandomX(-3, 3));
		if (xdir != 0)
			obj->SetRDir((10 + Random(21)) * (xdir / Abs(xdir)));
		else
			obj->SetRDir(-10 + Random(21));
		objects[objects_index++] = obj;
	}
	return objects;
}

// documented in /docs/sdk/script/fn
global func CastPXS(string mat, int am, int lev, int x, int y, int angs, int angw)
{
	if (!angw)
		angw = 360;
	for (var i = 0; i < am; i++)
	{
		var ang = angs - 90 + RandomX(-angw / 2, angw / 2);
		InsertMaterial(Material(mat), x, y, Cos(ang, lev) + RandomX(-3, 3), Sin(ang, lev) + RandomX(-3, 3));
	}
	return;
}

// documented in /docs/sdk/script/fn
global func DrawParticleLine(string particle, int x0, int y0, int x1, int y1, int prtdist, xdir, ydir, lifetime, proplist properties)
{
	// Right parameters?
	if (!properties)
		return 0;
	// Calculate required number of particles.
	var prtnum = Max(Distance(x0, y0, x1, y1) / prtdist, 2);
	var i = prtnum;
	// Create particles.
	while (i >= 0)
	{
		var i1, i2;
		i2 = i * 256 / prtnum;
		i1 = 256 - i2;

		CreateParticle(particle, x0 + (x1 - x0) * i / prtnum, y0 + (y1 - y0) * i-- / prtnum, xdir, ydir, lifetime, properties, 1);
	}
	// Succes, return number of created particles.
	return prtnum;
}



/** Place a nice shaped forest. If no area is given, the whole landscape is used (which is not recommended!).
	@param plants An array containing all plants that should be in the forest. plants[0] is the main plant, the others will be randomly scattered throughout the forest.
	@param x The starting X-coordinate of the forest.
	@param y The lowest line at which to start placing plants. Level ground is determined automatically, goind upwards.
	@param width The width of the forest
	@param foreground Will roughly make every third instance of plants[0] foreground
*/
global func PlaceForest(array plants, int x, int y, int width, bool foreground)
{
	// Parameter check
	if (GetLength(plants) == 0) return;
	if (!x) x = 0;
	if (!y) y = LandscapeHeight();
	if (!width) width = LandscapeWidth();
	if (this) { x = AbsX(x); y = AbsY(y); }

	// Roughly 20% of the size (10% per side) are taken for 'forest ending zones'. Plants will be smaller there.
	var end_zone = width * 10 / 100;
	// The width of the standard plants will roughly be the measure for our plant size
	var plant_size = plants[0]->GetDefWidth()/2;

	var growth, y_pos, plant, x_variance, variance = 0, count, j, spot;
	for (var i = plant_size; i < width; i += plant_size)
	{
		growth = 95;
		y_pos = y;
		x_variance = RandomX(-plant_size/2, plant_size/2);
		// End zone check
		if (i < end_zone)
			growth = BoundBy(90 / ((end_zone * 100 / plant_size)/100) * (i/plant_size), 10, 90);
		else if (i > width - end_zone)
			growth = BoundBy(90 / ((end_zone * 100 / plant_size)/100) * ((width-i)/plant_size), 10, 90);
		else if (!Random(10) && GetLength(plants) > 1)
		{
			variance = Random(GetLength(plants)-1)+1;
			// Scatter some other plants
			count = RandomX(2, 4);
			for (j = 0; j < count; j++)
			{
				spot = (plant_size*2 / count) * j + RandomX(-5, 5) - plant_size;
				y_pos = y;
				if (!GBackSolid(x + i + spot, y_pos)) continue;
				while (!GBackSky(x + i + spot, y_pos) && y_pos > 0) y_pos--;
				if (y_pos == 0) continue;
				plant = CreateObjectAbove(plants[variance], x + i + spot, y_pos + 5, NO_OWNER);
			}
			continue;
		}
		// No ground at y_pos?
		if (!GBackSolid(x + i + x_variance, y_pos)) continue;
		// Get level ground
		while (!GBackSky(x + i + x_variance, y_pos) && y_pos > 0) y_pos--;
		if (y_pos == 0) continue;

		plant = CreateObjectAbove(plants[0], x + i + x_variance, y_pos + 5, NO_OWNER);
		plant->SetCon(growth);
		if (foreground && !Random(3)) plant.Plane = 510;
		// Every ~7th plant: double plant!
		if (x_variance != 0 && !Random(7))
		{
			y_pos = y;
			if (!GBackSolid(x + i - x_variance, y_pos)) continue;
			while (!GBackSky(x + i - x_variance, y_pos) && y_pos > 0) y_pos--;
			if (y_pos == 0) continue;
			plant = CreateObjectAbove(plants[0], x + i - x_variance, y_pos + 5, NO_OWNER);
			plant->SetCon(growth);
			if (foreground && !Random(3)) plant.Plane = 510;
		}
	}
}
