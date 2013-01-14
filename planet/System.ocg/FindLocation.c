/*
	This script contains the function FindLocation which uses a parameter-system similar to FindObject.
	This function should mainly be used for placement of objects at the start of a scenario.
	FindLocation is not guaranteed to always return a spot if a fitting spot exists, it's just best effort.
	
	Examples:
	finds a tunnel spot:
	FindLocation([Loc_Tunnel()]);
	finds a floor spot but not in front of tunnel:
	FindLocation([Loc_Not(Find_Tunnel()), Loc_Wall(CNAT_Bottom)]);
*/

static const LOC_INVALID = 0;
static const LOC_SOLID = 1;
static const LOC_INRECT = 2;
static const LOC_TUNNEL = 3;
static const LOC_CONDITION = 4;
static const LOC_WALL = 5;
static const LOC_SPACE = 6;
static const LOC_NOT = 7;

global func Loc_Condition(function)
{
	return [LOC_CONDITION, function];
}

global func Loc_Not(cond)
{
	return [LOC_NOT, cond];
}

global func Loc_InRect(x, int y, int w, int h)
{
	if (x == nil) return [LOC_INVALID];
	if (GetType(x) == C4V_PropList) return [LOC_INRECT, x];
	return [LOC_INRECT, Rectangle(x, y, w, h)];
}

global func Loc_Tunnel(bool sky)
{
	return [LOC_TUNNEL, sky ?? false];
}
global func Loc_Sky() { return Loc_Tunnel(true); }

global func Loc_Solid()
{
	return [LOC_SOLID];
}

// implies not solid
global func Loc_Wall(int direction)
{
	var x = 0, y = 0;
	if(direction & CNAT_Left) x = -1;
	else
	if(direction & CNAT_Right) x = 1;
	
	if(direction & CNAT_Top) y = -1;
	else
	if(direction & CNAT_Bottom) y = 1;
	return [LOC_WALL, x, y, !!(direction & (CNAT_Left | CNAT_Right)), !!(direction & (CNAT_Top | CNAT_Bottom))];
}

global func Loc_Space(int space, bool vertical)
{
	return [LOC_SPACE, space, vertical];
}

global func FindLocation(array flags, int repeat)
{
	// check for are restriction
	var rect = nil;
	var xdir = 0, ydir = 0, xmod = nil, ymod = nil;
	
	for (var flag in flags)
	{
		if (flags[0] == LOC_INRECT)
		{
			rect = flags[1];
		}
		else if (flags[0] == LOC_WALL)
		{
			xdir = flags[1];
			ydir = flags[2];
			xmod = flags[3];
			ymod = flags[4];
		}
	}
	rect = rect ?? Rectangle(0, 0, LandscapeWidth(), LandscapeHeight());
		
	repeat = repeat ?? 5000;
	while (repeat-- > 0)
	{
		var x = RandomX(rect.x, rect.x + rect.w);
		var y = RandomX(rect.y, rect.y + rect.h);
		
		// find wall-spot
		if(xdir || ydir)
		{
			var lx = xdir;
			var ly = ydir;
			if (xmod) if (Random(2)) lx *= -1;
			if (ymod) if (Random(2)) ly *= -1;
			
			if (GBackSolid(x, y)) continue;
			var valid = false;
			var failsafe = 0;
			do
			{
				if (GBackSolid(x + lx, y + ly)) {valid = true; break;}
				x += lx;
				y += ly;
			} while (++failsafe < 100);
			if (!valid) continue;
		}
		
		// check every flag
		var valid = true;
		for (var flag in flags)
		{
			if(FindLocationConditionCheckIsValid(flag, x, y)) continue;
			valid = false;
			break;
		}
		if (valid)
		{
			return {x = x, y = y};
		}
	}
}

global func FindLocationConditionCheckIsValid(flag, x, y)
{

	if(flag[0] == LOC_NOT) return !FindLocationConditionCheckIsValid(flag[1]);
	
	// area restriction
	if (flag[0] == LOC_SOLID)
	{
		if (GBackSolid(x, y)) return true;
		return false;
	}
	
	// in front of "Tunnel"?
	if (flag[0] == LOC_TUNNEL)
	{
		if ((GetMaterial(x, y) == -1) && flag[1]) return true;
		if (GetMaterial(x, y) == Material("Tunnel")) return true;
		return false;
	}
	
	// custom condition call
	if (flag[0] == LOC_CONDITION)
	{
		if (Global->Call(flag[1], x, y)) return true;
		return false;
	}
	
	// has some space?
	if (flag[0] == LOC_SPACE)
	{
		var xd = 0, yd = 0;
		if (flag[2]) yd = flag[1];
		else xd = flag[1];
				
		var d1 = PathFree2(x, y, x + xd, y + yd);
		var d2 = PathFree2(x, y, x - xd, y - yd);
		var d = 0;
		if (d1) d += Distance(x, y, d1[0], d1[1]);
		if (d2) d += Distance(x, y, d2[0], d2[1]);
		if (d >= flag[1]) return true;
		return false;
	}
	
	// invalid flag?
	return true;
}