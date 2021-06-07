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

// Shader implementation somewhere in the middle between easy and extensible.

#ifndef INC_C4Shader
#define INC_C4Shader

#include "C4ForbidLibraryCompilation.h"
#include "lib/StdMeshMath.h"
#include "graphics/C4Surface.h"

#ifdef _WIN32
#include "platform/C4windowswrapper.h"
#endif

#ifndef USE_CONSOLE
#include <epoxy/gl.h>
#endif

#include <stack>

// Shader version
const int C4Shader_Version = 150; // GLSL 1.50 / OpenGL 3.2

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
const int C4Shader_Vertex_ColorPos = 70;
const int C4Shader_Vertex_PositionPos = 80;

class C4Shader
{
	friend class C4ShaderCall;
	friend class C4ScriptUniform;
public:
	C4Shader();
	~C4Shader();

private:

	StdStrBuf Name;

	// Program texts
	struct ShaderSlice {
		int Position;
		StdCopyStrBuf Text;
		StdCopyStrBuf Source;
		int SourceLine;
		int SourceTime;
	};
	typedef std::list<ShaderSlice> ShaderSliceList;
	ShaderSliceList VertexSlices, FragmentSlices;
	std::vector<std::string> SourceFiles;
	std::vector<std::string> Categories;
	std::set<int> ScriptShaders;

	int GetSourceFileId(const char *file) const;

	// Last refresh check
	C4TimeMilliseconds LastRefresh;
	bool ScriptSlicesLoaded = false;

	// Used texture coordinates
	int iTexCoords{0};

#ifndef USE_CONSOLE
	// shaders
	GLuint hProg{0};
	// shader variables
	struct Variable { int address; const char* name; };
	std::vector<Variable> Uniforms;
	std::vector<Variable> Attributes;
#endif

public:
	bool Initialised() const
	{
#ifndef USE_CONSOLE
		return hProg != 0;
#else
		return true;
#endif
	}

	// Uniform getters
#ifndef USE_CONSOLE
	GLint GetUniform(int iUniform) const
	{
		return iUniform >= 0 && static_cast<unsigned int>(iUniform) < Uniforms.size() ? Uniforms[iUniform].address : -1;
	}

	bool HaveUniform(int iUniform) const
	{
		return GetUniform(iUniform) != GLint(-1);
	}

	GLint GetAttribute(int iAttribute) const
	{
		return iAttribute >= 0 && static_cast<unsigned int>(iAttribute) < Attributes.size() ? Attributes[iAttribute].address : -1;
	}

#else
	int GetUniform(int iUniform) const
	{
		return -1;
	}
	bool HaveUniform(int iUniform) const
	{
		return false;
	}
	int GetAttribute(int iAttribute) const
	{
		return -1;
	}
#endif

	// Shader is composed from various slices
	void AddDefine(const char* name);
	void AddVertexSlice(int iPos, const char *szText);
	void AddFragmentSlice(int iPos, const char *szText);
	void AddVertexSlices(const char *szWhat, const char *szText, const char *szSource = "", int iFileTime = 0);
	void AddFragmentSlices(const char *szWhat, const char *szText, const char *szSource = "", int iFileTime = 0);
	bool LoadFragmentSlices(C4GroupSet *pGroupSet, const char *szFile);
	bool LoadVertexSlices(C4GroupSet *pGroupSet, const char *szFile);
	void SetScriptCategories(const std::vector<std::string>& categories);

	// Assemble and link the shader. Should be called again after new slices are added.
	bool Init(const char *szWhat, const char **szUniforms, const char **szAttributes);
	bool Refresh();

	void ClearSlices();
	void Clear();

private:
	void AddSlice(ShaderSliceList& slices, int iPos, const char *szText, const char *szSource, int line, int iFileTime);
	void AddSlices(ShaderSliceList& slices, const char *szWhat, const char *szText, const char *szSource, int iFileTime);
	bool LoadSlices(ShaderSliceList& slices, C4GroupSet *pGroupSet, const char *szFile);
	int ParsePosition(const char *szWhat, const char **ppPos);

	void LoadScriptSlices();
	void LoadScriptSlice(int id);

	StdStrBuf Build(const ShaderSliceList &Slices, bool fDebug = false);

#ifndef USE_CONSOLE
	GLuint Create(GLenum iShaderType, const char *szWhat, const char *szShader);
	void DumpInfoLog(const char *szWhat, GLuint hShader, bool forProgram);
#endif

public:
	static bool IsLogging();
};

#ifndef USE_CONSOLE
class C4ShaderCall
{
	friend class C4ScriptUniform;
public:
	C4ShaderCall(const C4Shader *pShader)
		: fStarted(false), pShader(pShader), iUnits(0)
	{ }
	~C4ShaderCall() { Finish(); }

	GLint GetAttribute(int iAttribute) const
	{
		return pShader->GetAttribute(iAttribute);
	}

private:
	bool fStarted;
	const C4Shader *pShader;
	int iUnits;

public:
	GLint AllocTexUnit(int iUniform);

	// Setting uniforms... Lots of code duplication here, not quite sure whether
	// something could be done about it.
	void SetUniform1i(int iUniform, int iX) const {
		if (pShader->HaveUniform(iUniform))
			glUniform1i(pShader->GetUniform(iUniform), iX);
	}
	void SetUniform2i(int iUniform, int iX, int iY) const {
		if (pShader->HaveUniform(iUniform))
			glUniform2i(pShader->GetUniform(iUniform), iX, iY);
	}
	void SetUniform3i(int iUniform, int iX, int iY, int iZ) const {
		if (pShader->HaveUniform(iUniform))
			glUniform3i(pShader->GetUniform(iUniform), iX, iY, iZ);
	}
	void SetUniform4i(int iUniform, int iX, int iY, int iZ, int iW) const {
		if (pShader->HaveUniform(iUniform))
			glUniform4i(pShader->GetUniform(iUniform), iX, iY, iZ, iW);
	}
	void SetUniform1ui(int iUniform, unsigned int iX) const {
		if (pShader->HaveUniform(iUniform))
			glUniform1ui(pShader->GetUniform(iUniform), iX);
	}
	void Setuniform2ui(int iUniform, unsigned int iX, unsigned int iY) const {
		if (pShader->HaveUniform(iUniform))
			glUniform2ui(pShader->GetUniform(iUniform), iX, iY);
	}
	void Setuniform3ui(int iUniform, unsigned int iX, unsigned int iY, unsigned int iZ) const {
		if (pShader->HaveUniform(iUniform))
			glUniform3ui(pShader->GetUniform(iUniform), iX, iY, iZ);
	}
	void Setuniform4ui(int iUniform, unsigned int iX, unsigned int iY, unsigned int iZ, unsigned int iW) const {
		if (pShader->HaveUniform(iUniform))
			glUniform4ui(pShader->GetUniform(iUniform), iX, iY, iZ, iW);
	}
	void SetUniform1f(int iUniform, float gX) const {
		if (pShader->HaveUniform(iUniform))
			glUniform1f(pShader->GetUniform(iUniform), gX);
	}
	void SetUniform2f(int iUniform, float gX, float gY) const {
		if (pShader->HaveUniform(iUniform))
			glUniform2f(pShader->GetUniform(iUniform), gX, gY);
	}
	void SetUniform3f(int iUniform, float gX, float gY, float gZ) const {
		if (pShader->HaveUniform(iUniform))
			glUniform3f(pShader->GetUniform(iUniform), gX, gY, gZ);
	}
	void SetUniform4f(int iUniform, float gX, float gY, float gZ, float gW) const {
		if (pShader->HaveUniform(iUniform))
			glUniform4f(pShader->GetUniform(iUniform), gX, gY, gZ, gW);
	}
	void SetUniform1iv(int iUniform, int iLength, const int *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform1iv(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform2iv(int iUniform, int iLength, const int *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform2iv(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform3iv(int iUniform, int iLength, const int *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform3iv(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform4iv(int iUniform, int iLength, const int *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform4iv(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform1uiv(int iUniform, int iLength, const unsigned int *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform1uiv(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform2uiv(int iUniform, int iLength, const unsigned int *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform2uiv(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform3uiv(int iUniform, int iLength, const unsigned int *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform3uiv(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform4uiv(int iUniform, int iLength, const unsigned int *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform4uiv(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform1fv(int iUniform, int iLength, const float *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform1fv(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform2fv(int iUniform, int iLength, const float *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform2fv(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform3fv(int iUniform, int iLength, const float *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform3fv(pShader->GetUniform(iUniform), iLength, pVals);
	}
	void SetUniform4fv(int iUniform, int iLength, const float *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniform4fv(pShader->GetUniform(iUniform), iLength, pVals);
	}

	// Matrices are in row-major order
	void SetUniformMatrix2x3fv(int iUniform, int iLength, const float* pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniformMatrix3x2fv(pShader->GetUniform(iUniform), iLength, GL_TRUE, pVals);
	}

	void SetUniformMatrix3x3fv(int iUniform, int iLength, const float *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniformMatrix3fv(pShader->GetUniform(iUniform), iLength, GL_TRUE, pVals);
	}

	void SetUniformMatrix3x4fv(int iUniform, int iLength, const float *pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniformMatrix4x3fv(pShader->GetUniform(iUniform), iLength, GL_TRUE, pVals);
	}

	void SetUniformMatrix4x4fv(int iUniform, int iLength, const float* pVals) const {
		if (pShader->HaveUniform(iUniform))
			glUniformMatrix4fv(pShader->GetUniform(iUniform), iLength, GL_TRUE, pVals);
	}

	void SetUniformMatrix3x3(int iUniform, const StdMeshMatrix& matrix)
	{
		if (pShader->HaveUniform(iUniform))
		{
			const float mat[9] = { matrix(0, 0), matrix(1, 0), matrix(2, 0), matrix(0, 1), matrix(1, 1), matrix(2, 1), matrix(0, 2), matrix(1, 2), matrix(2, 2) };
			glUniformMatrix3fv(pShader->GetUniform(iUniform), 1, GL_FALSE, mat);
		}
	}

	void SetUniformMatrix3x3Transpose(int iUniform, const StdMeshMatrix& matrix)
	{
		if (pShader->HaveUniform(iUniform))
		{
			const float mat[9] = { matrix(0, 0), matrix(0, 1), matrix(0, 2), matrix(1, 0), matrix(1, 1), matrix(1, 2), matrix(2, 0), matrix(2, 1), matrix(2, 2) };
			glUniformMatrix3fv(pShader->GetUniform(iUniform), 1, GL_FALSE, mat);
		}
	}

	void SetUniformMatrix3x4(int iUniform, const StdMeshMatrix& matrix)
	{
		if (pShader->HaveUniform(iUniform))
			glUniformMatrix4x3fv(pShader->GetUniform(iUniform), 1, GL_TRUE, matrix.data());
	}

	void SetUniformMatrix4x4(int iUniform, const StdMeshMatrix& matrix)
	{
		if (pShader->HaveUniform(iUniform))
		{
			const float mat[16] = { matrix(0, 0), matrix(1, 0), matrix(2, 0), 0.0f, matrix(0, 1), matrix(1, 1), matrix(2, 1), 0.0f, matrix(0, 2), matrix(1, 2), matrix(2, 2), 0.0f, matrix(0, 3), matrix(1, 3), matrix(2, 3), 1.0f };
			glUniformMatrix4fv(pShader->GetUniform(iUniform), 1, GL_FALSE, mat);
		}
	}

	void SetUniformMatrix4x4(int iUniform, const StdProjectionMatrix& matrix)
	{
		if (pShader->HaveUniform(iUniform))
			glUniformMatrix4fv(pShader->GetUniform(iUniform), 1, GL_TRUE, matrix.data());
	}

	void Start();
	void Finish();
};
#else // USE_CONSOLE
class C4ShaderCall {
	public:
	C4ShaderCall(const C4Shader *) {};
};
#endif

class C4ScriptShader
{
	friend class C4Shader;
	friend class C4ShaderCall;

public:
	enum ShaderType
	{
		VertexShader, // Note: Reloading is currently only implemented for fragment shaders.
		FragmentShader,
	};
private:
	struct ShaderInstance
	{
		ShaderType type;
		std::string source;
	};

	// Map of shader names -> ids. The indirection is there as each C4Shader
	// may load script shaders from multiple categories.
	std::map<std::string, std::set<int>> categories;
	// Map of ids -> script-loaded shaders.
	std::map<int, ShaderInstance> shaders;
	int NextID = 0;
	uint32_t LastUpdate = 0;

protected: // Interface for C4Shader friend class
	std::set<int> GetShaderIDs(const std::vector<std::string>& cats);

public: // Interface for script
	// Adds a shader, returns its id for removal.
	int Add(const std::string& shaderName, ShaderType type, const std::string& source);
	// Removes a shader, returning true on success.
	bool Remove(int id);
};

extern C4ScriptShader ScriptShader;

class C4ScriptUniform
{
	friend class C4Shader;

	struct Uniform
	{
#ifndef USE_CONSOLE
		GLenum type;
		union
		{
			int intVec[4];
			// TODO: Support for other uniform types.
		};
#endif
	};

	typedef std::map<std::string, Uniform> UniformMap;
	std::stack<UniformMap> uniformStack;

public:
	class Popper
	{
		C4ScriptUniform* p;
		size_t size;
	public:
		Popper(C4ScriptUniform* p) : p(p), size(p->uniformStack.size()) { }
		~Popper() { assert(size == p->uniformStack.size()); p->uniformStack.pop(); }
	};

	// Remove all uniforms.
	void Clear();
	// Walk the proplist `proplist.Uniforms` and add uniforms. Automatically pops when the return value is destroyed.
	std::unique_ptr<Popper> Push(C4PropList* proplist);
	// Apply uniforms to a shader call.
	void Apply(C4ShaderCall& call);

	C4ScriptUniform() { Clear(); }
};

#endif // INC_C4Shader
