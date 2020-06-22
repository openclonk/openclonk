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
#include "graphics/C4Shader.h"
#include "game/C4Application.h"
#include "graphics/C4DrawGL.h"

#ifndef USE_CONSOLE
// How often we check whether shader files got updated
const uint32_t C4Shader_RefreshInterval = 1000; // ms
#endif

struct C4ShaderPosName {
	int Position; const char *Name;
};

C4ShaderPosName C4SH_PosNames[] = {
	{ C4Shader_PositionInit,		 "init" },
	{ C4Shader_PositionCoordinate,	 "coordinate" },
	{ C4Shader_PositionTexture,		 "texture" },
	{ C4Shader_PositionMaterial,	 "material" },
	{ C4Shader_PositionNormal,		 "normal" },
	{ C4Shader_PositionLight,		 "light" },
	{ C4Shader_PositionColor,		 "color" },
	{ C4Shader_PositionFinish,		 "finish" },

	{ C4Shader_Vertex_TexCoordPos,	        "texcoord" },
	{ C4Shader_Vertex_NormalPos,            "normal" },
	{ C4Shader_Vertex_ColorPos,             "color" },
	{ C4Shader_Vertex_PositionPos,          "position" }
};

C4Shader::C4Shader()
	: LastRefresh()
{

}

C4Shader::~C4Shader()
{
	Clear();
}

int C4Shader::GetSourceFileId(const char *file) const
{
	auto it = std::find(SourceFiles.begin(), SourceFiles.end(), file);
	if (it == SourceFiles.end()) return -1;
	return std::distance(SourceFiles.begin(), it);
}

void C4Shader::AddDefine(const char* name)
{
	StdStrBuf define = FormatString("#define %s", name);
	AddVertexSlice(-1, define.getData());
	AddFragmentSlice(-1, define.getData());
}

void C4Shader::AddVertexSlice(int iPos, const char *szText)
{
	AddSlice(VertexSlices, iPos, szText, nullptr, 0, 0);
}

void C4Shader::AddFragmentSlice(int iPos, const char *szText)
{
	AddSlice(FragmentSlices, iPos, szText, nullptr, 0, 0);
}

void C4Shader::AddVertexSlices(const char *szWhat, const char *szText, const char *szSource, int iSourceTime)
{
	AddSlices(VertexSlices, szWhat, szText, szSource, iSourceTime);
}

void C4Shader::AddFragmentSlices(const char *szWhat, const char *szText, const char *szSource, int iSourceTime)
{
	AddSlices(FragmentSlices, szWhat, szText, szSource, iSourceTime);
}

bool C4Shader::LoadFragmentSlices(C4GroupSet *pGroups, const char *szFile)
{
	return LoadSlices(FragmentSlices, pGroups, szFile);
}

bool C4Shader::LoadVertexSlices(C4GroupSet *pGroups, const char *szFile)
{
	return LoadSlices(VertexSlices, pGroups, szFile);
}

void C4Shader::SetScriptCategories(const std::vector<std::string>& categories)
{
	assert(!ScriptSlicesLoaded && "Can't change shader categories after initialization");
	Categories = categories;
}

void C4Shader::LoadScriptSlices()
{
	ScriptShaders = ScriptShader.GetShaderIDs(Categories);
	for (auto& id : ScriptShaders)
	{
		LoadScriptSlice(id);
	}
	ScriptSlicesLoaded = true;
}

void C4Shader::LoadScriptSlice(int id)
{
	auto& s = ScriptShader.shaders.at(id);
	switch (s.type)
	{
	case C4ScriptShader::VertexShader:
		AddVertexSlices(Name.getData(), s.source.c_str(), FormatString("[script %d]", id).getData());
		break;
	case C4ScriptShader::FragmentShader:
		AddFragmentSlices(Name.getData(), s.source.c_str(), FormatString("[script %d]", id).getData());
		break;
	}
}

void C4Shader::AddSlice(ShaderSliceList& slices, int iPos, const char *szText, const char *szSource, int line, int iSourceTime)
{
	ShaderSlice Slice;
	Slice.Position = iPos;
	Slice.Text.Copy(szText);
	Slice.Source = szSource;
	Slice.SourceTime = iSourceTime;
	Slice.SourceLine = line;
	slices.push_back(Slice);
}

void C4Shader::AddSlices(ShaderSliceList& slices, const char *szWhat, const char *szText, const char *szSource, int iSourceTime)
{
	if (std::find(SourceFiles.cbegin(), SourceFiles.cend(), szSource) == SourceFiles.cend())
		SourceFiles.emplace_back(szSource);

	const char *pStart = szText, *pPos = szText;
	int iDepth = -1;
	int iPosition = -1;
	bool fGotContent = false; // Anything in the slice apart from comments and white-space?
	
#define SKIP_WHITESPACE do { while(isspace(*pPos)) { ++pPos; } } while (0)

	// Find slices
	while(*pPos) {
		// Comment? Might seem silly, but we don't want to get confused by braces in comments...
		if (*pPos == '/' && *(pPos + 1) == '/') {
			pPos += 2;
			while (*pPos && *pPos != '\n') pPos++;
			continue;
		}
		if (*pPos == '/' && *(pPos + 1) == '*') {
			pPos += 2;
			while (*pPos && (*pPos != '*' || *(pPos + 1) != '/'))
			{
				pPos++;
			}
			if (*pPos) pPos += 2;
			continue;
		}

		// Opening brace?
		if (*pPos == '{') {
			iDepth++; pPos++;
			continue;
		}
		if (*pPos == '}') {
			// End of slice?
			if (iPosition != -1 && !iDepth) {

				// Have a new slice!
				if (fGotContent)
				{
					StdStrBuf Str; Str.Copy(pStart, pPos - pStart);
					AddSlice(slices, iPosition, Str.getData(), szSource, SGetLine(szText, pStart), iSourceTime);
				}

				iPosition = -1;
				pStart = pPos+1;
				fGotContent = false;
			}
			if (iDepth >= 0)
				iDepth--;
			pPos++;
			continue;
		}

		// New slice? We need a newline followed by "slice". Don't do
		// the depth check, so that we also recognize slices inside
		// an ifdefed-out "void main() {" block.
		if (*pPos == '\n') {
			if (SEqual2(pPos+1, "slice") && !isalnum(*(pPos+6))) {
				const char *pSliceEnd = pPos; pPos += 6;
				SKIP_WHITESPACE;
				if(*pPos != '(') { pPos++; continue; }
				pPos++;

				// Now let's parse the position
				iPosition = ParsePosition(szWhat, &pPos);
				if (iPosition != -1) {
					// Make sure a closing parenthesis
					SKIP_WHITESPACE;
					if(*pPos != ')') { pPos++; continue; }
					pPos++;

					// Make sure an opening brace follows
					SKIP_WHITESPACE;
					if (*pPos == '{') {

						// Add code before "slice" as new slice
						if (fGotContent)
						{
							StdStrBuf Str; Str.Copy(pStart, pSliceEnd - pStart);
							AddSlice(slices, -1, Str.getData(), szSource, SGetLine(szText, pSliceEnd), iSourceTime);
						}

						iDepth = 0;
						pStart = pPos+1;
						fGotContent = false;
					} else {
						ShaderLogF("  gl: Missing opening brace in %s!", szWhat);
					}
					pPos++;
					continue;
				}
			}
		}

		// Otherwise: Continue
		if (!isspace(*pPos)) fGotContent = true;
		pPos++;
	}

	// Add final slice
	if (fGotContent)
	{
		StdStrBuf Str; Str.Copy(pStart, pPos - pStart);
		AddSlice(slices, iPosition, Str.getData(), szSource, SGetLine(szText, pStart), iSourceTime);
	}
#undef SKIP_WHITESPACE
}

int C4Shader::ParsePosition(const char *szWhat, const char **ppPos)
{
	const char *pPos = *ppPos;
	while (isspace(*pPos)) pPos++;

	// Expect a name
	const char *pStart = pPos;
	while (isalnum(*pPos)) pPos++;
	StdStrBuf Name; Name.Copy(pStart, pPos - pStart);

	// Lookup name
	int iPosition = -1;
	for (auto & PosName : C4SH_PosNames) {
		if (SEqual(Name.getData(), PosName.Name)) {
			iPosition = PosName.Position;
			break;
		}
	}
	if (iPosition == -1) {
		ShaderLogF("  gl: Unknown slice position in %s: %s", szWhat, Name.getData());
		return -1;
	}

	// Add modifier
	while (isspace(*pPos)) pPos++;
	if (*pPos == '+') {
		int iMod, iModLen;
		if (!sscanf(pPos+1, "%d%n", &iMod, &iModLen)) {
			ShaderLogF("  gl: Invalid slice modifier in %s", szWhat);
			return -1;
		}
		iPosition += iMod;
		pPos += 1+iModLen;
	}
	if (*pPos == '-') {
		int iMod, iModLen;
		if (!sscanf(pPos+1, "%d%n", &iMod, &iModLen)) {
			ShaderLogF("  gl: Invalid slice modifier in %s", szWhat);
			return -1;
		}
		iPosition -= iMod;
		pPos += 1+iModLen;
	}

	// Everything okay!
	*ppPos = pPos;
	return iPosition;
}

bool C4Shader::LoadSlices(ShaderSliceList& slices, C4GroupSet *pGroups, const char *szFile)
{
	// Search for our shaders
	C4Group *pGroup = pGroups->FindEntry(szFile);
	if(!pGroup) return false;
	// Load it, save the path for later reloading
	StdStrBuf Shader;
	if(!pGroup->LoadEntryString(szFile, &Shader))
		return false;
	// If it physically exists, save back creation time so we
	// can automatically reload it if it changes
	StdStrBuf Source = FormatString("%s" DirSep "%s", pGroup->GetFullName().getData(), szFile);
	int iSourceTime = 0;
	if(FileExists(Source.getData()))
		iSourceTime = FileTime(Source.getData());
	// Load
	StdStrBuf What = FormatString("file %s", Config.AtRelativePath(Source.getData()));
	AddSlices(slices, What.getData(), Shader.getData(), Source.getData(), iSourceTime);
	return true;
}

void C4Shader::ClearSlices()
{
	VertexSlices.clear();
	FragmentSlices.clear();
	iTexCoords = 0;
	// Script slices
	ScriptSlicesLoaded = false;
	Categories.clear();
	ScriptShaders.clear();
}

void C4Shader::Clear()
{
#ifndef USE_CONSOLE
	if (!hProg) return;
	// Need to be detached, then deleted
	glDeleteProgram(hProg);
	hProg = 0;
	// Clear uniform data
	Uniforms.clear();
	Attributes.clear();
#endif
}

bool C4Shader::Init(const char *szWhat, const char **szUniforms, const char **szAttributes)
{
	Name.Copy(szWhat);
	LastRefresh = C4TimeMilliseconds::Now();

	if (!ScriptSlicesLoaded)
	{
		Categories.emplace_back(szWhat);
		LoadScriptSlices();
	}

	StdStrBuf VertexShader = Build(VertexSlices, true),
		FragmentShader = Build(FragmentSlices, true);

	// Dump
	if (C4Shader::IsLogging())
	{
		ShaderLogF("******** Vertex shader for %s:", szWhat);
		ShaderLog(VertexShader.getData());
		ShaderLogF("******** Fragment shader for %s:", szWhat);
		ShaderLog(FragmentShader.getData());
	}

#ifndef USE_CONSOLE
	// Attempt to create shaders
	const GLuint hVert = Create(GL_VERTEX_SHADER,
	                            FormatString("%s vertex shader", szWhat).getData(),
	                            VertexShader.getData());
	const GLuint hFrag = Create(GL_FRAGMENT_SHADER,
	                            FormatString("%s fragment shader", szWhat).getData(),
	                            FragmentShader.getData());

	if(!hFrag || !hVert)
	{
		if (hFrag) glDeleteShader(hFrag);
		if (hVert) glDeleteShader(hVert);
		return false;
	}

	// Link program
	const GLuint hNewProg = glCreateProgram();
	pGL->ObjectLabel(GL_PROGRAM, hNewProg, -1, szWhat);
	glAttachShader(hNewProg, hVert);
	glAttachShader(hNewProg, hFrag);
	glLinkProgram(hNewProg);
	// Delete vertex and fragment shader after we linked the program
	glDeleteShader(hFrag);
	glDeleteShader(hVert);

	// Link successful?
	DumpInfoLog(FormatString("%s shader program", szWhat).getData(), hNewProg, true);
	GLint status;
	glGetProgramiv(hNewProg, GL_LINK_STATUS, &status);
	if(status != GL_TRUE) {
		glDeleteProgram(hNewProg);
		ShaderLogF("  gl: Failed to link %s shader!", szWhat);
		return false;
	}
	ShaderLogF("  gl: %s shader linked successfully", szWhat);

	// Everything successful, delete old shader
	if (hProg != 0) glDeleteProgram(hProg);
	hProg = hNewProg;

	// Allocate uniform and attribute arrays
	int iUniformCount = 0;
	if (szUniforms != nullptr)
		while (szUniforms[iUniformCount])
			iUniformCount++;
	Uniforms.resize(iUniformCount);

	int iAttributeCount = 0;
	if (szAttributes != nullptr)
		while (szAttributes[iAttributeCount])
			iAttributeCount++;
	Attributes.resize(iAttributeCount);

	// Get uniform and attribute locations. Note this is expected to fail for a few of them
	// because the respective uniforms got optimized out!
	for (int i = 0; i < iUniformCount; i++) {
		Uniforms[i].address = glGetUniformLocation(hProg, szUniforms[i]);
		Uniforms[i].name = szUniforms[i];
		ShaderLogF("Uniform %s = %d", szUniforms[i], Uniforms[i].address);
	}

	for (int i = 0; i < iAttributeCount; i++) {
		Attributes[i].address = glGetAttribLocation(hProg, szAttributes[i]);
		Attributes[i].name = szAttributes[i];
		ShaderLogF("Attribute %s = %d", szAttributes[i], Attributes[i].address);
	}

#endif

	return true;
}


bool C4Shader::Refresh()
{
	// Update last refresh. Keep a local copy around though to identify added script shaders.
	LastRefresh = C4TimeMilliseconds::Now();

	auto next = ScriptShader.GetShaderIDs(Categories);
	std::set<int> toAdd, toRemove;
	std::set_difference(ScriptShaders.begin(), ScriptShaders.end(), next.begin(), next.end(), std::inserter(toRemove, toRemove.end()));
	std::set_difference(next.begin(), next.end(), ScriptShaders.begin(), ScriptShaders.end(), std::inserter(toAdd, toAdd.end()));
	ScriptShaders = next;

	auto removeSlices = [&](ShaderSliceList::iterator& pSlice)
	{
		StdCopyStrBuf Source = pSlice->Source;

		// Okay, remove all slices that came from this file
		ShaderSliceList::iterator pNext;
		for (; pSlice != FragmentSlices.end(); pSlice = pNext)
		{
			pNext = pSlice; pNext++;
			if (SEqual(pSlice->Source.getData(), Source.getData()))
				FragmentSlices.erase(pSlice);
		}
	};

	// Find slices where the source file has updated.
	std::vector<StdCopyStrBuf> sourcesToUpdate;
	for (ShaderSliceList::iterator pSlice = FragmentSlices.begin(); pSlice != FragmentSlices.end(); pSlice++)
		if (pSlice->Source.getLength())
		{
			if (pSlice->Source.BeginsWith("[script "))
			{
				// TODO: Maybe save id instead of parsing the string here.
				int sid = -1;
				sscanf(pSlice->Source.getData(), "[script %d", &sid);
				if (toRemove.find(sid) != toRemove.end())
					removeSlices(pSlice);
				// Note: script slices don't change, so we don't have to handle updates like for files.
			}
			else if (FileExists(pSlice->Source.getData()) &&
			         FileTime(pSlice->Source.getData()) > pSlice->SourceTime)
			{
				sourcesToUpdate.push_back(pSlice->Source);
				removeSlices(pSlice);
			}
		}

	// Anything to do?
	if (toAdd.size() == 0 && toRemove.size() == 0 && sourcesToUpdate.size() == 0)
		return true;

	// Process file reloading.
	for (auto& Source : sourcesToUpdate)
	{
		char szParentPath[_MAX_PATH+1]; C4Group Group;
		StdStrBuf Shader;
		GetParentPath(Source.getData(),szParentPath);
		if(!Group.Open(szParentPath) ||
		   !Group.LoadEntryString(GetFilename(Source.getData()),&Shader) ||
		   !Group.Close())
		{
			ShaderLogF("  gl: Failed to refresh %s shader from %s!", Name.getData(), Source.getData());
			return false;
		}

		// Load slices
		int iSourceTime = FileTime(Source.getData());
		StdStrBuf WhatSrc = FormatString("file %s", Config.AtRelativePath(Source.getData()));
		AddFragmentSlices(WhatSrc.getData(), Shader.getData(), Source.getData(), iSourceTime);
	}
	
	// Process new script slices.
	for (int id : toAdd)
	{
		LoadScriptSlice(id);
	}

#ifndef USE_CONSOLE
	std::vector<const char*> UniformNames(Uniforms.size() + 1);
	for (std::size_t i = 0; i < Uniforms.size(); ++i)
		UniformNames[i] = Uniforms[i].name;
	UniformNames[Uniforms.size()] = nullptr;

	std::vector<const char*> AttributeNames(Attributes.size() + 1);
	for (std::size_t i = 0; i < Attributes.size(); ++i)
		AttributeNames[i] = Attributes[i].name;
	AttributeNames[Attributes.size()] = nullptr;
#endif

	// Reinitialise
	StdCopyStrBuf What(Name);
	if (!Init(What.getData(), 
#ifndef USE_CONSOLE
		&UniformNames[0],
		&AttributeNames[0]
#else
		nullptr,
		nullptr
#endif
		))
		return false;

	return true;
}

StdStrBuf C4Shader::Build(const ShaderSliceList &Slices, bool fDebug)
{
	// At the start of the shader set the #version and number of
	// available uniforms
	StdStrBuf Buf;
#ifndef USE_CONSOLE
	GLint iMaxFrags = 0, iMaxVerts = 0;
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &iMaxFrags);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &iMaxVerts);
#else
	int iMaxFrags = INT_MAX, iMaxVerts = INT_MAX;
#endif
	Buf.Format("#version %d\n"
	           "#define MAX_FRAGMENT_UNIFORM_COMPONENTS %d\n"
	           "#define MAX_VERTEX_UNIFORM_COMPONENTS %d\n",
	           C4Shader_Version, iMaxFrags, iMaxVerts);

	// Put slices
	int iPos = -1, iNextPos = -1;
	do
	{
		iPos = iNextPos; iNextPos = C4Shader_LastPosition+1;
		// Add all slices at the current level
		if (fDebug && iPos > 0)
			Buf.AppendFormat("\t// Position %d:\n", iPos);
		for (const auto & Slice : Slices)
		{
			if (Slice.Position < iPos) continue;
			if (Slice.Position > iPos)
			{
				iNextPos = std::min(iNextPos, Slice.Position);
				continue;
			}
			// Same position - add slice!
			if (fDebug)
			{
				if (Slice.Source.getLength())
				{
					// GLSL below 3.30 consider the line after a #line N directive to be N + 1; 3.30 and higher consider it N
					Buf.AppendFormat("\t// Slice from %s:\n#line %d %d\n", Slice.Source.getData(), Slice.SourceLine - (C4Shader_Version < 330), GetSourceFileId(Slice.Source.getData()) + 1);
				}
				else
					Buf.Append("\t// Built-in slice:\n#line 1 0\n");
				}
			Buf.Append(Slice.Text);
			if (Buf[Buf.getLength()-1] != '\n')
				Buf.AppendChar('\n');
		}
		// Add seperator - only priority (-1) is top-level
		if (iPos == -1) {
			Buf.Append("void main() {\n");
		}
	}
	while (iNextPos <= C4Shader_LastPosition);

	// Terminate
	Buf.Append("}\n");

	Buf.Append("// File number to name mapping:\n//\t  0: <built-in shader code>\n");
	for (int i = 0; i < SourceFiles.size(); ++i)
		Buf.AppendFormat("//\t%3d: %s\n", i + 1, SourceFiles[i].c_str());
	return Buf;
}

#ifndef USE_CONSOLE
GLuint C4Shader::Create(GLenum iShaderType, const char *szWhat, const char *szShader)
{
	// Create shader
	GLuint hShader = glCreateShader(iShaderType);
	pGL->ObjectLabel(GL_SHADER, hShader, -1, szWhat);

	// Compile
	glShaderSource(hShader, 1, &szShader, nullptr);
	glCompileShader(hShader);

	// Dump any information to log
	DumpInfoLog(szWhat, hShader, false);

	// Success?
	int status;
	glGetShaderiv(hShader, GL_COMPILE_STATUS, &status);
	if (status == GL_TRUE)
		return hShader;

	// Did not work :/
	glDeleteShader(hShader);
	return 0;
}

void C4Shader::DumpInfoLog(const char *szWhat, GLuint hShader, bool forProgram)
{
	// Get length of info line
	GLint iLength = 0;
	if (forProgram)
		glGetProgramiv(hShader, GL_INFO_LOG_LENGTH, &iLength);
	else
		glGetShaderiv(hShader, GL_INFO_LOG_LENGTH, &iLength);
	if(iLength <= 1) return;

	// Allocate buffer, get data
	std::vector<char> buf(iLength + 1);
	int iActualLength = 0;
	if (forProgram)
		glGetProgramInfoLog(hShader, iLength, &iActualLength, &buf[0]);
	else
		glGetShaderInfoLog(hShader, iLength, &iActualLength, &buf[0]);
	if(iActualLength > iLength || iActualLength <= 0) return;

	// Terminate, log
	buf[iActualLength] = '\0';
	ShaderLogF("  gl: Compiling %s:", szWhat);
	ShaderLog(&buf[0]);
}
#endif

bool C4Shader::IsLogging() { return Config.Graphics.DebugOpenGL != 0 || !!Application.isEditor; }

#ifndef USE_CONSOLE
GLint C4ShaderCall::AllocTexUnit(int iUniform)
{

	// Want to bind uniform automatically? If not, the caller will take
	// care of it.
	if (iUniform >= 0) {

		// If uniform isn't used, we should skip this. Also check texunit range.
		if (!pShader->HaveUniform(iUniform)) return 0;
		assert(iUnits < C4ShaderCall_MaxUnits);
		if (iUnits >= C4ShaderCall_MaxUnits) return 0;

		// Set the uniform
		SetUniform1i(iUniform, iUnits);
	}

	// Activate the texture
	GLint hTex = GL_TEXTURE0 + iUnits;
	glActiveTexture(hTex);
	iUnits++;
	return hTex;
}

void C4ShaderCall::Start()
{
	assert(!fStarted);
	assert(pShader->hProg != 0); // Shader must be initialized

	// Possibly refresh shader
	if (ScriptShader.LastUpdate > pShader->LastRefresh || C4TimeMilliseconds::Now() > pShader->LastRefresh + C4Shader_RefreshInterval)
		const_cast<C4Shader *>(pShader)->Refresh();

	// Activate shader
	glUseProgram(pShader->hProg);
	fStarted = true;
}

void C4ShaderCall::Finish()
{
	// Remove shader
	if (fStarted) {
		glUseProgram(0);
	}

	iUnits = 0;
	fStarted = false;
}

#endif

// global instance
C4ScriptShader ScriptShader;

std::set<int> C4ScriptShader::GetShaderIDs(const std::vector<std::string>& cats)
{
	std::set<int> result;
	for (auto& cat : cats)
		for (auto& id : categories[cat])
			result.emplace(id);
	return result;
}

int C4ScriptShader::Add(const std::string& shaderName, ShaderType type, const std::string& source)
{
	int id = NextID++;
	LastUpdate = C4TimeMilliseconds::Now().AsInt();
	// Hack: Always prepend a newline as the slice parser doesn't recognize
	// slices that don't begin with a newline.
	auto nsource = "\n" + source;
	shaders.emplace(std::make_pair(id, ShaderInstance{type, nsource}));
	categories[shaderName].emplace(id);
	return id;
}

bool C4ScriptShader::Remove(int id)
{
	// We have to perform a rather inefficient full search. We'll have to see
	// whether this turns out to be a performance issue.
	if (shaders.erase(id))
	{
		for (auto& kv : categories)
			if (kv.second.erase(id))
				break; // each id can appear in one category only
		LastUpdate = C4TimeMilliseconds::Now().AsInt();
		return true;
	}
	return false;
}

std::unique_ptr<C4ScriptUniform::Popper> C4ScriptUniform::Push(C4PropList* proplist)
{
#ifdef USE_CONSOLE
	return std::unique_ptr<C4ScriptUniform::Popper>();
#else
	C4Value ulist;
	if (!proplist->GetProperty(P_Uniforms, &ulist) || ulist.GetType() != C4V_PropList)
		return std::unique_ptr<C4ScriptUniform::Popper>();

	uniformStack.emplace();
	auto& uniforms = uniformStack.top();
	Uniform u;
	for (const C4Property* prop : *ulist.getPropList())
	{
		if (!prop->Key) continue;
		switch (prop->Value.GetType())
		{
		case C4V_Int:
			u.type = GL_INT;
			u.intVec[0] = prop->Value._getInt();
			break;
		case C4V_Array:
		{
			auto array = prop->Value._getArray();
			switch (array->GetSize())
			{
			case 1: u.type = GL_INT; break;
			case 2: u.type = GL_INT_VEC2; break;
			case 3: u.type = GL_INT_VEC3; break;
			case 4: u.type = GL_INT_VEC4; break;
			default: continue;
			}
			for (int32_t i = 0; i < array->GetSize(); i++)
			{
				auto& item = array->_GetItem(i);
				switch (item.GetType())
				{
				case C4V_Int:
					u.intVec[i] = item._getInt();
					break;
				default:
					goto skip;
				}
			}
			break;
		}
		default:
			continue;
		}
		// Uniform is now filled properly. Note that array contents are undefined for higher slots
		// when "type" only requires a smaller array.
		uniforms.insert({prop->Key->GetCStr(), u});
skip:;
	}
	// Debug
	/*
	for (auto& p : uniforms)
	{
		LogF("Uniform %s (type %d) = %d %d %d %d", p.first.c_str(), p.second.type, p.second.intVec[0], p.second.intVec[1], p.second.intVec[2], p.second.intVec[3]);
	}
	*/
	return std::make_unique<C4ScriptUniform::Popper>(this);
#endif
}

void C4ScriptUniform::Clear()
{
	uniformStack = std::stack<UniformMap>();
	uniformStack.emplace();
}

void C4ScriptUniform::Apply(C4ShaderCall& call)
{
#ifndef USE_CONSOLE
	for (auto& p : uniformStack.top())
	{
		// The existing SetUniform* methods only work for pre-defined indexed uniforms. The script
		// uniforms are unknown at shader compile time, so we have to use OpenGL functions directly
		// here.
		GLint loc = glGetUniformLocation(call.pShader->hProg, p.first.c_str());
		// Is this uniform defined in the shader?
		if (loc == -1) continue;
		auto& intVec = p.second.intVec;
		switch (p.second.type)
		{
		case GL_INT:      glUniform1iv(loc, 1, intVec); break;
		case GL_INT_VEC2: glUniform2iv(loc, 1, intVec); break;
		case GL_INT_VEC3: glUniform3iv(loc, 1, intVec); break;
		case GL_INT_VEC4: glUniform4iv(loc, 1, intVec); break;
		default:
			assert(false && "unsupported uniform type");
		}
	}
#endif
}
