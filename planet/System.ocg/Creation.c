/*--
		Creation.c
		Authors: Ringwaul, Tyron

		Creation of objects, particles or PSX.
--*/

// Creates amount objects of type id inside the indicated rectangle(optional) in the indicated material.
// Returns the number of iterations needed, or -1 when the placement failed.
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
				obj = CreateObject(id, rndx, rndy + objhgt / 2, NO_OWNER);
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
				obj = CreateObject(id, rndx, rndy + objhgt / 2, NO_OWNER);
				obj->SetR(Random(360));
				if (obj->Stuck() || nostuck)
					i++;
				else
					obj->RemoveObject();
			}
		}

	return j;
}

global func CastObjects(id def, int am, int lev, int x, int y, int angs, int angw)
{
	if (!angw)
		angw = 360;
	for (var i = 0; i < am; i++)
	{
		var obj = CreateObject(def , x, y, NO_OWNER);
		var ang = angs - 90 + RandomX(-angw / 2, angw / 2);
		var xdir = Cos(ang, lev) + RandomX(-3, 3);
		obj->SetR(Random(360));
		obj->SetXDir(xdir);
		obj->SetYDir(Sin(ang, lev) + RandomX(-3, 3));
		if(xdir != 0) obj->SetRDir((10 + Random(21)) * (xdir / Abs(xdir)));
		else
			obj->SetRDir(-10 + Random(21));
	}
	return;
}

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

global func DrawParticleLine (string particle, int x0, int y0, int x1, int y1, int prtdist, int a, int b0, int b1, int ydir)
{
	// Right parameters?
	if (!prtdist)
		return 0;
	// Calculate required number of particles.
	var prtnum = Max(Distance(x0, y0, x1, y1) / prtdist, 2);
	var i = prtnum;
	// Create particles.
	while (i >= 0)
	{
		var i1, i2, b;
		i2 = i * 256 / prtnum;
		i1 = 256 - i2;

		b = ((b0 & 16711935) * i1 + (b1 & 16711935) * i2) >> 8 & 16711935
			| ((b0 >> 8 & 16711935) * i1 + (b1 >> 8 & 16711935) * i2) & -16711936;
		if (!b && (b0 | b1))
			b++;
		CreateParticle(particle, x0 + (x1 - x0) * i / prtnum, y0 + (y1 - y0) * i-- / prtnum, 0, ydir, a, b);
	}
	// Succes, return number of created particles.
	return prtnum;
}