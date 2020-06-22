/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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

#ifndef C4LANDSCAPE_RENDER_H
#define C4LANDSCAPE_RENDER_H

#include "C4ForbidLibraryCompilation.h"
#include "graphics/C4FacetEx.h"
#include "graphics/C4Shader.h"
#include "graphics/C4Surface.h"

#include <chrono>

// Data we want to store per landscape pixel
enum C4LR_Byte {
	C4LR_Material,
	C4LR_BiasX,
	C4LR_BiasY,
	C4LR_Scaler,
	C4LR_Place,

	C4LR_ByteCount
};

// Uniform data we give the shader (constants from its viewpoint)
// Don't forget to update GetUniformName when introducing new uniforms!
enum C4LR_Uniforms
{
	C4LRU_ProjectionMatrix,

	C4LRU_LandscapeTex,
	C4LRU_ScalerTex,
	C4LRU_MaterialTex,
	C4LRU_LightTex,
	C4LRU_AmbientTex,

	C4LRU_Gamma,
	C4LRU_Resolution,
	C4LRU_Center,
	C4LRU_MatMapTex,
	C4LRU_MaterialDepth,
	C4LRU_MaterialSize,
	C4LRU_AmbientBrightness,
	C4LRU_AmbientTransform,
	C4LRU_Modulation,

	C4LRU_FrameCounter,
	C4LRU_Time,

	C4LRU_Count
};

enum C4LR_Attributes
{
	C4LRA_Position,
	C4LRA_LandscapeTexCoord,
	C4LRA_LightTexCoord,

	C4LRA_Count
};

// How much data we want to store per landscape pixel
const int C4LR_BytesPerPx = 3;

// How much data we can hold per surface, how much surfaces we therefore need.
const int C4LR_BytesPerSurface = 4;
const int C4LR_SurfaceCount = (C4LR_ByteCount + C4LR_BytesPerSurface - 1) / C4LR_BytesPerSurface;

class C4Landscape; class C4TextureMap;

class C4LandscapeRender
{
public:
	C4LandscapeRender() = default;
	virtual ~C4LandscapeRender() = default;

protected:
	int32_t iWidth{0}, iHeight{0};
	C4TextureMap *pTexs{nullptr};

public:

	virtual bool ReInit(int32_t iWidth, int32_t iHeight) = 0;
	virtual bool Init(int32_t iWidth, int32_t iHeight, C4TextureMap *pTexs, C4GroupSet *pGraphics) = 0;
	virtual void Clear() = 0;

	// Returns the rectangle of pixels that must be updated on changes in the given rect
	virtual C4Rect GetAffectedRect(C4Rect Rect) = 0;

	// Updates the landscape rendering to reflect the landscape contents in
	// the given rectangle
	virtual void Update(C4Rect Rect, C4Landscape *pSource) = 0;

	virtual void Draw(const C4TargetFacet &cgo, const class C4FoWRegion *Light, uint32_t clrMod) = 0;
};

#ifndef USE_CONSOLE
class C4LandscapeRenderGL : public C4LandscapeRender
{
public:
	C4LandscapeRenderGL();
	~C4LandscapeRenderGL() override;

private:
	// surfaces
	C4Surface *Surfaces[C4LR_SurfaceCount];

	// shader
	C4Shader Shader;
	C4Shader ShaderLight;
	static const char *UniformNames[];
	// VBO for landscape vertex data
	GLuint hVBO;
	// VAO IDs for rendering landscape w/ and w/o light
	unsigned int hVAOIDNoLight;
	unsigned int hVAOIDLight;

	// 1D texture for material map
	GLuint matMapTexture;
	// 2D texture array of material textures
	GLuint hMaterialTexture;
	// material texture positions in texture array
	std::vector<StdCopyStrBuf> MaterialTextureMap;
	// depth of material texture in layers
	int32_t iMaterialTextureDepth;
	// size of material textures (unzoomed)
	int32_t iMaterialWidth, iMaterialHeight;

	// scaler image
	C4FacetSurface fctScaler;

	// shader timer
	std::chrono::time_point<std::chrono::steady_clock> TimerStart;

public:
	bool ReInit(int32_t iWidth, int32_t iHeight) override;
	bool Init(int32_t iWidth, int32_t iHeight, C4TextureMap *pMap, C4GroupSet *pGraphics) override;
	void Clear() override;

	C4Rect GetAffectedRect(C4Rect Rect) override;
	void Update(C4Rect Rect, C4Landscape *pSource) override;

	void Draw(const C4TargetFacet &cgo, const C4FoWRegion *Light, uint32_t clrMod) override;

private:
	bool InitLandscapeTexture();
	bool InitMaterialTexture(C4TextureMap *pMap);
	bool LoadShader(C4GroupSet *pGraphics, C4Shader& shader, const char* name, int ssc);
	bool LoadShaders(C4GroupSet *pGraphics);
	bool InitVBO();
	void ClearShaders();
	bool LoadScaler(C4GroupSet *pGraphics);

	int CalculateScalerBitmask(int x, int y, C4Rect To, C4Landscape *pSource);

	int32_t LookupTextureTransition(const char *szFrom, const char *szTo);
	void AddTextureTransition(const char *szFrom, const char *szTo);
	void AddTextureAnim(const char *szTextureAnim);
	void AddTexturesFromMap(C4TextureMap *pMap);
	void BuildMatMap(uint32_t *pTex);
};
#endif

#endif // C4LANDSCAPE_RENDER_H
