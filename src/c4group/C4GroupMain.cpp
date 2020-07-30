/*
 * OpenClonk, http://www.openclonk.org
 *
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

/* C4Group command line executable */

#include "C4Include.h"

#include "c4group/C4Group.h"
#include "C4Version.h"
#include "c4group/C4Update.h"
#include "platform/StdRegistry.h"
#include "C4Licenses.h"
#ifdef _WIN32
#include "platform/C4windowswrapper.h"
#endif

int globalArgC;
char **globalArgV;
int iFirstCommand = 0;

extern bool fQuiet;
bool fRecursive = false;
bool fRegisterShell = false;
bool fUnregisterShell = false;
char strExecuteAtEnd[_MAX_PATH_LEN] = "";

int iResult = 0;

bool EraseItemSafe(const char *szFilename)
{
	return false;
}

void DisplayGroup(C4Group &grp, const char *filter = nullptr)
{
	const C4GroupHeader &head = grp.GetHeader();
	
	printf("Version: %d.%d  ", head.Ver1, head.Ver2);

	uint32_t crc = 0;
	bool crc_valid = GetFileCRC(grp.GetFullName().getData(), &crc);
	if (crc_valid)
		printf("File CRC: %u (%X) ", crc, crc);
	else
		printf("File CRC: <error accessing file> ");

	uint32_t contents_crc = grp.EntryCRC32();
	printf("Contents CRC: %u (%X)\n", contents_crc, contents_crc);

	// Find maximum file name length (matching filter)
	size_t max_fn_len = 0;
	for (const C4GroupEntry *entry = grp.GetFirstEntry(); entry != nullptr; entry = entry->Next)
	{
		if (filter == nullptr || WildcardMatch(filter, entry->FileName))
			max_fn_len = std::max(max_fn_len, strlen(entry->FileName));
	}

	// List files
	size_t file_count = 0;
	size_t byte_count = 0;
	for (const C4GroupEntry *entry = grp.GetFirstEntry(); entry != nullptr; entry = entry->Next)
	{
		if (filter != nullptr && !WildcardMatch(filter, entry->FileName))
			continue;

		printf("%*s %8u Bytes",
			int(max_fn_len),
			entry->FileName,
			entry->Size);
		if (entry->ChildGroup != 0)
			printf(" (Group)");
		if (entry->Executable != 0)
			printf(" (Executable)");
		printf("\n");

		++file_count;
		byte_count += entry->Size;
	}
	printf("%lu Entries, %lu Bytes\n", (unsigned long)file_count, (unsigned long)byte_count);
}

void PrintGroupInternals(C4Group &grp, int indent_level = 0)
{
	const C4GroupHeader &head = grp.GetHeader();
	int indent = indent_level * 4;

	printf("%*sHead.id: '%s'\n", indent, "", head.Id);
	printf("%*sHead.Ver1: %d\n", indent, "", head.Ver1);
	printf("%*sHead.Ver2: %d\n", indent, "", head.Ver2);
	printf("%*sHead.Entries: %d\n", indent, "", head.Entries);
	for (const C4GroupEntry * p = grp.GetFirstEntry(); p; p = p->Next)
	{
		printf("%*sEntry '%s':\n", indent, "", p->FileName);
		printf("%*s  Packed: %d\n", indent, "", p->Packed);
		printf("%*s  ChildGroup: %d\n", indent, "", p->ChildGroup);
		printf("%*s  Size: %d\n", indent, "", p->Size);
		printf("%*s  Offset: %d\n", indent, "", p->Offset);
		printf("%*s  Executable: %d\n", indent, "", p->Executable);
		if (p->ChildGroup != 0)
		{
			C4Group hChildGroup;
			if (hChildGroup.OpenAsChild(&grp, p->FileName))
				PrintGroupInternals(hChildGroup, indent_level + 1);
		}
	}
}


bool ProcessGroup(const char *FilenamePar)
{
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
	if (hGroup.Open(szFilename, true))
	{
		// No commands: display contents
		if (iFirstCommand >= argc)
		{
			DisplayGroup(hGroup);
		}
		// Process commands
		else
		{
			for (int iArg = iFirstCommand; iArg < argc; ++iArg)
			{
				// This argument is a command
				if (argv[iArg][0] == '-')
				{
					// Handle commands
					switch (argv[iArg][1])
					{
						// Sort
					case 's':
						// First sort parameter overrides default Clonk sort list
						C4Group_SetSortList(nullptr);
						// Missing argument
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
						{
							fprintf(stderr, "Missing argument for sort command\n");
						}
						// Sort, advance to next argument
						else
						{
							hGroup.Sort(argv[iArg + 1]);
							iArg++;
						}
						break;
						// View
					case 'l':
						if ((iArg + 1 >= argc) || (argv[iArg + 1][0] == '-'))
						{
							DisplayGroup(hGroup);
						}
						else
						{
							DisplayGroup(hGroup, argv[iArg + 1]);
							iArg++;
						}
						break;
						// Pack
					case 'p':
						Log("Packing...");
						// Close
						if (!hGroup.Close())
						{
							fprintf(stderr, "Closing failed: %s\n", hGroup.GetError());
						}
						// Pack
						else if (!C4Group_PackDirectory(szFilename))
						{
							fprintf(stderr, "Pack failed\n");
						}
						// Reopen
						else if (!hGroup.Open(szFilename))
						{
							fprintf(stderr, "Reopen failed: %s\n", hGroup.GetError());
						}
						break;
						// Pack To
					case 't':
						if ((iArg + 1 >= argc))
						{
							fprintf(stderr, "Pack failed: too few arguments\n");
							break;
						}
						++iArg;
						Log("Packing...");
						// Close
						if (!hGroup.Close())
						{
							fprintf(stderr, "Closing failed: %s\n", hGroup.GetError());
						}
						else if (!EraseItem(argv[iArg]))
						{
							fprintf(stderr, "Destination Clear failed\n");
							break;
						}
						// Pack
						else if (!C4Group_PackDirectoryTo(szFilename, argv[iArg]))
						{
							fprintf(stderr, "Pack failed\n");
							break;
						}
						free(szFilename);
						szFilename = strdup(argv[iArg]);
						// Reopen
						if (!hGroup.Open(szFilename))
						{
							fprintf(stderr, "Reopen failed: %s\n", hGroup.GetError());
						}
						break;
						// Unpack
					case 'u':
						LogF("Unpacking...");
						// Close
						if (!hGroup.Close())
						{
							fprintf(stderr, "Closing failed: %s\n", hGroup.GetError());
						}
						// Pack
						else if (!C4Group_UnpackDirectory(szFilename))
						{
							fprintf(stderr, "Unpack failed\n");
						}
						// Reopen
						else if (!hGroup.Open(szFilename))
						{
							fprintf(stderr, "Reopen failed: %s\n", hGroup.GetError());
						}
						break;
						// Unpack
					case 'x':
						Log("Exploding...");
						// Close
						if (!hGroup.Close())
						{
							fprintf(stderr, "Closing failed: %s\n", hGroup.GetError());
						}
						// Pack
						else if (!C4Group_ExplodeDirectory(szFilename))
						{
							fprintf(stderr, "Unpack failed\n");
						}
						// Reopen
						else if (!hGroup.Open(szFilename))
						{
							fprintf(stderr, "Reopen failed: %s\n", hGroup.GetError());
						}
						break;
						// Generate update
					case 'g':
						if ((iArg + 3 >= argc) || (argv[iArg + 1][0] == '-')
						    || (argv[iArg + 2][0] == '-')
						    || (argv[iArg + 3][0] == '-'))
						{
							fprintf(stderr, "Update generation failed: too few arguments\n");
						}
						else
						{
							C4UpdatePackage Upd;
							// Close
							if (!hGroup.Close())
							{
								fprintf(stderr, "Closing failed: %s\n", hGroup.GetError());
							}
							// generate
							else if (!Upd.MakeUpdate(argv[iArg + 1], argv[iArg + 2], szFilename, argv[iArg + 3]))
							{
								fprintf(stderr, "Update generation failed.\n");
							}
							// Reopen
							else if (!hGroup.Open(szFilename))
							{
								fprintf(stderr, "Reopen failed: %s\n", hGroup.GetError());
							}
							iArg += 3;
						}
						break;

						// Apply an update
					case 'y':
						{
							Log("Applying update...");
							unsigned long pid = 0;
							bool have_pid = false;

							if(iArg + 1 < argc)
							{
								errno = 0;
								pid = strtoul(argv[iArg+1], nullptr, 10);
								if(errno == 0)
									have_pid = true;
								else
									pid = 0;
							}

							if(C4Group_ApplyUpdate(hGroup, pid))
								{ if (argv[iArg][2]=='d') fDeleteGroup = true; }
							else
								fprintf(stderr,"Update failed.\n");

							if(have_pid) ++iArg;
						}
						break;
					case 'z':
						PrintGroupInternals(hGroup);
						break;
						// Undefined
					default:
						fprintf(stderr, "Unknown command: %s\n", argv[iArg]);
						break;
					}
				}
				else
				{
					fprintf(stderr, "Invalid parameter %s\n", argv[iArg]);
				}

			}
		}
		// Error: output status
		if (!SEqual(hGroup.GetError(), "No Error"))
		{
			fprintf(stderr, "Status: %s\n", hGroup.GetError());
		}
		// Close group file
		if (!hGroup.Close())
		{
			fprintf(stderr, "Closing: %s\n", hGroup.GetError());
		}
		// Delete group file if desired (i.e. after apply update)
		if (fDeleteGroup)
		{
			LogF("Deleting %s...\n", GetFilename(szFilename));
			EraseItem(szFilename);
		}
	}
	// Couldn't open group
	else
	{
		fprintf(stderr, "Status: %s\n", hGroup.GetError());
	}
	free(szFilename);
	// Done
	return true;
}

int RegisterShellExtensions()
{
#ifdef _WIN32
	wchar_t strModule[2048+1];
	wchar_t strCommand[2048+1];
	char strClass[128];
	int i;
	GetModuleFileNameW(nullptr, strModule, 2048);
	// Groups
	const char *strClasses =
	  "Clonk4.Definition;Clonk4.Folder;Clonk4.Group;Clonk4.Player;Clonk4.Scenario;Clonk4.Update;Clonk4.Weblink";
	for (i = 0; SCopySegment(strClasses, i, strClass); i++)
	{
		// Unpack
		_snwprintf(strCommand, 2048, L"\"%s\" \"%%1\" \"-u\"", strModule);
		if (!SetRegShell(GetWideChar(strClass), L"MakeFolder", L"C4Group Unpack", strCommand))
			return 0;
		// Explode
		_snwprintf(strCommand, 2048, L"\"%s\" \"%%1\" \"-x\"", strModule);
		if (!SetRegShell(GetWideChar(strClass), L"ExplodeFolder", L"C4Group Explode", strCommand))
			return 0;
	}
	// Directories
	const char *strClasses2 = "Directory";
	for (i = 0; SCopySegment(strClasses2, i, strClass); i++)
	{
		// Pack
		_snwprintf(strCommand, 2048, L"\"%s\" \"%%1\" \"-p\"", strModule);
		if (!SetRegShell(GetWideChar(strClass), L"MakeGroupFile", L"C4Group Pack", strCommand))
			return 0;
	}
	// Done
#endif
	return 1;
}

int UnregisterShellExtensions()
{
#ifdef _WIN32
	char strClass[128];
	int i;
	// Groups
	const char *strClasses =
	  "Clonk4.Definition;Clonk4.Folder;Clonk4.Group;Clonk4.Player;Clonk4.Scenario;Clonk4.Update;Clonk4.Weblink";
	for (i = 0; SCopySegment(strClasses, i, strClass); i++)
	{
		// Unpack
		if (!RemoveRegShell(strClass, "MakeFolder"))
			return 0;
		// Explode
		if (!RemoveRegShell(strClass, "ExplodeFolder"))
			return 0;
	}
	// Directories
	const char *strClasses2 = "Directory";
	for (i = 0; SCopySegment(strClasses2, i, strClass); i++)
	{
		// Pack
		if (!RemoveRegShell(strClass, "MakeGroupFile"))
			return 0;
	}
	// Done
#endif
	return 1;
}

int main(int argc, char *argv[])
{
#ifndef WIN32
	// Always line buffer mode, even if the output is not sent to a terminal
	setvbuf(stdout, nullptr, _IOLBF, 0);
#endif
	// Scan options
	fQuiet = true;
	int iFirstGroup = 0;
	for (int i = 1; i < argc; ++i)
	{
		// Option encountered
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
				// Quiet mode
			case 'v':
				fQuiet = false;
				break;
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
				// Show licenses
			case 'L':
			{
				fQuiet = false;
				std::string sep{"\n=================================\n"};
				for (const auto& license : OCLicenses)
					if (license.path.substr(0, 7) != "planet/")
						Log((sep + license.path + ": " + license.name + sep + license.content + "\n").c_str());
				return 0;
			}
				// Unknown
			default:
				fprintf(stderr, "Unknown option %s\n", argv[i]);
				break;
			}
		}
		else
		{
			// filename encountered: no more options expected
			iFirstGroup = i;
			break;
		}
	}
	iFirstCommand = iFirstGroup;
	while (iFirstCommand < argc && argv[iFirstCommand][0] != '-')
		++iFirstCommand;

	// Program info
	LogF("OpenClonk C4Group %s", C4VERSION);

	// Init C4Group
	C4Group_SetSortList(C4CFN_FLS);

	// Store command line parameters
	globalArgC = argc;
	globalArgV = argv;

	// Register shell
	if (fRegisterShell)
	{
		if (RegisterShellExtensions())
			printf("Shell extensions registered.\n");
		else
			printf("Error registering shell extensions.\n");
	}
	// Unregister shell
	if (fUnregisterShell)
	{
		if (UnregisterShellExtensions())
			printf("Shell extensions removed.\n");
		else
			printf("Error removing shell extensions.\n");
	}

	// At least one parameter (filename, not option or command): process file(s)
	if (iFirstGroup)
	{
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
		printf("Commands: -l List\n");
		printf("          -x Explode\n");
		printf("          -u Unpack\n");
		printf("          -p Pack\n");
		printf("          -t [filename] Pack To\n");
		printf("          -y [ppid] Apply update (waiting for ppid to terminate first)\n");
		printf("          -g [source] [target] [title] Make update\n");
		printf("          -s Sort\n");
		printf("\n");
		printf("Options:  -v Verbose -r Recursive\n");
		printf("          -i Register shell -u Unregister shell\n");
		printf("          -x:<command> Execute shell command when done\n");
		printf("          -L Show licenses and exit\n");
		printf("\n");
		printf("Examples: c4group pack.ocg -x\n");
		printf("          c4group update.ocu -g ver1.ocf ver2.ocf New_Version\n");
		printf("          c4group -i\n");
	}

	// Execute when done
	if (strExecuteAtEnd[0])
	{
		printf("Executing: %s\n", strExecuteAtEnd);
#ifdef _WIN32

		STARTUPINFOW startInfo;
		ZeroMem(&startInfo, sizeof(startInfo));
		startInfo.cb = sizeof(startInfo);

		PROCESS_INFORMATION procInfo;

		CreateProcessW(GetWideChar(strExecuteAtEnd), nullptr, nullptr, nullptr, false, 0, nullptr, nullptr, &startInfo, &procInfo);
#else
		switch (fork())
		{
			// Error
		case -1:
			fprintf(stderr, "Error forking.\n");
			break;
			// Child process
		case 0:
			execl(strExecuteAtEnd, strExecuteAtEnd, static_cast<char *>(nullptr)); // currently no parameters are passed to the executed program
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
