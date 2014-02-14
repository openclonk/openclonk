/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Some wrappers for easier access to the Windows registry */

#ifndef INC_STDREGISTRY
#define INC_STDREGISTRY

#ifdef _WIN32
#include "StdCompiler.h"
#include <C4windowswrapper.h>

StdCopyStrBuf GetRegistryString(const char *szSubKey, const char *szValueName);
bool SetRegistryString(const char *szSubKey, const char *szValueName, const char *szValue);

bool SetRegShell(const wchar_t *szClassName,
                 const wchar_t *szShellName,
                 const wchar_t *szShellCaption,
                 const wchar_t *szCommand,
                 bool fMakeDefault = false);

bool RemoveRegShell(const char *szClassName,
                    const char *szShellName);

bool StoreWindowPosition(HWND hwnd,
                         const char *szWindowName,
                         const char *szSubKey,
                         bool fStoreSize = true);

bool RestoreWindowPosition(HWND hwnd,
                           const char *szWindowName,
                           const char *szSubKey,
                           bool fHidden = false);

// config writer
class StdCompilerConfigWrite : public StdCompiler
{
public:

	// Construct with root key
	StdCompilerConfigWrite(HKEY hRoot, const char *szPath);
	~StdCompilerConfigWrite();

	// Properties
	virtual bool hasNaming() { return true; }
	virtual bool forceWrite() { return true; }
	virtual bool isRegistry() { return true; }

	// Naming
	virtual bool Name(const char *szName);
	virtual void NameEnd(bool fBreak = false);
	virtual bool FollowName(const char *szName);
	virtual bool Default(const char *szName);

	// Separators
	virtual bool Separator(Sep eSep);

	// Data writers
	virtual void DWord(int32_t &rInt);
	virtual void DWord(uint32_t &rInt);
	virtual void Word(int16_t &rShort);
	virtual void Word(uint16_t &rShort);
	virtual void Byte(int8_t &rByte);
	virtual void Byte(uint8_t &rByte);
	virtual void Boolean(bool &rBool);
	virtual void Character(char &rChar);
	virtual void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped);
	virtual void String(char **pszString, RawCompileType eType = RCT_Escaped);
	virtual void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped);

	// Passes
	virtual void Begin();
	virtual void End();

private:

	// Key stack
	int iDepth;
	struct Key
	{
		StdStrBuf Name;
		StdStrBuf LastChildName;  // last occuring child name to increase subindex if needed
		int32_t subindex; // incremented when multiple keys of the same name are encountered
		HKEY Handle;
		Key *Parent;
	} *pKey;
	StdStrBuf LastString; // assigned by String, reset by Name/NameEnd - contains last written string. Used for separators within strings.

	// Writing
	void CreateKey(HKEY hParent = 0);
	void WriteDWord(uint32_t iVal);
	void WriteString(const char *szStr);

};

// config reader
class StdCompilerConfigRead : public StdCompiler
{
public:

	// Construct with root key
	StdCompilerConfigRead(HKEY hRoot, const char *szPath);
	~StdCompilerConfigRead();

	// Properties
	virtual bool isCompiler() { return true; }
	virtual bool hasNaming() { return true; }
	virtual bool isRegistry() { return true; }

	// Naming
	virtual bool Name(const char *szName);
	virtual void NameEnd(bool fBreak = false);
	virtual bool FollowName(const char *szName);

	// Separators
	virtual bool Separator(Sep eSep);

	// Data writers
	virtual void DWord(int32_t &rInt);
	virtual void DWord(uint32_t &rInt);
	virtual void Word(int16_t &rShort);
	virtual void Word(uint16_t &rShort);
	virtual void Byte(int8_t &rByte);
	virtual void Byte(uint8_t &rByte);
	virtual void Boolean(bool &rBool);
	virtual void Character(char &rChar);
	virtual void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped);
	virtual void String(char **pszString, RawCompileType eType = RCT_Escaped);
	virtual void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped);

	// Passes
	virtual void Begin();
	virtual void End();

private:

	// Key stack
	int iDepth;
	struct Key
	{
		StdStrBuf Name;
		StdStrBuf LastChildName;  // last occuring child name to increase subindex if needed
		int32_t subindex; // incremented when multiple keys of the same name are encountered
		HKEY Handle; // for keys only
		Key *Parent;
		bool Virtual;
		DWORD Type; // for values only
	} *pKey;
	StdStrBuf LastString; // assigned by String, reset by Name/NameEnd - contains last read string. Used for separators within strings.

	// Reading
	uint32_t ReadDWord();
	StdStrBuf ReadString();

};

#endif

#endif // INC_STDREGISTRY
