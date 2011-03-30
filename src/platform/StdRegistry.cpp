/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004-2005, 2007  Matthes Bender
 * Copyright (c) 2005-2006  Peter Wortmann
 * Copyright (c) 2006  Sven Eberhardt
 * Copyright (c) 2009  Günther Brammer
 * Copyright (c) 2010  Armin Burgmeier
 * Copyright (c) 2010  Caesar
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

/* Some wrappers for easier access to the Windows registry */

#include "C4Include.h"
#include <StdRegistry.h>

#ifdef _WIN32
#include <C4windowswrapper.h>
#include <stdio.h>

bool DeleteRegistryValue(const char *szSubKey, const char *szValueName)
{
	return DeleteRegistryValue(HKEY_CURRENT_USER,szSubKey,szValueName);
}

bool DeleteRegistryValue(HKEY hKey, const char *szSubKey, const char *szValueName)
{
	long qerr;
	HKEY ckey;
	// Open the key
	if ((qerr=RegOpenKeyEx(hKey,
	                       szSubKey,
	                       0,
	                       KEY_ALL_ACCESS,
	                       &ckey
	                      ))!=ERROR_SUCCESS) return false;
	// Delete the key
	if ((qerr=RegDeleteValue(ckey,
	                         szValueName
	                        ))!=ERROR_SUCCESS) return false;
	// Close the key
	RegCloseKey(ckey);
	// Success
	return true;
}

bool SetRegistryDWord(const char *szSubKey, const char *szValueName, DWORD dwValue)
{
	return SetRegistryDWord(HKEY_CURRENT_USER,szSubKey,szValueName,dwValue);
}

bool GetRegistryDWord(const char *szSubKey, const char *szValueName, DWORD *lpdwValue)
{
	return GetRegistryDWord(HKEY_CURRENT_USER,szSubKey,szValueName,lpdwValue);
}

bool GetRegistryDWord(HKEY hKey, const char *szSubKey, const char *szValueName, DWORD *lpdwValue)
{
	long qerr;
	HKEY ckey;
	DWORD valtype;
	DWORD valsize=sizeof(DWORD);

	// Open the key
	if ((qerr=RegOpenKeyEx(hKey,
	                       szSubKey,
	                       0,
	                       KEY_READ,
	                       &ckey
	                      ))!=ERROR_SUCCESS) return false;

	// Get the value
	if ((qerr=RegQueryValueEx(ckey,
	                          szValueName,
	                          NULL,
	                          &valtype,
	                          (BYTE*) lpdwValue,
	                          &valsize
	                         ))!=ERROR_SUCCESS)  { RegCloseKey(ckey); return false; }

	// Close the key
	RegCloseKey(ckey);

	if (valtype!=REG_DWORD) return false;

	return true;
}

bool SetRegistryDWord(HKEY hKey, const char *szSubKey, const char *szValueName, DWORD dwValue)
{
	long qerr;
	HKEY ckey;
	DWORD disposition;
	// Open the key
	if ((qerr=RegCreateKeyEx(hKey,
	                         szSubKey,
	                         0,
	                         "",
	                         REG_OPTION_NON_VOLATILE,
	                         KEY_ALL_ACCESS,
	                         NULL,
	                         &ckey,
	                         &disposition
	                        ))!=ERROR_SUCCESS) return false;
	// Set the value
	if ((qerr=RegSetValueEx(ckey,
	                        szValueName,
	                        0,
	                        REG_DWORD,
	                        (BYTE*) &dwValue,
	                        sizeof(dwValue)
	                       ))!=ERROR_SUCCESS) { RegCloseKey(ckey); return false; }

	// Close the key
	RegCloseKey(ckey);
	// Success
	return true;
}

bool GetRegistryString(const char *szSubKey,
                       const char *szValueName,
                       char *sValue, DWORD dwValSize)
{
	long qerr;
	HKEY ckey;
	DWORD valtype;

	// Open the key
	if ((qerr=RegOpenKeyEx(HKEY_CURRENT_USER,
	                       szSubKey,
	                       0,
	                       KEY_READ,
	                       &ckey
	                      ))!=ERROR_SUCCESS) return false;

	// Get the value
	if ((qerr=RegQueryValueEx(ckey,
	                          szValueName,
	                          NULL,
	                          &valtype,
	                          (BYTE*) sValue,
	                          &dwValSize
	                         ))!=ERROR_SUCCESS)  { RegCloseKey(ckey); return false; }

	// Close the key
	RegCloseKey(ckey);

	if (valtype!=REG_SZ) return false;

	return true;
}

bool SetRegistryString(const char *szSubKey,
                       const char *szValueName,
                       const char *szValue)
{

	long qerr;
	HKEY ckey;
	DWORD disposition;

	// Open the key
	if ((qerr=RegCreateKeyEx(HKEY_CURRENT_USER,
	                         szSubKey,
	                         0,
	                         "",
	                         REG_OPTION_NON_VOLATILE,
	                         KEY_ALL_ACCESS,
	                         NULL,
	                         &ckey,
	                         &disposition
	                        ))!=ERROR_SUCCESS) return false;

	// Set the value
	if ((qerr=RegSetValueEx(ckey,
	                        szValueName,
	                        0,
	                        REG_SZ,
	                        (BYTE*) szValue,
	                        SLen(szValue)+1
	                       ))!=ERROR_SUCCESS) { RegCloseKey(ckey); return false; }

	// Close the key
	RegCloseKey(ckey);

	return true;
}

bool DeleteRegistryKey(HKEY hKey, const char *szSubKey)
{
	HKEY ckey;
	// Open the key
	if (RegOpenKeyEx(hKey, szSubKey, 0, KEY_ALL_ACCESS, &ckey) != ERROR_SUCCESS) return false;
	// Delete all subkeys
	char strChild[1024 + 1];
	while (RegEnumKey(ckey, 0, strChild, 1024) == ERROR_SUCCESS)
		if (!DeleteRegistryKey(ckey, strChild))
			return false;
	// Close the key
	RegCloseKey(ckey);

	// Delete the key
	if (RegDeleteKey(hKey, szSubKey) != ERROR_SUCCESS) return false;
	// Success
	return true;
}

bool DeleteRegistryKey(const char *szSubKey)
{
	return DeleteRegistryKey(HKEY_CURRENT_USER, szSubKey);
}

bool SetRegClassesRoot(const char *szSubKey,
                       const char *szValueName,
                       const char *szStringValue)
{

	long qerr;
	HKEY ckey;
	DWORD disposition;

	// Open the key
	if ((qerr=RegCreateKeyEx(HKEY_CLASSES_ROOT,
	                         szSubKey,
	                         0,
	                         "",
	                         REG_OPTION_NON_VOLATILE,
	                         KEY_ALL_ACCESS,
	                         NULL,
	                         &ckey,
	                         &disposition
	                        ))!=ERROR_SUCCESS) return false;

	// Set the value
	if ((qerr=RegSetValueEx(ckey,
	                        szValueName,
	                        0,
	                        REG_SZ,
	                        (BYTE*) szStringValue,
	                        SLen(szStringValue)+1
	                       ))!=ERROR_SUCCESS) { RegCloseKey(ckey); return false; }

	// Close the key
	RegCloseKey(ckey);

	return true;
}

bool SetRegClassesRootString(const char *szSubKey,
                             const char *szValueName,
                             const char *szValue)
{

	long qerr;
	HKEY ckey;
	DWORD disposition;

	// Open the key
	if ((qerr=RegCreateKeyEx(HKEY_CLASSES_ROOT,
	                         szSubKey,
	                         0,
	                         "",
	                         REG_OPTION_NON_VOLATILE,
	                         KEY_ALL_ACCESS,
	                         NULL,
	                         &ckey,
	                         &disposition
	                        ))!=ERROR_SUCCESS) return false;

	// Set the value
	if ((qerr=RegSetValueEx(ckey,
	                        szValueName,
	                        0,
	                        REG_SZ,
	                        (BYTE*) szValue,
	                        SLen(szValue)+1
	                       ))!=ERROR_SUCCESS) { RegCloseKey(ckey); return false; }

	// Close the key
	RegCloseKey(ckey);

	return true;
}

bool SetRegShell(const char *szClassName,
                 const char *szShellName,
                 const char *szShellCaption,
                 const char *szCommand,
                 bool fMakeDefault)
{
	char szKeyName[256+1];
	// Set shell caption
	sprintf(szKeyName,"%s\\Shell\\%s",szClassName,szShellName);
	if (!SetRegClassesRoot(szKeyName, NULL, szShellCaption)) return false;
	// Set shell command
	sprintf(szKeyName,"%s\\Shell\\%s\\Command",szClassName,szShellName);
	if (!SetRegClassesRoot(szKeyName, NULL, szCommand)) return false;
	// Set as default command
	if (fMakeDefault)
	{
		sprintf(szKeyName, "%s\\Shell", szClassName);
		if (!SetRegClassesRoot(szKeyName, NULL, szShellName)) return false;
	}
	return true;
}

bool RemoveRegShell(const char *szClassName,
                    const char *szShellName)
{
	char strKey[256+1];
	sprintf(strKey, "%s\\Shell\\%s", szClassName, szShellName);
	if (!DeleteRegistryKey(HKEY_CLASSES_ROOT, strKey)) return false;
	return true;
}

bool SetRegFileClass(const char *szClassRoot,
                     const char *szExtension,
                     const char *szClassName,
                     const char *szIconPath, int iIconNum,
                     const char *szContentType)
{
	char keyname[100];
	char iconpath[512];
	// Create root class entry
	if (!SetRegClassesRoot(szClassRoot,NULL,szClassName)) return false;
	// Set root class icon
	sprintf(keyname,"%s\\DefaultIcon",szClassRoot);
	sprintf(iconpath,"%s,%d",szIconPath,iIconNum);
	if (!SetRegClassesRoot(keyname,NULL,iconpath)) return false;
	// Set extension map entry
	sprintf(keyname,".%s",szExtension);
	if (!SetRegClassesRoot(keyname,NULL,szClassRoot)) return false;
	// Set extension content type
	sprintf(keyname,".%s",szExtension);
	if (!SetRegClassesRootString(keyname,"Content Type",szContentType)) return false;
	// Success
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
	char regstr[100],buffer2[5];
	int x,y,wdt,hgt;
	bool fSetSize=true;
	// No position stored: cannot restore
	if (!GetRegistryString(szSubKey,szWindowName,regstr,100))
		return false;
	if (SEqual(regstr,"Maximized"))
		return !!ShowWindow(hwnd,SW_MAXIMIZE | SW_NORMAL);
	if (SEqual(regstr,"Minimized"))
		return !!ShowWindow(hwnd,SW_MINIMIZE | SW_NORMAL);
	SCopySegment(regstr,0,buffer2,',',4); sscanf(buffer2,"%i",&x);
	SCopySegment(regstr,1,buffer2,',',4); sscanf(buffer2,"%i",&y);
	if (SCopySegment(regstr,2,buffer2,',',4)) sscanf(buffer2,"%i",&wdt); else fSetSize=false;
	if (SCopySegment(regstr,3,buffer2,',',4)) sscanf(buffer2,"%i",&hgt); else fSetSize=false;
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
	pKey->Handle = 0;
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
	Key *pnKey = new Key();
	pnKey->Handle = 0;
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
	LastString.Clear();
	return true;
}

void StdCompilerConfigWrite::NameEnd(bool fBreak)
{
	assert(iDepth);
	// Close current key
	if (pKey->Handle)
		RegCloseKey(pKey->Handle);
	LastString.Clear();
	// Pop
	Key *poKey = pKey;
	pKey = poKey->Parent;
	delete poKey;
	iDepth--;
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
	BOOL hr = DeleteRegistryKey(pKey->Handle, szName);
	RegDeleteValue(pKey->Handle, szName);
	// Handled
	return true;
}

bool StdCompilerConfigWrite::Separator(Sep eSep)
{
	// Append separators to last string
	char sep [] = { SeparatorToChar(eSep), '\0' };
	WriteString(sep);
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
	if (RegCreateKeyEx(hParent ? hParent : pKey->Parent->Handle,
	                   pKey->Name.getData(),
	                   0, "", REG_OPTION_NON_VOLATILE,
	                   KEY_WRITE, NULL,
	                   &pKey->Handle, NULL) != ERROR_SUCCESS)
		excCorrupt("Could not create key %s!", pKey->Name.getData());
}

void StdCompilerConfigWrite::WriteDWord(uint32_t iVal)
{
	// Set the value
	if (RegSetValueEx(pKey->Parent->Handle, pKey->Name.getData(),
	                  0, REG_DWORD, reinterpret_cast<const BYTE *>(&iVal),
	                  sizeof(iVal)) != ERROR_SUCCESS)
		excCorrupt("Could not write key %s!", pKey->Name.getData());
}

void StdCompilerConfigWrite::WriteString(const char *szString)
{
	// Append or set the value
	if (LastString.getLength()) LastString.Append(szString); else LastString.Copy(szString);
	if (RegSetValueEx(pKey->Parent->Handle, pKey->Name.getData(),
	                  0, REG_SZ, reinterpret_cast<const BYTE *>(LastString.getData()),
					  LastString.getLength() + 1) != ERROR_SUCCESS)
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
	if (RegOpenKeyEx(hRoot, szPath,
	                 0, KEY_READ,
	                 &pKey->Handle) != ERROR_SUCCESS)
		pKey->Handle = 0;
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
	StdStrBuf sName;
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
	if (RegOpenKeyEx(pKey->Handle, sName.getData(),
	                 0, KEY_READ,
	                 &hSubKey) != ERROR_SUCCESS)
	{
		hSubKey = 0;
		// Try to query value (exists?)
		if (RegQueryValueEx(pKey->Handle, sName.getData(),
		                    0, &dwType, NULL, NULL) != ERROR_SUCCESS)
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
	// Last string reset
	LastString.Clear();
	return fFound;
}

void StdCompilerConfigRead::NameEnd(bool fBreak)
{
	assert(iDepth);
	// Close current key
	if (pKey->Handle)
		RegCloseKey(pKey->Handle);
	LastString.Clear();
	// Pop
	Key *poKey = pKey;
	pKey = poKey->Parent;
	delete poKey;
	iDepth--;
}

bool StdCompilerConfigRead::FollowName(const char *szName)
{
	NameEnd(); return Name(szName);
}

bool StdCompilerConfigRead::Separator(Sep eSep)
{
	if (LastString.getData())
	{
		// separator within string: check if it is there
		if (LastString.getLength() && *LastString.getData() == SeparatorToChar(eSep))
		{
			LastString.Take(StdStrBuf(LastString.getData()+1, true));
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		// No separators outside strings
		return false;
	}
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
		StdStrBuf szVal = ReadString();
		rBool = (szVal == "true");
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
		StdStrBuf szVal = ReadString();
		rChar = *szVal.getData();
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
	if (!LastString) LastString.Take(ReadString());
	if (!LastString.getLength()) { *szString='\0'; return; }
	// when reading identifiers, only take parts of the string
	if (eType == RCT_Idtf || eType == RCT_IdtfAllowEmpty)
	{
		const char *s = LastString.getData();
		size_t ncpy = 0;
		while (isalnum((unsigned char)s[ncpy])) ++ncpy;
		SCopy(LastString.getData(), szString, Min<size_t>(iMaxLength, ncpy));
		LastString.Take(StdStrBuf(s+ncpy, true));
	}
	else
	{
		SCopy(LastString.getData(), szString, iMaxLength);
	}
}

void StdCompilerConfigRead::String(char **pszString, RawCompileType eType)
{
	if (!LastString) LastString.Take(ReadString());
	// when reading identifiers, only take parts of the string
	if (eType == RCT_Idtf || eType == RCT_IdtfAllowEmpty)
	{
		const char *s = LastString.getData();
		size_t ncpy = 0;
		while (isalnum((unsigned char)s[ncpy])) ++ncpy;
		StdStrBuf Result(LastString.getData(), ncpy, true);
		Result.getMData()[ncpy] = '\0';
		*pszString = Result.GrabPointer();
		LastString.Take(StdStrBuf(s+ncpy, true));
	}
	else
	{
		*pszString = LastString.GrabPointer();
	}
}

void StdCompilerConfigRead::Raw(void *pData, size_t iSize, RawCompileType eType)
{
	excCorrupt(0, "Raw values aren't supported for registry compilers!");
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
	// Read
	uint32_t iVal; DWORD iSize = sizeof(iVal);
	if (RegQueryValueEx(pKey->Parent->Handle, pKey->Name.getData(),
	                    0, NULL,
	                    reinterpret_cast<LPBYTE>(&iVal),
	                    &iSize) != ERROR_SUCCESS)
		{ excNotFound("Could not read value %s!", pKey->Name.getData()); return 0; }
	// Check size
	if (iSize != sizeof(iVal))
		{ excCorrupt("Wrong size of a DWord!"); return 0; }
	// Return
	return iVal;
}

StdStrBuf StdCompilerConfigRead::ReadString()
{
	// Virtual key?
	if (pKey->Virtual)
		{ excNotFound("Could not read value %s! Parent key doesn't exist!", pKey->Name.getData()); return StdStrBuf(); }
	// Wrong type?
	if (pKey->Type != REG_SZ)
		{ excNotFound("Wrong value type!"); return StdStrBuf(); }
	// Get size of string
	DWORD iSize;
	if (RegQueryValueEx(pKey->Parent->Handle, pKey->Name.getData(),
	                    0, NULL,
	                    NULL,
	                    &iSize) != ERROR_SUCCESS)
		{ excNotFound("Could not read value %s!", pKey->Name.getData()); return StdStrBuf(); }
	// Allocate string
	StdStrBuf Result; Result.SetLength(iSize - 1);
	// Read
	if (RegQueryValueEx(pKey->Parent->Handle, pKey->Name.getData(),
	                    0, NULL,
	                    reinterpret_cast<BYTE *>(Result.getMData()),
	                    &iSize) != ERROR_SUCCESS)
		{ excNotFound("Could not read value %s!", pKey->Name.getData()); return StdStrBuf(); }
	// Check size
	if (strlen(Result.getData()) + 1 != iSize)
		{ excCorrupt("Wrong size of a string!"); return StdStrBuf(); }
	return Result;
}

#endif // _WIN32
