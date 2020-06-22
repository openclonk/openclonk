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

#ifndef C4FOWDRAWSTRATEGY_H
#define C4FOWDRAWSTRATEGY_H

#include "C4ForbidLibraryCompilation.h"

#ifndef USE_CONSOLE

#include "graphics/C4DrawGL.h"
#include "graphics/C4Shader.h"

class C4FoWRegion;
class C4TargetFacet;
class C4FoWLight;

/** This class decomposes Triangle fans and quads into individual triangles.
 * This is used to so that all FoW elements can be drawn as triangles in a
 * single glDrawElements call. */
class C4FoWDrawTriangulator
{
public:
	C4FoWDrawTriangulator();

	void Fan();
	void Quads();

	void AddVertex();
	void Reset();

	const unsigned int* GetIndices() const { return &indices[0]; }
	unsigned int GetNIndices() const { return indices.size(); }

private:
	void FinishPrimitive();

	enum Mode {
		M_Fan,
		M_Quads
	};

	std::vector<unsigned int> indices; // TODO should be GLuint?
	unsigned int begin_vertices{0};
	unsigned int cur_vertices{0};
	Mode mode{M_Fan};
};

/** A C4FoWDrawStrategy is a connector to OpenGL calls used to draw the light.
   C4FoWLight tells this class which part of the light should be drawn now
   and subsequently pushes the vertices with the information whether a vertex
   is light or dark.
   
   This class is an abstract base class, it is up to the implementing classes
   to actually draw anything here.*/
class C4FoWDrawStrategy
{
public:
	C4FoWDrawStrategy() = default;
	virtual ~C4FoWDrawStrategy() = default;

	// Drawing phases
	enum DrawPhase {
		P_None,
		P_Fan,
		P_FanMaxed,
		P_Fade,
		P_Intermediate
	} phase{P_None};

	/** Called before each rendering pass */
	virtual void Begin(const C4FoWRegion* region) = 0;
	/** Called after each rendering pass */
	virtual void End(C4ShaderCall& call) { triangulator.Reset(); }

	virtual void DrawLightVertex(float x, float y) { triangulator.AddVertex(); }
	virtual void DrawDarkVertex(float x, float y) { triangulator.AddVertex(); }

	/** Called before rendering the inner triangle fan (the area with 100% light) */
	virtual void BeginFan() { triangulator.Fan(); phase = P_Fan; };
	/** Called after rendering the inner triangle fan */
	virtual void EndFan() { };
	
	/** Called before rendering the triangle fan existnsion (100% light, maxed out normals) */
	virtual void BeginFanMaxed() { triangulator.Quads(); phase = P_FanMaxed;  };
	/** Called after rendering the inner triangle fan */
	virtual void EndFanMaxed() { };

	/** Called before rendering the quads in which the light fades out */
	virtual void BeginFade() { triangulator.Quads(); phase = P_Fade; };
	/** Called after rendering the quads in which the light fades out */
	virtual void EndFade() { };

	/** Called before rendering the triangles that fill the space between the fadeout quads */
	virtual void BeginIntermediateFade() { triangulator.Fan(); phase = P_Intermediate; };
	/** Called after rendering the triangles that fill the space between the fadeout quads */
	virtual void EndIntermediateFade() { };

protected:
	C4FoWDrawTriangulator triangulator;
};

/** This draw strategy is the default draw strategy that draws the light
    onto the given region. */
class C4FoWDrawLightTextureStrategy : public C4FoWDrawStrategy
{
public:
	C4FoWDrawLightTextureStrategy(const C4FoWLight* light);
	~C4FoWDrawLightTextureStrategy() override;

	void DrawLightVertex(float x, float y) override;
	void DrawDarkVertex(float x, float y) override;
	void Begin(const C4FoWRegion* region) override;
	void End(C4ShaderCall& call) override;

private:
	void DrawVertex(float x, float y, bool shadeLight);
	
	static const float C4FoWSmooth;

	const C4FoWLight* light;
	const C4FoWRegion* region;

	struct Vertex {
		float x, y;           // position in upper half of texture
		float r1, g1, b1, a1; // color for first pass
		float r2, g2, b2, a2; // color for second pass
		float r3, g3, b3, a3; // color for third pass
	};

	GLuint bo[2];
	std::vector<Vertex> vertices;
	unsigned int vbo_size;
	unsigned int ibo_size;
	unsigned int vaoids[3]; // Three VAOs for the three passes
};

/** This draw strategy is the debug draw strategy (press Ctrl+F7,...) that
    draws a wireframe of the light triangles (except the inner fan, but you can
    change that in the code below) directly onto the screen. */
class C4FoWDrawWireframeStrategy : public C4FoWDrawStrategy
{
public:
	C4FoWDrawWireframeStrategy(const C4FoWLight* light, const C4TargetFacet *screen);
	~C4FoWDrawWireframeStrategy() override;
	//  : light(light), screen(screen), vbo(0) {};

	void DrawLightVertex(float x, float y) override;
	void DrawDarkVertex(float x, float y) override;
	void Begin(const C4FoWRegion* region) override;
	void End(C4ShaderCall& call) override;

private:
	struct Vertex {
		float x, y;
		float r, g, b, a;
	};

	void DrawVertex(Vertex& vertex);

	const C4TargetFacet* screen;

	GLuint bo[2];
	std::vector<Vertex> vertices;
	unsigned int vbo_size;
	unsigned int ibo_size;
	unsigned int vaoid;
};

#endif

#endif
