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
	~StdCompilerConfigWrite();

	// Properties
	virtual bool hasNaming() override { return true; }
	virtual bool isRegistry() override { return true; }

	// Naming
	virtual bool Name(const char *szName) override;
	virtual void NameEnd(bool fBreak = false) override;
	virtual bool FollowName(const char *szName) override;
	virtual bool Default(const char *szName) override;

	// Separators
	virtual bool Separator(Sep eSep) override;

	// Data writers
	virtual void DWord(int32_t &rInt) override;
	virtual void DWord(uint32_t &rInt) override;
	virtual void Word(int16_t &rShort) override;
	virtual void Word(uint16_t &rShort) override;
	virtual void Byte(int8_t &rByte) override;
	virtual void Byte(uint8_t &rByte) override;
	virtual void Boolean(bool &rBool) override;
	virtual void Character(char &rChar) override;
	virtual void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) override;
	virtual void String(char **pszString, RawCompileType eType = RCT_Escaped) override;
	virtual void String(std::string &str, RawCompileType type = RCT_Escaped) override;
	virtual void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override;

	// Passes
	virtual void Begin() override;
	virtual void End() override;

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
	virtual bool isDeserializer() override { return true; }
	virtual bool hasNaming() override { return true; }
	virtual bool isRegistry() override { return true; }

	// Naming
	virtual bool Name(const char *szName) override;
	virtual void NameEnd(bool fBreak = false) override;
	virtual bool FollowName(const char *szName) override;

	// Separators
	virtual bool Separator(Sep eSep) override;

	// Data writers
	virtual void DWord(int32_t &rInt) override;
	virtual void DWord(uint32_t &rInt) override;
	virtual void Word(int16_t &rShort) override;
	virtual void Word(uint16_t &rShort) override;
	virtual void Byte(int8_t &rByte) override;
	virtual void Byte(uint8_t &rByte) override;
	virtual void Boolean(bool &rBool) override;
	virtual void Character(char &rChar) override;
	virtual void String(char *szString, size_t iMaxLength, RawCompileType eType = RCT_Escaped) override;
	virtual void String(char **pszString, RawCompileType eType = RCT_Escaped) override;
	virtual void String(std::string &str, RawCompileType type = RCT_Escaped) override;
	virtual void Raw(void *pData, size_t iSize, RawCompileType eType = RCT_Escaped) override;

	// Passes
	virtual void Begin() override;
	virtual void End() override;

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
