/**
	Math.c
	Any kind of help with calculations.
	
	@author Maikel, flgr, Newton, Tyron, Zapper
*/

// Returns the offset to x.
// documented in /docs/sdk/script/fn
global func AbsX(int x)
{
	return x - GetX();
}

// Returns the offset to y.
// documented in /docs/sdk/script/fn
global func AbsY(int y)
{
	return y - GetY();
}

// Supports negative values, and can deliver random values between two bounds.
// documented in /docs/sdk/script/fn
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

// Returns the sign of x.
global func Sign(int x)
{
	return (x>0)-(x<0);
}

// Tangens.
// documented in /docs/sdk/script/fn
global func Tan(int angle, int radius, int prec)
{
	return radius * Sin(angle, radius * 100, prec) / Cos(angle, radius * 100, prec);
}

global func Normalize(int angle, int start, int precision)
{
	precision = precision ?? 1;
	var range = precision * 360;
	var end = start + range;
	
	while (angle < start)
	{
		angle += range;
	}
	while (angle >= end)
	{
		angle -= range;
	}

	return angle;
}

global func ComDirLike(int comdir1, int comdir2)
{
	if (comdir1 == comdir2)
		return true;
	if (comdir1 == COMD_Stop || comdir2 == COMD_Stop)
		return false;
	if (comdir1 == COMD_None || comdir2 == COMD_None)
		return false;
	if (comdir1 % 8 + 1 == comdir2)
		return true;
	if (comdir1 == comdir2 % 8 + 1)
		return true;
	return false;
}

// the shortest direction (left/right) to turn from one angle to another
// (for example for homing projectiles or aiming)
global func GetTurnDirection(
	int from /* the angle at which the turning starts */
	, int to /* the angle that should be turned towards */)
{
/*
	// code for a homing missile
	var dir = GetTurnDirection(my_angle, target_angle);
	SetR(GetR() + dir / 10);
	SetSpeed(Sin(GetR(), 10), -Cos(GetR(), 10));
*/
	 var dir;
	 /*if (to < from)*/dir = to-from;
	 //else dir = from-to;

	 var dif = 360-from + to;
	 var dif2 = 360-to + from;
	 if (dif < 180)dir=+dif;
	 else
	 if (dif2 < 180)dir=-dif2;
	 
	 return dir;
}

// documented in /docs/sdk/script/fn
global func SetBit(int old_val, int bit_nr, bool bit)
{
	if (GetBit(old_val, bit_nr) != (bit != 0))
		return ToggleBit(old_val, bit_nr);
	return old_val;
}

// documented in /docs/sdk/script/fn
global func GetBit(int value, int bit_nr)
{
	return (value & (1 << bit_nr)) != 0;
}

// documented in /docs/sdk/script/fn
global func ToggleBit(int old_val, int bit_nr)
{
	return old_val ^ (1 << bit_nr);
}

// Returns -1 for DIR_Left and +1 for DIR_Right or 0 if no object context is present
global func GetCalcDir()
{
	if (!this) return 0;
	return GetDir() * 2 - 1;
}

// Moves param 'a' towards param 'b' by 'max' amount per frame.
global func MoveTowards(int a, int b, int max)
{
	if (b == nil) return false;
	if (max == nil) max = 1;
	if (a < b) return BoundBy(a + max, a, b);
	if (a > b) return BoundBy(a - max, b, a);
}

global func FindHeight(int x)
{
	var y = 0;
	while (!GBackSemiSolid(x, y) && y < LandscapeHeight())
		y += 10;
	while (GBackSemiSolid(x, y) && y)
		y--;
	return y;
}

/*
	Returns the normal vector of the (solid) landscape at a point relative to an object.
	Can f.e. be used to bounce projectiles.
*/
global func GetSurfaceVector(int x, int y)
{
	var normal = [0, 0];
	
	for (var fac = 1; fac <= 4; fac *= 2)
	{
		if (GBackSolid(x + fac, y)) --normal[0];
		if (GBackSolid(x - fac, y)) ++normal[0];
			
		if (GBackSolid(x, y + fac)) --normal[1];
		if (GBackSolid(x, y - fac)) ++normal[1];
	}
	
	return normal;
}

// Mathematical modulo operation for calculations in ℤ/nℤ.
//
// Examples:
// (12 % 10) == Mod(12, 10) == 2
// (-1 % 10) == -1
// Mod(-1, 10) == 9
global func Mod(int dividend, int divisor)
{
	var a = dividend, b = divisor;
	return (a % b + b) % b;
}

// Returns whether the line from (x1, y1) to (x2, y2) overlaps with the line from (x3, y3) to (x4, y4).
// Whenever the two lines share a starting or ending point they are not considered to be overlapping.
global func IsLineOverlap(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
	// Same starting or ending point is not overlapping.
	if ((x1 == x3 && y1 == y3) || (x1 == x4 && y1 == y4) || (x2 == x3 && y2 == y3) || (x2 == x4 && y2 == y4))
		return false;	
	
	// Check if line from (x1, y1) to (x2, y2) crosses the line from (x3, y3) to (x4, y4).
	var d1x = x2 - x1, d1y = y2 - y1, d2x = x4 - x3, d2y = y4 - y3, d3x = x3 - x1, d3y = y3 - y1;
	var a = d1y * d3x - d1x * d3y;
	var b = d2y * d3x - d2x * d3y;
	var c = d2y * d1x - d2x * d1y;
	if (!c) 
		return !a && Inside(x3, x1, x2) && Inside(y3, y1, y2); // lines are parallel
	return a * c >= 0 && !(a * a / (c * c + 1)) && b * c >= 0 && !(b * b/(c * c + 1));
}
