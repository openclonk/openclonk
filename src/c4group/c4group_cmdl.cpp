/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2006, 2008  Sven Eberhardt
 * Copyright (c) 2003-2004, 2007  Matthes Bender
 * Copyright (c) 2004, 2007-2008  Peter Wortmann
 * Copyright (c) 2005  Günther Brammer
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

/* C4Group command line executable */

// Version    1.0 November 1997
//            1.1 November 1997
//            1.2 February 1998
//            1.3 March    1998
//            1.4 April    1998
//            1.5 May      1998
//            1.6 November 1998
//            1.7 December 1998
//            1.8 February 1999
//            1.9 May      1999
//            2.0 June     1999
//            2.6 March    2001
//            2.7 June     2001
//            2.8 June     2002
//         4.95.0 November 2003
//         4.95.4 July     2005 PORT/HEAD mixmax

#include <C4Include.h>
#include <C4ConfigShareware.h>
#include <StdRegistry.h>
#include <C4Group.h>
#include <C4Version.h>
#include <C4Update.h>

#include <shellapi.h>
#include <conio.h>

int globalArgC;
char **globalArgV;
int iFirstCommand = -1;

bool fQuiet = false;
bool fRecursive = false;
bool fRegisterShell = false;
bool fUnregisterShell = false;
bool fPromptAtEnd = false;
char strExecuteAtEnd[_MAX_PATH + 1] = "";

int iResult = 0;

C4ConfigShareware Config;
C4Config *GetCfg() { return &Config; }

#ifdef _WIN32
#ifdef _DEBUG
int dbg_printf(const char *strMessage, ...)
{
	va_list args; va_start(args, strMessage);
	// Compose formatted message
	StdStrBuf Buf;
	Buf.FormatV(strMessage, args);
	// Log
	OutputDebugString(Buf.getData());
	return printf(Buf.getData());
}
#define printf dbg_printf
#endif
#endif

bool ProcessGroup(const char *szFilename)
{

	C4Group hGroup;
	int iArg;
	bool fDeleteGroup = false;
	hGroup.SetStdOutput(true);

	int argc = globalArgC;
	char **argv = globalArgV;

	// Current filename
	if (!fQuiet)
		printf("Group: %s\n",szFilename);

	// Open group file
	if (hGroup.Open(szFilename, true))
	{
		// No commands: display contents
		if (iFirstCommand<0)
		{
			hGroup.View("*");
		}

		// Process commands
		else
			for (iArg=iFirstCommand; iArg<argc; iArg++)
			{
				// This argument is a command
				if (argv[iArg][0]=='-')
				{
					// Handle commands
					switch (argv[iArg][1])
					{
						// Add
					case 'a':
						if ((iArg+1>=argc) || (argv[iArg+1][0]=='-'))
							printf("Missing argument for add command\n");
						else
						{
							if ((argv[iArg][2]=='s') || (argv[iArg][2] && (argv[iArg][3]=='s')) )
							{
								if ((iArg+2>=argc) || (argv[iArg+2][0]=='-'))
									printf("Missing argument for add as command\n");
								else
									{ hGroup.Add(argv[iArg+1],argv[iArg+2]); iArg+=2;  }
							}
							else
#ifdef _WIN32
								{ hGroup.Add(argv[iArg+1]); iArg++; }
#else
								{ hGroup.Add(argv[iArg+1], argv[iArg+1]); iArg++; }
#endif
						}
						break;
						// Move
					case 'm':
						if ((iArg+1>=argc) || (argv[iArg+1][0]=='-'))
							printf("Missing argument for move command\n");
						else
#ifdef _WIN32
							{ hGroup.Move(argv[iArg+1]); iArg++; }
#else
							{ hGroup.Move(argv[iArg+1], argv[iArg+1]); iArg++; }
#endif
						break;
						// Extract
					case 'e':
						if ((iArg+1>=argc) || (argv[iArg+1][0]=='-'))
							printf("Missing argument for extract command\n");
						else
						{
							if ((argv[iArg][2]=='t') || (argv[iArg][2] && (argv[iArg][3]=='s')) )
							{
								if ((iArg+2>=argc) || (argv[iArg+2][0]=='-'))
									printf("Missing argument for extract as command\n");
								else
									{ hGroup.Extract(argv[iArg+1],argv[iArg+2]); iArg+=2; }
							}
							else
								{ hGroup.Extract(argv[iArg+1]); iArg++; }
						}
						break;
						// Delete
					case 'd':
						if ((iArg+1>=argc) || (argv[iArg+1][0]=='-'))
							printf("Missing argument for delete command\n");
						else
							{ hGroup.Delete(argv[iArg+1], fRecursive); iArg++; }
						break;
						// Sort
					case 's':
						// First sort parameter overrides default Clonk sort list
						C4Group_SetSortList(NULL);
						// Missing argument
						if ((iArg+1>=argc) || (argv[iArg+1][0]=='-'))
							printf("Missing argument for sort command\n");
						// Sort, advance to next argument
						else
							{ hGroup.Sort(argv[iArg+1]); iArg++; }
						break;
						// Rename
					case 'r':
						if ((iArg+2>=argc) || (argv[iArg+1][0]=='-') || (argv[iArg+2][0]=='-'))
							printf("Missing argument(s) for rename command\n");
						else
							{ hGroup.Rename(argv[iArg+1],argv[iArg+2]); iArg+=2; }
						break;
						// View
					case 'v':
						if ((iArg+1>=argc) || (argv[iArg+1][0]=='-'))
							{ hGroup.View("*"); }
						else
							{ hGroup.View(argv[iArg+1]); iArg++; }
						break;
						// Make original
					case 'o':
						hGroup.MakeOriginal(true);
						break;
						// Pack
					case 'p':
						printf("Packing...\n");
						// Close
						if (!hGroup.Close()) printf("Closing failed: %s\n",hGroup.GetError());
						// Pack
						else if (!C4Group_PackDirectory(szFilename)) printf("Pack failed\n");
						// Reopen
						else if (!hGroup.Open(szFilename)) printf("Reopen failed: %s\n",hGroup.GetError());
						break;
						// Unpack
					case 'u':
						printf("Unpacking...\n");
						// Close
						if (!hGroup.Close()) printf("Closing failed: %s\n",hGroup.GetError());
						// Unpack
						else if (!C4Group_UnpackDirectory(szFilename)) printf("Unpack failed\n");
						// Reopen
						else if (!hGroup.Open(szFilename)) printf("Reopen failed: %s\n",hGroup.GetError());
						break;
						// Unpack
					case 'x':
						printf("Exploding...\n");
						// Close
						if (!hGroup.Close()) printf("Closing failed: %s\n",hGroup.GetError());
						// Explode
						else if (!C4Group_ExplodeDirectory(szFilename)) printf("Unpack failed\n");
						// Reopen
						else if (!hGroup.Open(szFilename)) printf("Reopen failed: %s\n",hGroup.GetError());
						break;
						// Print maker
					case 'k':
						printf("%s\n",hGroup.GetMaker());
						break;
						// Generate update
					case 'g':
						if ((iArg + 3 >= argc) || (argv[iArg+1][0] == '-') || (argv[iArg+2][0] == '-') || (argv[iArg+3][0] == '-'))
							printf("Update generation failed: too few arguments\n");
						else
						{
							C4UpdatePackage Upd;
							// Close
							if (!hGroup.Close()) printf("Closing failed: %s\n",hGroup.GetError());
							// generate
							else if (!Upd.MakeUpdate(argv[iArg+1], argv[iArg+2], szFilename, argv[iArg+3]))
								printf("Update generation failed.\n");
							// Reopen
							else if (!hGroup.Open(szFilename)) printf("Reopen failed: %s\n",hGroup.GetError());
							iArg+=3;
						}
						break;
						// Apply update
					case 'y':
						printf("Applying update...\n");
						if (C4Group_ApplyUpdate(hGroup))
							{ if (argv[iArg][2]=='d') fDeleteGroup = true; }
						else
							printf("Update failed.\n");
						break;
						// Optimize update generation target
					case 'z':
						if ((iArg + 1 >= argc) || (argv[iArg+1][0] == '-'))
							printf("Missing parameter for optimization\n");
						else
						{
							printf("Optimizing %s...\n", argv[iArg+1]);
							if (!C4UpdatePackage::Optimize(&hGroup, argv[iArg+1]))
								printf("Optimization failed.\n");
							iArg++;
						}
						break;
#ifdef _DEBUG
						// Print internals
					case 'q':
						hGroup.PrintInternals();
						break;
#endif
						// Wait
					case 'w':
						if ((iArg+1>=argc) || (argv[iArg+1][0]=='-'))
							printf("Missing argument for wait command\n");
						else
						{
							int iMilliseconds = 0;
							sscanf(argv[iArg+1], "%d", &iMilliseconds);
							// Wait for specified time
							if (iMilliseconds > 0)
							{
								printf("Waiting...\n");
								Sleep(iMilliseconds);
							}
							// Wait for specified process to end
							else
							{
								printf("Waiting for %s to end", argv[iArg+1]);
								for (int i = 0; FindWindow(NULL, argv[iArg+1]) && (i < 5); i++)
								{
									Sleep(1000);
									printf(".");
								}
								printf("\n");
							}
							iArg++;
						}
						break;
						// Undefined
					default:
						printf("Unknown command: %s\n",argv[iArg]);
						break;
					}
				}
				else
				{
					printf("Invalid parameter %s\n",argv[iArg]);
				}

			}

		// Error: output status
		if (!SEqual(hGroup.GetError(),"No Error"))
			printf("Status: %s\n",hGroup.GetError());

		// Close group file
		if (!hGroup.Close())
			printf("Closing: %s\n",hGroup.GetError());

		// Delete group file if desired (i.e. after apply update)
		if (fDeleteGroup)
		{
			printf("Deleting %s...\n", GetFilename(szFilename));
			EraseItem(szFilename);
		}

	}

	// Couldn't open group
	else
	{
		printf("Status: %s\n",hGroup.GetError());
	}

	// Done
	return true;
}

int RegisterShellExtensions()
{
#ifdef _WIN32
	char strModule[2048];
	char strCommand[2048];
	char strClass[128];
	GetModuleFileName(NULL, strModule, 2048);
	// Groups
	const char *strClasses = "Clonk4.Definition;Clonk4.Folder;Clonk4.Group;Clonk4.Player;Clonk4.Scenario;Clonk4.Update;Clonk4.Weblink;Clonk4.Object";
	for (int i = 0; SCopySegment(strClasses, i, strClass); i++)
	{
		// Unpack
		sprintf(strCommand, "\"%s\" \"%%1\" \"-u\"", strModule);
		if (!SetRegShell(strClass, "MakeFolder", "C4Group Unpack", strCommand)) return 0;
		// Explode
		sprintf(strCommand, "\"%s\" \"%%1\" \"-x\"", strModule);
		if (!SetRegShell(strClass, "ExplodeFolder", "C4Group Explode", strCommand)) return 0;
	}
	// Directories
	const char *strClasses2 = "Directory";
	for (int i = 0; SCopySegment(strClasses2, i, strClass); i++)
	{
		// Pack
		sprintf(strCommand, "\"%s\" \"%%1\" \"-p\"", strModule);
		if (!SetRegShell(strClass, "MakeGroupFile", "C4Group Pack", strCommand)) return 0;
	}
	// Done
#endif
	return 1;
}

int UnregisterShellExtensions()
{
#ifdef _WIN32
	char strModule[2048];
	char strClass[128];
	GetModuleFileName(NULL, strModule, 2048);
	// Groups
	const char *strClasses = "Clonk4.Definition;Clonk4.Folder;Clonk4.Group;Clonk4.Player;Clonk4.Scenario;Clonk4.Update;Clonk4.Weblink";
	for (int i = 0; SCopySegment(strClasses, i, strClass); i++)
	{
		// Unpack
		if (!RemoveRegShell(strClass, "MakeFolder")) return 0;
		// Explode
		if (!RemoveRegShell(strClass, "ExplodeFolder")) return 0;
	}
	// Directories
	const char *strClasses2 = "Directory";
	for (int i = 0; SCopySegment(strClasses2, i, strClass); i++)
	{
		// Pack
		if (!RemoveRegShell(strClass, "MakeGroupFile")) return 0;
	}
	// Done
#endif
	return 1;
}

bool Log(const char *msg)
{
	if (!fQuiet)
		printf("%s\n", msg);
	return 1;
}

bool LogFatal(const char *msg) { return Log(msg); }

bool LogF(const char *strMessage, ...)
{
	va_list args; va_start(args, strMessage);
	// Compose formatted message
	StdStrBuf Buf;
	Buf.FormatV(strMessage, args);
	// Log
	return Log(Buf.getData());
}

void StdCompilerWarnCallback(void *pData, const char *szPosition, const char *szError)
{
	const char *szName = reinterpret_cast<const char *>(pData);
	if (!szPosition || !*szPosition)
		LogF("WARNING: %s: %s", szName, szError);
	else
		LogF("WARNING: %s (%s): %s", szName, szPosition, szError);
}


int main(int argc, char *argv[])
{

	// Scan options (scan including first parameter - this means the group filename cannot start with a '/'...)
	for (int i = 1; i < argc; i++)
	{
		// Option encountered
		if (argv[i][0]=='/')
			switch (argv[i][1])
			{
				// Quiet mode
			case 'q': fQuiet = true; break;
				// Recursive mode
			case 'r': fRecursive = true; break;
				// Register shell
			case 'i': fRegisterShell = true; break;
				// Unregister shell
			case 'u': fUnregisterShell = true; break;
				// Prompt at end
			case 'p': fPromptAtEnd = true; break;
				// Execute at end
			case 'x': SCopy(argv[i] + 3, strExecuteAtEnd, _MAX_PATH); break;
				// Unknown
			default: printf("Unknown option %s\n",argv[i]); break;
			}
		// Command encountered: no more options expected
		if (argv[i][0]=='-')
		{
			iFirstCommand = i;
			break;
		}
	}

	// Program info
	if (!fQuiet)
		printf("RedWolf Design C4Group %s\n", C4VERSION);

	// Registration check
	Config.Init();
	bool fWasQuiet = fQuiet; fQuiet = true; // prevent premature logging of registration error
	Config.Load(false);
	fQuiet = fWasQuiet;

	// Init C4Group
	C4Group_SetMaker(Config.General.Name); // This makes no sense if unregistered but it is assumed that no groups can be written if unregistered
	C4Group_SetTempPath(Config.General.TempPath);
	C4Group_SetSortList(C4CFN_FLS);

	// Display current working directory
	if (!fQuiet)
	{
		char strWorkingDirectory[_MAX_PATH+1] = "";
		GetCurrentDirectory(_MAX_PATH, strWorkingDirectory);
		printf("Location: %s\n", strWorkingDirectory);
	}

	// Store command line parameters
	globalArgC = argc;
	globalArgV = argv;

	// Register shell
	if (fRegisterShell)
		if (RegisterShellExtensions())
			printf("Shell extensions registered.\n");
		else
			printf("Error registering shell extensions. You might need administrator rights.\n");
	// Unregister shell
	if (fUnregisterShell)
		if (UnregisterShellExtensions())
			printf("Shell extensions removed.\n");
		else
			printf("Error removing shell extensions.\n");

	// At least one parameter (filename, not option or command): process file(s)
	if ((argc>1) && (argv[1][0] != '/') && (argv[1][0] != '-')) // ...remember filenames cannot start with a forward slash because of options format
	{
		// Wildcard in filename: use file search
		if (SCharCount('*',argv[1]))
			ForEachFile(argv[1], &ProcessGroup);
		// Only one file
		else
			ProcessGroup(argv[1]);
	}

	// Too few parameters: output help (if we didn't register stuff)
	else if (!fRegisterShell && !fUnregisterShell)
	{
		printf("\n");
		printf("Usage:    c4group group [options] command(s)\n\n");
		printf("Commands: -a[s] Add [as]  -m Move  -e[t] Extract [to]\n");
		printf("          -v View  -d Delete  -r Rename  -s Sort\n");
		printf("          -p Pack  -u Unpack  -x Explode\n");
		printf("          -k Print maker\n");
		printf("          -g [source] [target] [title] Make update\n");
		printf("          -y[d] Apply update [and delete group file]\n");
		printf("          -z Optimize a group to be similar (smaller update)\n");
		printf("\n");
		printf("Options:  /q Quiet /r Recursive /p Prompt at end\n");
		printf("          /i Register shell /u Unregister shell\n");
		printf("          /x:<command> Execute shell command when done\n");
		printf("\n");
		printf("Examples: c4group pack.c4g -a myfile.dat -v *.dat\n");
		printf("          c4group pack.c4g -as myfile.dat myfile.bin\n");
		printf("          c4group pack.c4g -et *.dat \\data\\mydatfiles\\\n");
		printf("          c4group pack.c4g -et myfile.dat myfile.bak\n");
		printf("          c4group pack.c4g -s \"*.bin|*.dat\"\n");
		printf("          c4group pack.c4g -x\n");
		printf("          c4group pack.c4g /q -k\n");
		printf("          c4group update.c4u -g ver1.c4f ver2.c4f New_Version\n");
		printf("          c4group ver1.c4f -z ver2.c4f\n");
		printf("          c4group /i\n");
	}

	// Prompt at end
	if (fPromptAtEnd)
	{
		printf("\nDone. Press any key to continue.\n");
		_getch();
	}

	// Execute when done
	if (strExecuteAtEnd[0])
	{
		printf("Executing: %s\n", strExecuteAtEnd);

		STARTUPINFO startInfo;
		ZeroMem(&startInfo, sizeof(startInfo));
		startInfo.cb = sizeof(startInfo);

		PROCESS_INFORMATION procInfo;

		CreateProcess(strExecuteAtEnd, NULL, NULL, NULL, false, 0, NULL, NULL, &startInfo, &procInfo);
	}

	// Done
	return iResult;

}
