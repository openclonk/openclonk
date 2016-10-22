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

#ifndef C4FOW_H
#define C4FOW_H

#include "C4ForbidLibraryCompilation.h"
#include "graphics/C4Surface.h"
#include "graphics/C4FacetEx.h"
#include "lib/C4Rect.h"
#include "object/C4Object.h"
#include "landscape/fow/C4FoWLight.h"
#include "landscape/fow/C4FoWAmbient.h"
#include "graphics/C4Shader.h"

/** Simple transformation class which allows translation and scales in x and y.
 * This is typically used to initialize shader uniforms to transform fragment
 * coordinates to some texture coordinates (e.g. landscape coordinates or
 * light texture coordinates). */
class C4FragTransform
{
public:
	C4FragTransform(): x(1.0f), y(1.0f), x0(0.0f), y0(0.0f) {}

	// Multiplies from left
	inline void Translate(float dx, float dy)
	{
		x0 += dx;
		y0 += dy;
	}

	// Multiplies from left
	inline void Scale(float sx, float sy)
	{
		x *= sx;
		y *= sy;

		x0 *= sx;
		y0 *= sy;
	}

	inline void Get2x3(float transform[6])
	{
		transform[0] = x;
		transform[1] = 0.f;
		transform[2] = x0;
		transform[3] = 0.f;
		transform[4] = y;
		transform[5] = y0;
	}

private:
	float x, y;
	float x0, y0;
};

enum C4FoWFramebufShaderUniforms {
	C4FoWFSU_ProjectionMatrix, // projection matrix
	C4FoWFSU_Texture,          // source texture

	C4FoWFSU_Count
};

enum C4FoWFramebufShaderAttributes {
	C4FoWFSA_Position,
	C4FoWFSA_TexCoord,

	C4FoWFSA_Count
};

enum C4FoWRenderShaderUniforms {
	C4FoWRSU_ProjectionMatrix, // projection matrix
	C4FoWRSU_VertexOffset,     // offset applied to vertex (TODO: could be encoded in projection matrix)

	C4FoWRSU_Count
};

enum C4FoWRenderShaderAttributes {
	C4FoWRSA_Position,
	C4FoWRSA_Color,

	C4FoWRSA_Count
};

/**
	This class holds all lights for the objects. It forwards the update, invalidation and render calls each to the
	lights.
*/
class C4FoW
{
public:
	C4FoW();
	~C4FoW();

private:
	/** linked list of all lights */
	class C4FoWLight *pLights;

	/** linked list of all dead light objects to be deleted on next render pass*/
	class C4FoWLight *deleted_lights;

public:
	C4FoWAmbient Ambient;

	// Shader to use for updating the frame buffer
	C4Shader *GetFramebufShader();
	// Shader to use for rendering the lights
	C4Shader *GetRenderShader();

	void ClearDeletedLights();

	/** Updates the view range of the given object in its associated light or create a new light if none exists yet. */
	void Add(C4Object *pObj);
	/** Removes the light associated with the given object, if any */
	void Remove(C4Object *pObj);

	/** Update all light beams within the given rectangle */
	void Update(C4Rect r, C4Player *player);
	/** Triggers the recalculation of all light beams within the given rectangle because the landscape changed. */
	void Invalidate(C4Rect r);

	void Render(class C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen, C4Player *pPlr, const StdProjectionMatrix& projectionMatrix);

private:
#ifndef USE_CONSOLE
	// Shader for updating the frame buffer
	C4Shader FramebufShader;
	C4Shader RenderShader;
#endif
};

#endif // C4FOW_H
