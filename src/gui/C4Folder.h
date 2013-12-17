/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013, The OpenClonk Team and contributors
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

#ifndef INC_C4Folder
#define INC_C4Folder

const int C4MaxFolderSort = 4096;

class C4FolderHead
{
public:
	int32_t Index;                      // Folder index in scenario selection dialog
	char Sort[C4MaxFolderSort + 1];     // Folder-defined group sort list (to be used for folder maps)
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
