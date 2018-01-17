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

#include "C4Include.h"
#include "platform/StdRegistry.h"

#ifdef _WIN32

StdCopyStrBuf GetRegistryString(const char *szSubKey, const char *szValueName)
{
	HKEY ckey;

	// Open the key
	if (RegOpenKeyExW(HKEY_CURRENT_USER, GetWideChar(szSubKey), 0, KEY_READ, &ckey)!=ERROR_SUCCESS)
		return StdCopyStrBuf();

	// Get the value
	DWORD dwValSize = 128;
	BYTE *sValue = new BYTE[dwValSize];
	while(true)
	{
		DWORD valtype;
		switch(RegQueryValueExW(ckey, GetWideChar(szValueName), nullptr, &valtype,
			sValue, &dwValSize))
		{
		case ERROR_SUCCESS:
			RegCloseKey(ckey);
			if (valtype == REG_SZ)
			{
				StdCopyStrBuf nrv(reinterpret_cast<wchar_t*>(sValue));
				delete[] sValue;
				return nrv;
			} else {
		default:
				delete[] sValue;
				return StdCopyStrBuf();
			}
			break;
		case ERROR_MORE_DATA:
			delete[] sValue;
			sValue = new BYTE[dwValSize];
			break;
		}
	}
}

bool SetRegistryString(const char *szSubKey,
                       const char *szValueName,
                       const char *szValue)
{

	long qerr;
	HKEY ckey;
	DWORD disposition;

	// Open the key
	if ((qerr=RegCreateKeyExW(HKEY_CURRENT_USER,
	                         GetWideChar(szSubKey),
	                         0,
	                         nullptr,
	                         REG_OPTION_NON_VOLATILE,
	                         KEY_ALL_ACCESS,
	                         nullptr,
	                         &ckey,
	                         &disposition
	                        ))!=ERROR_SUCCESS) return false;

	// Set the value
	StdBuf v = GetWideCharBuf(szValue);
	if ((qerr=RegSetValueExW(ckey,
	                        GetWideChar(szValueName),
	                        0,
	                        REG_SZ,
	                        getBufPtr<BYTE>(v),
	                        v.getSize()
	                       ))!=ERROR_SUCCESS) { RegCloseKey(ckey); return false; }

	// Close the key
	RegCloseKey(ckey);

	return true;
}

static bool DeleteRegistryKey(HKEY hKey, const wchar_t *szSubKey)
{
	HKEY ckey;
	// Open the key
	if (RegOpenKeyExW(hKey, szSubKey, 0, KEY_ALL_ACCESS, &ckey) != ERROR_SUCCESS) return false;
	// Delete all subkeys
	wchar_t strChild[1024 + 1];
	while (RegEnumKeyW(ckey, 0, strChild, 1024) == ERROR_SUCCESS)
		if (!DeleteRegistryKey(ckey, strChild))
			return false;
	// Close the key
	RegCloseKey(ckey);

	// Delete the key
	if (RegDeleteKeyW(hKey, szSubKey) != ERROR_SUCCESS) return false;
	// Success
	return true;
}

static bool SetRegClassesRoot(const wchar_t *szSubKey,
                       const wchar_t *szValueName,
                       const wchar_t *szStringValue)
{

	long qerr;
	HKEY ckey;
	DWORD disposition;

	// Open the key
	if ((qerr=RegCreateKeyExW(HKEY_CLASSES_ROOT,
	                         szSubKey,
	                         0,
	                         nullptr,
	                         REG_OPTION_NON_VOLATILE,
	                         KEY_ALL_ACCESS,
	                         nullptr,
	                         &ckey,
	                         &disposition
	                        ))!=ERROR_SUCCESS) return false;

	// Set the value
	if ((qerr=RegSetValueExW(ckey,
	                        szValueName,
	                        0,
	                        REG_SZ,
	                        (const BYTE*)szStringValue,
	                        (wcslen(szStringValue) + 1) * sizeof(wchar_t)
	                       ))!=ERROR_SUCCESS) { RegCloseKey(ckey); return false; }

	// Close the key
	RegCloseKey(ckey);

	return true;
}

bool SetRegShell(const wchar_t *szClassName,
                 const wchar_t *szShellName,
                 const wchar_t *szShellCaption,
                 const wchar_t *szCommand,
                 bool fMakeDefault)
{
	wchar_t szKeyName[256+1];
	// Set shell caption
	_snwprintf(szKeyName,256,L"%s\\Shell\\%s",szClassName,szShellName);
	if (!SetRegClassesRoot(szKeyName, nullptr, szShellCaption)) return false;
	// Set shell command
	_snwprintf(szKeyName,256,L"%s\\Shell\\%s\\Command",szClassName,szShellName);
	if (!SetRegClassesRoot(szKeyName, nullptr, szCommand)) return false;
	// Set as default command
	if (fMakeDefault)
	{
		_snwprintf(szKeyName, 256,L"%s\\Shell", szClassName);
		if (!SetRegClassesRoot(szKeyName, nullptr, szShellName)) return false;
	}
	return true;
}

bool RemoveRegShell(const char *szClassName,
                    const char *szShellName)
{
	wchar_t strKey[256+1];
	_snwprintf(strKey, 256, L"%s\\Shell\\%s", GetWideChar(szClassName).p, GetWideChar(szShellName).p);
	if (!DeleteRegistryKey(HKEY_CLASSES_ROOT, strKey)) return false;
	return true;
}

//------------------------------ Window Position ------------------------------------------

bool StoreWindowPosition(HWND hwnd,
                         const char *szWindowName,
                         const char *szSubKey,
                         bool fStoreSize)
{
	RECT winpos;
	char regstr[100];
	if (IsZoomed(hwnd))
		return SetRegistryString(szSubKey,szWindowName,"Maximized");
	if (IsIconic(hwnd))
		return SetRegistryString(szSubKey,szWindowName,"Minimized");
	if (!GetWindowRect(hwnd,&winpos)) return false;
	if (fStoreSize) sprintf(regstr,"%ld,%ld,%ld,%ld",winpos.left,winpos.top,winpos.right-winpos.left,winpos.bottom-winpos.top);
	else sprintf(regstr,"%ld,%ld",winpos.left,winpos.top);
	return SetRegistryString(szSubKey,szWindowName,regstr);
}

bool RestoreWindowPosition(HWND hwnd,
                           const char *szWindowName,
                           const char *szSubKey,
                           bool fHidden)
{
	char buffer2[5];
	int x,y,wdt,hgt;
	bool fSetSize=true;
	StdCopyStrBuf regstr = GetRegistryString(szSubKey,szWindowName);
	// No position stored: cannot restore
	if (regstr.isNull())
		return false;
	if (regstr == "Maximized")
		return !!ShowWindow(hwnd,SW_MAXIMIZE | SW_NORMAL);
	if (regstr == "Minimized")
		return !!ShowWindow(hwnd,SW_MINIMIZE | SW_NORMAL);
	SCopySegment(regstr.getData(),0,buffer2,',',4); sscanf(buffer2,"%i",&x);
	SCopySegment(regstr.getData(),1,buffer2,',',4); sscanf(buffer2,"%i",&y);
	if (SCopySegment(regstr.getData(),2,buffer2,',',4)) sscanf(buffer2,"%i",&wdt); else fSetSize=false;
	if (SCopySegment(regstr.getData(),3,buffer2,',',4)) sscanf(buffer2,"%i",&hgt); else fSetSize=false;
	if (!fSetSize)
	{
		RECT winpos; if (!GetWindowRect(hwnd,&winpos)) return false;
		wdt=winpos.right-winpos.left; hgt=winpos.bottom-winpos.top;
	}
	// Move window
	WINDOWPLACEMENT wp; memset(&wp, 0, sizeof(WINDOWPLACEMENT)); wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hwnd, &wp);
	RECT normalpos;
	normalpos.left = x; normalpos.right  = wdt + x;
	normalpos.top  = y; normalpos.bottom = hgt + y;
	wp.rcNormalPosition = normalpos;
	if (SetWindowPlacement(hwnd, &wp))
		return false;
	// Hide window
	if (fHidden)
		return !!ShowWindow(hwnd, SW_HIDE);
	// Show window
	return !!ShowWindow(hwnd, SW_NORMAL);
}

//------------------------------ Registry compiler ------------------------------------------

// *** StdCompilerConfigWrite

StdCompilerConfigWrite::StdCompilerConfigWrite(HKEY hRoot, const char *szPath)
		: iDepth(0), pKey(new Key())
{
	pKey->Name = szPath;
	pKey->subindex = 0;
	pKey->Handle = nullptr;
	CreateKey(hRoot);
}

StdCompilerConfigWrite::~StdCompilerConfigWrite()
{
	assert(!iDepth);
	if (pKey->Handle) RegCloseKey(pKey->Handle);
	delete pKey;
}

bool StdCompilerConfigWrite::Name(const char *szName)
{
	// Open parent key (if not already done so)
	CreateKey();
	// Push new subkey onto the stack
	auto *pnKey = new Key();
	pnKey->Handle = nullptr;
	pnKey->subindex = 0;
	if (pKey->LastChildName == szName)
		pnKey->Name.Format("%s%d", szName, (int)++pKey->subindex);
	else
	{
		pnKey->Name = szName;
		pKey->LastChildName = szName;
	}
	pnKey->Parent = pKey;
	pKey = pnKey;
	iDepth++;
	last_written_string.clear();
	return true;
}

void StdCompilerConfigWrite::NameEnd(bool fBreak)
{
	assert(iDepth);
	// Close current key
	if (pKey->Handle)
		RegCloseKey(pKey->Handle);
	// Pop
	Key *poKey = pKey;
	pKey = poKey->Parent;
	delete poKey;
	iDepth--;
	last_written_string.clear();
}

bool StdCompilerConfigWrite::FollowName(const char *szName)
{
	NameEnd(); return Name(szName);
}

bool StdCompilerConfigWrite::Default(const char *szName)
{
	// Open parent
	CreateKey();
	// Remove key/value (failsafe)
	DeleteRegistryKey(pKey->Handle, GetWideChar(szName));
	RegDeleteValueW(pKey->Handle, GetWideChar(szName));
	// Handled
	return true;
}

bool StdCompilerConfigWrite::Separator(Sep eSep)
{
	// Just append separator and re-write last string
	const char s[2] = { SeparatorToChar(eSep) , '\0' };
	WriteString(s);
	return true;
}

void StdCompilerConfigWrite::DWord(int32_t &rInt)
{
	WriteDWord(rInt);
}
void StdCompilerConfigWrite::DWord(uint32_t &rInt)
{
	WriteDWord(rInt);
}
void StdCompilerConfigWrite::Word(int16_t &rInt)
{
	WriteDWord(rInt);
}
void StdCompilerConfigWrite::Word(uint16_t &rInt)
{
	WriteDWord(rInt);
}
void StdCompilerConfigWrite::Byte(int8_t &rInt)
{
	WriteDWord(rInt);
}
void StdCompilerConfigWrite::Byte(uint8_t &rInt)
{
	WriteDWord(rInt);
}
void StdCompilerConfigWrite::Boolean(bool &rBool)
{
	WriteDWord(rBool ? 1 : 0);
}
void StdCompilerConfigWrite::Character(char &rChar)
{
	WriteString(FormatString("%c", rChar).getData());
}

void StdCompilerConfigWrite::String(char *szString, size_t iMaxLength, RawCompileType eType)
{
	WriteString(szString);
}

void StdCompilerConfigWrite::String(char **pszString, RawCompileType eType)
{
	WriteString(pszString ? *pszString : "");
}

void StdCompilerConfigWrite::String(std::string &str, RawCompileType eType)
{
	WriteString(str.c_str());
}

void StdCompilerConfigWrite::Raw(void *pData, size_t iSize, RawCompileType eType)
{
	excCorrupt("Raw values aren't supported for registry compilers!");
}

void StdCompilerConfigWrite::Begin()
{
	assert(!iDepth);
}

void StdCompilerConfigWrite::End()
{
	assert(!iDepth);
}

void StdCompilerConfigWrite::CreateKey(HKEY hParent)
{
	// Already open?
	if (pKey->Handle)
		return;
	// Open/Create registry key
	if (RegCreateKeyExW(hParent ? hParent : pKey->Parent->Handle,
	                   pKey->Name.GetWideChar(),
	                   0, nullptr, REG_OPTION_NON_VOLATILE,
	                   KEY_WRITE, nullptr,
	                   &pKey->Handle, nullptr) != ERROR_SUCCESS)
		excCorrupt("Could not create key %s!", pKey->Name.getData());
}

void StdCompilerConfigWrite::WriteDWord(uint32_t iVal)
{
	// Set the value
	if (RegSetValueExW(pKey->Parent->Handle, pKey->Name.GetWideChar(),
	                  0, REG_DWORD, reinterpret_cast<const BYTE *>(&iVal),
	                  sizeof(iVal)) != ERROR_SUCCESS)
		excCorrupt("Could not write key %s!", pKey->Name.getData());
}

void StdCompilerConfigWrite::WriteString(const char *szString)
{
	// Append to write-string and just re-write
	// This is probably pretty inefficient, but config saving only uses it for a few key overloads
	last_written_string += szString;
	StdBuf v = GetWideCharBuf(last_written_string.c_str());
	if (RegSetValueExW(pKey->Parent->Handle, pKey->Name.GetWideChar(),
	                  0, REG_SZ, getBufPtr<BYTE>(v), v.getSize()) != ERROR_SUCCESS)
		excCorrupt("Could not write key %s!", pKey->Name.getData());
}

// *** StdCompilerConfigRead

StdCompilerConfigRead::StdCompilerConfigRead(HKEY hRoot, const char *szPath)
		: iDepth(0), pKey(new Key())
{
	pKey->Name = szPath;
	pKey->Virtual = false;
	pKey->subindex = 0;
	// Open root
	if (RegOpenKeyExW(hRoot, GetWideChar(szPath),
	                 0, KEY_READ,
	                 &pKey->Handle) != ERROR_SUCCESS)
		pKey->Handle = nullptr;
}

StdCompilerConfigRead::~StdCompilerConfigRead()
{
	assert(!iDepth);
	if (pKey->Handle) RegCloseKey(pKey->Handle);
	delete pKey;
}

bool StdCompilerConfigRead::Name(const char *szName)
{
	// Adjust key name for lists
	StdCopyStrBuf sName;
	if (pKey->LastChildName == szName)
		sName.Format("%s%d", szName, (int)++pKey->subindex);
	else
	{
		sName = szName;
		pKey->LastChildName = szName;
	}
	bool fFound = true;
	// Try to open registry key
	HKEY hSubKey; DWORD dwType = 0;
	if (RegOpenKeyExW(pKey->Handle, sName.GetWideChar(),
	                 0, KEY_READ,
	                 &hSubKey) != ERROR_SUCCESS)
	{
		hSubKey = nullptr;
		// Try to query value (exists?)
		if (RegQueryValueExW(pKey->Handle, sName.GetWideChar(),
		                    nullptr, &dwType, nullptr, nullptr) != ERROR_SUCCESS)
			fFound = false;
	}
	// Push new subkey on the stack
	Key *pnKey = new Key();
	pnKey->Handle = hSubKey;
	pnKey->Name = sName;
	pnKey->subindex = 0;
	pnKey->Parent = pKey;
	pnKey->Virtual = !fFound;
	pnKey->Type = dwType;
	pKey = pnKey;
	iDepth++;
	ResetLastString();
	return fFound;
}

void StdCompilerConfigRead::NameEnd(bool fBreak)
{
	assert(iDepth);
	// Close current key
	if (pKey->Handle)
		RegCloseKey(pKey->Handle);
	// Pop
	Key *poKey = pKey;
	pKey = poKey->Parent;
	delete poKey;
	iDepth--;
	ResetLastString();
}

void StdCompilerConfigRead::ResetLastString()
{
	has_read_string = false;
	has_separator_mismatch = false;
	last_read_string.clear();
}

bool StdCompilerConfigRead::FollowName(const char *szName)
{
	NameEnd(); return Name(szName);
}

bool StdCompilerConfigRead::Separator(Sep eSep)
{
	// Make sure there's a string to work on
	ReadString();
	// Match?
	if (last_read_string.size() && (last_read_string.front() == SeparatorToChar(eSep)))
	{
		// Match found. Just advance.
		last_read_string = last_read_string.substr(1);
		return true;
	}
	else
	{
		// Separator mismatch? Let all read attempts fail until the correct separator is found or the naming ends.
		has_separator_mismatch = true;
		return false;
	}
}

void StdCompilerConfigRead::NoSeparator() {
       has_separator_mismatch = false;
}

void StdCompilerConfigRead::DWord(int32_t &rInt)
{
	rInt = int32_t(ReadDWord());
}
void StdCompilerConfigRead::DWord(uint32_t &rInt)
{
	rInt = ReadDWord();
}
void StdCompilerConfigRead::Word(int16_t &rInt)
{
	rInt = int16_t(ReadDWord());
}
void StdCompilerConfigRead::Word(uint16_t &rInt)
{
	rInt = uint16_t(ReadDWord());
}
void StdCompilerConfigRead::Byte(int8_t &rInt)
{
	rInt = int8_t(ReadDWord());
}
void StdCompilerConfigRead::Byte(uint8_t &rInt)
{
	rInt = uint8_t(ReadDWord());
}
void StdCompilerConfigRead::Boolean(bool &rBool)
{
	try
	{
		ReadString();
		rBool = (!has_separator_mismatch && (last_read_string == "true"));
	}
	catch (NotFoundException *pExc)
	{
		delete pExc;
		uint32_t iVal = ReadDWord();
		rBool = !! iVal;
	}
}
void StdCompilerConfigRead::Character(char &rChar)
{
	try
	{
		ReadString();
		rChar = (last_read_string.length() && !has_separator_mismatch) ? last_read_string.front() : '\0';
	}
	catch (NotFoundException *pExc)
	{
		delete pExc;
		uint32_t iVal = ReadDWord();
		rChar = char(iVal);
	}
}

void StdCompilerConfigRead::String(char *szString, size_t iMaxLength, RawCompileType eType)
{
	std::string s;
	String(s, eType);
	SCopy(s.c_str(), szString, iMaxLength);
}

void StdCompilerConfigRead::String(char **pszString, RawCompileType eType)
{
	// Read string, copy into buffer and release buffer (to use buffer allocation method)
	std::string s;
	String(s, eType);
	StdStrBuf sbuf(s.c_str(), true);
	*pszString = sbuf.GrabPointer();
}

void StdCompilerConfigRead::String(std::string &str, RawCompileType type)
{
	// Read from string until end marker is found
	ReadString();
	if (has_separator_mismatch || !last_read_string.length()) { str = "\0"; return; }
	size_t string_end_pos = 0;
	const char *s = last_read_string.c_str();
	size_t spos = 0;
	while (!IsStringEnd(s[spos], type)) ++spos;
	if (spos < last_read_string.length())
	{
		str = last_read_string.substr(0, spos);
		last_read_string = last_read_string.substr(spos);
	}
	else
	{
		str = last_read_string;
		last_read_string.clear();
	}
}

void StdCompilerConfigRead::Raw(void *pData, size_t iSize, RawCompileType eType)
{
	excCorrupt(nullptr, "Raw values aren't supported for registry compilers!");
}

void StdCompilerConfigRead::Begin()
{
	assert(!iDepth);
}

void StdCompilerConfigRead::End()
{
	assert(!iDepth);
}

uint32_t StdCompilerConfigRead::ReadDWord()
{
	// Virtual key?
	if (pKey->Virtual)
		{ excNotFound("Could not read value %s! Parent key doesn't exist!", pKey->Name.getData()); return 0; }
	// Wrong type?
	if (pKey->Type != REG_DWORD && pKey->Type != REG_DWORD_LITTLE_ENDIAN)
		{ excNotFound("Wrong value type!"); return 0; }
	// Clear previous string
	ResetLastString();
	// Read
	uint32_t iVal; DWORD iSize = sizeof(iVal);
	if (RegQueryValueExW(pKey->Parent->Handle, pKey->Name.GetWideChar(),
	                    nullptr, nullptr,
	                    reinterpret_cast<LPBYTE>(&iVal),
	                    &iSize) != ERROR_SUCCESS)
		{ excNotFound("Could not read value %s!", pKey->Name.getData()); return 0; }
	// Check size
	if (iSize != sizeof(iVal))
		{ excCorrupt("Wrong size of a DWord!"); return 0; }
	// Return
	return iVal;
}

void StdCompilerConfigRead::ReadString()
{
	// Already read?
	if (!has_read_string)
	{
		ResetLastString();
		// Virtual key?
		if (pKey->Virtual)
		{
			excNotFound("Could not read value %s! Parent key doesn't exist!", pKey->Name.getData()); return;
		}
		// Wrong type?
		if (pKey->Type != REG_SZ)
		{
			excNotFound("Wrong value type!"); return;
		}
		// Get size of string
		DWORD iSize;
		if (RegQueryValueExW(pKey->Parent->Handle, pKey->Name.GetWideChar(),
			nullptr, nullptr,
			nullptr,
			&iSize) != ERROR_SUCCESS)
		{
			excNotFound("Could not read value %s!", pKey->Name.getData()); return;
		}
		// Allocate string
		StdBuf Result; Result.SetSize(iSize);
		// Read
		if (RegQueryValueExW(pKey->Parent->Handle, pKey->Name.GetWideChar(),
			nullptr, nullptr,
			reinterpret_cast<BYTE *>(Result.getMData()),
			&iSize) != ERROR_SUCCESS)
		{
			excNotFound("Could not read value %s!", pKey->Name.getData()); return;
		}
		// Check size
		if (wcslen(getBufPtr<wchar_t>(Result)) + 1 != iSize / sizeof(wchar_t))
		{
			excCorrupt("Wrong size of a string!"); return;
		}
		// Remember string
		StdStrBuf str_result(getBufPtr<wchar_t>(Result));
		last_read_string = str_result.getData();
		has_read_string = true;
	}
}

#endif // _WIN32
