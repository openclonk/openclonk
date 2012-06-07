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