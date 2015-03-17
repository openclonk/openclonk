/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2013, The OpenClonk Team and contributors
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

/* OpenGL implementation of NewGfx */


#if !defined(INC_StdGL) && !defined(USE_CONSOLE)
#define INC_StdGL

#ifdef _WIN32
#include <C4windowswrapper.h>
#endif
#include <GL/glew.h>

#ifdef USE_COCOA
#import "ObjectiveCAssociated.h"
#endif
#include <C4Draw.h>
#include <C4Shader.h>

class C4Window;

class C4DrawGLError: public std::exception
{
public:
	C4DrawGLError(const StdStrBuf& buf): Buf(buf) {}
	virtual ~C4DrawGLError() throw() {}

	virtual const char* what() const throw() { return Buf.getData(); }

private:
	StdCopyStrBuf Buf;
};

// Shader combinations
static const int C4SSC_MOD2 = 1; // signed addition instead of multiplication for clrMod
static const int C4SSC_BASE = 2; // use a base texture instead of just a single color
static const int C4SSC_OVERLAY = 4; // use a colored overlay on top of base texture
static const int C4SSC_LIGHT = 8; // use dynamic+ambient lighting
static const int C4SSC_NORMAL = 16; // extract normals from normal map instead of (0,0,1)

// Uniform data we give the sprite shader (constants from its viewpoint)
enum C4SS_Uniforms
{
	C4SSU_ClrMod, // always
	C4SSU_BaseTex, // C4SSC_BASE
	C4SSU_OverlayTex, // C4SSC_OVERLAY
	C4SSU_OverlayClr, // C4SSC_OVERLAY

	C4SSU_LightTex, // C4SSC_LIGHT
	C4SSU_LightTransform, // C4SSC_LIGHT
	C4SSU_NormalTex, // C4SSC_LIGHT | C4SSC_NORMAL

	C4SSU_AmbientTex, // C4SSC_LIGHT
	C4SSU_AmbientTransform, // C4SSC_LIGHT
	C4SSU_AmbientBrightness, // C4SSC_LIGHT

	C4SSU_Bones, // for meshes

	C4SSU_Count
};

// one OpenGL context
class CStdGLCtx
#ifdef USE_COCOA
	: public ObjectiveCAssociated
#endif
{
public:
	CStdGLCtx();  // ctor
	~CStdGLCtx() { Clear(); }; // dtor

	void Clear();               // clear objects

#ifdef USE_WIN32_WINDOWS
	bool Init(C4Window * pWindow, C4AbstractApp *pApp, HWND hWindow = NULL);
	std::vector<int> EnumerateMultiSamples() const;
#else
	bool Init(C4Window * pWindow, C4AbstractApp *pApp);
#endif

	bool Select(bool verbose = false);              // select this context
	void Deselect();              // select this context

	bool PageFlip();            // present scene

protected:
	void SelectCommon();
	// this handles are declared as pointers to structs
	C4Window * pWindow; // window to draw in
#ifdef USE_WIN32_WINDOWS
	HGLRC hrc;                  // rendering context
	HWND hWindow; // used if pWindow==NULL
	HDC hDC;                    // device context handle
	static bool InitGlew(HINSTANCE hInst);
#elif defined(USE_X11)
	/*GLXContext*/void * ctx;
#endif

	friend class CStdGL;
	friend class C4Surface;
};

// OpenGL encapsulation
class CStdGL : public C4Draw
{
public:
	CStdGL();
	~CStdGL();
protected:

	int iPixelFormat;           // used pixel format

	GLenum sfcFmt;              // texture surface format
	CStdGLCtx * pMainCtx;         // main GL context
	CStdGLCtx *pCurrCtx;        // current context (owned if fullscreen)
	int iClrDpt;                // color depth
	// texture for smooth lines
	GLuint lines_tex;
	// programs for drawing points, lines, quads

	// Sprite shaders -- there is a variety of shaders to avoid
	// conditionals in the GLSL code.
	C4Shader SpriteShader;
	C4Shader SpriteShaderMod2;
	C4Shader SpriteShaderBase;
	C4Shader SpriteShaderBaseMod2;
	C4Shader SpriteShaderBaseOverlay;
	C4Shader SpriteShaderBaseOverlayMod2;

	C4Shader SpriteShaderLight;
	C4Shader SpriteShaderLightMod2;
	C4Shader SpriteShaderLightBase;
	C4Shader SpriteShaderLightBaseMod2;
	C4Shader SpriteShaderLightBaseOverlay;
	C4Shader SpriteShaderLightBaseOverlayMod2;
	C4Shader SpriteShaderLightBaseNormal;
	C4Shader SpriteShaderLightBaseNormalMod2;
	C4Shader SpriteShaderLightBaseNormalOverlay;
	C4Shader SpriteShaderLightBaseNormalOverlayMod2;
public:
	// General
	void Clear();
	void Default();
	virtual bool IsOpenGL() { return true; }
	virtual bool IsShaderific() { return true; }
	virtual bool OnResolutionChanged(unsigned int iXRes, unsigned int iYRes); // reinit clipper for new resolution
	// Clipper
	bool UpdateClipper(); // set current clipper to render target
	bool PrepareMaterial(StdMeshMatManager& mat_manager, StdMeshMaterialLoader& loader, StdMeshMaterial& mat);
	// Surface
	bool PrepareRendering(C4Surface * sfcToSurface); // check if/make rendering possible to given surface
	virtual CStdGLCtx *CreateContext(C4Window * pWindow, C4AbstractApp *pApp);
#ifdef USE_WIN32_WINDOWS
	virtual CStdGLCtx *CreateContext(HWND hWindow, C4AbstractApp *pApp);
#endif
	// Blit
	void SetupMultiBlt(C4ShaderCall& call, const C4BltTransform* pTransform, GLuint baseTex, GLuint overlayTex, GLuint normalTex, DWORD dwOverlayModClr);
	void ResetMultiBlt();
	virtual void PerformMesh(StdMeshInstance &instance, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform);
	void FillBG(DWORD dwClr=0);
	// Drawing
	virtual void PerformMultiPix(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices);
	virtual void PerformMultiLines(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, float width);
	virtual void PerformMultiTris(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, const C4BltTransform* pTransform, C4TexRef* pTex, C4TexRef* pOverlay, C4TexRef* pNormal, DWORD dwOverlayClrMod);
	// device objects
	bool RestoreDeviceObjects();    // restore device dependent objects
	bool InvalidateDeviceObjects(); // free device dependent objects
	bool DeviceReady() { return !!pMainCtx; }
	bool InitShaders(C4GroupSet* pGroups); // load shaders from given group
	C4Shader* GetSpriteShader(int ssc);
	C4Shader* GetSpriteShader(bool haveBase, bool haveOverlay, bool haveNormal);

	struct
	{
		bool LowMaxVertexUniformCount;
	} Workarounds;

protected:
	bool CreatePrimarySurfaces(unsigned int iXRes, unsigned int iYRes, int iColorDepth, unsigned int iMonitor);

	bool CheckGLError(const char *szAtOp);
	virtual bool Error(const char *szMsg);

	bool CreateSpriteShader(C4Shader& shader, const char* name, int ssc, C4GroupSet* pGroups);

	friend class C4Surface;
	friend class C4TexRef;
	friend class C4Pattern;
	friend class CStdGLCtx;
	friend class C4StartupOptionsDlg;
	friend class C4FullScreen;
	friend class C4Window;
	friend class C4ShaderCall;
};

// Global access pointer
extern CStdGL *pGL;

#endif // INC_StdGL
