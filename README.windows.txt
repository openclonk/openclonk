Requirements
============

You can build on Windows using either:

* vc6 (Microsoft Visual C++ 6.0)
	plus PlatformSDK 2003
	plus DXSDK 8.1

* vc7 (Microsoft Visual C++ 2003)
	you might have to set the correct DXSDK include and library directories
	professional edition required to compile resources

* vc9 (Microsoft Visual C++ 2008)
	you might have to set the correct DXSDK include and library directories
	professional edition required to compile resources

* MinGW
	plus MSYS (or any other shell that can run configure and make)
	plus DXSDK 9 (if you want DirectX support)
	this is currently only tested by crosscompiling from Linux.
	only g++-4.1 and older can compile clonk, g++-4.2 and newer are incompatible.


NoNetwork
=========

If you are using the public source package, will be able to build
the "NoNetwork" configurations only.


Notes for MinGW
===============

To build using MinGW, you need from http://www.mingw.org/:
 * MinGW-5.1.3.exe (or newer)
 * MSYS-1.0.10.exe

With MinGW-*.exe, install from the "current" distribution the MinGW base tools
and g++, the C++ compiler. Then install msys.

If you want to edit the build files (for example for a new source file), you
need the MSYS DTK, Autoconf 2.6x and Automake 1.10.x. You might need to build
the last two from source.

If you want DirectX support, get a DirectX 9 SDK from Microsoft. Copy the
contents of it's include dir to the include dir of your MinGW installation,
and pass --with-directx to configure below.

Open a shell (the MSYS one under windows), cd to this directory, and execute:

   ./configure && make

If you want a debugbuild, pass --enable-debug to configure. Other options are
listed by configure --help.

If you get an error like "directx/dxfile.h:240: stray '\32' in program", then
delete that char from that file (Whyever someone would want an ASCII 32 in a
header is beyond me). Some editors (SciTE for example) display such unusual
characters instead of representing them with space.

On subsequent build runs, you only have to execute make.

If you want to edit the build files, pass --enable-maintainer-mode to configure.
When you do that, make will automatically update the build system.

If you want to separate the source directory and the output files, you can call
configure from another directory. You can call configure by it's relative path,
but using the full path helps gdb find the source files. Example:

    mkdir build
    cd build
    /path/to/clonksource/configure --with-directx CXXFLAGS='-Os'
    make

