Requirements
============

OSX 10.6 or higher (might also work with 10.5)
an Intel mac
brew (https://github.com/mxcl/homebrew) or macports (http://www.macports.org/)
Xcode
Apple X11
CMake (http://www.cmake.org/)

Build
=====

-Install dependencies using brew or port (libjpeg, libpng, freetype, glew, libogg, libvorbis, libvorbisfile)
-Launch the CMake GUI application
-Click Browse Source… button, navigate to your openclonk repository folder
-Also Specify location where you want to build
-Click Configure and use default native compilers
-Wait
-<Potentially include additional steps to make CMake find the right libraries>
-If you want 64-bit builds set the CMAKE_OSX_ARCHITECTURES setting to "x86_64"
	-For universal builds set it to "x86_64 i386", but then you'll also need universal versions of the dependencies
-Click Configure button again for good measure
-Click Generate
-Launch xcode and load the project. Select the desired configuration and build.

It should be pretty straight forward, hopefully.

Additional CMake hints
=====================

FREETYPE_LIBRARY should be set to /usr/X11/lib/libfreetype.6.dylib
ZLIB_LIBRARY to /usr/lib/libz.dylib
ZLIB_INCLUDE_DIR to /usr/include

Situation with Xcode 4.3+
========================

[This applies only to CMake versions prior to 2.8-8, later versions should deal with Xcode 4.3+ just fine]
Xcode is now a self-contained application bundle which confuses CMake.
The CMake git repo contains necessary fixes but those haven't been incorporated into a new CMake release yet so to use those you have to build the cmake command line tool yourself by
	* cloning git://cmake.org/cmake.git
	* running git checkout next
	* running ./configure, make and sudo make install
The project generation command I (Mortimer) used was `cmake -G Xcode -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++`, obviously specifying clang as the compiler. There is a related CMake option called USE_APPLE_CLANG which should be ON by default.
After that I had a proper Xcode 4.3 project.
To use the CMake GUI for setting some library paths I put the self-built cmake command from /usr/local/bin into CMake 2.8-7.app/Contents/bin/ but one could have probably done that by editing CMakeCache.txt or setting via however the cli syntax for setting variables is.
