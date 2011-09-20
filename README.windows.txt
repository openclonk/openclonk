Requirements
============

You can build on Windows using either:

* vc10 (Microsoft Visual C++ 2010)
  - you need CMake (http://www.cmake.org/cmake/resources/software.html) to
    create the "solution"
  - you might have to set the correct DXSDK include and library directories

* MinGW and MSYS
  - plus DXSDK 9 (if you want DirectX support)

* Some other compilers and IDEs which are supported by CMake might also work

OpenClonk requires some additional libraries. Prebuilt versions of them can be
found on http://openclonk.org.

Building the installer
======================

The installer is created with NSIS. makensis needs to be in the PATH, and
the dlls used by openclonk in the build directory. To create the installer,
build the "setup" target if using CMake, or if using autotools, run:

    make setup_openclonk.exe

Get NSIS from http://nsis.sourceforge.net/.

Notes for MinGW
===============

You need gcc, g++, mingw-runtime, w32api, msys, msyscore, autoconf, automake,
and any packages needed by these.

Get the library package from openclonk.org and unpack it into the mingw
directory.

If you want DirectX support, get a DirectX 9 SDK from Microsoft. Copy the
contents of its include dir to the include dir of your MinGW installation,
and pass --with-directx to configure below.

Start msys (your MinGW directory, e.g. C:\MinGW -> msys.bat),
cd to this directory, and execute:

    ./autogen.sh && ./configure && make

To compile a debugbuild, pass --enable-debug to configure. Other options are
listed by ./configure --help.

On subsequent build runs, you only have to execute make.

If you want to separate the source directory and the output files, you can call
configure from another directory. You can call configure by it's relative path,
but using the full path helps gdb find the source files. Example:

    mkdir build
    cd build
    /path/to/clonksource/configure --with-directx CXXFLAGS='-Os'
    make

