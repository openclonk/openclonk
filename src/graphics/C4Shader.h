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

// Shader implementation somewhere in the middle between easy and extensible.

#ifndef INC_C4Shader
#define INC_C4Shader

#include "StdBuf.h"
#include "C4Surface.h"

// Shader version
const int C4Shader_Version = 120; // GLSL 1.20 / OpenGL 2.1

// Maximum number of texture coordinates
const int C4Shader_MaxTexCoords = 8;

// Maximum number of texture units per shader call
const int C4ShaderCall_MaxUnits = 32;

// Positions in fragment shader
const int C4Shader_PositionInit = 0;
const int C4Shader_PositionCoordinate = 20;
const int C4Shader_PositionTexture = 40;
const int C4Shader_PositionMaterial = 60;
const int C4Shader_PositionNormal = 80;
const int C4Shader_PositionLight = 100;
const int C4Shader_PositionColor = 120;
const int C4Shader_PositionFinish = 140;
const int C4Shader_LastPosition = 256;

// Positions in vertex shader
const int C4Shader_Vertex_TexCoordPos = 50;
const int C4Shader_Vertex_NormalPos = 60;
const int C4Shader_Vertex_PositionPos = 80;

class C4Shader
{
	friend class C4ShaderCall;
public:
	C4Shader();
	~C4Shader();

private:

	// Program texts
	struct ShaderSlice {
		int Position;
		StdCopyStrBuf Text;
		StdCopyStrBuf Source;
		int SourceTime;
	};
	typedef std::list<ShaderSlice> ShaderSliceList;
	ShaderSliceList VertexSlices, FragmentSlices;

	// Used texture coordinates
	int iTexCoords;

	// shaders
	GLhandleARB hVert, hFrag, hProg;
	// shader variables
	int iUniformCount;
	GLint *pUniforms;

public:
	enum VertexAttribIndex
	{
		// These correspond to the locations nVidia uses for the
		// respective gl_* attributes, so make sure whatever you
		// use for custom ones doesn't conflict with these UNLESS
		// you're not using the pre-defined ones in your shader
		VAI_Vertex = 0,
		VAI_Normal = 2,
		VAI_Color = 3,
		VAI_TexCoord0 = 8, // and upwards through TexCoord7 = 15

		// Make sure you move these if we implement multitexturing
		VAI_BoneWeights,
		VAI_BoneWeightsMax = VAI_BoneWeights + 1,
		VAI_BoneIndices,
		VAI_BoneIndicesMax = VAI_BoneIndices + VAI_BoneWeightsMax - VAI_BoneWeights
	};

	bool Initialised() const { return hVert != 0; }

	// Uniform getters
	GLint GetUniform(int iUniform) const {
		return iUniform >= 0 && iUniform < iUniformCount ? pUniforms[iUniform] : -1;
	}
	bool HaveUniform(int iUniform) const {
		return GetUniform(iUniform) != GLint(-1);
	}

	// Shader is composed from various slices
	void AddVertexSlice(int iPos, const char *szText);
	void AddFragmentSlice(int iPos, const char *szText, const char *szSource = "", int iFileTime = 0);
	void AddVertexSlices(const char *szWhat, const char *szText, const char *szSource = "", int iFileTime = 0);
	void AddFragmentSlices(const char *szWhat, const char *szText, const char *szSource = "", int iFileTime = 0);
	bool LoadSlices(C4GroupSet *pGroupSet, const char *szFile);

	// Add default vertex code (2D - no transformation)
	void AddVertexDefaults();

	// Allocate a texture coordinate, returning its ID to be used with glMultiTexCoord.
	// The texture coordinate will be visible to both shaders under the given name.
	// Note that in contrast to uniforms, these will not disappear if not used!
	GLenum AddTexCoord(const char *szName);

	// Assemble and link the shader. Should be called again after new slices are added.
	bool Init(const char *szWhat, const char **szUniforms);
	bool Refresh(const char *szWhat, const char **szUniforms);

	void ClearSlices();
	void Clear();

private:
	void AddSlice(ShaderSliceList& slices, int iPos, const char *szText, const char *szSource, int iFileTime);
	void AddSlices(ShaderSliceList& slices, const char *szWhat, const char *szText, const char *szSource, int iFileTime);
	int ParsePosition(const char *szWhat, const char **ppPos);

	StdStrBuf Build(const ShaderSliceList &Slices, bool fDebug = false);
	GLhandleARB Create(GLenum iShaderType, const char *szWhat, const char *szShader);
	void DumpInfoLog(const char *szWhat, GLhandleARB hShader);
	int GetObjectStatus(GLhandleARB hObj, GLenum type);

public:
	static bool IsLogging();
};

class C4ShaderCall
{
public:
	C4ShaderCall(const C4Shader *pShader) 
		: fStarted(false), pShader(pShader), iUnits(0)
	{ }
	~C4ShaderCall() { Finish(); }

private:
	bool fStarted;
	const C4Shader *pShader;
	int iUnits;
	GLenum hUnit[C4ShaderCall_MaxUnits];

public:
	GLint AllocTexUnit(int iUniform, GLenum iType);

	// Setting uniforms... Lots of code duplication here, not quite sure whether
	// something could be done about it.
	void SetUniform1i(int iUniform, int iX) const {
		if (pShader->HaveUniform(iUniform))
			glUniform1iARB(pShader->GetUniform(iUniform), iX);
	}
	void SetUniform1f(int iUniform, float gX) const {
		if (pShader->HaveUniform(iUniform))
			glUniform1fARB(pShader->GetUniform(iUniform), gX);
	}
	void SetUniform2f(int iUniform, float gX, float gY) const {
		if (pShader->HaveUniform(iUniform))
			glUniform2fARB(pShader->GetUniform(iUniform), gX, gY);
	}
	void SetUniform1iv(int iUniform, int iLength, const int *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform1ivARB(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform1fv(int iUniform, int iLength, const float *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform1fvARB(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform2fv(int iUniform, int iLength, const float *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform2fvARB(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform3fv(int iUniform, int iLength, const float *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform3fvARB(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform4fv(int iUniform, int iLength, const float *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform4fvARB(pShader->GetUniform(iUniform), iLength, pVals);
	}

	// Matrices are in row-major order
	void SetUniformMatrix2x3fv(int iUniform, int iLength, const float* pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniformMatrix3x2fv(pShader->GetUniform(iUniform), iLength, GL_TRUE, pVals);
	}

	void SetUniformMatrix3x4fv(int iUniform, int iLength, const float *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniformMatrix4x3fv(pShader->GetUniform(iUniform), iLength, GL_TRUE, pVals);
	}

	void SetUniformMatrix4x4fv(int iUniform, int iLength, const float* pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniformMatrix4fvARB(pShader->GetUniform(iUniform), iLength, GL_TRUE, pVals);
	}

	void Start();
	void Finish();
};

#endif // INC_C4Shader
