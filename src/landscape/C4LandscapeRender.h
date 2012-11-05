
#ifndef C4LANDSCAPE_RENDER_H
#define C4LANDSCAPE_RENDER_H

#include "C4Surface.h"
#include "C4FacetEx.h"

// Data we want to store per landscape pixel
enum C4LR_Byte {
	C4LR_Material,
	C4LR_BiasX,
	C4LR_BiasY,
	C4LR_Scaler,

	C4LR_ByteCount
};

// Uniform data we give the shader (constants from its viewpoint)
// Don't forget to update GetUniformName when introducing new uniforms!
enum C4LR_Uniforms
{
	C4LRU_LandscapeTex,
	C4LRU_ScalerTex,
	C4LRU_MaterialTex,

	C4LRU_Resolution,
	C4LRU_MatMap,
	C4LRU_MatMapTex,
	C4LRU_MaterialDepth,
	C4LRU_MaterialSize,

	C4LRU_Count
};

// How much data we want to store per landscape pixel
const int C4LR_BytesPerPx = 3;

// How much data we can hold per surface, how much surfaces we therefore need.
const int C4LR_BytesPerSurface = 4;
const int C4LR_SurfaceCount = (C4LR_ByteCount + C4LR_BytesPerSurface - 1) / C4LR_BytesPerSurface;

// How many mip-map levels should be used at maximum?
const int C4LR_MipMapCount = 6;

class C4Landscape; class C4TextureMap;

class C4LandscapeRender
{
public:
	C4LandscapeRender()
		: iWidth(0), iHeight(0), pTexs(NULL) { }
	virtual ~C4LandscapeRender()
		{}

protected:
	int32_t iWidth, iHeight;
	C4TextureMap *pTexs;

public:

	virtual bool Init(int32_t iWidth, int32_t iHeight, C4TextureMap *pTexs, C4GroupSet *pGraphics) = 0;
	virtual void Clear() = 0;

	// Returns the rectangle of pixels that must be updated on changes in the given rect
	virtual C4Rect GetAffectedRect(C4Rect Rect) = 0;

	// Updates the landscape rendering to reflect the landscape contents in
	// the given rectangle
	virtual void Update(C4Rect Rect, C4Landscape *pSource) = 0;

	virtual void Draw(const C4TargetFacet &cgo) = 0;
};

#ifdef USE_GL
class C4LandscapeRenderGL : public C4LandscapeRender
{
public:
	C4LandscapeRenderGL();
	~C4LandscapeRenderGL();

private:
	// surfaces
	C4Surface *Surfaces[C4LR_SurfaceCount];

	// shader sources
	StdStrBuf LandscapeShader;
	StdStrBuf LandscapeShaderPath;
	int iLandscapeShaderTime;
	// shaders
	GLhandleARB hVert, hFrag, hProg;
	// shader variables
	GLhandleARB hUniforms[C4LRU_Count];

	// 3D texture of material textures
	GLuint hMaterialTexture[C4LR_MipMapCount];
	// material texture positions in 3D texture
	std::vector<StdCopyStrBuf> MaterialTextureMap;
	// depth of material texture in layers
	int32_t iMaterialTextureDepth;
	// size of material textures (unzoomed)
	int32_t iMaterialWidth, iMaterialHeight;

	// scaler image
	C4FacetSurface fctScaler;


public:
	virtual bool Init(int32_t iWidth, int32_t iHeight, C4TextureMap *pMap, C4GroupSet *pGraphics);
	virtual void Clear();

	virtual C4Rect GetAffectedRect(C4Rect Rect);
	virtual void Update(C4Rect Rect, C4Landscape *pSource);

	virtual void Draw(const C4TargetFacet &cgo);

	void RefreshShaders();

private:
	bool InitLandscapeTexture();
	bool InitMaterialTexture(C4TextureMap *pMap);
	bool LoadShaders(C4GroupSet *pGraphics);
	bool LoadScaler(C4GroupSet *pGraphics);

	void DumpInfoLog(const char *szWhat, GLhandleARB hShader, int32_t iWorkaround);
	int GetObjectStatus(GLhandleARB hObj, GLenum type);
	GLhandleARB CreateShader(GLenum iShaderType, const char *szWhat, const char *szCode, int32_t iWorkaround);

	bool InitShaders();
	void ClearShaders();

	int32_t LookupTextureTransition(const char *szFrom, const char *szTo);
	void AddTextureTransition(const char *szFrom, const char *szTo);
	void AddTextureAnim(const char *szTextureAnim);
	void AddTexturesFromMap(C4TextureMap *pMap);
	void BuildMatMap(GLfloat *pFMap, GLubyte *pIMap);
};
#endif

class C4LandscapeRenderClassic : public C4LandscapeRender
{
public:
	C4LandscapeRenderClassic();
	~C4LandscapeRenderClassic();

private:
	C4Surface *Surface32;

public:
	virtual bool Init(int32_t iWidth, int32_t iHeight, C4TextureMap *pMap, C4GroupSet *pGraphics);
	virtual void Clear();

	virtual C4Rect GetAffectedRect(C4Rect Rect);
	virtual void Update(C4Rect Rect, C4Landscape *pSource);

	virtual void Draw(const C4TargetFacet &cgo);

};

#endif // C4LANDSCAPE_RENDER_H
