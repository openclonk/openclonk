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

#ifndef INC_C4Folder
#define INC_C4Folder

class StdCompiler;
class C4Group;

const int C4MaxFolderSort = 4096;

class C4FolderHead
  {
  public:
		int32_t Index;											// Folder index in scenario selection dialog
    char Sort[C4MaxFolderSort + 1];			// Folder-defined group sort list (to be used for folder maps)
  public:
    void Default();
    void CompileFunc(StdCompiler *pComp);
  };

class C4Folder
  {
  public:
		C4Folder();
	public:
		C4FolderHead Head;
  public:
    void Default();
	  //void Clear();
    bool Load(C4Group &hGroup);
		//bool Save(C4Group &hGroup);
    void CompileFunc(StdCompiler *pComp);
	protected:
		bool Compile(const char *szSource);
		//bool Decompile(char **ppOutput, int32_t *ipSize);
  };

#endif // INC_C4Folder
