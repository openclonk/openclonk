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
static const LOC_MATERIAL = 3;
static const LOC_FUNC = 4;
static const LOC_WALL = 5;
static const LOC_SPACE = 6;
static const LOC_NOT = 7;
static const LOC_SKY = 8;
static const LOC_LIQUID = 9;
static const LOC_OR = 10;
static const LOC_AND = 11;
static const LOC_MAXTRIES = 12;

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
	if (GetType(x) == C4V_PropList) return [LOC_INRECT, x];
	return [LOC_INRECT, Rectangle(x, y, w, h)];
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
global func Loc_Wall(int direction)
{
	var x = 0, y = 0;
	if(direction & CNAT_Left) x = -1;
	else
	if(direction & CNAT_Right) x = 1;
	
	if(direction & CNAT_Top) y = -1;
	else
	if(direction & CNAT_Bottom) y = 1;
	
	var both_left_right = !!((direction & CNAT_Left) && (direction & CNAT_Right));
	var both_top_bottom = !!((direction & CNAT_Top) && (direction & CNAT_Bottom));
	
	return [LOC_WALL, x, y, both_left_right, both_top_bottom];
}

/*
	returns a spot with enough space either vertically or horizontally.
	For example Loc_Space(20, true) would only return points where a Clonk could stand.
*/
global func Loc_Space(int space, bool vertical)
{
	return [LOC_SPACE, space, vertical];
}

global func FindLocation(condition1, ...)
{
	var rect = nil;
	var xdir = 0, ydir = 0, xmod = nil, ymod = nil;
	var flags = [];
	// maximum number of tries
	var repeat = 5000;
	
	//  put parameters in array and filter out special parameters
	for (var i = 0; i < 10; ++i)
	{
		var f = Par(i);
		if (!f) continue;
		
		if (f[0] == LOC_INRECT)
		{
			rect = f[1];
		}
		else if (f[0] == LOC_WALL)
		{
			xdir = f[1];
			ydir = f[2];
			xmod = f[3];
			ymod = f[4];
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
	rect = rect ?? Rectangle(0, 0, LandscapeWidth(), LandscapeHeight());
	
	// repeat until a spot is found or max. number of tries exceeded
	while (repeat-- > 0)
	{
		var x = RandomX(rect.x, rect.x + rect.w);
		var y = RandomX(rect.y, rect.y + rect.h);
		
		// find wall-spot
		// this part just moves the random point to left/right/up/down until a wall is hit
		if (xdir || ydir)
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
			if (Global->FindLocationConditionCheckIsValid(flag, x, y)) continue;
			valid = false;
			break;
		}
		if (valid)
		{
			return {x = x, y = y};
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
	
	if (flag[0] == LOC_INRECT)
	{
		var rect = flag[1];
		return Inside(x, rect.x, rect.x + rect.w) && Inside(y, rect.y, rect.y + rect.h);
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
		
	// custom condition call
	if (flag[0] == LOC_FUNC)
	{
		return Global->Call(flag[1], x, y);
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
	
	// invalid flag? always fulfilled
	return true;
}
