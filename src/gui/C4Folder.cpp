/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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

/* Core component of a folder */

#include <C4Include.h>
#include <C4Folder.h>

#include <C4Random.h>
#include <C4Group.h>
#include <C4Components.h>
#include <C4Game.h>


//================= C4FolderHead ====================

void C4FolderHead::Default()
{
	Index = 0;
	Sort[0] = 0;
}

void C4FolderHead::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Index,                     "Index",                0));
	pComp->Value(mkNamingAdapt(mkStringAdaptMA(Sort),     "Sort",                 ""));
}

//=================== C4Folder ======================

C4Folder::C4Folder()
{
	Default();
}

void C4Folder::Default()
{
	Head.Default();
}

bool C4Folder::Load(C4Group &hGroup)
{
	char *pSource;
	// Load
	if (!hGroup.LoadEntry(C4CFN_FolderCore, &pSource, NULL, 1)) return false;
	// Compile
	if (!Compile(pSource)) { delete [] pSource; return false; }
	delete [] pSource;
	// Success
	return true;
}

/*bool C4Folder::Save(C4Group &hGroup)
  {
  char *Buffer; int32_t BufferSize;
  if (!Decompile(&Buffer,&BufferSize))
    return false;
  if (!hGroup.Add(C4Folder, Buffer, BufferSize, false, true))
    { StdBuf Buf; Buf.Take(Buffer, BufferSize); return false; }
  return true;
  }*/

void C4Folder::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Head, "Head"));
}

bool C4Folder::Compile(const char *szSource)
{
	Default();
	return CompileFromBuf_LogWarn<StdCompilerINIRead>(*this, StdStrBuf(szSource), C4CFN_FolderCore);
}

