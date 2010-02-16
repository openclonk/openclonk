dnl Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de

dnl Permission to use, copy, modify, and/or distribute this software for any
dnl purpose with or without fee is hereby granted, provided that the above
dnl copyright notice and this permission notice appear in all copies.

dnl THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
dnl WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
dnl MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
dnl ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
dnl WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
dnl ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
dnl OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(clonk, m4_format([[[%s.%s.%s.%s]]], m4_include(version)))
AC_COPYRIGHT([©2005-2009 Günther Brammer])
AC_CONFIG_SRCDIR([/src/C4Game.cpp])
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])
AC_CANONICAL_HOST
dnl foreign to tell automake to shut up,
dnl and subdir-objects because it makes the Makefile smaller.
dnl no-define because PACKAGE and VERSION are not used
AM_INIT_AUTOMAKE([foreign nostdinc no-define 1.10])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_LANG([C++])
AX_CHECK_COMPILER_FLAGS([-std=gnu++0x], [CXX="${CXX} -std=gnu++0x"])
AC_COMPILE_IFELSE([
void f(struct D&&); int main() { return 0; }
], [AC_DEFINE([HAVE_RVALUE_REF], [], [C++ Compiler has rvalue references, a C++0x feature.])], [AX_PROG_CXX_REFTOTEMP])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_RANLIB

case $host in
  *-*-mingw32* | *-*-cygwin* | *-*-windows*)
    win32=true; osx=false;;
  *-*-darwin*)
    win32=false; osx=true;
    m4_ifdef([AC_PROG_OBJCXX], [AC_PROG_OBJCXX], [AC_MSG_ERROR([this configure script was created with an autoconf version older than 2.65 and does not support macosx.])])
    ;;
  *)
    win32=false; osx=false;;
esac

# Detect Wine. Unfortunately, at the moment winegcc does not meet the demands
AC_CHECK_HEADER([windows.h], [win32=true], [], [[ ]])

AM_CONDITIONAL(WIN32, [test $win32 = true])
AM_CONDITIONAL(MACOSX, [test $osx = true])

# various used headers
dnl the whitespace is there to prevent AC_INCLUDES_DEFAULT
AC_CHECK_HEADERS([stdint.h unistd.h poll.h sys/stat.h sys/types.h locale.h sys/socket.h signal.h langinfo.h execinfo.h sys/inotify.h sys/syscall.h], , , [[ ]])
# Mingw does not ship with multimon.h
AC_CHECK_HEADERS([multimon.h io.h direct.h share.h], [], [], [[#include <windows.h>]])
# iconv
AX_ICONV
# vasprintf is a GNU extension
AC_CHECK_FUNCS(vasprintf)

AC_ARG_ENABLE([debug],
  [AC_HELP_STRING([--enable-debug], [debugging options [default=no]])],
  , [enable_debug=no])
if test $enable_debug = yes; then
  AC_DEFINE([_DEBUG], 1, [compile with debug options])
else
  AC_DEFINE([NDEBUG], 1, [compile without debug options])
fi

AC_ARG_ENABLE([debugrec],
  [AC_HELP_STRING([--enable-debugrec], [write additional debug control to records [default=no]])],
    , [enable_debugrec=no])
if test $enable_debugrec = yes; then
  AC_DEFINE([DEBUGREC], 1, [Activate DebugRecs])
fi

AC_ARG_ENABLE([console],
  [AC_HELP_STRING([--enable-console],[compile as pure console application [default=no]])],
  , [enable_console=no])

AC_ARG_ENABLE([sound],
  [AC_HELP_STRING([--enable-sound],[compile with sound support [default=yes]])],
  , [if test $enable_console = yes; then enable_sound=no; else if test $win32 = true; then enable_sound=no; else enable_sound=yes; fi; fi])

# GTK+
GTK_REQUIRED="glib-2.0 >= 2.4 gtk+-2.0 >= 2.4"
AC_ARG_WITH([gtk],
  [AC_HELP_STRING([--with-gtk], [Use gtk for the developer mode [default=auto]])],
  , [AS_IF([test $enable_console = no],[PKG_CHECK_EXISTS([$GTK_REQUIRED], [with_gtk=yes], [with_gtk=no])],[with_gtk=no])])
AS_IF([test $with_gtk = yes],[
  PKG_CHECK_MODULES(GTK, [$GTK_REQUIRED])
  AC_DEFINE([WITH_GLIB], 1, [Glib])
  AC_DEFINE([WITH_DEVELOPER_MODE], 1, [Developer mode])
  DEVELOPER_MODE=true
])
AM_CONDITIONAL(DEVELOPER_MODE, [test $with_gtk = yes])

AC_ARG_WITH([sdl],
  AS_HELP_STRING([--with-sdl], [Use SDL @<:@default=yes (no for win32)@:>@]),
  , [if test $win32 = true; then with_sdl=no; else if test $enable_console = yes; then with_sdl=no; else with_sdl=yes; fi fi])
if test $with_sdl = yes; then
  # Check for SDL
  SDL_VERSION=1.2.0
  AM_PATH_SDL($SDL_VERSION,
    [AC_DEFINE(HAVE_SDL, 1, [Define to 1 if you have SDL.])],
    [AC_MSG_ERROR([libSDL not found.])])
  AC_SUBST(SDL_CFLAGS)
  AC_SUBST(SDL_LIBS)
  if test $enable_sound = yes; then
    # Check for SDL_mixer library
    AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio,
      [AC_DEFINE(HAVE_LIBSDL_MIXER,1,[Define to 1 if you have SDL_mixer.])
      SDL_LIBS="-lSDL_mixer $SDL_LIBS"],
      [AC_MSG_ERROR([SDL_mixer not found.])],
      [$SDL_LIBS])
  fi
  AC_ARG_ENABLE(sdlmainloop,
    [AC_HELP_STRING([--enable-sdlmainloop],[use SDL instead of X11 or Win32 [default=no]])],
    , [if test $osx = true; then enable_sdlmainloop=yes; else enable_sdlmainloop=no; fi ])
else
  enable_sdlmainloop=no
fi
if test $enable_sdlmainloop = yes; then
  with_x=no
  AC_DEFINE(USE_SDL_MAINLOOP, 1, [Define to 1 if SDL is used for the main loop])
fi
AM_CONDITIONAL(SDL_MAIN_LOOP, [test $enable_sdlmainloop = yes])

if test $enable_sound = yes; then
  AC_ARG_ENABLE([mp3],
    [AC_HELP_STRING([--enable-mp3],[enable mp3 support [default=no]])],
    , [enable_mp3=no])
  if test $enable_mp3 = yes; then
    AC_DEFINE([USE_MP3], 1, [MP3 music])
  fi
fi

if test $enable_console = no; then
  if test $win32 = true; then
    AC_SUBST(WINDOWS_CFLAGS, "-mwindows")
  fi
else
  # console mode
  AC_DEFINE([USE_CONSOLE], 1, [dedicated server mode])
  with_x=no
  enable_sdlmainloop=no
  VL_LIB_READLINE
fi

# Check for libjpeg
AC_CHECK_LIB(jpeg, jpeg_read_header, [
  CLONK_LIBS="-ljpeg $CLONK_LIBS"
], [
  AC_MSG_ERROR([libjpeg not found.])
])

# Check for libpng
AC_CHECK_LIB(png, png_read_image, [
  CLONK_LIBS="-lpng $CLONK_LIBS"
], [
  AC_MSG_ERROR([libpng not found.])
], [-lz])

# Check for libz
AC_CHECK_LIB(z, deflate, [
  Z_LIBS="-lz $Z_LIBS"
], [
  AC_MSG_ERROR([libz not found.])
])

# Check for libfreetype
if test $enable_console = no; then
  if test "$cross_compiling" = no; then
    AC_CHECK_PROG(FREETYPE_CONFIG, freetype-config, freetype-config)
  fi
  if test $FREETYPE_CONFIG; then
    AC_SUBST(FREETYPE_CFLAGS, [`$FREETYPE_CONFIG --cflags`])
    AC_SUBST(FREETYPE_LIBS, [`$FREETYPE_CONFIG --libs`])
  else
    PKG_CHECK_MODULES(FREETYPE, freetype2)
  fi
  AC_DEFINE(HAVE_FREETYPE, 1, [Define to 1 if you have FreeType2.])
fi

# X11, Xpm, Xrandr
AC_PATH_XTRA
if test $have_x = yes; then
  AC_DEFINE(USE_X11, 1, [Define to 1 if the X Window System is used])
  CLONK_LIBS="$X_LIBS $CLONK_LIBS"
  AC_CHECK_HEADERS([X11/keysym.h X11/extensions/xf86vmode.h X11/xpm.h X11/extensions/Xrandr.h], ,
    [AC_MSG_ERROR([A required X11 header was not found.])], [[#include <X11/Xlib.h>]])
  AC_CHECK_LIB(X11, XOpenDisplay, [CLONK_LIBS="-lX11 $CLONK_LIBS"],
    [AC_MSG_ERROR([libX11 not found.])], [$X_LIBS])
  AC_CHECK_LIB(Xpm, XpmCreatePixmapFromData, [CLONK_LIBS="-lXpm $CLONK_LIBS"],
    [AC_MSG_ERROR([libXpm not found.])], [$X_LIBS])
  AC_CHECK_LIB(Xxf86vm, XF86VidModeQueryVersion, [CLONK_LIBS="-lXxf86vm $CLONK_LIBS"],
    [AC_MSG_ERROR([XF86VidMode not found.])], [$X_LIBS])
  AC_CHECK_LIB(Xrandr, XRRQueryExtension, [CLONK_LIBS="-lXrandr $CLONK_LIBS"],
    [AC_MSG_ERROR([libXrandr not found.])], [$X_LIBS])
fi

# OpenGL
AC_ARG_WITH([gl],
  [AC_HELP_STRING([--with-gl],[compile with opengl support [default=yes]])],
  , [if test $enable_console = yes; then with_gl=no; else with_gl=yes; fi])
if test $with_gl = yes; then
  if test $osx = true; then
    CLONK_LIBS="-framework OpenGL $CLONK_LIBS"
    AC_CHECK_LIB(GLEW, glewInit, [CLONK_LIBS="-lGLEW $CLONK_LIBS"],
      [AC_MSG_ERROR([glew not found.])])
  elif test $win32 = true; then
    AC_CHECK_LIB(opengl32, main, [CLONK_LIBS="-lopengl32 $CLONK_LIBS"],
      [AC_MSG_ERROR([opengl32 not found.])])
    AC_CHECK_LIB(glu32, main, [CLONK_LIBS="-lglu32 $CLONK_LIBS"],
      [AC_MSG_ERROR([glu32 not found.])])
    AC_CHECK_LIB(glew32, main, [CLONK_LIBS="-lglew32 $CLONK_LIBS"],
      [AC_MSG_ERROR([glew not found.])])
    AC_SUBST([GLEW_CFLAGS],["-DGLEW_STATIC"])
  else
    AC_CHECK_LIB(GL, glBegin, [CLONK_LIBS="-lGL $CLONK_LIBS"],
      [AC_MSG_ERROR([libGL not found.])])
    AC_CHECK_LIB(GLU, gluOrtho2D, [CLONK_LIBS="-lGLU $CLONK_LIBS"],
      [AC_MSG_ERROR([libGLU not found.])])
    AC_CHECK_LIB(GLEW, glewInit, [CLONK_LIBS="-lGLEW $CLONK_LIBS"],
      [AC_MSG_ERROR([glew not found.])])
  fi
  AC_DEFINE([USE_GL], 1, [OpenGL graphics])
fi

# DirectX
AC_ARG_WITH([directx],
  [AC_HELP_STRING([--with-directx],[compile with directx support [default=no]])],
  , [with_directx="no"])
if test "$with_directx" = yes; then
  AC_CHECK_LIB(d3d9, main, [CLONK_LIBS="-ld3d9 $CLONK_LIBS"],
    [AC_MSG_ERROR([d3d9 not found.])])
  AC_DEFINE([USE_DIRECTX], 1, [DirectX graphics])
fi

# OpenSSL
AC_CHECK_LIB(crypto, BIO_new, [OPENSSL_LIBS="-lcrypto"],
  [AC_MSG_ERROR([libcrypto (OpenSSL) not found.])])

if test $win32 = false; then
  # pthread
  AX_PTHREAD( , [AC_MSG_ERROR([No pthread support.])])
  AC_SUBST(PTHREAD_LIBS)
  AC_SUBST(PTHREAD_CFLAGS)

  # inotify syscall workaround
  AC_CHECK_DECL(SYS_inotify_init,
  	AC_DEFINE([HAVE_SYSCALL_INOTIFY], 1, [inotify reachable using syscall]),,
  	[#include <sys/syscall.h>])

  # FMod is used for windows only
  AM_CONDITIONAL(USE_FMOD, false)
else
  # Windows
  for WIN32LIB in gdi32 comdlg32 winmm ws2_32 vfw32
  do
    AC_CHECK_LIB([$WIN32LIB], main, [CLONK_LIBS="-l$WIN32LIB $CLONK_LIBS"], [AC_MSG_ERROR([$WIN32LIB not found.])])
  done

  # FMod (Library comes with standard)
  if test "$enable_sound" = yes; then
    AC_DEFINE([USE_FMOD], 1, [Wether FMOD shall be used])
  fi
  AM_CONDITIONAL(USE_FMOD, test "$enable_sound" = yes)

  # windres
  AC_CHECK_TOOL(WINDRES, windres, no)
  if test $WINDRES = no; then
    AC_MSG_ERROR([windres not found.])
  fi
fi

AC_SUBST(CLONK_LIBS)
AC_SUBST(Z_LIBS)
AC_SUBST(OPENSSL_LIBS)

# precompiled headers and -Wextra
AC_MSG_CHECKING([whether we are using a GNU C++ compiler version 4.1 or newer])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[#if defined(__GNUC__) && (((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 1)) || (__GNUC__ > 4))
// all ok
#else
       choke me
#endif
]])], RECENT_GCC="yes", RECENT_GCC="no")
AC_MSG_RESULT([$RECENT_GCC])
AM_CONDITIONAL(RECENT_GCC, [test $RECENT_GCC = yes])

AC_OUTPUT

echo "Configuration:
  Debug:	$enable_debug
  Debugrec:	$enable_debugrec
  Pure console:	$enable_console
  DirectX:	$with_directx
  OpenGL:	$with_gl
  Sound:	$enable_sound
  GTK+: 	$with_gtk
  X11:  	$have_x"

if test "$enable_sdlmainloop" = yes; then echo "  SDL:  	mainloop";
else echo "  SDL:  	$with_sdl"; fi

