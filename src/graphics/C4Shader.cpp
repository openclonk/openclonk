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

#include "C4Include.h"
#include "C4Shader.h"
#include "C4Application.h"
#include "graphics/C4DrawGL.h"

// How often we check whether shader files got updated
const uint32_t C4Shader_RefreshInterval = 1000; // ms

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
	: iTexCoords(0)
	, LastRefresh()
#ifndef USE_CONSOLE
	, hProg(0)
#endif
{

}

C4Shader::~C4Shader()
{
	Clear();
}

void C4Shader::AddDefine(const char* name)
{
	StdStrBuf define = FormatString("#define %s", name);
	AddVertexSlice(-1, define.getData());
	AddFragmentSlice(-1, define.getData());
}

void C4Shader::AddVertexSlice(int iPos, const char *szText)
{
	AddSlice(VertexSlices, iPos, szText, NULL, 0);
}

void C4Shader::AddFragmentSlice(int iPos, const char *szText, const char *szSource, int iSourceTime)
{
	AddSlice(FragmentSlices, iPos, szText, szSource, iSourceTime);
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

void C4Shader::AddSlice(ShaderSliceList& slices, int iPos, const char *szText, const char *szSource, int iSourceTime)
{
	ShaderSlice Slice;
	Slice.Position = iPos;
	Slice.Text.Copy(szText);
	Slice.Source = szSource;
	Slice.SourceTime = iSourceTime;
	slices.push_back(Slice);
}

void C4Shader::AddSlices(ShaderSliceList& slices, const char *szWhat, const char *szText, const char *szSource, int iSourceTime)
{
	const char *pStart = szText, *pPos = szText;
	int iDepth = -1;
	int iPosition = -1;
	bool fGotContent = false; // Anything in the slice apart from comments and white-space?

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
			while (*pPos && (*pPos != '*' || *(pPos+1) != '/')) pPos++;
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
					AddSlice(slices, iPosition, Str.getData(), szSource, iSourceTime);
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
				while(isspace(*pPos)) pPos++;
				if(*pPos != '(') { pPos++; continue; }
				pPos++;

				// Now let's parse the position
				iPosition = ParsePosition(szWhat, &pPos);
				if (iPosition != -1) {
					// Make sure a closing parenthesis
					while(isspace(*pPos)) pPos++;
					if(*pPos != ')') { pPos++; continue; }
					pPos++;

					// Make sure an opening brace follows
					while(isspace(*pPos)) pPos++;
					if (*pPos == '{') {

						// Add code before "slice" as new slice
						if (fGotContent)
						{
							StdStrBuf Str; Str.Copy(pStart, pSliceEnd - pStart);
							AddSlice(slices, -1, Str.getData(), szSource, iSourceTime);
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
		AddSlice(slices, iPosition, Str.getData(), szSource, iSourceTime);
	}

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
	for (unsigned int i = 0; i < sizeof(C4SH_PosNames) / sizeof(*C4SH_PosNames); i++) {
		if (SEqual(Name.getData(), C4SH_PosNames[i].Name)) {
			iPosition = C4SH_PosNames[i].Position;
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
}

void C4Shader::Clear()
{
#ifndef USE_CONSOLE
	if (!hProg) return;
	// Need to be detached, then deleted
	glDeleteObjectARB(hProg);
	hProg = 0;
	// Clear uniform data
	Uniforms.clear();
	Attributes.clear();
#endif
}

bool C4Shader::Init(const char *szWhat, const char **szUniforms, const char **szAttributes)
{
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
	const GLint hVert = Create(GL_VERTEX_SHADER_ARB,
	               FormatString("%s vertex shader", szWhat).getData(),
	               VertexShader.getData());
	const GLint hFrag = Create(GL_FRAGMENT_SHADER_ARB,
	               FormatString("%s fragment shader", szWhat).getData(),
	               FragmentShader.getData());

	if(!hFrag || !hVert)
	{
		if (hVert) glDeleteObjectARB(hVert);
		return false;
	}

	// Link program
	const GLint hNewProg = glCreateProgramObjectARB();
#ifdef GL_KHR_debug
	if (glObjectLabel)
		glObjectLabel(GL_PROGRAM, hNewProg, -1, szWhat);
#endif
	glAttachObjectARB(hNewProg, hVert);
	glAttachObjectARB(hNewProg, hFrag);
	glLinkProgramARB(hNewProg);
	// Delete vertex and fragment shader after we linked the program
	glDeleteObjectARB(hFrag);
	glDeleteObjectARB(hVert);

	// Link successful?
	DumpInfoLog(FormatString("%s shader program", szWhat).getData(), hNewProg);
	if(GetObjectStatus(hNewProg, GL_OBJECT_LINK_STATUS_ARB) != 1) {
		glDeleteObjectARB(hNewProg);
		ShaderLogF("  gl: Failed to link %s shader!", szWhat);
		return false;
	}
	ShaderLogF("  gl: %s shader linked successfully", szWhat);

	// Everything successful, delete old shader
	if (hProg != 0) glDeleteObjectARB(hProg);
	hProg = hNewProg;

	// Allocate uniform and attribute arrays
	int iUniformCount = 0;
	if (szUniforms != NULL)
		while (szUniforms[iUniformCount])
			iUniformCount++;
	Uniforms.resize(iUniformCount);

	int iAttributeCount = 0;
	if (szAttributes != NULL)
		while (szAttributes[iAttributeCount])
			iAttributeCount++;
	Attributes.resize(iAttributeCount);

	// Get uniform and attribute locations. Note this is expected to fail for a few of them
	// because the respective uniforms got optimized out!
	for (int i = 0; i < iUniformCount; i++) {
		Uniforms[i].address = glGetUniformLocationARB(hProg, szUniforms[i]);
		Uniforms[i].name = szUniforms[i];
		ShaderLogF("Uniform %s = %d", szUniforms[i], Uniforms[i].address);
	}

	for (int i = 0; i < iAttributeCount; i++) {
		Attributes[i].address = glGetAttribLocationARB(hProg, szAttributes[i]);
		Attributes[i].name = szAttributes[i];
		ShaderLogF("Attribute %s = %d", szAttributes[i], Attributes[i].address);
	}

#endif

	Name.Copy(szWhat);
	LastRefresh = C4TimeMilliseconds::Now();
	return true;
}


bool C4Shader::Refresh()
{
	// Update last refresh. Align across engine for reasons.
	LastRefresh = C4TimeMilliseconds::Now();
	LastRefresh -= LastRefresh.AsInt() % C4Shader_RefreshInterval;
	// Find a slice where the source file has updated
	ShaderSliceList::iterator pSlice;
	for (pSlice = FragmentSlices.begin(); pSlice != FragmentSlices.end(); pSlice++)
		if (pSlice->Source.getLength() &&
			FileExists(pSlice->Source.getData()) &&
			FileTime(pSlice->Source.getData()) > pSlice->SourceTime)
			break;
	if (pSlice == FragmentSlices.end()) return true;
	StdCopyStrBuf Source = pSlice->Source;

	// Okay, remove all slices that came from this file
	ShaderSliceList::iterator pNext;
	for (; pSlice != FragmentSlices.end(); pSlice = pNext)
	{
		pNext = pSlice; pNext++;
		if (SEqual(pSlice->Source.getData(), Source.getData()))
			FragmentSlices.erase(pSlice);
	}

	// Load new shader
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

#ifndef USE_CONSOLE
	std::vector<const char*> UniformNames(Uniforms.size() + 1);
	for (std::size_t i = 0; i < Uniforms.size(); ++i)
		UniformNames[i] = Uniforms[i].name;
	UniformNames[Uniforms.size()] = NULL;

	std::vector<const char*> AttributeNames(Attributes.size() + 1);
	for (std::size_t i = 0; i < Attributes.size(); ++i)
		AttributeNames[i] = Attributes[i].name;
	AttributeNames[Attributes.size()] = NULL;
#endif

	// Reinitialise
	StdCopyStrBuf What(Name);
	if (!Init(What.getData(), 
#ifndef USE_CONSOLE
		&UniformNames[0],
		&AttributeNames[0]
#else
		0,
		0
#endif
		))
		return false;

	// Retry in case there have been more changes
	return Refresh();
}

StdStrBuf C4Shader::Build(const ShaderSliceList &Slices, bool fDebug)
{

	// At the start of the shader set the #version and number of
	// available uniforms
	StdStrBuf Buf;
#ifndef USE_CONSOLE
	GLint iMaxFrags = 0, iMaxVerts = 0;
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB, &iMaxFrags);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, &iMaxVerts);
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
		for (ShaderSliceList::const_iterator pSlice = Slices.begin(); pSlice != Slices.end(); pSlice++)
		{
			if (pSlice->Position < iPos) continue;
			if (pSlice->Position > iPos)
			{
				iNextPos = std::min(iNextPos, pSlice->Position);
				continue;
			}
			// Same position - add slice!
			if (fDebug)
			{
				if (pSlice->Source.getLength())
					Buf.AppendFormat("\t// Slice from %s:\n", pSlice->Source.getData());
				else
					Buf.Append("\t// Built-in slice:\n");
				}
			Buf.Append(pSlice->Text);
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
	return Buf;
}

#ifndef USE_CONSOLE
GLhandleARB C4Shader::Create(GLenum iShaderType, const char *szWhat, const char *szShader)
{
	// Create shader
	GLhandleARB hShader = glCreateShaderObjectARB(iShaderType);
#ifdef GL_KHR_debug
	if (glObjectLabel)
		glObjectLabel(GL_SHADER, hShader, -1, szWhat);
#endif

	// Compile
	glShaderSourceARB(hShader, 1, &szShader, 0);
	glCompileShaderARB(hShader);

	// Dump any information to log
	DumpInfoLog(szWhat, hShader);

	// Success?
	if(GetObjectStatus(hShader, GL_OBJECT_COMPILE_STATUS_ARB) == 1)
		return hShader;

	// Did not work :/
	glDeleteObjectARB(hShader);
	return 0;
}

void C4Shader::DumpInfoLog(const char *szWhat, GLhandleARB hShader)
{
	// Get length of info line
	int iLength = 0;
	glGetObjectParameterivARB(hShader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &iLength);
	if(iLength <= 1) return;

	// Allocate buffer, get data
	char *pBuf = new char [iLength + 1];
	int iActualLength = 0;
	glGetInfoLogARB(hShader, iLength, &iActualLength, pBuf);
	if(iActualLength > iLength || iActualLength <= 0) return;

	// Terminate, log
	pBuf[iActualLength] = '\0';
	ShaderLogF("  gl: Compiling %s:", szWhat);
	ShaderLog(pBuf);
	delete[] pBuf;
}

int C4Shader::GetObjectStatus(GLhandleARB hObj, GLenum type)
{
	int iStatus = 0;
	glGetObjectParameterivARB(hObj, type, &iStatus);
	return iStatus;
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
	if (C4TimeMilliseconds::Now() > pShader->LastRefresh + C4Shader_RefreshInterval)
		const_cast<C4Shader *>(pShader)->Refresh();

	// Activate shader
	glUseProgramObjectARB(pShader->hProg);
	fStarted = true;
}

void C4ShaderCall::Finish()
{
	// Remove shader
	if (fStarted) {
		glUseProgramObjectARB(0);
	}

	iUnits = 0;
	fStarted = false;
}

#endif
