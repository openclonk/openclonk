namespace
{
	const DWORD OFN_HIDEREADONLY = 1 << 0;
	const DWORD OFN_OVERWRITEPROMPT = 1 << 1;
	const DWORD OFN_FILEMUSTEXIST = 1 << 2;
	const DWORD OFN_ALLOWMULTISELECT = 1 << 3;

	const DWORD OFN_EXPLORER = 0; // ignored
}
#ifdef USE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif