/*--
		Math.c
		Authors: Maikel, flgr, Newton, Tyron, Zapper

		Any kind of help with calculation.
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

// Returns the sign of x.
global func Sign(int x)
{
	if (x > 0)
		return 1;
	else if (x < 0)
		return -1;
	return 0;
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
	 /*if(to < from)*/dir=to-from;
	 //else dir=from-to;

	 var dif=360-from+to;
	 var dif2=360-to+from;
	 if(dif < 180)dir=+dif;
	 else
	 if(dif2 < 180)dir=-dif2;
	 
	 return dir;
}

global func SetBit(int old_val, int bit_nr, bool bit)
{
	if (GetBit(old_val, bit_nr) != (bit != 0))
		return ToggleBit(old_val, bit_nr);
	return old_val;
}

global func GetBit(int value, int bit_nr)
{
	return (value & (1 << bit_nr)) != 0;
}

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

// Ensure that the first rectangle is fully with the second one and returns an adjusted rectangle. Both rectangles can be created with Rectangle()
global func RectangleEnsureWithin(proplist first, proplist second)
{
	if (GetType(first) != C4V_PropList) return {};
	if (GetType(second) != C4V_PropList) return {};

	var adjusted = { x = first.x, y = first.y, w = first.w, h = first.h };
	if (first.x < second.x) adjusted.x = second.x;
	if (first.w > second.w) adjusted.w = second.w - (adjusted.x - second.x);
	if (adjusted.x + adjusted.w > second.x + second.w) adjusted.w = second.w - (adjusted.x - second.x);
	if (first.y < second.y) adjusted.y = second.y;
	if (first.h > second.h) adjusted.h = second.h - (adjusted.y - second.y);
	if (adjusted.y + adjusted.h > second.y + second.h) adjusted.h = second.h - (adjusted.y - second.y);

	return adjusted;
}

//Moves param 'a' towards param 'b' by 'max' amount per frame
global func MoveTowards(int a, int b, int max)
{
	if(b == nil) return false;
	if(max == nil) max = 1;
	if(a < b) return BoundBy(a + max,a,b);
	if(a > b) return BoundBy(a - max,b,a);
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