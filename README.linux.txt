Requirements
============

To build on Linux you need the following packages (Debian names given):

make gcc-4.1 g++-4.1
automake autoconf
libc6-dev libx11-dev libxxf86vm-dev libxpm-dev libglew1.5-dev libgl1-mesa-dev
libpng12-dev libssl-dev libsdl1.2-dev libsdl-mixer1.2-dev libssl-dev
libgtk2.0-dev libjpeg62-dev zlib1g-dev

Build
=====

If you build from version control, you need to run this:

  autoreconf -i && ./configure 'CXX=g++-4.1' && make

To build from tarball, run this:

  ./configure 'CXX=g++-4.1' && make

If you want a debug build, pass --enable-debug to configure, for the developer mode
--with-gtk. Other options are listed by ./configure --help.

On subsequent build runs, you only have to execute make.

