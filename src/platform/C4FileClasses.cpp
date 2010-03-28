/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
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

/* Some class entries in the Windows registry */

#include <C4Include.h>
#include <C4FileClasses.h>

#include <StdRegistry.h>

#define C4FileClassContentType "application/vnd.clonk.c4group"

bool SetProtocol(const char *szProtocol, const char *szCommand, const char *szModule)
{

	if (!SetRegClassesRoot(szProtocol,NULL,"URL: Protocol")) return false;
	if (!SetRegClassesRoot(szProtocol,"URL Protocol","")) return false;

	char szCmd[_MAX_PATH+1],szKey[_MAX_PATH+1];
	sprintf(szCmd,szCommand,szModule);
	sprintf(szKey,"%s\\shell\\open\\command",szProtocol);
	if (!SetRegClassesRoot(szKey,"",szCmd)) return false;

	char szIconpath[_MAX_PATH+1];
	sprintf(szIconpath,"%s,1",szModule);
	sprintf(szKey,"%s\\DefaultIcon",szProtocol);
	if (!SetRegClassesRoot(szKey,"",szIconpath)) return false;

	return true;
}

bool SetC4FileClasses(const char *szEnginePath)
{

	if (!SetRegFileClass("Clonk4.Scenario",   "c4s", "Clonk 4 Scenario",          szEnginePath, 1, C4FileClassContentType)) return false;
	if (!SetRegFileClass("Clonk4.Group",      "c4g", "Clonk 4 Group",             szEnginePath, 2, C4FileClassContentType)) return false;
	if (!SetRegFileClass("Clonk4.Folder",     "c4f", "Clonk 4 Folder",            szEnginePath, 3, C4FileClassContentType)) return false;
	if (!SetRegFileClass("Clonk4.Player",     "c4p", "Clonk 4 Player",            szEnginePath, 4, C4FileClassContentType)) return false;
	if (!SetRegFileClass("Clonk4.Definition", "c4d", "Clonk 4 Object Definition", szEnginePath, 6, C4FileClassContentType)) return false;
	if (!SetRegFileClass("Clonk4.Object",     "c4i", "Clonk 4 Object Info",       szEnginePath, 7, C4FileClassContentType)) return false;
	if (!SetRegFileClass("Clonk4.Material",   "c4m", "Clonk 4 Material",          szEnginePath, 8, "text/plain")) return false;
	if (!SetRegFileClass("Clonk4.Binary",     "c4b", "Clonk 4 Binary",            szEnginePath, 9, "application/octet-stream")) return false;
	if (!SetRegFileClass("Clonk4.Video",      "c4v", "Clonk 4 Video",             szEnginePath, 10, "video/avi")) return false;
	if (!SetRegFileClass("Clonk4.Weblink",    "c4l", "Clonk 4 Weblink",           szEnginePath, 11, C4FileClassContentType)) return false;
	if (!SetRegFileClass("Clonk4.Key",        "c4k", "Clonk 4 Key",               szEnginePath, 12, "application/octet-stream")) return false;
	if (!SetRegFileClass("Clonk4.Update",     "c4u", "Clonk 4 Update",            szEnginePath, 13, C4FileClassContentType)) return false;

	if (!SetProtocol("clonk", "%s %%1 /Fullscreen", szEnginePath)) return false;

	char strCommand[2048];
	// c4k installation: send to engine
	sprintf(strCommand, "\"%s\" \"%%1\"", szEnginePath);
	if (!SetRegShell("Clonk4.Key", "Register", "Register", strCommand)) return false;
	// c4u application: send to engine
	sprintf(strCommand, "\"%s\" \"%%1\"", szEnginePath);
	if (!SetRegShell("Clonk4.Update", "Update", "Update", strCommand, true)) return false;

	// kill old App Paths registration
	DeleteRegistryKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Clonk.exe");

	return true;
}
