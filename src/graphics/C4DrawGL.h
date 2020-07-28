/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

#include "C4ForbidLibraryCompilation.h"
#if !defined(INC_StdGL) && !defined(USE_CONSOLE)
#define INC_StdGL

#ifdef _WIN32
#include "platform/C4windowswrapper.h"
#endif

#include <epoxy/gl.h>

#ifdef USE_COCOA
#import "platform/ObjectiveCAssociated.h"
#endif
#include "graphics/C4Draw.h"
#include "graphics/C4Shader.h"

class C4Window;

class C4DrawGLError: public std::exception
{
public:
	C4DrawGLError(const StdStrBuf& buf): Buf(buf) {}
	~C4DrawGLError() throw() override = default;

	const char* what() const throw() override { return Buf.getData(); }

private:
	StdCopyStrBuf Buf;
};

// Uniform data we give the sprite shader (constants from its viewpoint)
enum C4SS_Uniforms
{
	C4SSU_ProjectionMatrix, // 4x4
	C4SSU_ModelViewMatrix,  // 4x4
	C4SSU_NormalMatrix,     // 3x3, transpose-inverse of modelview matrix

	C4SSU_ClrMod, // always
	C4SSU_Gamma, // always
	C4SSU_Resolution, // always

	C4SSU_BaseTex, // C4SSC_BASE
	C4SSU_OverlayTex, // C4SSC_OVERLAY
	C4SSU_OverlayClr, // C4SSC_OVERLAY

	C4SSU_LightTex, // C4SSC_LIGHT
	C4SSU_LightTransform, // C4SSC_LIGHT
	C4SSU_NormalTex, // C4SSC_LIGHT | C4SSC_NORMAL

	C4SSU_AmbientTex, // C4SSC_LIGHT
	C4SSU_AmbientTransform, // C4SSC_LIGHT
	C4SSU_AmbientBrightness, // C4SSC_LIGHT

	C4SSU_MaterialAmbient, // for meshes
	C4SSU_MaterialDiffuse, // for meshes
	C4SSU_MaterialSpecular, // for meshes
	C4SSU_MaterialEmission, // for meshes
	C4SSU_MaterialShininess, // for meshes

	C4SSU_Bones, // for meshes
	C4SSU_CullMode, // for meshes

	C4SSU_FrameCounter, // for custom shaders

	C4SSU_Count
};

// Attribute data for sprites and meshes
enum C4SS_Attributes
{
	C4SSA_Position, // 2d for sprites, 3d for meshes
	C4SSA_Normal,  // meshes only
	C4SSA_TexCoord, // 2d
	C4SSA_Color,    // sprites only, 4d

	C4SSA_BoneIndices0,
	C4SSA_BoneIndices1,

	C4SSA_BoneWeights0,
	C4SSA_BoneWeights1,

	C4SSA_Count
};

// one OpenGL context
class CStdGLCtx
#ifdef USE_COCOA
	: public ObjectiveCAssociated
#endif
{
public:
	CStdGLCtx();  // ctor
	virtual ~CStdGLCtx() { Clear(); } // dtor

	virtual void Clear(bool multisample_change = false);               // clear objects

#ifdef USE_WGL
	std::vector<int> EnumerateMultiSamples() const;
#endif
	virtual bool Init(C4Window * pWindow, C4AbstractApp *pApp);

	virtual bool Select(bool verbose = false);              // select this context
	virtual void Deselect();              // select this context

	virtual bool PageFlip();            // present scene

protected:
	void SelectCommon();
	// this handles are declared as pointers to structs
	C4Window * pWindow{nullptr}; // window to draw in
#ifdef USE_WGL
	HDC hDC{nullptr};                    // device context handle
#elif defined(USE_SDL_MAINLOOP)
	void * ctx;
#endif

	// Global list of all OpenGL contexts in use
	static std::list<CStdGLCtx*> contexts;
	std::list<CStdGLCtx*>::iterator this_context;

	// VAOs available on this context
	std::vector<GLuint> hVAOs;
	// VAOs to be deleted the next time this context is being made current.
	std::vector<unsigned int> VAOsToBeDeleted;

	friend class CStdGL;
	friend class C4Surface;
};

#ifdef WITH_QT_EDITOR
// OpenGL context with Qt as backend. Implemented as subclass to allow co-existance with a different backend for fullscreen.
class CStdGLCtxQt : public CStdGLCtx
{
public:
	CStdGLCtxQt();
	~CStdGLCtxQt() override { Clear(); }

	void Clear(bool multisample_change = false) override;               // clear objects
	bool Init(C4Window * pWindow, C4AbstractApp *pApp) override;
	bool Select(bool verbose = false) override;              // select this context
	void Deselect() override;              // select this context
	bool PageFlip() override;            // present scene

private:
	class QOpenGLContext *context = nullptr;
	class QOffscreenSurface *surface = nullptr;
};
#endif

// OpenGL encapsulation
class CStdGL : public C4Draw
{
public:
	CStdGL();
	~CStdGL() override;
protected:

	int iPixelFormat;           // used pixel format

	GLenum sfcFmt;              // texture surface format
	CStdGLCtx * pMainCtx{nullptr};         // main GL context
	CStdGLCtx *pCurrCtx;        // current context (owned if fullscreen)
	// texture for smooth lines
	GLuint lines_tex;

	// The orthographic projection matrix
	StdProjectionMatrix ProjectionMatrix;

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

	// Generic VBOs for rendering arbitrary points, lines and
	// triangles, used by PerformMultiBlt. Use more than one VBO, so that
	// two PerformMultiBlt calls in quick succession can use two different
	// buffers. Otherwise, the second call would need to wait until the
	// rendering pipeline has actually drained the buffer. Each buffer
	// starts with a fixed size. If more than this many vertices need to
	// be rendered (can happen e.g. for PXS), then the buffer is resized.
	static const unsigned int N_GENERIC_VBOS = 16;
	static const unsigned int GENERIC_VBO_SIZE = 3 * 64; // vertices
	GLuint GenericVBOs[N_GENERIC_VBOS];
	unsigned int GenericVBOSizes[N_GENERIC_VBOS];
	unsigned int CurrentVBO{0};
	// We need twice as much VAOs, since the sprite rendering routines work
	// both with and without textures (in which case we either need texture
	// coordinates or not).
	unsigned int GenericVAOs[N_GENERIC_VBOS * 2];

	// VAO IDs currently in use.
	std::set<unsigned int> VAOIDs;
	std::set<unsigned int>::iterator NextVAOID;

	bool has_khr_debug;

public:
	// Create a new (unique) VAO ID. A VAO ID is a number that identifies
	// a certain VAO across all OpenGL contexts. This indirection is needed
	// because, unlike most other GL state, VAOs are not shared between
	// OpenGL contexts.
	unsigned int GenVAOID();

	// Free the given VAO ID, i.e. it can be re-used for new VAOs. This causes
	// the VAO associated with this ID to be deleted in all OpenGL contexts.
	void FreeVAOID(unsigned int vaoid);

	// Return a VAO with the given vao ID in the "vao" output variable.
	// If the function returns false, the VAO was newly created, otherwise
	// an existing VAO is returned.
	bool GetVAO(unsigned int vaoid, GLuint& vao);

	// General
	void Clear() override;
	void Default() override ;
	bool OnResolutionChanged(unsigned int iXRes, unsigned int iYRes) override; // reinit clipper for new resolution
	// Clipper
	bool UpdateClipper() override; // set current clipper to render target
	const StdProjectionMatrix& GetProjectionMatrix() const { return ProjectionMatrix; }
	bool PrepareMaterial(StdMeshMatManager& mat_manager, StdMeshMaterialLoader& loader, StdMeshMaterial& mat) override;
	// Surface
	bool PrepareRendering(C4Surface * sfcToSurface) override; // check if/make rendering possible to given surface
	bool PrepareSpriteShader(C4Shader& shader, const char* name, int ssc, C4GroupSet* pGroups, const char* const* additionalDefines, const char* const* additionalSlices) override;
	bool EnsureMainContextSelected() override;

	CStdGLCtx *CreateContext(C4Window * pWindow, C4AbstractApp *pApp) override;
	// Blit
	void SetupMultiBlt(C4ShaderCall& call, const C4BltTransform* pTransform, GLuint baseTex, GLuint overlayTex, GLuint normalTex, DWORD dwOverlayModClr, StdProjectionMatrix* out_modelview);
	void PerformMesh(StdMeshInstance &instance, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform) override;
	void FillBG(DWORD dwClr=0) override;
	// Drawing
	void PerformMultiPix(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, C4ShaderCall* shader_call) override;
	void PerformMultiLines(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, float width, C4ShaderCall* shader_call) override;
	void PerformMultiTris(C4Surface* sfcTarget, const C4BltVertex* vertices, unsigned int n_vertices, const C4BltTransform* pTransform, C4TexRef* pTex, C4TexRef* pOverlay, C4TexRef* pNormal, DWORD dwOverlayClrMod, C4ShaderCall* shader_call) override;
	void PerformMultiBlt(C4Surface* sfcTarget, DrawOperation op, const C4BltVertex* vertices, unsigned int n_vertices, bool has_tex, C4ShaderCall* shader_call);
	// device objects
	bool RestoreDeviceObjects() override;    // restore device dependent objects
	bool InvalidateDeviceObjects() override; // free device dependent objects
	bool DeviceReady() override { return !!pMainCtx; }
	bool InitShaders(C4GroupSet* pGroups); // load shaders from given group
	C4Shader* GetSpriteShader(int ssc);
	C4Shader* GetSpriteShader(bool haveBase, bool haveOverlay, bool haveNormal);

	struct
	{
		bool LowMaxVertexUniformCount;
		bool ForceSoftwareTransform;
	} Workarounds;
	void ObjectLabel(uint32_t identifier, uint32_t name, int32_t length, const char * label);

protected:
	bool CheckGLError(const char *szAtOp);
	const char* GLErrorString(GLenum code);
	bool Error(const char *szMsg) override;

	friend class C4Surface;
	friend class C4TexRef;
	friend class C4Pattern;
	friend class CStdGLCtx;
	friend class C4StartupOptionsDlg;
	friend class C4FullScreen;
	friend class C4Window;
	friend class C4ShaderCall;
	friend class C4FoWRegion;
#ifdef WITH_QT_EDITOR
	friend class CStdGLCtxQt;
#endif
};

// Global access pointer
extern CStdGL *pGL;

#endif // INC_StdGL
