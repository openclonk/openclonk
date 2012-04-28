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
global func GetTurnDirection(int from, int to)
{
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