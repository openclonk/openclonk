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
#include "landscape/fow/C4FoW.h"
#include "graphics/C4Draw.h"

#include <cfloat>


C4FoW::C4FoW() = default;

C4FoW::~C4FoW()
{
	if (deleted_lights)
	{
		ClearDeletedLights();
	}
}

void C4FoW::ClearDeletedLights()
{
#ifndef USE_CONSOLE
	// Kill any dead lights
	while (deleted_lights)
	{
		C4FoWLight *light = deleted_lights;
		deleted_lights = deleted_lights->getNext();
		delete light;
	}
#endif
}

C4Shader *C4FoW::GetFramebufShader()
{
#ifndef USE_CONSOLE
	// Not created yet?
	if (!FramebufShader.Initialised())
	{
		// Create the frame buffer shader. The details are in C4FoWRegion, but
		// this is about how to utilise old frame buffer data in the lights texture.
		// Or put in other words: This shader is responsible for fading lights out.
		const char* FramebufVertexShader =
			"in vec2 oc_Position;\n"
			"in vec2 oc_TexCoord;\n"
			"out vec2 texcoord;\n"
			"uniform mat4 projectionMatrix;\n"
			"\n"
			"slice(position)\n"
			"{\n"
			"  gl_Position = projectionMatrix * vec4(oc_Position, 0.0, 1.0);\n"
			"}\n"
			"\n"
			"slice(texcoord)\n"
			"{\n"
			"  texcoord = oc_TexCoord;\n"
			"}";

		const char* FramebufFragmentShader =
			"in vec2 texcoord;\n"
			"uniform sampler2D tex;\n"
			"out vec4 fragColor;\n"
			"\n"
			"slice(color)\n"
			"{\n"
			"  fragColor = texture(tex, texcoord);\n"
			"}";

		FramebufShader.AddVertexSlices("built-in FoW framebuf shader", FramebufVertexShader);
		FramebufShader.AddFragmentSlices("built-in FoW framebuf shader", FramebufFragmentShader);

		const char *szUniforms[C4FoWFSU_Count + 1];
		szUniforms[C4FoWFSU_ProjectionMatrix] = "projectionMatrix";
		szUniforms[C4FoWFSU_Texture] = "tex";
		szUniforms[C4FoWFSU_Count] = nullptr;

		const char *szAttributes[C4FoWFSA_Count + 1];
		szAttributes[C4FoWFSA_Position] = "oc_Position";
		szAttributes[C4FoWFSA_TexCoord] = "oc_TexCoord";
		szAttributes[C4FoWFSA_Count] = nullptr;

		if (!FramebufShader.Init("framebuf", szUniforms, szAttributes)) {
			FramebufShader.ClearSlices();
			return nullptr;
		}
	}
	return &FramebufShader;
#else
	return nullptr;
#endif
}

C4Shader *C4FoW::GetRenderShader()
{
#ifndef USE_CONSOLE
	// Not created yet?
	if (!RenderShader.Initialised())
	{
		// Create the render shader. Fairly simple pass-through.
		const char* RenderVertexShader =
			"in vec2 oc_Position;\n"
			"in vec4 oc_Color;\n"
			"out vec4 vtxColor;\n"
			"uniform mat4 projectionMatrix;\n"
			"uniform vec2 vertexOffset;\n"
			"\n"
			"slice(position)\n"
			"{\n"
			"  gl_Position = projectionMatrix * vec4(oc_Position + vertexOffset, 0.0, 1.0);\n"
			"}\n"
			"\n"
			"slice(color)\n"
			"{\n"
			"  vtxColor = oc_Color;\n"
			"}";

		const char* RenderFragmentShader =
			"in vec4 vtxColor;\n"
			"out vec4 fragColor;\n"
			"\n"
			"slice(color)\n"
			"{\n"
			"  fragColor = vtxColor;\n"
			"}";

		RenderShader.AddVertexSlices("built-in FoW render shader", RenderVertexShader);
		RenderShader.AddFragmentSlices("built-in FoW render shader", RenderFragmentShader);

		const char* szUniforms[C4FoWRSU_Count + 1];
		szUniforms[C4FoWRSU_ProjectionMatrix] = "projectionMatrix";
		szUniforms[C4FoWRSU_VertexOffset] = "vertexOffset";
		szUniforms[C4FoWRSU_Count] = nullptr;

		const char* szAttributes[C4FoWRSA_Count + 1];
		szAttributes[C4FoWRSA_Position] = "oc_Position";
		szAttributes[C4FoWRSA_Color] = "oc_Color";
		szAttributes[C4FoWRSA_Count] = nullptr;

		if (!RenderShader.Init("fowRender", szUniforms, szAttributes)) {
			RenderShader.ClearSlices();
			return nullptr;
		}

	}
	return &RenderShader;
#else
	return nullptr;
#endif
}


void C4FoW::Add(C4Object *pObj)
{
#ifndef USE_CONSOLE
	// No view range? Probably want to remove instead
	if(!pObj->lightRange && !pObj->lightFadeoutRange)
	{
		Remove(pObj);
		return;
	}

	// Safety
	if (!pObj->Status) return;

	// Look for matching light
	C4FoWLight *pLight;
	for (pLight = pLights; pLight; pLight = pLight->getNext())
		if (pLight->getObj() == pObj)
			break;

	if (pLight)
	{

		// Update reach and light color
		pLight->SetReach(pObj->lightRange, pObj->lightFadeoutRange);
		pLight->SetColor(pObj->lightColor);
	}
	else
	{
		// Create new light otherwise
		pLight = new C4FoWLight(pObj);
		pLight->pNext = pLights;
		pLights = pLight;
	}
#endif
}

void C4FoW::Remove(C4Object *pObj)
{
#ifndef USE_CONSOLE
	// Look for matching light
	C4FoWLight *pPrev = nullptr, *pLight;
	for (pLight = pLights; pLight; pPrev = pLight, pLight = pLight->getNext())
		if (pLight->getObj() == pObj)
			break;
	if (!pLight)
		return;

	// Remove from list
	(pPrev ? pPrev->pNext : pLights) = pLight->getNext();

	// Delete on next render pass
	pLight->pNext = deleted_lights;
	deleted_lights = pLight;
#endif
}

void C4FoW::Invalidate(C4Rect r)
{
#ifndef USE_CONSOLE
	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		pLight->Invalidate(r);
#endif
}

void C4FoW::Update(C4Rect r, C4Player *pPlr)
{
#ifndef USE_CONSOLE
	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		if (pLight->IsVisibleForPlayer(pPlr))
			pLight->Update(r);
#endif
}

void C4FoW::Render(C4FoWRegion *pRegion, const C4TargetFacet *pOnScreen, C4Player *pPlr, const StdProjectionMatrix& projectionMatrix)
{
#ifndef USE_CONSOLE
	// Delete any dead lights
	ClearDeletedLights();
	// Set up shader. If this one doesn't work, we're really in trouble.
	C4Shader *pShader = GetRenderShader();
	assert(pShader);
	if (!pShader) return;

	C4ShaderCall Call(pShader);
	Call.Start();
	Call.SetUniformMatrix4x4(C4FoWRSU_ProjectionMatrix, projectionMatrix);

	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		if (pLight->IsVisibleForPlayer(pPlr))
			pLight->Render(pRegion, pOnScreen, Call);

	Call.Finish();
#endif
}
