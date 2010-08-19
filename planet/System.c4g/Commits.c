/*--
		Commits.c
		Authors: flgr, Joern, Tyron, Newton, Ringwaul, Sven2, Maikel
		
		Useful user committed functions, this should really be organized into more sensible categories.
--*/


// Returns the offset to x.
global func AbsX(int x) 
{
	return x - GetX(); 
}

// Returns the offset to y.
global func AbsY(int y) 
{
	return y - GetY(); 
}

// Supports negative values, and can deliver random values between two bounds.
global func RandomX(int start, int end) 
{
	var swap;
	// Values swapped: reswap them.
	if (start > end) 
	{ 
		swap = start;
		start = end;
		end = swap; 
	}
	// Return random factor.
	return Random(end - start + 1) + start;
}

// Tangens.
global func Tan(int angle, int radius, int prec)
{
	return radius * Sin(angle, radius * 100, prec) / Cos(angle, radius * 100, prec);
}

global func Normalize(int angle, int start, int precision)
{
	if (!precision) 
		precision = 1;
	var end = precision * 360 + start;
	
	while (angle < start)
		angle += precision * 360;
	while (angle >= end) 
		angle -= precision * 360;

	return angle;
}

// Does not set the speed of an object. But you can set two components of the velocity vector with this function.
global func SetSpeed(int x_dir, int y_dir, int prec)
{
	SetXDir(x_dir, prec);
	SetYDir(y_dir, prec);
	return;
}

// Sets an objects's speed and its direction, doesn't it?
global func SetVelocity(int angle, int speed)
{
	var x_dir = Sin(angle, speed);
	var y_dir = -Cos(angle, speed);

	SetXDir(x_dir);
	SetYDir(y_dir);
	return;
}

// Sets the completion of this to new_con. 
global func SetCon(int new_con)
{
	return DoCon(new_con - GetCon());
}

// Adds value to the account of iPlayer.
global func DoWealth(int plr, int value)
{
	return SetWealth(plr, value + GetWealth(plr));
}

// Sets both the X and Y-coordinate of one vertex.
global func SetVertexXY(int index, int x, int y)
{
	// Set vertices.
	SetVertex(index, VTX_X, x);
	SetVertex(index, VTX_Y, y);
	return;
}

// Returns the number of stuck vertices. (of this) 
global func VerticesStuck()
{
	var vertices = 0;
	// Loop through vertices.
	for (var i = -1; i < GetVertexNum(); i++)
		// Solid?
		if (GBackSolid(GetVertex(i, VTX_X), GetVertex(i, VTX_Y)))
			// Count vertices.
			vertices++;
	return vertices;
}

// Creates amount objects of type id inside the indicated rectangle(optional) in the indicated material. 
// Returns the number of iterations needed, or -1 when the placement failed.
global func PlaceObjects(id id, int amount, string mat_str, int x, int y, int wdt, int hgt, bool onsf, bool nostuck)
{
	var i, j;
	var rndx, rndy, obj;
	var mtype, mat;
	var func, objhgt = id->GetDefCoreVal("Height", "DefCore");
	
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

global func CheckVisibility(int plr)
{
	var visible = this["Visibility"];
	if (GetType(visible) == C4V_Array) 
		visible = visible[0];

	// Not visible at all.
	if (visible == VIS_None) 
		return false;
	// Visible for all.
	if (visible == VIS_All) 
		return true;

	// Object is owned by the indicated player.
	if (GetOwner() == plr)
	{ 
		if (visible & VIS_Owner) 
			return true; 
	}
	// Object belongs to a player, hostile to plr.
	else if (Hostile(GetOwner(), plr))
	{
		if (visible & VIS_Enemies)
			return true;
	}
	// Object belongs to a player, friendly to plr.
	else
	{ 
		if (visible & VIS_Allies)
			return true;
	}

	if (visible & VIS_Select)
		if (this["Visibility"][1 + plr / 32] & 1 << plr)
		return true;

	return false;
}



global func MaterialDepthCheck(int x, int y, string mat, int depth)
{
	var travelled;
	var xval = x;
	var yval = y;

	//If depth is equal to zero, the function will always measure the depth of the material.
	//If depth is not equal to zero, the function will return true if the material is as deep or deeper than depth (in pixels).
	if (depth == nil)
		depth = LandscapeHeight();

	while (travelled != depth) 
	{
		if (GetMaterial(xval, yval) == Material(mat)) 
		{
			travelled++;
			yval++; 
		}
		if (GetMaterial(xval, yval) != Material(mat))
			return travelled; // Returns depth of material.
	}
	if (travelled == depth)
		return true;
	return false;
}



global func LaunchProjectile(int angle, int dist, int speed, int x, int y, bool rel_x)
{
	// dist: Distance object travels on angle. Offset from calling object.
	// x: X offset from container's center
	// y: Y offset from container's center
	// rel_x: if true, makes the X offset relative to container direction. (x=+30 will become x=-30 when Clonk turns left. This way offset always stays in front of a Clonk.)

	var x_offset = Sin(angle, dist);
	var y_offset = -Cos(angle, dist);

	if (Contained() != nil && rel_x == true) 
		if (Contained()->GetDir() == 0) 
			x = -x;

	if (Contained() != nil)
	{
		Exit(x_offset + x, y_offset + y, angle);
		SetVelocity(angle, speed);
		return true;
	}

	if (Contained() == nil) 
	{
		SetPosition(GetX() + x_offset + x, GetY() + y_offset + y);
		SetR(angle);
		SetVelocity(angle, speed);
		return true;
	}
	return false;
}

global func ComDirTransform(int comdir, int tocomdir)
{
	if (comdir == tocomdir) 
		return comdir;
	if (comdir == COMD_Stop) 
		return tocomdir;
	if (comdir == (tocomdir + 3) % 8 + 1) 
		return COMD_Stop;
	if (Inside(comdir, tocomdir + 1, tocomdir + 3)) 
		return comdir - 1;
	if (Inside(comdir, tocomdir - 1, tocomdir - 3)) 
		return comdir + 1;
	if (Inside(comdir, tocomdir - 7, tocomdir - 5)) 
		return (comdir + 6) % 8 + 1;
	return comdir % 8 + 1;
}

global func ComDirLike(int comdir1, int comdir2)
{
	if (comdir1 == comdir2) 
		return true;
	if (comdir1 == COMD_Stop || comdir2 == COMD_Stop)
		return false;
	if (comdir1 % 8 + 1 == comdir2)
		return true;
	if (comdir1 == comdir2 % 8 + 1) 
		return true;
	return false;
}

global func SetObjAlpha(int by_alpha)
{
	var clr_mod = GetClrModulation();
	
	if (!clr_mod) 
		clr_mod = by_alpha << 24; 
	else 
		clr_mod = clr_mod & 16777215 | by_alpha << 24;
	return SetClrModulation(clr_mod);
}

global func FindPosInMat(int &iToX, int &iToY, string sMat, int iXStart, int iYStart, int iWidth, int iHeight, int iSize)
{
	var iX, iY;
	for(var i = 0; i < 500; i++)
	{
		iX = iXStart+Random(iWidth);
		iY = iYStart+Random(iHeight);
		if(GetMaterial(AbsX(iX),AbsY(iY))==Material(sMat) &&
		   GetMaterial(AbsX(iX+iSize),AbsY(iY+iSize))==Material(sMat) &&
		   GetMaterial(AbsX(iX+iSize),AbsY(iY-iSize))==Material(sMat) &&
		   GetMaterial(AbsX(iX-iSize),AbsY(iY-iSize))==Material(sMat) &&
		   GetMaterial(AbsX(iX-iSize),AbsY(iY+iSize))==Material(sMat)
		) {
			iToX = iX; iToY = iY;
			return true; // Location found.
		}
	}
	return false; // No location found.
}