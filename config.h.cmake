
/* Generate minidumps on crash */
#cmakedefine HAVE_DBGHELP 1

/* Use backward-cpp to print stack traces on crash */
#cmakedefine HAVE_BACKWARD 1

/* The backtrace function is declared in execinfo.h and works */
#cmakedefine HAVE_EXECINFO_H 1

/* Define to 1 if you have the <history.h> header file. */
#cmakedefine HAVE_HISTORY_H 1

/* Define to 1 if you have the <io.h> header file. */
#cmakedefine HAVE_IO_H 1

/* Define if you have a readline compatible library */
#cmakedefine HAVE_LIBREADLINE 1

/* Define to 1 if you have the <locale.h> header file. */
#cmakedefine HAVE_LOCALE_H 1

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

/* Define to 1 if you have the `vasprintf' function. */
#cmakedefine HAVE_VASPRINTF 1

/* Define to 1 if you have the <X11/extensions/Xrandr.h> header file. */
#cmakedefine HAVE_X11_EXTENSIONS_XRANDR_H 1

/* compile without debug options */
#cmakedefine NDEBUG 1

/* MP3 music */
#cmakedefine USE_MP3 1

/* Glib */
#cmakedefine WITH_GLIB 1

/* compile with debug options */
#cmakedefine _DEBUG 1

#ifndef USE_CONSOLE
/* The widgets for the windows and the editor GUI */
#cmakedefine USE_SDL_MAINLOOP 1
#cmakedefine USE_WIN32_WINDOWS 1
#cmakedefine USE_COCOA 1
#cmakedefine USE_GTK 1

/* Enable automatic update system */
#cmakedefine WITH_AUTOMATIC_UPDATE 1
#cmakedefine WITH_APPDIR_INSTALLATION 1

/* Select an audio provider */
#define AUDIO_TK AUDIO_TK_${Audio_TK_UPPER}
#else
#define AUDIO_TK AUDIO_TK_NONE
#endif // USE_CONSOLE

#define AUDIO_TK_NONE 0
#define AUDIO_TK_OPENAL 1
#define AUDIO_TK_SDL_MIXER 3

/* Include OpenAL extensions (alext.h) for sound modifiers */
#cmakedefine HAVE_ALEXT 1

/* Path to data directory */
#ifdef WITH_APPDIR_INSTALLATION
#define OC_SYSTEM_DATA_DIR "../share/games/openclonk"
#else
#define OC_SYSTEM_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/games/openclonk"
#endif

/* Path to /proc/self/exe (Linux) or equivalent */
#cmakedefine PROC_SELF_EXE "${PROC_SELF_EXE}"
