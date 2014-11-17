
#include "C4Include.h"
#include "C4FoWAmbient.h"

namespace
{

template<typename IFT>
double AmbientForPix(int x0, int y0, int R, const IFT& ift)
{
	double d = 0.;

	for(int y = y0 - R; y <= y0 + R; ++y)
	{
		for(int x = x0 - R; x <= x0 + R; ++x)
		{
			const double l2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);
			if(l2 > R*R || ift(x, y)) continue;

			if(x == x0 && y == y0)
			{
				// Central pixel, this corresponds to the integral
				// for a round pixel with the same area as a square pixel
				d += 2 * sqrt(M_PI);
			}
			else
			{
				// Non-central pixel; assume same weight over the full
				// pixel.
				d += 1. / sqrt(l2);
			}
		}
	}

	return d;
}

// Everything is sky, independent of the landscape
// This is used to obtain the normalization factor
struct IFTFull {
	IFTFull() {}
	bool operator()(int x, int y) const { return false; }
};

struct IFTZoom {
	IFTZoom(const C4Landscape& landscape, double sx, double sy):
		Landscape(landscape), sx(sx), sy(sy) {}

	// Returns whether zoomed coordinate is IFT or not
	bool operator()(int x, int y) const
	{
		// Landscape coordinates
		const int lx = BoundBy(static_cast<int>((x + 0.5) * sx), 0, Landscape.Width - 1);
		const int ly = BoundBy(static_cast<int>((y + 0.5) * sy), 0, Landscape.Height - 1);
		// IFT check
		return (Landscape._GetPix(lx, ly) & IFT) != 0;
	}

	const C4Landscape& Landscape;
	const double sx;
	const double sy;
};

} // anonymous namespace

C4FoWAmbient::C4FoWAmbient()
	: Tex(0), SizeX(0), SizeY(0), LandscapeX(0), LandscapeY(0)
{
}

C4FoWAmbient::~C4FoWAmbient()
{
	Clear();
}

void C4FoWAmbient::Clear()
{
	if(Tex != 0) glDeleteTextures(1, &Tex);
	Tex = 0;
	SizeX = SizeY = 0;
	LandscapeX = LandscapeY = 0;
}

void C4FoWAmbient::CreateFromLandscape(const C4Landscape& landscape, double resolution, double radius, double full_coverage)
{
	// Clear old map
	if(Tex != 0) Clear();

	// Number of zoomed pixels
	LandscapeX = landscape.Width;
	LandscapeY = landscape.Height;
	SizeX = Min<unsigned int>(static_cast<unsigned int>(ceil(LandscapeX / resolution)), pDraw->MaxTexSize);
	SizeY = Min<unsigned int>(static_cast<unsigned int>(ceil(LandscapeY / resolution)), pDraw->MaxTexSize);

	// Factor to go from zoomed to landscape coordinates
	const double zoom_x = static_cast<double>(landscape.Width) / (float)(SizeX);
	const double zoom_y = static_cast<double>(landscape.Height) / (float)(SizeY);

	// Zoomed radius
	const double R = radius / sqrt(zoom_x * zoom_y);
	// Normalization factor with the full circle
	// The analytic result is 2*R*M_PI, and this number is typically close to it
	const double norm = AmbientForPix(0, 0, R, IFTFull()) * full_coverage;
	// Create the ambient map
	IFTZoom iftZoom(landscape, zoom_x, zoom_y);
	float* ambient = new float[SizeX * SizeY];
	for(unsigned int y = 0; y < SizeY; ++y)
	{
		for(unsigned int x = 0; x < SizeX; ++x)
		{
			ambient[y * SizeX + x] = Min(AmbientForPix(x, y, R, iftZoom) / norm, 1.0);
		}
	}

#if 0
	CSurface8 debug(SizeX, SizeY);
	for(unsigned int y = 0; y < SizeY; ++y)
	{
		for(unsigned int x = 0; x < SizeX; ++x)
		{
			debug.SetPix(x, y, int(ambient[y * SizeX + x] * 255. + 0.5));
			CStdPalette pal;
			for(int i = 0; i < 256; ++i)
				pal.Colors[i] = i + (i << 8) + (i << 16);
			debug.Save("Ambient.bmp", &pal);
		}
	}
#endif

	// Store it in a GL texture
	glGenTextures(1, &Tex);
	glBindTexture(GL_TEXTURE_2D, Tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, 1, SizeX, SizeY, 0, GL_RED, GL_FLOAT, ambient);
	delete[] ambient;

	LogF("Created %ux%u ambient map", SizeX, SizeY);
}
