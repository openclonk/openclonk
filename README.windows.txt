Requirements
============

You can build on Windows using either:

* vc9 (Microsoft Visual C++ 2008)
	you need CMake (http://www.cmake.org/cmake/resources/software.html) to create the "solution"
	you might have to set the correct DXSDK include and library directories

* MinGW and MSYS
	plus DXSDK 9 (if you want DirectX support)

* Some other compilers and IDEs which are supported by CMake might also work

Notes for MinGW
===============

You need gcc, g++, mingw-runtime, w32api, msys, msyscore, autoconf, automake,
and any packages needed by these. The versions listed below are known to work,
though newer versions are probably also good.

http://sourceforge.net/project/downloading.php?group_id=200665&filename=tdm-mingw-1.905.0-webdl.exe&a=64029035
(from http://sourceforge.net/project/showfiles.php?group_id=200665&package_id=238465)
(see also http://www.tdragon.net/recentgcc/)
- The default options work. Use an installation path without spaces to be on
  the save side.

http://sourceforge.net/project/downloading.php?group_id=2435&filename=MSYS-1.0.11-20090120-dll.tar.gz&a=78351117
http://sourceforge.net/project/downloading.php?group_id=2435&filename=msysCORE-1.0.11-20080826.tar.gz&a=60784616
(from http://sourceforge.net/project/showfiles.php?group_id=2435&package_id=24963)
- extract these into the same directory as installed above, merging the
  directory contents
- create a shortcut to msys.bat

http://sourceforge.net/project/downloading.php?group_id=2435&filename=perl-5.6.1-MSYS-1.0.11-1.tar.bz2&a=91743036
http://sourceforge.net/project/downloading.php?group_id=2435&filename=crypt-1.1-1-MSYS-1.0.11-1.tar.bz2&a=95002722
http://sourceforge.net/project/downloading.php?group_id=2435&filename=autoconf2.5-2.61-1-bin.tar.bz2&a=93276645
http://sourceforge.net/project/downloading.php?group_id=2435&filename=automake1.10-1.10-1-bin.tar.bz2&a=66135072
http://sourceforge.net/project/downloading.php?group_id=2435&filename=autoconf-4-1-bin.tar.bz2&a=70428585
http://sourceforge.net/project/downloading.php?group_id=2435&filename=automake-3-1-bin.tar.bz2&a=12881354
(from http://sourceforge.net/project/showfiles.php?group_id=2435&package_id=67879)
- you need to extract these from the msys shell with tar (other methods won't
  work). This assumes that the archives are in C:\Path\To\Downloads\, adjust
  the command as necessary:
    cd /
    for f in /c/Path/To/Downloads/*.bz2; do tar -xjf $f; done && echo done
 
Get the library package from openclonk.org and unpack it into the same
directory as the other archives.

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

