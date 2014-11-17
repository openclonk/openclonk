
#ifndef C4FOWAMBIENT_H
#define C4FOWAMBIENT_H

//#include <GL/gl.h>
#include <C4Landscape.h>

/**
	This class manages a texture that holds the ambient light intensity
*/
class C4FoWAmbient
{
public:
	C4FoWAmbient();
	~C4FoWAmbient();

	GLuint Tex;

private:
	// Landscape size
	unsigned int LandscapeX;
	unsigned int LandscapeY;
	// Size of ambient map
	unsigned int SizeX;
	unsigned int SizeY;

public:
	void Clear();

	// High resolution will make the map coarser, but speed up the generation process
	// and save video memory.
	// The radius specifies the radius of landscape pixels that are sampled around each pixel
	// to obtain the ambient light.
	// full_coverage is a number between 0 and 1, and it specifies what portion of the full circle
	// needs to be illuminated for full ambient light intensity.
	void CreateFromLandscape(const C4Landscape& landscape, double resolution, double radius, double full_coverage);

	unsigned int GetLandscapeWidth() const { return LandscapeX; }
	unsigned int GetLandscapeHeight() const { return LandscapeY; }
};

#endif // C4FOWAMBIENT_H
