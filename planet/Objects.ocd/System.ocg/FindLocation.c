/**
	FindLocation.c
	This script contains the function FindLocation which uses a parameter-system similar to FindObject.
	This function should mainly be used for placement of objects at the start of a scenario.
	FindLocation is not guaranteed to always return a spot if a fitting spot exists, it's just best effort.
	
	Examples:
	finds a tunnel spot:
	FindLocation([Loc_Tunnel()]);
	finds a floor spot but not in front of tunnel:
	FindLocation([Loc_Not(Find_Tunnel()), Loc_Wall(CNAT_Bottom)]);
	
	@author Zapper
*/

static const LOC_INVALID = 0;
static const LOC_SOLID = 1;
static const LOC_INAREA = 2;
static const LOC_MATERIAL = 3;
static const LOC_MATERIALVAL = 4;
static const LOC_FUNC = 5;
static const LOC_WALL = 6;
static const LOC_SPACE = 7;
static const LOC_NOT = 8;
static const LOC_SKY = 9;
static const LOC_LIQUID = 10;
static const LOC_OR = 11;
static const LOC_AND = 12;
static const LOC_MAXTRIES = 13;

/*
	Returns a spot where a custom function returned "true".
	For example: Loc_Condition(LargeCaveMushroom.GoodSpot)
	with "func GoodSpot(int x, int y)"
*/
global func Loc_Func(function)
{
	return [LOC_FUNC, function];
}

global func Loc_Not(cond)
{
	return [LOC_NOT, cond];
}

global func Loc_Or(...)
{
	var conds = [LOC_OR];
	for (var i = 0; i < 10; ++i)
	{
		if (Par(i) != nil)
			PushBack(conds, Par(i));
	}
	return conds;
}

global func Loc_And(...)
{
	var conds = [LOC_AND];
	for (var i = 0; i < 10; ++i)
	{
		if (Par(i) != nil)
			PushBack(conds, Par(i));
	}
	return conds;
}
/*
	only returns results in a rectangle defined by either the coordinates and size x, y, w, h or by passing a rectangle-proplist
	as "x" to allow Loc_InRect(Rectangle(x, y, w, h))
*/	
global func Loc_InRect(x, int y, int w, int h)
{
	if (x == nil) return [LOC_INVALID];
	if (GetType(x) == C4V_PropList) return [LOC_INAREA, x];
	return [LOC_INAREA, Rectangle(x, y, w, h)];
}

/*
only returns results in area defined by a proplist
*/
global func Loc_InArea(a)
{
	if (a == nil) return [LOC_INVALID];
	return[LOC_INAREA, a];
}

/*
	Returns a point in a given material or material-texture-combination.
	The texture-parameter is optional.
*/
global func Loc_Material(string material, string texture)
{
	if (texture)
		return [LOC_MATERIAL, Material(material), texture];
	else
		return [LOC_MATERIAL, Material(material)];
}

global func Loc_MaterialVal(string entry, string section, int entry_nr, compare)
{
	return [LOC_MATERIALVAL, entry, section, entry_nr, compare];
}

global func Loc_Tunnel()
{
	return Loc_Material("Tunnel");
}

/*
	Sets the maximum number of trying to find a position with a random point.
	Should probably be larger than 1000 unless you are very sure that such a spot is common.
*/
global func Loc_MaxTries(int number_of_tries)
{
	return [LOC_MAXTRIES, number_of_tries];
}

global func Loc_Sky() {	return [LOC_SKY]; }
global func Loc_Solid() { return [LOC_SOLID]; }
global func Loc_Liquid() { return [LOC_LIQUID]; }

/*
	returns a point on a wall. /direction/ can be either CNAT_Bottom, CNAT_Top, CNAT_Left, CNAT_Right.
	Note that this implies that you are looking for a non-solid spot.
*/
global func Loc_Wall(int direction, wall_condition1, ...)
{
	var x = 0, y = 0;
	if (direction & CNAT_Left) x = -1;
	else
	if (direction & CNAT_Right) x = 1;
	
	if (direction & CNAT_Top) y = -1;
	else
	if (direction & CNAT_Bottom) y = 1;
	
	var both_left_right = !!((direction & CNAT_Left) && (direction & CNAT_Right));
	var both_top_bottom = !!((direction & CNAT_Top) && (direction & CNAT_Bottom));
	
	var wall_conditions = [];
	for (var i = 1; i < 10; ++i)
	{
		var condition = Par(i);
		if (!condition) 
			continue;
		PushBack(wall_conditions, condition);
	}
	
	return [LOC_WALL, x, y, both_left_right, both_top_bottom, wall_conditions];
}

/*
	returns a spot with enough space either vertically or horizontally.
	For example Loc_Space(20, CNAT_Top) would only return points where a Clonk could stand.
*/
global func Loc_Space(int space, int direction)
{
	return [LOC_SPACE, space, direction ?? 0x0f];
}

global func FindLocation(condition1, ...)
{
	var area = nil;
	var xdir = 0, ydir = 0, xmod = nil, ymod = nil, wall_conditions = [];
	var flags = [];
	// maximum number of tries
	var repeat = 5000;
	
	//  put parameters in array and filter out special parameters
	for (var i = 0; i < 10; ++i)
	{
		var f = Par(i);
		if (!f) continue;
		
		if (f[0] == LOC_INAREA)
		{
			area = f[1];
			PushBack(flags, f); // re-check area afterwards to account for wall attachments, etc. pushing position out of range
		}
		else if (f[0] == LOC_WALL)
		{
			xdir = f[1];
			ydir = f[2];
			xmod = f[3];
			ymod = f[4];
			wall_conditions = f[5];
		}
		else if (f[0] == LOC_MAXTRIES)
		{
			repeat = f[1];
		}
		else
		{
			PushBack(flags, f);
		}
	}
	area = area ?? Shape->LandscapeRectangle();
	
	// repeat until a spot is found or max. number of tries exceeded
	var outpos = {};
	while (repeat-- > 0)
	{
		if (!area->GetRandomPoint(outpos)) return nil; // invalid shape or nothing found
		var x = outpos.x, y = outpos.y;
		var valid = true;
		
		// find wall-spot
		// this part just moves the random point to left/right/up/down until a wall is hit
		if (xdir || ydir)
		{
			if (GBackSolid(x, y)) continue;
			var lx = xdir;
			var ly = ydir;
			if (xmod) if (Random(2)) lx *= -1;
			if (ymod) if (Random(2)) ly *= -1;

			valid = false;
			var failsafe = 0;
			do
			{
				if (GBackSolid(x + lx, y + ly)) 
				{
					valid = true;
					for (var flag in wall_conditions)
					{
						if (Global->FindLocationConditionCheckIsValid(flag, x + lx, y + ly)) continue;
						valid = false;
						break;
					}
					break;
				}
				x += lx;
				y += ly;
			} while (++failsafe < 100);
			if (!valid) continue;
		}
		
		// check every flag
		for (var flag in flags)
		{
			if (Global->FindLocationConditionCheckIsValid(flag, x, y)) continue;
			valid = false;
			break;
		}
		if (valid)
		{
			// Store back position as LOC_WALL etc. may have modified it.
			outpos.x = x;
			outpos.y = y;
			return outpos;
		}
	}
	
	// no spot found
	return nil;
}

global func FindLocationConditionCheckIsValid(flag, x, y)
{
	if (flag[0] == LOC_NOT) return !FindLocationConditionCheckIsValid(flag[1], x, y);
	
	if (flag[0] == LOC_OR)
	{
		var max = GetLength(flag);
		for (var i = 1; i < max; ++i)
			if (FindLocationConditionCheckIsValid(flag[i], x, y)) return true;
		return false;
	}
	
	if (flag[0] == LOC_AND)
	{
		var max = GetLength(flag);
		for (var i = 1; i < max; ++i)
			if (!FindLocationConditionCheckIsValid(flag[i], x, y)) return false;
		return true;
	}
	
	if (flag[0] == LOC_INAREA)
	{
		var area = flag[1];
		area = area ?? Shape->LandscapeRectangle();
		return area->IsPointContained(x, y);
	}
	
	if (flag[0] == LOC_SOLID)
		return GBackSolid(x, y);
	
	if (flag[0] == LOC_SKY)
		return GBackSky(x, y);
	
	if (flag[0] == LOC_LIQUID)
		return GBackLiquid(x, y);
	
	if (flag[0] == LOC_MATERIAL)
	{
		if ((GetMaterial(x, y) != flag[1])) return false;
		if (GetLength(flag) > 2)
			if (GetTexture(x, y) != flag[2]) return false;
		return true;
	}
	
	if (flag[0] == LOC_MATERIALVAL)
	{
		var mat = GetMaterial(x, y);
		var mat_val = GetMaterialVal(flag[1], flag[2], mat, flag[3]);
		return mat_val == flag[4];
	}
		
	// custom condition call
	if (flag[0] == LOC_FUNC)
	{
		return Global->Call(flag[1], x, y);
	}
	
	// has some space?
	if (flag[0] == LOC_SPACE)
	{
		var dist = flag[1], dirs = flag[2];
		// if only one direction is given in one dimension, the other dimension is tested from a center point halfway off in that dimension
		var cy = y + dist * ((dirs & CNAT_Bottom)/CNAT_Bottom - (dirs & CNAT_Top)/CNAT_Top) / 2;
		var cx = x + dist * ((dirs & CNAT_Right)/CNAT_Right - (dirs & CNAT_Left)/CNAT_Left) / 2;
		// check all desired directions
		if (dirs & CNAT_Top) if (!PathFree(cx, y, cx, y-dist)) return false;
		if (dirs & CNAT_Bottom) if (!PathFree(cx, y, cx, y + dist)) return false;
		if (dirs & CNAT_Left) if (!PathFree(x, cy, x-dist, cy)) return false;
		if (dirs & CNAT_Right) if (!PathFree(x, cy, x + dist, cy)) return false;
		return true;
	}
	
	// invalid flag? always fulfilled
	return true;
}
