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
