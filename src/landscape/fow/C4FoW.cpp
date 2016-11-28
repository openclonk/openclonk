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

#include <float.h>


C4FoW::C4FoW()
	: pLights(nullptr), deleted_lights(nullptr)
{
}

C4FoW::~C4FoW()
{
	if (deleted_lights)
	{
		if (pDraw) pDraw->EnsureMainContextSelected();
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
		szUniforms[C4FoWRSU_LightSourcePosition] = "lightSourcePosition";
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

C4Shader *C4FoW::GetDirectionalRenderShader()
{
#ifndef USE_CONSOLE
	// Not created yet?
	if (!DirectionalRenderShader.Initialised())
	{
		// Create the directional render shader. Similar to the
		// normal render shader except discards pixels not in the
		// direction of the light (w/ fade-out).
		const char* DirectionalRenderVertexShader =
			"in vec2 oc_Position;\n"
			"in vec4 oc_Color;\n"
			"out vec4 vtxColor;\n"
                        "out vec2 vtxLightDir;\n"
			"uniform mat4 projectionMatrix;\n"
                        "uniform vec2 lightSourcePosition;\n"
			"\n"
			"slice(position)\n"
			"{\n"
			"  gl_Position = projectionMatrix * vec4(oc_Position, 0.0, 1.0);\n"
			"}\n"
			"\n"
			"slice(color)\n"
			"{\n"
			"  vtxColor = oc_Color;\n"
                        "  vtxLightDir = (oc_Position - lightSourcePosition);\n"
			"}";

		const char* DirectionalRenderFragmentShader =
			"in vec4 vtxColor;\n"
			"in vec2 vtxLightDir;\n"
			"out vec4 fragColor;\n"
			"\n"
			// TODO: these are going to be uniforms:
			"const vec2 lightDirection = vec2(1.0, 0.0);\n"
			"const float PI = 3.141592653589793238462643383;\n"
			"const float lightAngularRangeCos = cos(20.0 * PI / 180.0);\n" // 50 deg
			"const float lightAngularFadeCos = cos((20.0 + 30.0) * PI / 180.0);\n" // 80 deg
			"\n"
			"const float lightAngularDistance = 5.0;\n"
			"const float lightAngularDistanceFade = 5.0 + 15.0;\n"
			"\n"
			"slice(color)\n"
			"{\n"
			"  float lightLen = sqrt(vtxLightDir.x * vtxLightDir.x + vtxLightDir.y * vtxLightDir.y);\n"
			"  float angDiffCos = dot(lightDirection, vtxLightDir) / lightLen;\n"
			"\n"
			"  float angOneMinusIntensity;\n"
			"  if (angDiffCos <= lightAngularFadeCos)\n"
			"    angOneMinusIntensity = 1.0;\n"
                        //"  float dist = min(1.0, 0.02 * sqrt(vtxLightDir.x * vtxLightDir.x + vtxLightDir.y * vtxLightDir.y));\n"
			//"  fragColor = vec4(0.0, 0.5/1.5, 0.5/1.5, 1.0);\n"
			//"  fragColor = vtxColor;\n"
			"  else if (angDiffCos <= lightAngularRangeCos)\n"
			// TODO: this uses linear interpolation on the cosine values, which is not linear in the angle. Might need to use acos() here if things look funny.
			"    angOneMinusIntensity = (lightAngularRangeCos - angDiffCos) / (lightAngularRangeCos - lightAngularFadeCos);\n"
			"  else\n"
			"    angOneMinusIntensity = 0.0;\n"
                        "\n"
                        "  float distOneMinusIntensity;\n"
			"  if (lightLen >= lightAngularDistanceFade)\n"
			"    distOneMinusIntensity = 1.0;\n"
			"  else if (lightLen >= lightAngularDistance)\n"
			"    distOneMinusIntensity = (lightLen - lightAngularDistance) / (lightAngularDistanceFade - lightAngularDistance);\n"
			"  else\n"
			"    distOneMinusIntensity = 0.0;\n"
			"\n"
                        "  fragColor = vec4(vtxColor.rgb, (1.0 - distOneMinusIntensity*angOneMinusIntensity) * vtxColor.a);\n"
			"}";

		DirectionalRenderShader.AddVertexSlices("built-in FoW directional render shader", DirectionalRenderVertexShader);
		DirectionalRenderShader.AddFragmentSlices("built-in FoW directional render shader", DirectionalRenderFragmentShader);

		const char* szUniforms[C4FoWRSU_Count + 1];
		szUniforms[C4FoWRSU_ProjectionMatrix] = "projectionMatrix";
		szUniforms[C4FoWRSU_VertexOffset] = "vertexOffset";
		szUniforms[C4FoWRSU_LightSourcePosition] = "lightSourcePosition";
		szUniforms[C4FoWRSU_Count] = nullptr;

		const char* szAttributes[C4FoWRSA_Count + 1];
		szAttributes[C4FoWRSA_Position] = "oc_Position";
		szAttributes[C4FoWRSA_Color] = "oc_Color";
		szAttributes[C4FoWRSA_Count] = nullptr;

		if (!DirectionalRenderShader.Init("fowDirectionalRender", szUniforms, szAttributes)) {
			DirectionalRenderShader.ClearSlices();
			return nullptr;
		}

	}
	return &DirectionalRenderShader;
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

	C4Shader *pDirectionalShader = GetDirectionalRenderShader();
	assert(pDirectionalShader);
	if (!pDirectionalShader) return;

	for (C4FoWLight *pLight = pLights; pLight; pLight = pLight->getNext())
		if (pLight->IsVisibleForPlayer(pPlr))
			pLight->Render(pRegion, pOnScreen, projectionMatrix, *pShader, *pDirectionalShader);
#endif
}
