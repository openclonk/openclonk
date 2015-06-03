/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2014-2015, The OpenClonk Team and contributors
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

#ifndef C4FOWDRAWSTRATEGY_H
#define C4FOWDRAWSTRATEGY_H

#include "C4DrawGL.h"
#include <list>

class C4FoWRegion;
class C4TargetFacet;
class C4FoWLight;

/** A C4FoWDrawStrategy is a connector to OpenGL calls used to draw the light.
   C4FoWLight tells this class which part of the light should be drawn now
   and subsequently pushes the vertices with the information whether a vertex
   is light or dark.
   
   This class is an abstract base class, it is up to the implementing classes
   to actually draw anything here.*/
class C4FoWDrawStrategy
{
public:
	C4FoWDrawStrategy() : phase(P_None) {}
	virtual ~C4FoWDrawStrategy() {}

	// Drawing phases
	enum DrawPhase {
		P_None,
		P_Fan,
		P_FanMaxed,
		P_Fade,
		P_Intermediate
	} phase;

	/** Returns in how many rendering passes the light should be rendered */
	virtual int32_t GetRequestedPasses() { return 1; };
	/** Called before each rendering pass */
	virtual void Begin(int32_t pass) = 0;
	/** Called after each rendering pass */
	virtual void End(int32_t pass) = 0;

	virtual void DrawLightVertex(float x, float y) = 0;
	virtual void DrawDarkVertex(float x, float y) = 0;

	/** Called before rendering the inner triangle fan (the area with 100% light) */
	virtual void BeginFan() { glBegin(GL_TRIANGLE_FAN); phase = P_Fan; };
	/** Called after rendering the inner triangle fan */
	virtual void EndFan() { glEnd(); };
	
	/** Called before rendering the triangle fan existnsion (100% light, maxed out normals) */
	virtual void BeginFanMaxed() { glBegin(GL_QUADS); phase = P_FanMaxed;  };
	/** Called after rendering the inner triangle fan */
	virtual void EndFanMaxed() { glEnd(); };

	/** Called before rendering the quads in which the light fades out */
	virtual void BeginFade() { glBegin(GL_QUADS); phase = P_Fade; };
	/** Called after rendering the quads in which the light fades out */
	virtual void EndFade() { glEnd(); };

	/** Called before rendering the triangles that fill the space between the fadeout quads */
	virtual void BeginIntermediateFade() { glBegin(GL_TRIANGLE_FAN); phase = P_Intermediate; };
	/** Called after rendering the triangles that fill the space between the fadeout quads */
	virtual void EndIntermediateFade() { glEnd(); };
};

/** This draw strategy is the default draw strategy that draws the light
    onto the given region. */
class C4FoWDrawLightTextureStrategy : public C4FoWDrawStrategy
{
public:
	C4FoWDrawLightTextureStrategy(const C4FoWLight* light, const C4FoWRegion* region) : light(light), region(region) {};

	virtual int32_t GetRequestedPasses() { return 2; };
	virtual void DrawLightVertex(float x, float y);
	virtual void DrawDarkVertex(float x, float y);
	virtual void Begin(int32_t pass);
	virtual void End(int32_t pass);

private:
	void DrawVertex(float x, float y, bool shadeLight);
	
	static const float C4FoWSmooth;

	const C4FoWLight* light;
	const C4FoWRegion* region;

	int32_t pass;
};

/** This draw strategy is the debug draw strategy (press Ctrl+F7,...) that
    draws a wireframe of the light triangles (except the inner fan, but you can
    change that in the code below) directly onto the screen. */
class C4FoWDrawWireframeStrategy : public C4FoWDrawStrategy
{
public:
	C4FoWDrawWireframeStrategy(const C4FoWLight* light, const C4TargetFacet *screen)
	  : light(light), screen(screen) {};

	virtual void DrawLightVertex(float x, float y);
	virtual void DrawDarkVertex(float x, float y);
	virtual void Begin(int32_t pass);
	virtual void End(int32_t pass);

private:
	void DrawVertex(float x, float y);

	const C4FoWLight* light;
	const C4TargetFacet* screen;
};

#endif
