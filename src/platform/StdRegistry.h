/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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
#include "lib/StdCompiler.h"
#include "platform/C4windowswrapper.h"

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
	~StdCompilerConfigWrite() override;

	// Properties
	bool hasNaming() override { return true; }
	bool isRegistry() override { return true; }

	// Naming
	bool Name(const char *szName) override;
	void NameEnd(bool fBreak = false) override;
	bool FollowName(const char *szName) override;
	bool Default(const char *szName) override;

	// Separators
	bool Separator(Sep eSep) override;

	// Data writers
	void DWord(int32_t &rInt) override;
	void DWord(uint32_t &rInt) override;
	void Word(int16_t &rShort) override;
	void Word(uint16_t &rShort) override;
	void Byte(int8_t &rByte) override;
	void Byte(uint8_t &rByte) override;
	void Boolean(bool &rBool) override;
	void Character(char &rChar) override;
	void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) override;
	void String(char **pszString, RawCompileType eType = RCT_Escaped) override;
	void String(std::string &str, RawCompileType type = RCT_Escaped) override;
	void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override;

	// Passes
	void Begin() override;
	void End() override;

private:

	// Key stack
	int iDepth;
	struct Key
	{
		StdCopyStrBuf Name;
		StdCopyStrBuf LastChildName;  // last occuring child name to increase subindex if needed
		int32_t subindex; // incremented when multiple keys of the same name are encountered
		HKEY Handle;
		Key *Parent;
	} *pKey;

	// Current string for writing with separators
	std::string last_written_string;

	// Writing
	void CreateKey(HKEY hParent = nullptr);
	void WriteDWord(uint32_t iVal);
	void WriteString(const char *szStr);

};

// config reader
class StdCompilerConfigRead : public StdCompiler
{
public:

	// Construct with root key
	StdCompilerConfigRead(HKEY hRoot, const char *szPath);
	~StdCompilerConfigRead() override;

	// Properties
	bool isDeserializer() override { return true; }
	bool hasNaming() override { return true; }
	bool isRegistry() override { return true; }

	// Naming
	bool Name(const char *szName) override;
	void NameEnd(bool fBreak = false) override;
	bool FollowName(const char *szName) override;

	// Separators
	bool Separator(Sep eSep) override;
	void NoSeparator() override;

	// Data writers
	void DWord(int32_t &rInt) override;
	void DWord(uint32_t &rInt) override;
	void Word(int16_t &rShort) override;
	void Word(uint16_t &rShort) override;
	void Byte(int8_t &rByte) override;
	void Byte(uint8_t &rByte) override;
	void Boolean(bool &rBool) override;
	void Character(char &rChar) override;
	void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) override;
	void String(char **pszString, RawCompileType eType = RCT_Escaped) override;
	void String(std::string &str, RawCompileType type = RCT_Escaped) override;
	void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override;

	// Passes
	void Begin() override;
	void End() override;

private:

	// Key stack
	int iDepth;
	struct Key
	{
		StdCopyStrBuf Name;
		StdCopyStrBuf LastChildName;  // last occuring child name to increase subindex if needed
		int32_t subindex; // incremented when multiple keys of the same name are encountered
		HKEY Handle; // for keys only
		Key *Parent;
		bool Virtual;
		DWORD Type; // for values only
	} *pKey;

	// Current string for reading with separators
	std::string last_read_string;
	bool has_read_string = false;
	bool has_separator_mismatch = false;

	// Reading
	uint32_t ReadDWord();
	void ReadString();
	void ResetLastString();

};

#endif

#endif // INC_STDREGISTRY
