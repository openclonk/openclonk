/*--
		Proplists.c

		General helper functions that create or work with proplists.
--*/

// creates a proplists with the properties x, y, w, h that represents a rectangle
// satisfies that the resulting rectangle's x|y point is in the top-left corner and the width and height are positive
global func Rectangle(int x2, int y2, int w2, int h2)
{
/*
// creates a rectangle representing the landscape
var rect = Rectangle(0, 0, LandscapeWidth(), LandscapeHeight());
*/
	// normalize
	if(w2 < 0)
	{
		x2 += w2;
		w2 = -w2;
	}
	if(h2 < 0)
	{
		y2 += h2;
		h2 = - h2;
	}
	return {x = x2, y = y2, w = w2, h = h2};
}

// Checks whether or not a given point (x,y) is inside a rectangle.
// set global_coordinates TRUE to indicate that the given coordinates are from a global context
// (e.g. object->GetX()) but the rectangle has a local one. If so, the calling object's position
// will be subtracted
global func Rectangle_IsInside(proplist rect, int x, int y, bool global_coordinates)
{
	if (global_coordinates)
	{
		if (this)
		{
			x -= GetX();
			y -= GetY();
		}
	}
	if (Inside(x, rect.x, rect.x + rect.w) && Inside(y, rect.y, rect.y + rect.h))
		return true;
	return false;
}