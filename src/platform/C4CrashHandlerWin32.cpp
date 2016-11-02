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

// Crash handler, Win32 version

#include "C4Include.h"

#ifdef HAVE_DBGHELP

// Dump generation on crash
#include "C4Version.h"
#include "platform/C4windowswrapper.h"
#include <dbghelp.h>
#include <fcntl.h>
#include <string.h>
#include <tlhelp32.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <assert.h>
#if defined(__CRT_WIDE) || (defined(_MSC_VER) && _MSC_VER >= 1900)
#define USE_WIDE_ASSERT
#endif

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
	void SafeTextDump(LPEXCEPTION_POINTERS exc, int fd, const wchar_t *dump_filename)
	{
#if defined(_MSC_VER)
#	define LOG_SNPRINTF _snprintf
#else
#	define LOG_SNPRINTF snprintf
#endif
#define LOG_STATIC_TEXT(text) write(fd, text, sizeof(text) - 1)
#define LOG_DYNAMIC_TEXT(...) write(fd, DumpBuffer, LOG_SNPRINTF(DumpBuffer, DumpBufferSize-1, __VA_ARGS__))

// Figure out which kind of format string will output a pointer in hex
#if defined(PRIdPTR)
#	define POINTER_FORMAT_SUFFIX PRIdPTR
#elif defined(_MSC_VER)
#	define POINTER_FORMAT_SUFFIX "Ix"
#elif defined(__GNUC__)
#	define POINTER_FORMAT_SUFFIX "zx"
#else
#	define POINTER_FORMAT_SUFFIX "p"
#endif
#if OC_MACHINE == OC_MACHINE_X64
#	define POINTER_FORMAT "0x%016" POINTER_FORMAT_SUFFIX
#elif OC_MACHINE == OC_MACHINE_X86
#	define POINTER_FORMAT "0x%08" POINTER_FORMAT_SUFFIX
#else
#	define POINTER_FORMAT "0x%" POINTER_FORMAT_SUFFIX
#endif

#ifndef STATUS_ASSERTION_FAILURE
#	define STATUS_ASSERTION_FAILURE ((DWORD)0xC0000420L)
#endif

		LOG_STATIC_TEXT("**********************************************************************\n");
		LOG_STATIC_TEXT("* UNHANDLED EXCEPTION\n");
		if (OC_BUILD_ID[0] != '\0')
			LOG_STATIC_TEXT("* Build Identifier: " OC_BUILD_ID "\n");
		if (exc->ExceptionRecord->ExceptionCode != STATUS_ASSERTION_FAILURE && dump_filename && dump_filename[0] != L'\0')
		{
			int cch = WideCharToMultiByte(CP_UTF8, 0, dump_filename, -1, SymbolBuffer, sizeof(SymbolBuffer), nullptr, nullptr);
			if (cch > 0)
			{
				LOG_STATIC_TEXT("* A crash dump may have been written to ");
				write(fd, SymbolBuffer, cch - 1);
				LOG_STATIC_TEXT("\n");
				LOG_STATIC_TEXT("* If this file exists, please send it to a developer for investigation.\n");
			}
		}
		LOG_STATIC_TEXT("**********************************************************************\n");
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
		LOG_EXCEPTION(STATUS_ASSERTION_FAILURE,           "The thread specified a pre- or postcondition that did not hold.");
#undef LOG_EXCEPTION
		default:
			LOG_DYNAMIC_TEXT("%#08x: The thread raised an unknown exception.\n", static_cast<unsigned int>(exc->ExceptionRecord->ExceptionCode));
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
			default: LOG_DYNAMIC_TEXT("tried to access (%#x) memory", static_cast<unsigned int>(exc->ExceptionRecord->ExceptionInformation[0])); break;
			}
			LOG_DYNAMIC_TEXT(" at address " POINTER_FORMAT ".\n", static_cast<size_t>(exc->ExceptionRecord->ExceptionInformation[1]));
			if (exc->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
			{
				if (exc->ExceptionRecord->NumberParameters >= 3)
					LOG_DYNAMIC_TEXT("The NTSTATUS code that resulted in this exception was " POINTER_FORMAT ".\n", static_cast<size_t>(exc->ExceptionRecord->ExceptionInformation[2]));
				else
					LOG_STATIC_TEXT("The NTSTATUS code that resulted in this exception was not provided.\n");
			}
			break;

		case STATUS_ASSERTION_FAILURE:
			if (exc->ExceptionRecord->NumberParameters < 3)
			{
				LOG_STATIC_TEXT("Additional information for the exception was not provided.\n");
				break;
			}
#ifdef USE_WIDE_ASSERT
#	define ASSERTION_INFO_FORMAT "%ls"
#	define ASSERTION_INFO_TYPE wchar_t *
#else
#	define ASSERTION_INFO_FORMAT "%s"
#	define ASSERTION_INFO_TYPE char *
#endif
			LOG_DYNAMIC_TEXT("Additional information for the exception:\n    Assertion that failed: " ASSERTION_INFO_FORMAT "\n    File: " ASSERTION_INFO_FORMAT "\n    Line: %d\n",
				reinterpret_cast<ASSERTION_INFO_TYPE>(exc->ExceptionRecord->ExceptionInformation[0]),
				reinterpret_cast<ASSERTION_INFO_TYPE>(exc->ExceptionRecord->ExceptionInformation[1]),
				(int) exc->ExceptionRecord->ExceptionInformation[2]);
			break;
		}

		// Dump registers
#if OC_MACHINE == OC_MACHINE_X64
		LOG_STATIC_TEXT("\nProcessor registers (x86_64):\n");
		LOG_DYNAMIC_TEXT("RAX: " POINTER_FORMAT ", RBX: " POINTER_FORMAT ", RCX: " POINTER_FORMAT ", RDX: " POINTER_FORMAT "\n",
			static_cast<size_t>(exc->ContextRecord->Rax), static_cast<size_t>(exc->ContextRecord->Rbx),
			static_cast<size_t>(exc->ContextRecord->Rcx), static_cast<size_t>(exc->ContextRecord->Rdx));
		LOG_DYNAMIC_TEXT("RBP: " POINTER_FORMAT ", RSI: " POINTER_FORMAT ", RDI: " POINTER_FORMAT ",  R8: " POINTER_FORMAT "\n",
			static_cast<size_t>(exc->ContextRecord->Rbp), static_cast<size_t>(exc->ContextRecord->Rsi),
			static_cast<size_t>(exc->ContextRecord->Rdi), static_cast<size_t>(exc->ContextRecord->R8));
		LOG_DYNAMIC_TEXT(" R9: " POINTER_FORMAT ", R10: " POINTER_FORMAT ", R11: " POINTER_FORMAT ", R12: " POINTER_FORMAT "\n",
			static_cast<size_t>(exc->ContextRecord->R9), static_cast<size_t>(exc->ContextRecord->R10),
			static_cast<size_t>(exc->ContextRecord->R11), static_cast<size_t>(exc->ContextRecord->R12));
		LOG_DYNAMIC_TEXT("R13: " POINTER_FORMAT ", R14: " POINTER_FORMAT ", R15: " POINTER_FORMAT "\n",
			static_cast<size_t>(exc->ContextRecord->R13), static_cast<size_t>(exc->ContextRecord->R14),
			static_cast<size_t>(exc->ContextRecord->R15));
		LOG_DYNAMIC_TEXT("RSP: " POINTER_FORMAT ", RIP: " POINTER_FORMAT "\n",
			static_cast<size_t>(exc->ContextRecord->Rsp), static_cast<size_t>(exc->ContextRecord->Rip));
#elif OC_MACHINE == OC_MACHINE_X86
		LOG_STATIC_TEXT("\nProcessor registers (x86):\n");
		LOG_DYNAMIC_TEXT("EAX: " POINTER_FORMAT ", EBX: " POINTER_FORMAT ", ECX: " POINTER_FORMAT ", EDX: " POINTER_FORMAT "\n",
			static_cast<size_t>(exc->ContextRecord->Eax), static_cast<size_t>(exc->ContextRecord->Ebx),
			static_cast<size_t>(exc->ContextRecord->Ecx), static_cast<size_t>(exc->ContextRecord->Edx));
		LOG_DYNAMIC_TEXT("ESI: " POINTER_FORMAT ", EDI: " POINTER_FORMAT "\n",
			static_cast<size_t>(exc->ContextRecord->Esi), static_cast<size_t>(exc->ContextRecord->Edi));
		LOG_DYNAMIC_TEXT("EBP: " POINTER_FORMAT ", ESP: " POINTER_FORMAT ", EIP: " POINTER_FORMAT "\n",
			static_cast<size_t>(exc->ContextRecord->Ebp), static_cast<size_t>(exc->ContextRecord->Esp),
			static_cast<size_t>(exc->ContextRecord->Eip));
#endif
#if OC_MACHINE == OC_MACHINE_X64 || OC_MACHINE == OC_MACHINE_X86
		LOG_DYNAMIC_TEXT("EFLAGS: 0x%08x (%c%c%c%c%c%c%c)\n", static_cast<unsigned int>(exc->ContextRecord->EFlags),
			exc->ContextRecord->EFlags & 0x800 ? 'O' : '.',	// overflow
			exc->ContextRecord->EFlags & 0x400 ? 'D' : '.',	// direction
			exc->ContextRecord->EFlags &  0x80 ? 'S' : '.',	// sign
			exc->ContextRecord->EFlags &  0x40 ? 'Z' : '.',	// zero
			exc->ContextRecord->EFlags &  0x10 ? 'A' : '.',	// auxiliary carry
			exc->ContextRecord->EFlags &   0x4 ? 'P' : '.',	// parity
			exc->ContextRecord->EFlags &   0x1 ? 'C' : '.');	// carry
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
						LOG_DYNAMIC_TEXT("%02x ", (unsigned int)*reinterpret_cast<unsigned char*>(dump_row_cursor)); // Safe, since it's inside the VM of our process
				}
				LOG_STATIC_TEXT("   ");
				// Text dump
				for (intptr_t dump_row_cursor = dump_row_base; dump_row_cursor < dump_row_base + 16; ++dump_row_cursor)
				{
					if (dump_row_cursor < dump_min || dump_row_cursor > dump_max)
						LOG_STATIC_TEXT(" ");
					else
					{
						unsigned char c = *reinterpret_cast<unsigned char*>(dump_row_cursor); // Safe, since it's inside the VM of our process
						if (c < 0x20 || (c > 0x7e && c < 0xa1))
							LOG_STATIC_TEXT(".");
						else
							LOG_DYNAMIC_TEXT("%c", static_cast<char>(c));
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
			auto frame = STACKFRAME64();
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
				module->SizeOfStruct = sizeof(*module);
				DWORD64 image_base = 0;
				if (SymGetModuleInfo64(process, frame.AddrPC.Offset, module))
				{
					LOG_DYNAMIC_TEXT("%s", module->ModuleName);
					image_base = module->BaseOfImage;
				}
				DWORD64 disp64;
				symbol->MaxNameLen = DumpBufferSize - sizeof(*symbol);
				symbol->SizeOfStruct = sizeof(*symbol);
				if (SymFromAddr(process, frame.AddrPC.Offset, &disp64, symbol))
				{
					LOG_DYNAMIC_TEXT("!%s+%#lx", symbol->Name, static_cast<long>(disp64));
				}
				else if (image_base > 0)
				{
					LOG_DYNAMIC_TEXT("+%#lx", static_cast<long>(frame.AddrPC.Offset - image_base));
				}
				else
				{
					LOG_DYNAMIC_TEXT("%#lx", static_cast<long>(frame.AddrPC.Offset));
				}
				DWORD disp;
				line->SizeOfStruct = sizeof(*line);
				if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &disp, line))
				{
					LOG_DYNAMIC_TEXT(" [%s @ %u]", line->FileName, static_cast<unsigned int>(line->LineNumber));
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
				LOG_DYNAMIC_TEXT("%32ls loaded at " POINTER_FORMAT " - " POINTER_FORMAT " (%ls)\n", module->szModule,
					reinterpret_cast<size_t>(module->modBaseAddr), reinterpret_cast<size_t>(module->modBaseAddr + module->modBaseSize),
					module->szExePath);
			}
			CloseHandle(snapshot);
		}
#undef POINTER_FORMAT_SUFFIX
#undef POINTER_FORMAT
#undef LOG_SNPRINTF
#undef LOG_DYNAMIC_TEXT
#undef LOG_STATIC_TEXT
	}
}
LONG WINAPI GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
	enum
	{
		MDST_BuildId = LastReservedStream + 1
	};

	if (!FirstCrash) return EXCEPTION_EXECUTE_HANDLER;
	FirstCrash = false;

	// Open dump file
	// Work on the assumption that the config isn't corrupted
	wchar_t *filename = reinterpret_cast<wchar_t*>(DumpBuffer);
	const size_t filename_buffer_size = DumpBufferSize / sizeof(wchar_t);
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, ::Config.General.UserDataPath, strnlen(::Config.General.UserDataPath, sizeof(::Config.General.UserDataPath)), filename, filename_buffer_size))
	{
		// Conversion failed; the likely reason for this is a corrupted config.
		assert (GetLastError() == ERROR_NO_UNICODE_TRANSLATION);
		// Fall back to the temporary files directory to write dump.
		DWORD temp_size = GetTempPath(filename_buffer_size, filename);
		if (temp_size == 0 || temp_size > filename_buffer_size)
		{
			// Getting the temp path failed as well; dump to current working directory as a last resort.
			temp_size = GetCurrentDirectory(filename_buffer_size, filename);
			if (temp_size == 0 || temp_size > filename_buffer_size)
			{
				// We don't really have any directory where we can store the dump, so just
				// write the text log (we already have a FD for that)
				filename[0] = L'\0';
			}
		}
	}
	HANDLE file = INVALID_HANDLE_VALUE;

	if (filename[0] != L'\0')
	{
		// There is some path where we want to store our data
		const wchar_t tmpl[] = TEXT(C4ENGINENICK) L"-crash-YYYY-MM-DD-HH-MM-SS.dmp";
		size_t path_len = wcslen(filename);
		if (path_len + sizeof(tmpl) / sizeof(*tmpl) > filename_buffer_size)
		{
			// Somehow the length of the required path is too long to fit in
			// our buffer. Don't dump anything then.
			filename[0] = L'\0';
		}
		else
		{
			// Make sure the path ends in a backslash.
			if (filename[path_len - 1] != L'\\')
			{
				filename[path_len] = L'\\';
				filename[++path_len] = L'\0';
			}
			SYSTEMTIME st;
			GetSystemTime(&st);
			wsprintf(&filename[path_len], L"%s-crash-%04d-%02d-%02d-%02d-%02d-%02d.dmp",
				TEXT(C4ENGINENICK), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		}
	}

	if (filename[0] != L'\0')
	{
		file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		// If we can't create a *new* file to dump into, don't dump at all.
		if (file == INVALID_HANDLE_VALUE)
			filename[0] = L'\0';
	}

	// Write dump (human readable format)
	if (GetLogFD() != -1)
		SafeTextDump(pExceptionPointers, GetLogFD(), filename);

	if (file != INVALID_HANDLE_VALUE)
	{
		auto user_stream_info = MINIDUMP_USER_STREAM_INFORMATION();
		auto user_stream = MINIDUMP_USER_STREAM();
		char build_id[] = OC_BUILD_ID;
		if (OC_BUILD_ID[0] != '\0')
		{
			user_stream.Type = MDST_BuildId;
			user_stream.Buffer = build_id;
			user_stream.BufferSize = sizeof(build_id) - 1;	// don't need the terminating NUL
			user_stream_info.UserStreamCount = 1;
			user_stream_info.UserStreamArray = &user_stream;
		}

		MINIDUMP_EXCEPTION_INFORMATION ExpParam;
		ExpParam.ThreadId = GetCurrentThreadId();
		ExpParam.ExceptionPointers = pExceptionPointers;
		ExpParam.ClientPointers = true;
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
							file, MiniDumpNormal, &ExpParam, &user_stream_info, nullptr);
		CloseHandle(file);
	}

	// Pass exception
	return EXCEPTION_EXECUTE_HANDLER;
}

#ifndef NDEBUG
namespace {
	// Assertion logging hook. This will replace the prologue of the standard assertion
	// handler with a trampoline to assertion_handler(), which logs the assertion, then
	// replaces the trampoline with the original prologue, and calls the handler.
	// If the standard handler returns control to assertion_handler(), it will then
	// restore the hook.
#ifdef USE_WIDE_ASSERT
	typedef void (__cdecl *ASSERT_FUNC)(const wchar_t *, const wchar_t *, unsigned);
	const ASSERT_FUNC assert_func = 
		&_wassert;
#else
	typedef void (__cdecl *ASSERT_FUNC)(const char *, const char *, int);
	const ASSERT_FUNC assert_func = 
		&_assert;
#endif
	unsigned char trampoline[] = {
#if OC_MACHINE == OC_MACHINE_X64
		// MOV rax, 0xCCCCCCCCCCCCCCCC
		0x48 /* REX.W */, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
		// JMP rax
		0xFF, 0xE0
#elif OC_MACHINE == OC_MACHINE_X86
		// NOP ; to align jump target
		0x90,
		// MOV eax, 0xCCCCCCCC
		0xB8, 0xCC, 0xCC, 0xCC, 0xCC,
		// JMP eax
		0xFF, 0xE0
#endif
	};
	unsigned char trampoline_backup[sizeof(trampoline)];
	void HookAssert(ASSERT_FUNC hook)
	{
		// Write hook function address to trampoline
		memcpy(trampoline + 2, (void*)&hook, sizeof(void*));
		// Make target location writable
		DWORD old_protect = 0;
		if (!VirtualProtect((LPVOID)assert_func, sizeof(trampoline), PAGE_EXECUTE_READWRITE, &old_protect))
			return;
		// Take backup of old target function and replace it with trampoline
		memcpy(trampoline_backup, (void*)assert_func, sizeof(trampoline_backup));
		memcpy((void*)assert_func, trampoline, sizeof(trampoline));
		// Restore memory protection
		VirtualProtect((LPVOID)assert_func, sizeof(trampoline), old_protect, &old_protect);
		// Flush processor caches. Not strictly necessary on x86 and x64.
		FlushInstructionCache(GetCurrentProcess(), (LPCVOID)assert_func, sizeof(trampoline));
	}
	void UnhookAssert()
	{
		DWORD old_protect = 0;
		if (!VirtualProtect((LPVOID)assert_func, sizeof(trampoline_backup), PAGE_EXECUTE_READWRITE, &old_protect))
			// Couldn't make assert function writable. Abort program (it's what assert() is supposed to do anyway).
			abort();
		// Replace function with backup
		memcpy((void*)assert_func, trampoline_backup, sizeof(trampoline_backup));
		VirtualProtect((LPVOID)assert_func, sizeof(trampoline_backup), old_protect, &old_protect);
		FlushInstructionCache(GetCurrentProcess(), (LPCVOID)assert_func, sizeof(trampoline_backup));
	}

	struct dump_thread_t {
		HANDLE thread;
#ifdef USE_WIDE_ASSERT
		const wchar_t
#else
		const char
#endif
			*expression, *file;
		size_t line;
	};
	// Helper function to get a valid thread context for the main thread
	static DWORD WINAPI dump_thread(LPVOID t)
	{
		dump_thread_t *data = static_cast<dump_thread_t*>(t);

		// Stop calling thread so we can take a snapshot
		if (SuspendThread(data->thread) == -1)
			return FALSE;

		// Get thread info
		auto ctx = CONTEXT();
#ifndef CONTEXT_ALL
#define CONTEXT_ALL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | \
	CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS | CONTEXT_EXTENDED_REGISTERS)
#endif
		ctx.ContextFlags = CONTEXT_ALL;
		BOOL result = GetThreadContext(data->thread, &ctx);

		// Setup a fake exception to log
		auto erec = EXCEPTION_RECORD();
		erec.ExceptionCode = STATUS_ASSERTION_FAILURE;
		erec.ExceptionFlags = 0L;
		erec.ExceptionInformation[0] = (ULONG_PTR)data->expression;
		erec.ExceptionInformation[1] = (ULONG_PTR)data->file;
		erec.ExceptionInformation[2] = (ULONG_PTR)data->line;
		erec.NumberParameters = 3;

		erec.ExceptionAddress = (LPVOID)
#if OC_MACHINE == OC_MACHINE_X64
			ctx.Rip
#elif OC_MACHINE == OC_MACHINE_X86
			ctx.Eip
#else
			0
#endif
			;
		EXCEPTION_POINTERS eptr;
		eptr.ContextRecord = &ctx;
		eptr.ExceptionRecord = &erec;

		// Log
		if (GetLogFD() != -1)
			SafeTextDump(&eptr, GetLogFD(), nullptr);

		// Continue caller
		if (ResumeThread(data->thread) == -1)
			abort();
		return result;
	}

	// Replacement assertion handler
#ifdef USE_WIDE_ASSERT
	void __cdecl assertion_handler(const wchar_t *expression, const wchar_t *file, unsigned line)
#else
	void __cdecl assertion_handler(const char *expression, const char *file, int line)
#endif
	{
		// Dump thread status on a different thread because we can't get a valid thread context otherwise
		HANDLE this_thread;
		DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &this_thread, 0, FALSE, DUPLICATE_SAME_ACCESS);
		dump_thread_t dump_thread_data = {
			this_thread,
			expression, file, line
		};
		HANDLE ctx_thread = CreateThread(nullptr, 0L, &dump_thread, &dump_thread_data, 0L, nullptr);
		WaitForSingleObject(ctx_thread, INFINITE);
		CloseHandle(this_thread);
		CloseHandle(ctx_thread);
		// Unhook _wassert/_assert
		UnhookAssert();
		// Call old _wassert/_assert
		assert_func(expression, file, line);
		// If we get here: rehook
		HookAssert(&assertion_handler);
	}
}
#endif

void InstallCrashHandler()
{
	// Disable process-wide callback filter for exceptions on Windows Vista.
	// Newer versions of Windows already get this disabled by the application
	// manifest. Without turning this off, we won't be able to handle crashes
	// inside window procedures on 64-bit Windows, regardless of whether we
	// are 32 or 64 bit ourselves.
	typedef BOOL (WINAPI *SetProcessUserModeExceptionPolicyProc)(DWORD);
	typedef BOOL (WINAPI *GetProcessUserModeExceptionPolicyProc)(LPDWORD);
	HMODULE kernel32 = LoadLibrary(TEXT("kernel32"));
	const SetProcessUserModeExceptionPolicyProc SetProcessUserModeExceptionPolicy =
		(SetProcessUserModeExceptionPolicyProc)GetProcAddress(kernel32, "SetProcessUserModeExceptionPolicy");
	const GetProcessUserModeExceptionPolicyProc GetProcessUserModeExceptionPolicy =
		(GetProcessUserModeExceptionPolicyProc)GetProcAddress(kernel32, "GetProcessUserModeExceptionPolicy");
#ifndef PROCESS_CALLBACK_FILTER_ENABLED
#	define PROCESS_CALLBACK_FILTER_ENABLED 0x1
#endif
	if (SetProcessUserModeExceptionPolicy && GetProcessUserModeExceptionPolicy)
	{
		DWORD flags;
		if (GetProcessUserModeExceptionPolicy(&flags))
		{
			SetProcessUserModeExceptionPolicy(flags & ~PROCESS_CALLBACK_FILTER_ENABLED);
		}
	}
	FreeLibrary(kernel32);

	SetUnhandledExceptionFilter(GenerateDump);

#ifndef NDEBUG
	// Hook _wassert/_assert, unless we're running under a debugger
	if (!IsDebuggerPresent())
		HookAssert(&assertion_handler);
#endif
}

#else

void InstallCrashHandler()
{
	// no-op
}

#endif // HAVE_DBGHELP
