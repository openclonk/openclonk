/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2005, 2007-2008, 2010  Günther Brammer
 * Copyright (c) 2005, 2008  Peter Wortmann
 * Copyright (c) 2005  Sven Eberhardt
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2007  Julian Raschke
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2011  Nicolas Hake
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

// Crash handler, Win32 version

#include "C4Include.h"

#ifdef GENERATE_MINI_DUMP

// Dump generation on crash
#include <dbghelp.h>
#include <fcntl.h>
#include <string.h>
#include <tlhelp32.h>

static bool FirstCrash = true;

namespace {
#define OC_MACHINE_UNKNOWN 0x0
#define OC_MACHINE_X86     0x1
#define OC_MACHINE_X64     0x2
#if defined(_M_X64) || defined(__amd64)
#	define OC_MACHINE OC_MACHINE_X64
#elif defined(_M_IX86) || defined(__i386__)
#	define OC_MACHINE OC_MACHINE_X86
#else
#	define OC_MACHINE OC_MACHINE_UNKNOWN
#endif

	const size_t DumpBufferSize = 2048;
	char DumpBuffer[DumpBufferSize];
	char SymbolBuffer[DumpBufferSize];
	// Dump crash info in a human readable format. Uses a static buffer to avoid heap allocations
	// from an exception handler. For the same reason, this also doesn't use Log/LogF etc.
	void SafeTextDump(LPEXCEPTION_POINTERS exc, int fd)
	{
#if defined(_MSC_VER)
#	define LOG_SNPRINTF _snprintf
#else
#	define LOG_SNPRINTF snprintf
#endif
#define LOG_STATIC_TEXT(text) write(fd, text, sizeof(text) - 1)
#define LOG_DYNAMIC_TEXT(...) write(fd, DumpBuffer, LOG_SNPRINTF(DumpBuffer, DumpBufferSize-1, __VA_ARGS__))
#if OC_MACHINE == OC_MACHINE_X64
#	if defined(_MSC_VER)
#		define POINTER_FORMAT "0x%016Ix"
#	elif defined(__GNUC__)
#		define POINTER_FORMAT "0x%016lx"
#	else
#		define POINTER_FORMAT "0x%016p"
#	endif
#elif OC_MACHINE == OC_MACHINE_X86
#	define POINTER_FORMAT "0x%08x"
#else
#	define POINTER_FORMAT "0x%p"
#endif
		
		// Log exception type
		switch (exc->ExceptionRecord->ExceptionCode)
		{
#define LOG_EXCEPTION(code, text) case code: LOG_STATIC_TEXT(#code ": " text "\n"); break
		LOG_EXCEPTION(EXCEPTION_ACCESS_VIOLATION,         "The thread tried to read from or write to a virtual address for which it does not have the appropriate access.");
		LOG_EXCEPTION(EXCEPTION_ILLEGAL_INSTRUCTION,      "The thread tried to execute an invalid instruction.");
		LOG_EXCEPTION(EXCEPTION_IN_PAGE_ERROR,            "The thread tried to access a page that was not present, and the system was unable to load the page.");
		LOG_EXCEPTION(EXCEPTION_NONCONTINUABLE_EXCEPTION, "The thread tried to continue execution after a noncontinuable exception occurred.");
		LOG_EXCEPTION(EXCEPTION_PRIV_INSTRUCTION,         "The thread tried to execute an instruction whose operation is not allowed in the current machine mode.");
		LOG_EXCEPTION(EXCEPTION_STACK_OVERFLOW,           "The thread used up its stack.");
		LOG_EXCEPTION(EXCEPTION_GUARD_PAGE,               "The thread accessed memory allocated with the PAGE_GUARD modifier.");
#undef LOG_EXCEPTION
		default:
			LOG_DYNAMIC_TEXT("%#08x: The thread raised an unknown exception.\n", exc->ExceptionRecord->ExceptionCode);
			break;
		}
		if (exc->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE)
			LOG_STATIC_TEXT("This is a non-continuable exception.\n");
		else
			LOG_STATIC_TEXT("This is a continuable exception.\n");

		// For some exceptions, there is a defined meaning to the ExceptionInformation field
		switch (exc->ExceptionRecord->ExceptionCode)
		{
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_IN_PAGE_ERROR:
			if (exc->ExceptionRecord->NumberParameters < 2)
			{
				LOG_STATIC_TEXT("Additional information for the exception was not provided.\n");
				break;
			}
			LOG_STATIC_TEXT("Additional information for the exception: The thread ");
			switch (exc->ExceptionRecord->ExceptionInformation[0])
			{
#ifndef EXCEPTION_READ_FAULT
#	define EXCEPTION_READ_FAULT 0
#	define EXCEPTION_WRITE_FAULT 1
#	define EXCEPTION_EXECUTE_FAULT 8
#endif
			case EXCEPTION_READ_FAULT: LOG_STATIC_TEXT("tried to read from memory"); break;
			case EXCEPTION_WRITE_FAULT: LOG_STATIC_TEXT("tried to write to memory"); break;
			case EXCEPTION_EXECUTE_FAULT: LOG_STATIC_TEXT("caused an user-mode DEP violation"); break;
			default: LOG_DYNAMIC_TEXT("tried to access (%#x) memory", exc->ExceptionRecord->ExceptionInformation[0]); break;
			}
			LOG_DYNAMIC_TEXT(" at address " POINTER_FORMAT ".\n", exc->ExceptionRecord->ExceptionInformation[1]);
			if (exc->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
			{
				if (exc->ExceptionRecord->NumberParameters >= 3)
					LOG_DYNAMIC_TEXT("The NTSTATUS code that resulted in this exception was " POINTER_FORMAT ".\n", exc->ExceptionRecord->ExceptionInformation[2]);
				else
					LOG_STATIC_TEXT("The NTSTATUS code that resulted in this exception was not provided.\n");
			}
			break;
		}

		// Dump registers
#if OC_MACHINE == OC_MACHINE_X64
		LOG_STATIC_TEXT("\nProcessor registers (x86_64):\n");
		LOG_DYNAMIC_TEXT("RAX: " POINTER_FORMAT ", RBX: " POINTER_FORMAT ", RCX: " POINTER_FORMAT ", RDX: " POINTER_FORMAT "\n",
			exc->ContextRecord->Rax, exc->ContextRecord->Rbx, exc->ContextRecord->Rcx, exc->ContextRecord->Rdx);
		LOG_DYNAMIC_TEXT("RBP: " POINTER_FORMAT ", RSI: " POINTER_FORMAT ", RDI: " POINTER_FORMAT ",  R8: " POINTER_FORMAT "\n",
			exc->ContextRecord->Rbp, exc->ContextRecord->Rsi, exc->ContextRecord->Rdi, exc->ContextRecord->R8);
		LOG_DYNAMIC_TEXT(" R9: " POINTER_FORMAT ", R10: " POINTER_FORMAT ", R11: " POINTER_FORMAT ", R12: " POINTER_FORMAT "\n",
			exc->ContextRecord->R9, exc->ContextRecord->R10, exc->ContextRecord->R11, exc->ContextRecord->R12);
		LOG_DYNAMIC_TEXT("R13: " POINTER_FORMAT ", R14: " POINTER_FORMAT ", R15: " POINTER_FORMAT "\n",
			exc->ContextRecord->R13, exc->ContextRecord->R14, exc->ContextRecord->R15);
		LOG_DYNAMIC_TEXT("RSP: " POINTER_FORMAT ", RIP: " POINTER_FORMAT "\n",
			exc->ContextRecord->Rsp, exc->ContextRecord->Rip);
#elif OC_MACHINE == OC_MACHINE_X86
		LOG_STATIC_TEXT("\nProcessor registers (x86):\n");
		LOG_DYNAMIC_TEXT("EAX: " POINTER_FORMAT ", EBX: " POINTER_FORMAT ", ECX: " POINTER_FORMAT ", EDX: " POINTER_FORMAT "\n",
			exc->ContextRecord->Eax, exc->ContextRecord->Ebx, exc->ContextRecord->Ecx, exc->ContextRecord->Edx);
		LOG_DYNAMIC_TEXT("ESI: " POINTER_FORMAT ", EDI: " POINTER_FORMAT "\n",
			exc->ContextRecord->Esi, exc->ContextRecord->Edi);
		LOG_DYNAMIC_TEXT("EBP: " POINTER_FORMAT ", ESP: " POINTER_FORMAT ", EIP: " POINTER_FORMAT "\n",
			exc->ContextRecord->Ebp, exc->ContextRecord->Esp, exc->ContextRecord->Eip);
#endif
#if OC_MACHINE == OC_MACHINE_X64 || OC_MACHINE == OC_MACHINE_X86
		LOG_DYNAMIC_TEXT("EFLAGS: " POINTER_FORMAT "(%c%c%c%c%c%c%c)\n", exc->ContextRecord->EFlags,
			exc->ContextRecord->EFlags & 0x800 ? 'O' : '.',
			exc->ContextRecord->EFlags & 0x400 ? 'D' : '.',
			exc->ContextRecord->EFlags &  0x80 ? 'S' : '.',
			exc->ContextRecord->EFlags &  0x40 ? 'Z' : '.',
			exc->ContextRecord->EFlags &  0x10 ? 'A' : '.',
			exc->ContextRecord->EFlags &   0x4 ? 'P' : '.',
			exc->ContextRecord->EFlags &   0x1 ? 'C' : '.');
#endif

		// Dump stack
		LOG_STATIC_TEXT("\nStack contents:\n");
		MEMORY_BASIC_INFORMATION stack_info;
		intptr_t stack_pointer = 
#if OC_MACHINE == OC_MACHINE_X64
			exc->ContextRecord->Rsp
#elif OC_MACHINE == OC_MACHINE_X86
			exc->ContextRecord->Esp
#endif
			;
		if (VirtualQuery(reinterpret_cast<LPCVOID>(stack_pointer), &stack_info, sizeof(stack_info)))
		{
			intptr_t stack_base = reinterpret_cast<intptr_t>(stack_info.BaseAddress);
			intptr_t dump_min = std::max<intptr_t>(stack_base, (stack_pointer - 256) & ~0xF);
			intptr_t dump_max = std::min<intptr_t>(stack_base + stack_info.RegionSize, (stack_pointer + 256) | 0xF);

			for (intptr_t dump_row_base = dump_min & ~0xF; dump_row_base < dump_max; dump_row_base += 0x10)
			{
				LOG_DYNAMIC_TEXT(POINTER_FORMAT ": ", dump_row_base);
				// Hex dump
				for (intptr_t dump_row_cursor = dump_row_base; dump_row_cursor < dump_row_base + 16; ++dump_row_cursor)
				{
					if (dump_row_cursor < dump_min || dump_row_cursor > dump_max)
						LOG_STATIC_TEXT("   ");
					else
						LOG_DYNAMIC_TEXT("%02x ", (unsigned int)(unsigned char)*reinterpret_cast<char*>(dump_row_cursor)); // Safe, since it's inside the VM of our process
				}
				LOG_STATIC_TEXT("   ");
				// Text dump
				for (intptr_t dump_row_cursor = dump_row_base; dump_row_cursor < dump_row_base + 16; ++dump_row_cursor)
				{
					if (dump_row_cursor < dump_min || dump_row_cursor > dump_max)
						LOG_STATIC_TEXT(" ");
					else
					{
						char c = *reinterpret_cast<char*>(dump_row_cursor); // Safe, since it's inside the VM of our process
						if (c < 0x20 || (c > 0x7e && c < 0xa1))
							LOG_STATIC_TEXT(".");
						else
							LOG_DYNAMIC_TEXT("%c", c);
					}
				}
				LOG_STATIC_TEXT("\n");
			}
		}
		else
		{
			LOG_STATIC_TEXT("[Failed to access stack memory]\n");
		}

		// Initialize DbgHelp.dll symbol functions
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
		HANDLE process = GetCurrentProcess();
		if (SymInitialize(process, 0, true))
		{
			LOG_STATIC_TEXT("\nStack trace:\n");
			STACKFRAME64 frame = {0};
			DWORD image_type;
			CONTEXT context = *exc->ContextRecord;
			// Setup frame info
			frame.AddrPC.Mode = AddrModeFlat;
			frame.AddrStack.Mode = AddrModeFlat;
			frame.AddrFrame.Mode = AddrModeFlat;
#if OC_MACHINE == OC_MACHINE_X64
			image_type = IMAGE_FILE_MACHINE_AMD64;
			frame.AddrPC.Offset = context.Rip;
			frame.AddrStack.Offset = context.Rsp;
			// Some compilers use rdi for their frame pointer instead. Let's hope they're in the minority.
			frame.AddrFrame.Offset = context.Rbp;
#elif OC_MACHINE == OC_MACHINE_X86
			image_type = IMAGE_FILE_MACHINE_I386;
			frame.AddrPC.Offset = context.Eip;
			frame.AddrStack.Offset = context.Esp;
			frame.AddrFrame.Offset = context.Ebp;
#endif
			// Dump stack trace
			SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO*>(SymbolBuffer);
			static_assert(DumpBufferSize >= sizeof(*symbol), "SYMBOL_INFO too large to fit into buffer");
			IMAGEHLP_MODULE64 *module = reinterpret_cast<IMAGEHLP_MODULE64*>(SymbolBuffer);
			static_assert(DumpBufferSize >= sizeof(*module), "IMAGEHLP_MODULE64 too large to fit into buffer");
			IMAGEHLP_LINE64 *line = reinterpret_cast<IMAGEHLP_LINE64*>(SymbolBuffer);
			static_assert(DumpBufferSize >= sizeof(*line), "IMAGEHLP_LINE64 too large to fit into buffer");
			int frame_number = 0;
			while (StackWalk64(image_type, process, GetCurrentThread(), &frame, &context, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0))
			{
				LOG_DYNAMIC_TEXT("#%3d ", frame_number);
				if (SymGetModuleInfo64(process, frame.AddrPC.Offset, module))
				{
					LOG_DYNAMIC_TEXT("%s!", module->ImageName);
				}
				DWORD64 disp64;
				symbol->MaxNameLen = DumpBufferSize - sizeof(*symbol);
				if (SymFromAddr(process, frame.AddrPC.Offset, &disp64, symbol))
				{
					LOG_DYNAMIC_TEXT("%s + %#lx bytes", symbol->Name, static_cast<long>(disp64));
				}
				else
				{
					LOG_DYNAMIC_TEXT("[" POINTER_FORMAT "]", frame.AddrPC.Offset);
				}
				DWORD disp;
				if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &disp, line))
				{
					LOG_DYNAMIC_TEXT(" (%s Line %d + %#lx bytes)", line->FileName, line->LineNumber, static_cast<long>(disp));
				}
				LOG_STATIC_TEXT("\n");
				++frame_number;
			}
			SymCleanup(process);
		}
		else
		{
			LOG_STATIC_TEXT("[Stack trace not available: failed to initialize Debugging Help Library]\n");
		}

		// Dump loaded modules
		HANDLE snapshot;
		while((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0)) == INVALID_HANDLE_VALUE)
			if (GetLastError() != ERROR_BAD_LENGTH) break;
		if (snapshot != INVALID_HANDLE_VALUE)
		{
			LOG_STATIC_TEXT("\nLoaded modules:\n");
			MODULEENTRY32 *module = reinterpret_cast<MODULEENTRY32*>(SymbolBuffer);
			static_assert(DumpBufferSize >= sizeof(*module), "MODULEENTRY32 too large to fit into buffer");
			module->dwSize = sizeof(*module);
			for (BOOL success = Module32First(snapshot, module); success; success = Module32Next(snapshot, module))
			{
				LOG_DYNAMIC_TEXT("%32s loaded at " POINTER_FORMAT " - " POINTER_FORMAT " (%s)\n", module->szModule,
					module->modBaseAddr, module->modBaseAddr + module->modBaseSize, module->szExePath);
			}
			CloseHandle(snapshot);
		}

		// (Try to) log it
		LOG_STATIC_TEXT("FATAL: Clonk crashed! Some developer might be interested in Clonk.dmp...");
#undef LOG_SNPRINTF
#undef LOG_DYNAMIC_TEXT
#undef LOG_STATIC_TEXT
	}
}
LONG WINAPI GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
	if (!FirstCrash) return EXCEPTION_EXECUTE_HANDLER;
	FirstCrash = false;

	// Open dump file
	const char *szFilename = "Clonk.dmp"; // dump to working directory
	HANDLE file = CreateFile(szFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);

	// Write dump (human readable format)
	SafeTextDump(pExceptionPointers, GetLogFD());

	MINIDUMP_EXCEPTION_INFORMATION ExpParam;
	ExpParam.ThreadId = GetCurrentThreadId();
	ExpParam.ExceptionPointers = pExceptionPointers;
	ExpParam.ClientPointers = true;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
	                  file, MiniDumpNormal, &ExpParam, NULL, NULL);
	CloseHandle(file);

	// Pass exception
	return EXCEPTION_EXECUTE_HANDLER;
}

void InstallCrashHandler()
{
	SetUnhandledExceptionFilter(GenerateDump);
}

#else

void InstallCrashHandler()
{
	// no-op
}

#endif // GENERATE_MINI_DUMP
