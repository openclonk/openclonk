
/* Generate minidumps on crash */
#cmakedefine HAVE_DBGHELP 1

/* Define to 1 if you have the <direct.h> header file. */
#cmakedefine HAVE_DIRECT_H 1

/* The backtrace function is declared in execinfo.h and works */
#cmakedefine HAVE_EXECINFO_H 1

/* Define to 1 if you have the <history.h> header file. */
#cmakedefine HAVE_HISTORY_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <io.h> header file. */
#cmakedefine HAVE_IO_H 1

/* Define if you have a readline compatible library */
#cmakedefine HAVE_LIBREADLINE 1

/* Define to 1 if you have the <locale.h> header file. */
#cmakedefine HAVE_LOCALE_H 1

/* Define to 1 if your stdlib has std::make_unique */
#cmakedefine HAVE_MAKE_UNIQUE 1

/* Define to 1 if you have the <poll.h> header file. */
#cmakedefine HAVE_POLL_H 1

/* Define if you have POSIX threads libraries and header files. */
#cmakedefine HAVE_PTHREAD 1

/* Define to 1 if you have SDL. */
#cmakedefine HAVE_SDL 1

/* Define to 1 if you have the <share.h> header file. */
#cmakedefine HAVE_SHARE_H 1

/* Define to 1 if you have the <signal.h> header file. */
#cmakedefine HAVE_SIGNAL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <sys/eventfd.h> header file. */
#cmakedefine HAVE_SYS_EVENTFD_H 1

/* Define to 1 if you have the <sys/file.h> header file. */
#cmakedefine HAVE_SYS_FILE_H 1

/* Define to 1 if you have the <sys/inotify.h> header file. */
#cmakedefine HAVE_SYS_INOTIFY_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#cmakedefine HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/timerfd.h> header file. */
#cmakedefine HAVE_SYS_TIMERFD_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if your compiler supports variadic templates */
#cmakedefine HAVE_VARIADIC_TEMPLATES 1

/* Define to 1 if you have the `vasprintf' function. */
#cmakedefine HAVE_VASPRINTF 1

/* Define to 1 if you have the `__mingw_vasprintf' function. */
#cmakedefine HAVE___MINGW_VASPRINTF 1

/* Define to 1 if you have the <X11/extensions/Xrandr.h> header file. */
#cmakedefine HAVE_X11_EXTENSIONS_XRANDR_H 1

/* Define to 1 if you have the <X11/keysym.h> header file. */
#cmakedefine HAVE_X11_KEYSYM_H 1

/* compile without debug options */
#cmakedefine NDEBUG 1

/* MP3 music */
#cmakedefine USE_MP3 1

/* Define to 1 if SDL is used for the main loop */
#cmakedefine USE_SDL_MAINLOOP 1

/* Define to 1 if the X Window System is used */
#cmakedefine USE_X11 1

/* Use Apple Cocoa for the UI */
#cmakedefine USE_COCOA 1

/* Enable automatic update system */
#cmakedefine WITH_AUTOMATIC_UPDATE 1

/* Developer mode */
#cmakedefine WITH_DEVELOPER_MODE 1

/* Define to 1 if the userParam parameter to GLDEBUGPROCARB is const, as the
   spec requires. */
#cmakedefine GLDEBUGPROCARB_USERPARAM_IS_CONST 1

/* Glib */
#cmakedefine WITH_GLIB 1

/* compile with debug options */
#cmakedefine _DEBUG 1

/* Define to 1 if you have support for precompiled headers */
#cmakedefine HAVE_PRECOMPILED_HEADERS 1

/* Select an audio provider */
#define AUDIO_TK_NONE 0
#define AUDIO_TK_OPENAL 1
#define AUDIO_TK_SDL_MIXER 3
#define AUDIO_TK AUDIO_TK_${Audio_TK_UPPER}

/* Include OpenAL extensions (alext.h) for sound modifiers */
#cmakedefine HAVE_ALEXT 1

#ifdef USE_CONSOLE
/* FIXME: Sort this out in CMake instead of here */
#undef USE_COCOA
#undef USE_SDL_MAINLOOP
#undef USE_X11
#undef WITH_AUTOMATIC_UPDATE
#undef WITH_DEVELOPER_MODE

#undef AUDIO_TK
#define AUDIO_TK AUDIO_TK_NONE

#endif
