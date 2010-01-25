/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2007, 2009  Günther Brammer
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
//            2.8 June           2002
//         4.95.0 November 2003
//         4.95.4 July     2005 PORT/HEAD mixmax
//         4.95.4 September 2005 Unix-flavour

#include <C4Include.h>
#include <C4Group.h>
#include <C4Version.h>
#include <C4Update.h>
#include <C4ConfigShareware.h>
#include <StdRegistry.h>

int globalArgC;
char **globalArgV;
int iFirstCommand = 0;

bool fQuiet = true;
bool fRecursive = false;
bool fRegisterShell = false;
bool fUnregisterShell = false;
char strExecuteAtEnd[_MAX_PATH + 1] = "";

int iResult = 0;

bool Log(const char *msg) {
  if (!fQuiet)
    printf("%s\n", msg);
  return 1;
}
#define IMPLEMENT_LOGF(func) \
	bool func(const char *msg, ...) { \
		va_list args; va_start(args, msg); \
		StdStrBuf Buf; \
		Buf.FormatV(msg, args); \
		return Log(Buf.getData()); \
	}

IMPLEMENT_LOGF(DebugLogF)
IMPLEMENT_LOGF(LogF)
IMPLEMENT_LOGF(LogSilentF)

bool ProcessGroup(const char *FilenamePar) {

  C4Group hGroup;
  hGroup.SetStdOutput(!fQuiet);
	bool fDeleteGroup = false;

  int argc = globalArgC;
  char **argv = globalArgV;

  // Strip trailing slash
  char * szFilename = strdup(FilenamePar);
  size_t len = strlen(szFilename);
  if (szFilename[len-1] == DirectorySeparator) szFilename[len-1] = 0;
  // Current filename
  LogF("Group: %s", szFilename);

  // Open group file
  if (hGroup.Open(szFilename, true)) {
    // No commands: display contents
    if (iFirstCommand >= argc) {
      hGroup.SetStdOutput(true);
				hGroup.View("*");
      hGroup.SetStdOutput(!fQuiet);
    }
    // Process commands
    else {
      for (int iArg = iFirstCommand; iArg < argc; ++iArg) {
        // This argument is a command
        if (argv[iArg][0] == '-') {
					// Handle commands
          switch (argv[iArg][1]) {
            // Add
          case 'a':
            if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
              fprintf(stderr, "Missing argument for add command\n");
            else {
              if ((argv[iArg][2] == 's') || (argv[iArg][2] && (argv[iArg][3] == 's'))) {
                if ((iArg + 2 >= argc) || (argv[iArg + 2][0] == '-')) {
                  fprintf(stderr, "Missing argument for add as command\n");
                } else {
                  hGroup.Add(argv[iArg + 1], argv[iArg + 2]);
                  iArg += 2;
                }
              } else {
                while (iArg + 1 < argc && argv[iArg + 1][0] != '-') {
                  ++iArg;
#ifdef _WIN32
                  // manually expand wildcards
                  hGroup.Add(argv[iArg]);
#else
                  hGroup.Add(argv[iArg], GetFilename(argv[iArg]));
#endif
                }
              }
            }
            break;
            // Move
          case 'm':
            if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-')) {
              fprintf(stderr, "Missing argument for move command\n");
            } else {
              while (iArg + 1 < argc && argv[iArg + 1][0] != '-') {
                ++iArg;
#ifdef _WIN32
                // manually expand wildcards
                hGroup.Move(argv[iArg]);
#else
                hGroup.Move(argv[iArg], argv[iArg]);
#endif
              }
            }
            break;
            // Extract
          case 'e':
            if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-')) {
              fprintf(stderr, "Missing argument for extract command\n");
            } else {
              if ((argv[iArg][2] == 't') || (argv[iArg][2] && (argv[iArg][3] == 's'))) {
                if ((iArg + 2 >= argc) || (argv[iArg + 2][0] == '-')) {
                  fprintf(stderr, "Missing argument for extract as command\n");
                } else {
                  hGroup.Extract(argv[iArg + 1], argv[iArg + 2]);
                  iArg += 2;
                }
              } else {
                hGroup.Extract(argv[iArg + 1]);
                iArg++;
              }
            }
            break;
            // Delete
          case 'd':
            if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-')) {
              fprintf(stderr, "Missing argument for delete command\n");
            } else {
              hGroup.Delete(argv[iArg + 1], fRecursive);
              iArg++;
            }
            break;
            // Sort
          case 's':
            // First sort parameter overrides default Clonk sort list
            C4Group_SetSortList(NULL);
            // Missing argument
            if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-')) {
              fprintf(stderr, "Missing argument for sort command\n");
            }
            // Sort, advance to next argument
            else {
              hGroup.Sort(argv[iArg + 1]);
              iArg++;
            }
            break;
            // Rename
          case 'r':
            if ((iArg + 2 >= argc) || (argv[iArg + 1][0] == '-')
                || (argv[iArg + 2][0] == '-')) {
              fprintf(stderr, "Missing argument(s) for rename command\n");
            } else {
              hGroup.Rename(argv[iArg + 1], argv[iArg + 2]);
              iArg += 2;
            }
            break;
            // View
          case 'l':
            hGroup.SetStdOutput(true);
            if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-')) {
              hGroup.View("*");
            } else {
              hGroup.View(argv[iArg + 1]);
              iArg++;
            }
            hGroup.SetStdOutput(!fQuiet);
            break;
            // Make original
          case 'o':
            hGroup.MakeOriginal(true);
            break;
            // Pack
          case 'p':
            Log("Packing...");
            // Close
            if (!hGroup.Close()) {
              fprintf(stderr, "Closing failed: %s\n", hGroup.GetError());
            }
            // Pack
            else if (!C4Group_PackDirectory(szFilename)) {
              fprintf(stderr, "Pack failed\n");
            }
            // Reopen
            else if (!hGroup.Open(szFilename)) {
              fprintf(stderr, "Reopen failed: %s\n", hGroup.GetError());
            }
            break;
            // Unpack
          case 'u':
            LogF("Unpacking...");
            // Close
            if (!hGroup.Close()) {
              fprintf(stderr, "Closing failed: %s\n", hGroup.GetError());
            }
            // Pack
            else if (!C4Group_UnpackDirectory(szFilename)) {
              fprintf(stderr, "Unpack failed\n");
            }
            // Reopen
            else if (!hGroup.Open(szFilename)) {
              fprintf(stderr, "Reopen failed: %s\n", hGroup.GetError());
            }
            break;
            // Unpack
          case 'x':
            Log("Exploding...");
            // Close
            if (!hGroup.Close()) {
              fprintf(stderr, "Closing failed: %s\n", hGroup.GetError());
            }
            // Pack
            else if (!C4Group_ExplodeDirectory(szFilename)) {
              fprintf(stderr, "Unpack failed\n");
            }
            // Reopen
            else if (!hGroup.Open(szFilename)) {
              fprintf(stderr, "Reopen failed: %s\n", hGroup.GetError());
            }
            break;
            // Print maker
          case 'k':
            printf("%s\n", hGroup.GetMaker());
            break;
            // Generate update
          case 'g':
            if ((iArg + 3 >= argc) || (argv[iArg + 1][0] == '-')
                || (argv[iArg + 2][0] == '-')
                || (argv[iArg + 3][0] == '-')) {
              fprintf(stderr, "Update generation failed: too few arguments\n");
            } else {
              C4UpdatePackage Upd;
              // Close
              if (!hGroup.Close()) {
                fprintf(stderr, "Closing failed: %s\n", hGroup.GetError());
              }
              // generate
              else if (!Upd.MakeUpdate(argv[iArg + 1], argv[iArg + 2], szFilename, argv[iArg + 3])) {
                fprintf(stderr, "Update generation failed.\n");
              }
              // Reopen
              else if (!hGroup.Open(szFilename)) {
                fprintf(stderr, "Reopen failed: %s\n", hGroup.GetError());
              }
              iArg += 3;
            }
            break;

            // Apply an update
          case 'y':
            Log("Applying update...");
            if (C4Group_ApplyUpdate(hGroup))
							{ if (argv[iArg][2]=='d') fDeleteGroup = true; }
						else
              fprintf(stderr,"Update failed.\n");
            break;
#ifdef _DEBUG
          case 'z':
            hGroup.PrintInternals();
            break;
#endif
          // Undefined
          default:
            fprintf(stderr, "Unknown command: %s\n", argv[iArg]);
            break;
          }
        } else {
          fprintf(stderr, "Invalid parameter %s\n", argv[iArg]);
        }

      }
    }
    // Error: output status
    if (!SEqual(hGroup.GetError(), "No Error")) {
      fprintf(stderr, "Status: %s\n", hGroup.GetError());
    }
    // Close group file
    if (!hGroup.Close()) {
      fprintf(stderr, "Closing: %s\n", hGroup.GetError());
    }
		// Delete group file if desired (i.e. after apply update)
		if (fDeleteGroup) {
			LogF("Deleting %s...\n", GetFilename(szFilename));
			EraseItem(szFilename);
		}
  }
  // Couldn't open group
  else {
    fprintf(stderr, "Status: %s\n", hGroup.GetError());
  }
  free(szFilename);
  // Done
  return true;
}

int RegisterShellExtensions() {
#ifdef _WIN32
  char strModule[2048];
  char strCommand[2048];
  char strClass[128];
  int i;
  GetModuleFileName(NULL, strModule, 2048);
  // Groups
  const char *strClasses =
    "Clonk4.Definition;Clonk4.Folder;Clonk4.Group;Clonk4.Player;Clonk4.Scenario;Clonk4.Update;Clonk4.Weblink";
  for (i = 0; SCopySegment(strClasses, i, strClass); i++) {
    // Unpack
    sprintf(strCommand, "\"%s\" \"%%1\" \"-u\"", strModule);
    if (!SetRegShell(strClass, "MakeFolder", "C4Group Unpack", strCommand))
      return 0;
    // Explode
    sprintf(strCommand, "\"%s\" \"%%1\" \"-x\"", strModule);
    if (!SetRegShell(strClass, "ExplodeFolder", "C4Group Explode", strCommand))
      return 0;
  }
  // Directories
  const char *strClasses2 = "Directory";
  for (i = 0; SCopySegment(strClasses2, i, strClass); i++) {
    // Pack
    sprintf(strCommand, "\"%s\" \"%%1\" \"-p\"", strModule);
    if (!SetRegShell(strClass, "MakeGroupFile", "C4Group Pack", strCommand))
      return 0;
  }
  // Done
#endif
  return 1;
}

int UnregisterShellExtensions() {
#ifdef _WIN32
  char strModule[2048];
  char strClass[128];
  int i;
  GetModuleFileName(NULL, strModule, 2048);
  // Groups
  const char *strClasses =
    "Clonk4.Definition;Clonk4.Folder;Clonk4.Group;Clonk4.Player;Clonk4.Scenario;Clonk4.Update;Clonk4.Weblink";
  for (i = 0; SCopySegment(strClasses, i, strClass); i++) {
    // Unpack
    if (!RemoveRegShell(strClass, "MakeFolder"))
      return 0;
    // Explode
    if (!RemoveRegShell(strClass, "ExplodeFolder"))
      return 0;
  }
  // Directories
  const char *strClasses2 = "Directory";
  for (i = 0; SCopySegment(strClasses2, i, strClass); i++) {
    // Pack
    if (!RemoveRegShell(strClass, "MakeGroupFile"))
      return 0;
  }
  // Done
#endif
  return 1;
}

int main(int argc, char *argv[]) {
#ifndef WIN32
  // Always line buffer mode, even if the output is not sent to a terminal
	setvbuf(stdout, NULL, _IOLBF, 0);
#endif
  // Scan options
  int iFirstGroup = 0;
  for (int i = 1; i < argc; ++i) {
    // Option encountered
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
        // Quiet mode
      case 'v':
        fQuiet = false;
        break;
        // Silent mode
        //case 's': fSilent=true; break;
        // Recursive mode
      case 'r':
        fRecursive = true;
        break;
        // Register shell
      case 'i':
        fRegisterShell = true;
        break;
        // Unregister shell
      case 'u':
        fUnregisterShell = true;
        break;
				// Execute at end
			case 'x': SCopy(argv[i] + 3, strExecuteAtEnd, _MAX_PATH); break;
        // Unknown
      default:
        fprintf(stderr, "Unknown option %s\n", argv[i]);
        break;
      }
    } else {
      // filename encountered: no more options expected
      iFirstGroup = i;
      break;
    }
  }
  iFirstCommand = iFirstGroup;
  while (iFirstCommand < argc && argv[iFirstCommand][0] != '-')
    ++iFirstCommand;

  // Program info
  LogF("RedWolf Design C4Group %s", C4VERSION);

  // Registration check
/*  Config.Init();
  Config.Load(false);*/

  // Init C4Group
/*  C4Group_SetMaker(Config.General.Name);
  C4Group_SetTempPath(Config.General.TempPath);*/
  C4Group_SetSortList(C4CFN_FLS);

  // Store command line parameters
  globalArgC = argc;
  globalArgV = argv;

  // Register shell
  if (fRegisterShell) {
    if (RegisterShellExtensions())
      printf("Shell extensions registered.\n");
    else
      printf("Error registering shell extensions.\n");
  }
  // Unregister shell
  if (fUnregisterShell) {
    if (UnregisterShellExtensions())
      printf("Shell extensions removed.\n");
    else
      printf("Error removing shell extensions.\n");
  }

  // At least one parameter (filename, not option or command): process file(s)
  if (iFirstGroup) {
#ifdef _WIN32
    // Wildcard in filename: use file search
    if (SCharCount('*', argv[1]))
      ForEachFile(argv[1], &ProcessGroup);
    // Only one file
    else
      ProcessGroup(argv[1]);
#else
    for (int i = iFirstGroup; i < argc && argv[i][0] != '-'; ++i)
      ProcessGroup(argv[i]);
#endif
  }
  // Too few parameters: output help (if we didn't register stuff)
  else if (!fRegisterShell && !fUnregisterShell) {
    printf("\n");
    printf("Usage:    c4group [options] group(s) command(s)\n\n");
    printf("Commands: -a[s] Add [as]  -m Move  -e[t] Extract [to]\n");
    printf("          -l List  -d Delete  -r Rename  -s Sort\n");
    printf("          -p Pack  -u Unpack  -x Explode\n");
    printf("          -k Print maker\n");
    printf("          -g [source] [target] [title] Make update\n");
    printf("          -y Apply update\n");
    printf("\n");
    printf("Options:  -v Verbose -r Recursive\n");
    printf("          -i Register shell -u Unregister shell\n");
    printf("          -x:<command> Execute shell command when done\n");
    printf("\n");
    printf("Examples: c4group pack.c4g -a myfile.dat -l \"*.dat\"\n");
    printf("          c4group pack.c4g -as myfile.dat myfile.bin\n");
    printf("          c4group -v pack.c4g -et \"*.dat\" \\data\\mydatfiles\\\n");
    printf("          c4group pack.c4g -et myfile.dat myfile.bak\n");
    printf("          c4group pack.c4g -s \"*.bin|*.dat\"\n");
    printf("          c4group pack.c4g -x\n");
    printf("          c4group pack.c4g -k\n");
    printf("          c4group update.c4u -g ver1.c4f ver2.c4f New_Version\n");
    printf("          c4group -i\n");
  }

	// Execute when done
	if (strExecuteAtEnd[0])
		{
		printf("Executing: %s\n", strExecuteAtEnd);
#ifdef _WIN32

		STARTUPINFO startInfo;
		ZeroMem(&startInfo, sizeof(startInfo));
		startInfo.cb = sizeof(startInfo);

		PROCESS_INFORMATION procInfo;

		CreateProcess(strExecuteAtEnd, NULL, NULL, NULL, false, 0, NULL, NULL, &startInfo, &procInfo);
#else
		switch (fork())
		  {
		  // Error
		  case -1:
		    fprintf(stderr, "Error forking.\n");
		    break;
		  // Child process
		  case 0:
		    execl(strExecuteAtEnd, strExecuteAtEnd, static_cast<char *>(0)); // currently no parameters are passed to the executed program
		    exit(1);
		  // Parent process
		  default:
		    break;
		  }
#endif
		}
  // Done
  return iResult;

}
