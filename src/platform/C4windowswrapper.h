#ifdef _WIN64
# define WINVER 0x0501
# define _WIN32_WINDOWS 0x0501
# define _WIN32_WINNT  0x0501
# define _WIN32_IE 0x0501
# define _AMD64_ 1
#else
# define WINVER 0x0500
# define _WIN32_WINDOWS 0x0500
# define _WIN32_WINNT  0x0501
# define _WIN32_IE 0x0501
# define _X86_ 1
#endif
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
# define NOMINMAX
#endif
#include <windows.h>
#undef RGB
#undef GetRValue
#undef GetGValue
#undef GetBValue
#undef TextOut
#undef GetObject
#undef CreateFont
#undef LoadBitmap
#undef DrawText

