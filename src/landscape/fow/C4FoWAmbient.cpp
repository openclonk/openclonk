/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2014-2016, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "landscape/fow/C4FoWAmbient.h"
#include "landscape/fow/C4FoW.h"
#include "graphics/C4Draw.h"

namespace
{

template<typename LightMap>
double AmbientForPix(int x0, int y0, double R, const LightMap& light_map)
{
	double d = 0.;

	const int Ri = static_cast<int>(R);
	for(int y = 1; y <= Ri; ++y)
	{
		// quarter circle
		int max_x = static_cast<int>(sqrt(R * R - y * y));
		for(int x = 1; x <= max_x; ++x)
		{
			const double l = sqrt(x*x + y*y);
			assert(l <= R);

			if(light_map(x0 + x, y0 + y)) d += 1. / l;
			if(light_map(x0 + x, y0 - y)) d += 1. / l;
			if(light_map(x0 - x, y0 - y)) d += 1. / l;
			if(light_map(x0 - x, y0 + y)) d += 1. / l;
		}

		// Vertical/Horizontal lines
		const double l = static_cast<double>(y);
		if(light_map(x0 + y, y0)) d += 1. / l;
		if(light_map(x0 - y, y0)) d += 1. / l;
		if(light_map(x0, y0 + y)) d += 1. / l;
		if(light_map(x0, y0 - y)) d += 1. / l;
	}

	// Central pix
	if(light_map(x0, y0)) d += 2 * sqrt(M_PI); // int_0^2pi int_0^1/sqrt(pi) 1/r dr r dphi

	return d;
}

// Everything is illuminated, independent of the landscape
// This is used to obtain the normalization factor
struct LightMapFull {
	LightMapFull() {}
	bool operator()(int x, int y) const { return true; }
};

struct LightMapZoom {
	LightMapZoom(const C4Landscape& landscape, double sx, double sy):
		Landscape(landscape), sx(sx), sy(sy) {}

	// Returns whether zoomed coordinate is LightMap or not
	bool operator()(int x, int y) const
	{
		// Landscape coordinates
		const int lx = Clamp(static_cast<int>((x + 0.5) * sx), 0, Landscape.GetWidth() - 1);
		const int ly = Clamp(static_cast<int>((y + 0.5) * sy), 0, Landscape.GetHeight() - 1);
		// LightMap check
		return ::Landscape._GetLight(lx, ly);
	}

	const C4Landscape& Landscape;
	const double sx;
	const double sy;
};

} // anonymous namespace

C4FoWAmbient::C4FoWAmbient() :
#ifndef USE_CONSOLE
	Tex(0),
#endif
	Resolution(0.), Radius(0.), FullCoverage(0.),
	SizeX(0), LandscapeX(0), SizeY(0), LandscapeY(0),
	Brightness(1.)
{
}

C4FoWAmbient::~C4FoWAmbient()
{
	Clear();
}

void C4FoWAmbient::Clear()
{
#ifndef USE_CONSOLE
	if(Tex != 0) glDeleteTextures(1, &Tex);
	Tex = 0;
#endif
	Resolution = Radius = FullCoverage = 0.;
	SizeX = SizeY = 0;
	LandscapeX = LandscapeY = 0;
	Brightness = 1.;
}

void C4FoWAmbient::CreateFromLandscape(const C4Landscape& landscape, double resolution, double radius, double full_coverage)
{
	assert(resolution >= 1.);
	assert(radius >= 1.);
	assert(full_coverage > 0 && full_coverage <= 1.);

	// Clear old map
	Clear();

	Resolution = resolution;
	Radius = radius;
	FullCoverage = full_coverage;

	// Number of zoomed pixels
	LandscapeX = Landscape.GetWidth();
	LandscapeY = Landscape.GetHeight();
	SizeX = std::min<unsigned int>(static_cast<unsigned int>(ceil(LandscapeX / resolution)), pDraw->MaxTexSize);
	SizeY = std::min<unsigned int>(static_cast<unsigned int>(ceil(LandscapeY / resolution)), pDraw->MaxTexSize);

#ifndef USE_CONSOLE
	glGenTextures(1, &Tex);
	glBindTexture(GL_TEXTURE_2D, Tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SizeX, SizeY, 0, GL_RED, GL_FLOAT, nullptr);

	const C4TimeMilliseconds begin = C4TimeMilliseconds::Now();
	UpdateFromLandscape(landscape, C4Rect(0, 0, Landscape.GetWidth(), Landscape.GetHeight()));
	uint32_t dt = C4TimeMilliseconds::Now() - begin;
	LogF("Created %ux%u ambient map in %g secs", SizeX, SizeY, dt / 1000.);
#endif
}

void C4FoWAmbient::UpdateFromLandscape(const C4Landscape& landscape, const C4Rect& update)
{
#ifndef USE_CONSOLE
	// Nothing to do?
	if(update.Wdt == 0 || update.Hgt == 0) return;

	assert(Tex != 0);

	// Factor to go from zoomed to landscape coordinates
	const double zoom_x = static_cast<double>(Landscape.GetWidth()) / SizeX;
	const double zoom_y = static_cast<double>(Landscape.GetHeight()) / SizeY;
	// Update region in zoomed coordinates
	const unsigned int left = std::max(static_cast<int>( (update.x - Radius) / zoom_x), 0);
	const unsigned int right = std::min(static_cast<unsigned int>( (update.x + update.Wdt + Radius) / zoom_x), SizeX - 1) + 1;
	const unsigned int top = std::max(static_cast<int>( (update.y - Radius) / zoom_y), 0);
	const unsigned int bottom = std::min(static_cast<unsigned int>( (update.y + update.Hgt + Radius) / zoom_y), SizeY - 1) + 1;
	assert(right > left);
	assert(bottom > top);
	// Zoomed radius
	const double R = Radius / sqrt(zoom_x * zoom_y);
	// Normalization factor with the full circle
	// The analytic result is 2*R*M_PI, and this number is typically close to it
	const double norm = AmbientForPix(0, 0, R, LightMapFull()) * FullCoverage;
	// Create the ambient map
	LightMapZoom light_mapZoom(landscape, zoom_x, zoom_y);
	float* ambient = new float[(right - left) * (bottom - top)];
	for(unsigned int y = top; y < bottom; ++y)
	{
		for(unsigned int x = left; x < right; ++x)
		{
			ambient[(y - top) * (right - left) + (x - left)] = std::min(AmbientForPix(x, y, R, light_mapZoom) / norm, 1.0);
		}
	}

#if 0
	CSurface8 debug(SizeX, SizeY);
	for(unsigned int y = 0; y < SizeY; ++y)
	{
		for(unsigned int x = 0; x < SizeX; ++x)
		{
			debug.SetPix(x, y, int(ambient[y * SizeX + x] * 255. + 0.5));
		}
	}

	CStdPalette pal;
	for(int i = 0; i < 256; ++i)
		pal.Colors[i] = i + (i << 8) + (i << 16);
	debug.Save("Ambient.bmp", &pal);
#endif

	// Update the texture
	glBindTexture(GL_TEXTURE_2D, Tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, left, top, (right - left), (bottom - top), GL_RED, GL_FLOAT, ambient);
	delete[] ambient;
#endif
}

void C4FoWAmbient::GetFragTransform(const FLOAT_RECT& vpRect, const C4Rect& clipRect, const C4Rect& outRect, float ambientTransform[6]) const
{
	C4FragTransform trans;
	// Invert Y coordinate
	trans.Scale(1, -1);
	trans.Translate(0, outRect.Hgt);
	// Clip offset
	trans.Translate(-clipRect.x, -clipRect.y);
	// Clip normalization (0,0 -> 1,1)
	trans.Scale(1.0f / clipRect.Wdt, 1.0f / clipRect.Hgt);
	// Viewport normalization
	trans.Scale(vpRect.right - vpRect.left, vpRect.bottom - vpRect.top);
	// Viewport offset
	trans.Translate(vpRect.left, vpRect.top);
	// Landscape normalization
	trans.Scale(1.0f / LandscapeX, 1.0f / LandscapeY);

	// Extract matrix
	trans.Get2x3(ambientTransform);
}
