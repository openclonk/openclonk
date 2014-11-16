#ifndef C4FOWBEAMTRIANGLE_H
#define C4FOWBEAMTRIANGLE_H

/** A simple data structure holding data, mainly positions, about each beam
    that is used for the rendering. The coordinates are global.
*/
class C4FoWBeamTriangle
{
public:
	C4FoWBeamTriangle() : clipLeft(false), clipRight(false) {};

	// whether this triangle is the last one in a row because the triangle
	// that would normally be left/right of it is clipped
	bool clipLeft, clipRight;

	float fanLX, fanLY,   // left point of the triangle that has 100% light
	      fanRX, fanRY,   // right point of the triangle that has 100% light
	      fadeLX, fadeLY, // left point of the quad in which the light fades out
	      fadeRY, fadeRX, // right point of the quad in which the light fades out
	      fadeIX, fadeIY; // intermediate fade point filling the space between the fade quads

	// whether the next triangle's edge is shorter than the edge of this one
	bool descending;
};

#endif