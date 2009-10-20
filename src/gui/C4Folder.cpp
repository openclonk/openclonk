/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007  Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
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

