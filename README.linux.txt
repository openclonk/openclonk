Requirements
============

To build on DEB-based Linux distributions (Debian, Ubuntu etc.) you need the
following packages:

  make gcc g++
  cmake OR automake autoconf
  libc6-dev libx11-dev libxxf86vm-dev libxrandr-dev libxpm-dev libglew1.5-dev
  libgl1-mesa-dev libpng12-dev libsdl1.2-dev libsdl-mixer1.2-dev libgtk2.0-dev
  libjpeg62-dev zlib1g-dev libboost-dev

To build on RPM-based Linux distributions (Red Hat, Fedora, Mandariva,
SuSE etc.) you need the following packages:

  make gcc gcc-c++
  cmake OR automake autoconf
  libX11-devel libXxf86vm-devel libXrandr-devel libXpm-devel glew-devel
  mesa-libGL-devel libpng-devel SDL-devel SDL_mixer1.2-dev gtk2-devel
  libjpeg-devel zlib-devel boost-devel


Build using cmake
=====================

To build OpenClonk, execute the following command inside of the source tree:

  cmake . && make

By default, the binary will be built without debugging support. If you want
to generate a debug build, pass the parameter -DCMAKE_BUILD_TYPE=Debug to
your cmake invocation.

Please note that you do not need to build a Debug binary if you only want to
debug C4Script code.

You can see other build variables with:

  cmake . -N -L


Build using autotools
=====================

If you build from version control, you need to run this:

  autoreconf -i && ./configure && make

To build from tarball, run this:

  ./configure && make

If you want a debug build, pass --enable-debug to configure, for the developer
mode --with-gtk. Other options are listed by ./configure --help.

On subsequent build runs, you only have to execute make.


Running
=======

You need to move the compiled binary to the data folder before running. I
recommend using a symbolic link like this:

  cd planet/
  ln -s ../clonk
  ./clonk
