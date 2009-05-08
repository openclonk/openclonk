Requirements
============

To build on Linux you need the following packages (Debian names given):

build-essential gcc-4.1 g++-4.1
automake autoconf
libx11-dev libxxf86vm-dev libxpm-dev libgl1-mesa-dev libpng12-dev libssl-dev
libsdl1.2-dev libsdl-mixer1.2-dev libssl-dev libgtk2.0-dev libjpeg62-dev

Build
=====

Open a shell, cd into the source directory, and run:

  ./configure 'CXX=g++-4.1' && make

If you want a debug build, pass --enable-debug to configure, for the developer mode
--with-gtk. Other options are listed by ./configure --help.

On subsequent build runs, you only have to execute make.

If you want to edit the build files, pass --enable-maintainer-mode to configure.
When you do that, make will automatically update the build system.

The following additional arguments to configure are currently used for release builds:

'--with-gtk' '--with-internal-libpng' '--without-internal-libjpeg' \
'CFLAGS=-Os -g -finline-functions -ffast-math -DBIG_C4INCLUDE' \
'CXXFLAGS=-Os -g -finline-functions -ffast-math -DBIG_C4INCLUDE' \
'OPENSSL_LIBS=/usr/lib/libcrypto.a -ldl'
