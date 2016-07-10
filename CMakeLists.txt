# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2009-2016, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

cmake_minimum_required (VERSION 3.0.2)

# Don't allow people to build "Release" builds because there's no reason to do that.
# Use one of RelWithDebInfo or MinSizeRel instead.
set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo MinSizeRel CACHE STRING "List of supported configuration types." FORCE)

# Set a default build type as the empty build type is without either debug
# symbols or optimizations and thus completely useless.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

project (openclonk CXX C)

# CMP0054: Only interpret if() arguments as variables or keywords when unquoted
# (since CMake 3.1)
if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Version)

set(CMAKE_ADDITIONAL_DEPS_PATH "${CMAKE_CURRENT_BINARY_DIR}/deps" CACHE PATH "Additional directory to search for libraries and headers")
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_ADDITIONAL_DEPS_PATH})

############################################################################
# User selectable options
############################################################################
include(CMakeDependentOption)
option(PROJECT_FOLDERS "Put source files into subfolders in project file" ON)
CMAKE_DEPENDENT_OPTION(USE_COCOA         "Use Apple Cocoa widgets." ON "APPLE" OFF)
CMAKE_DEPENDENT_OPTION(USE_WIN32_WINDOWS "Use Microsoft Desktop App User Interface widgets." ON "WIN32" OFF)
CMAKE_DEPENDENT_OPTION(USE_SDL_MAINLOOP  "Use SDL to create windows etc. Qt editor." ON "NOT USE_COCOA AND NOT USE_WIN32_WINDOWS" OFF)
CMAKE_DEPENDENT_OPTION(USE_GTK           "Use GTK+ widgets." ON "NOT USE_COCOA AND NOT USE_WIN32_WINDOWS AND NOT USE_SDL_MAINLOOP" OFF)
option(WITH_AUTOMATIC_UPDATE "Automatic updates are downloaded from the project website." OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ${PROJECT_FOLDERS})

############################################################################
# Assemble compiler flags
############################################################################
include(CheckCXXCompilerFlag)

# Required for CMake >= 3.1.0 and Qt >= 5.7
set(CMAKE_CXX_STANDARD 14)

CHECK_CXX_COMPILER_FLAG("-std=gnu++14" USE_GCC_STD_14)
if(USE_GCC_STD_14)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")
endif()

# Enable link-time code generation. We have to do this manually because while
# there is a INTERPROCEDURAL_OPTIMIZATION cmake flag, it's only implemented
# for icc so far; https://cmake.org/Bug/view.php?id=15939
function(add_linker_flags)
	include(CMakeParseArguments)
	set(options optimized debug)
	set(oneValueArgs FLAGS)
	set(multiValueArgs MODULES)

	cmake_parse_arguments(_alf "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	# Adds some linker flags to all optimized build configurations
	set(_configurations "")
	if(_alf_optimized)
		list(APPEND _configurations MINSIZEREL RELWITHDEBINFO RELEASE)
	endif()
	if(_alf_debug)
		list(APPEND _configurations DEBUG)
	endif()

	foreach(_module ${_alf_MODULES})
		string(TOUPPER "${_module}" _obj_type)
		foreach(_config ${_configurations})
			set(CMAKE_${_obj_type}_LINKER_FLAGS_${_config} "${CMAKE_${_obj_type}_LINKER_FLAGS_${_config}} ${_alf_FLAGS}" PARENT_SCOPE)
		endforeach()
	endforeach()
endfunction()

CHECK_CXX_COMPILER_FLAG("-flto" USE_GCC_STYLE_LTCG)
if(USE_GCC_STYLE_LTCG)
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -flto")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -flto")
	add_linker_flags(optimized MODULES exe shared FLAGS -flto)

	# Use GCC's ar and ranlib wrappers if necessary, because the plain ones 
	# don't understand lto objects without an explicit plugin parameter
	if(CMAKE_C_COMPILER MATCHES "gcc$")
		set(LTCG_NEEDS_AR_WRAPPER 1)
		set(LTCG_AR_WRAPPER_PREFIX "${CMAKE_C_COMPILER}")
	elseif(CMAKE_C_COMPILER MATCHES "cc$")
		set(LTCG_NEEDS_AR_WRAPPER 1)
		set(LTCG_AR_WRAPPER_PREFIX "gcc")
	else()
		set(LTCG_NEEDS_AR_WRAPPER 0)
	endif()

	if(LTCG_NEEDS_AR_WRAPPER)
		find_program(AR_WRAPPER "${LTCG_AR_WRAPPER_PREFIX}-ar")
		if (AR_WRAPPER)
			message("Using ${AR_WRAPPER} instead of ${CMAKE_AR} to support lto objects.")
			set(CMAKE_AR "${AR_WRAPPER}" CACHE FILEPATH "Path to an ar that supports lto objects." FORCE)
		endif()
		find_program(RANLIB_WRAPPER "${LTCG_AR_WRAPPER_PREFIX}-ranlib")
		if (RANLIB_WRAPPER)
			message("Using ${RANLIB_WRAPPER} instead of ${CMAKE_RANLIB} to support lto objects.")
			set(CMAKE_RANLIB "${RANLIB_WRAPPER}" CACHE FILEPATH "Path to a ranlib that supports lto objects." FORCE)
		endif()
	endif()
endif()

if(MSVC)
	# Disable non-standard conversion from string literal to (nonconst) char*
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:strictStrings")

	# Enable multi-core builds
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

	# Enable LTCG for release builds
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ob2 /GL")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /Ob2 /GL")
	add_linker_flags(optimized MODULES exe shared static FLAGS "/LTCG:incremental")

	# Activate edit-and-continue
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI /Gy")

	# do not link the release CRT in debug builds
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:MSVCRT")
	set(HAVE_PRECOMPILED_HEADERS ON CACHE INTERNAL "Compiler supports precompiled headers")

	# Suppress warnings about "non-secure" functions
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
	# Disable warning C4244: 'conversion' conversion from 'type1' to 'type2', possible loss of data
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244")
	# Disable warning C4267: 'var' : conversion from 'size_t' to 'type', possible loss of data (64 bit build only)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wextra-tokens -Wpointer-arith -Wno-cast-align -Wno-reorder -Wno-unused-parameter -Wnon-virtual-dtor -Woverloaded-virtual")
elseif(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wredundant-decls -Wendif-labels -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Winit-self -Wsign-promo -Wno-reorder -Wno-unused-parameter -Wnon-virtual-dtor -Woverloaded-virtual")
endif()

if(WIN32 AND MINGW)
	# Activate DEP and ASLR
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--nxcompat -Wl,--dynamicbase")
endif()

if(UNIX)
	add_definitions("-DOC_SYSTEM_DATA_DIR=\"${CMAKE_INSTALL_PREFIX}/share/games/openclonk\"")
endif()

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc -headerpad_max_install_names")
endif()

############################################################################
# Check for compiler quirks and features
############################################################################
include(CheckCXXSourceCompiles)
include(RequireCXXSourceCompiles)
include(CheckCXXSymbolExists)

REQUIRE_CXX_SOURCE_COMPILES("#include <memory>\nint main() { std::unique_ptr<int> a; std::shared_ptr<int> b; }" HAVE_C11_SMART_PTRS)
REQUIRE_CXX_SOURCE_COMPILES("int main() { static_assert(true, \"\"); }" HAVE_STATIC_ASSERT)
REQUIRE_CXX_SOURCE_COMPILES("#include <memory>\nint main() { auto a = std::make_unique<int>(0); return !!a; }" HAVE_MAKE_UNIQUE)
REQUIRE_CXX_SOURCE_COMPILES("#include <utility>\ntemplate<std::size_t... Is> int foo(std::index_sequence<Is...>) {return 0;} int main() { return foo(std::index_sequence_for<int,int>{}); }" HAVE_INDEX_SEQUENCE)
REQUIRE_CXX_SOURCE_COMPILES("template<class... T> class C; int main() { return 0; }" HAVE_VARIADIC_TEMPLATES)

# g++'s libstdc++ doesn't properly support <regex> until 4.9.
# They ship a header that declares functions, but they don't ship an
# implementation for some things (like std::regex_iterator).
# This needs to test *linking*, not compilation; cmake does both at the same
# time, so the test below works.
REQUIRE_CXX_SOURCE_COMPILES("#include <regex>\nint main() { std::cregex_iterator ri; }" HAVE_WORKING_REGEX " If you are using gcc, please update to gcc 4.9.")
check_cxx_symbol_exists(vasprintf stdio.h HAVE_VASPRINTF)
check_cxx_symbol_exists(__mingw_vasprintf stdio.h HAVE___MINGW_VASPRINTF)

############################################################################
# Check for required system headers
############################################################################
include(CheckIncludeFileCXX)
include(CheckIncludeFilesCXX)
CHECK_INCLUDE_FILE_CXX(direct.h HAVE_DIRECT_H)
CHECK_INCLUDE_FILE_CXX(io.h HAVE_IO_H)
CHECK_INCLUDE_FILE_CXX(locale.h HAVE_LOCALE_H)
CHECK_INCLUDE_FILE_CXX(share.h HAVE_SHARE_H)
CHECK_INCLUDE_FILE_CXX(signal.h HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE_CXX(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILE_CXX(inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE_CXX(sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE_CXX(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE_CXX(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE_CXX(execinfo.h HAVE_EXECINFO_H)
CHECK_INCLUDE_FILE_CXX(poll.h HAVE_POLL_H)
CHECK_INCLUDE_FILE_CXX(sys/inotify.h HAVE_SYS_INOTIFY_H)
CHECK_INCLUDE_FILE_CXX(sys/timerfd.h HAVE_SYS_TIMERFD_H)
CHECK_INCLUDE_FILE_CXX(sys/socket.h HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE_CXX(sys/eventfd.h HAVE_SYS_EVENTFD_H)
CHECK_INCLUDE_FILE_CXX(sys/file.h HAVE_SYS_FILE_H)
CHECK_INCLUDE_FILES_CXX("X11/Xlib.h;X11/extensions/Xrandr.h" HAVE_X11_EXTENSIONS_XRANDR_H)
CHECK_CXX_SOURCE_COMPILES("#include <getopt.h>\nint main(int argc, char * argv[]) { getopt_long(argc, argv, \"\", 0, 0); }" HAVE_GETOPT_H)

############################################################################
# Locate libraries
############################################################################
# Find win32 SDK, since a lot of users don't have the include path in their environment
if(MSVC)
	get_filename_component(WINSDK_ROOT "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]" ABSOLUTE CACHE)
	mark_as_advanced(WINSDK_ROOT)
	list(APPEND CMAKE_INCLUDE_PATH "${WINSDK_ROOT}/Include")
	include_directories(SYSTEM ${WINSDK_ROOT}/Include)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		list(APPEND CMAKE_LIBRARY_PATH ${WINSDK_ROOT}/Lib/x64)
	else()
		list(APPEND CMAKE_LIBRARY_PATH ${WINSDK_ROOT}/Lib)
	endif()
endif()

SET(JPEG_NAMES ${JPEG_NAMES} libjpeg jpeg-static)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Freetype REQUIRED)
include_directories(
	${JPEG_INCLUDE_DIR}
	${PNG_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${FREETYPE_INCLUDE_DIRS})
link_directories(
	${FREETYPE_LIBRARY_DIRS})

if(CMAKE_SYSTEM MATCHES "Windows")
	message(STATUS "Using Win32 threading.")
else()
	find_package("Threads" REQUIRED)
	SET(HAVE_PTHREAD ${CMAKE_USE_PTHREADS_INIT} CACHE INTERNAL "libpthread available")
endif()

# FINDLIB works the same as find_library, but also marks the resulting var as
# advanced so it doesn't show in GUIs by default
macro(FINDLIB lib)
	find_library(${ARGV})
	mark_as_advanced(${lib})
endmacro(FINDLIB)

if(HAVE_EXECINFO_H)
	check_cxx_symbol_exists(backtrace execinfo.h HAVE_EXECINFO_WO_LINK)
	mark_as_advanced(HAVE_EXECINFO_WO_LINK)
	if(NOT HAVE_EXECINFO_WO_LINK)
		FINDLIB(EXECINFO_LIBRARY execinfo)
	endif()
endif()

if(NOT HAVE_GETOPT_H)
	include_directories(SYSTEM thirdparty/getopt)
	add_subdirectory(thirdparty/getopt)
	set_property(TARGET getopt PROPERTY FOLDER "Third-party libraries")
	set(GETOPT_LIBRARIES getopt)
endif()

# TinyXML
find_package(TinyXML)
if(NOT TinyXML_FOUND)
	add_subdirectory(thirdparty/tinyxml)
	set_property(TARGET tinyxml PROPERTY FOLDER "Third-party libraries")
	set(TinyXML_INCLUDE_DIRS thirdparty/tinyxml)
	set(TinyXML_LIBRARIES tinyxml)
	set(TinyXML_FOUND TRUE)
endif()

include_directories(SYSTEM ${TinyXML_INCLUDE_DIRS})

find_package(OpenGL)
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
add_definitions(${GLEW_DEFINITIONS})
# On some platforms, GLEW declares the userParam parameter of
# GLDEBUGPROCARB as const void *, and on other platforms, it's void *.
# Test which one works here.
set(CMAKE_REQUIRED_INCLUDES "${GLEW_INCLUDE_DIRS}")
CHECK_CXX_SOURCE_COMPILES("#include <GL/glew.h>\nvoid GLAPIENTRY OpenGLDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam) {}\nint main() { GLDEBUGPROCARB proc = &OpenGLDebugProc; }" GLDEBUGPROCARB_USERPARAM_IS_CONST)

if(UNIX AND NOT APPLE)
	FINDLIB(X11_LIBRARIES X11)
	if(USE_GTK)
		FINDLIB(XRANDR_LIBRARIES Xrandr)
	endif()
endif()

if (WIN32)
	find_package(DbgHelp)
	if(DBGHELP_FOUND)
		include_directories(${DBGHELP_INCLUDE_DIR})
	endif()
	set(HAVE_DBGHELP ${DBGHELP_FOUND})
endif()

find_package(Upnp)
if(UPNP_FOUND)
	include_directories(SYSTEM ${UPNP_INCLUDE_DIR})
endif()

find_package(Readline)
if(READLINE_FOUND)
	include_directories(SYSTEM ${READLINE_INCLUDE_DIR})
endif()
SET(HAVE_LIBREADLINE ${READLINE_FOUND} CACHE INTERNAL "libreadline available")

find_package(GTK3 COMPONENTS gthread gio gobject glib OPTIONAL_COMPONENTS gtksourceview)
if(USE_GTK)
	SET(WITH_GLIB ${GTK3_FOUND})
	include_directories(SYSTEM ${GTK3_INCLUDE_DIRS})
	# Set GTK link directory. This needs to be done before add_executable,
	# otherwise the path is not used for linking clonk
	link_directories(${GTK3_LIBRARY_DIRS})
endif()

# Select an audio library
find_package("Audio")
if(Audio_FOUND)
	include_directories(${Audio_INCLUDE_DIRS})
endif()

# SDL
if(USE_SDL_MAINLOOP)
	find_package(SDL2 REQUIRED)
else()
	# for gamepads
	find_package(SDL2)
endif()
set(HAVE_SDL ${SDL2_FOUND})
include_directories(SYSTEM ${SDL2_INCLUDE_DIRS})

# Qt5 for editor
find_path(Qt5DIR qt.pro PATHS ${CMAKE_ADDITIONAL_DEPS_PATH}/qt-5.5.0)
find_package(Qt5Widgets PATHS ${Qt5DIR}/qtbase/lib/cmake/Qt5Widgets)
if(Qt5Widgets_FOUND)
	SET(WITH_QT_EDITOR ${Qt5Widgets_FOUND} "Qt editor dialogues available")
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTOUIC ON)
	# As moc files are generated in the binary dir, tell CMake
	# to always look for includes there:
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
else()
	message("Qt5Widgets not found. Building openclonk without editor.")		
	UNSET(WITH_QT_EDITOR)
endif()

############################################################################
# generated source files
############################################################################
find_program(GLIB_COMPILE_RESOURCES glib-compile-resources)

add_custom_command(
	OUTPUT mape-resource.c
	COMMAND
		${GLIB_COMPILE_RESOURCES} "--internal" "--generate"
		"--target" "mape-resource.c"
		"--sourcedir" ${CMAKE_CURRENT_SOURCE_DIR}/src/res
		"${CMAKE_CURRENT_SOURCE_DIR}/src/res/mape.xml"
	MAIN_DEPENDENCY src/res/mape.xml
	DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/ocd.ico
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/ocf.ico
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/ocg.ico
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/ocm.ico
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/ocs.ico
	VERBATIM
)

add_custom_command(
	OUTPUT engine-resource.c
	COMMAND
		${GLIB_COMPILE_RESOURCES} "--internal" "--generate"
		"--target" "engine-resource.c"
		"--sourcedir" ${CMAKE_CURRENT_SOURCE_DIR}/src/res
		"${CMAKE_CURRENT_SOURCE_DIR}/src/res/engine.xml"
	MAIN_DEPENDENCY src/res/engine.xml
	DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Brush_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Halt_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Picker_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Cursor_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Play_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Dynamic_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Line_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Rect_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Exact_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Mouse_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Static_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/Fill_Trans.png
		${CMAKE_CURRENT_SOURCE_DIR}/src/res/oc.ico
	VERBATIM
)

############################################################################
# Mac OS bundle setup
############################################################################

if(APPLE)
	list(APPEND OC_BUNDLE_RESOURCES
		src/res/oc.icns
		src/res/ocd.icns src/res/ocf.icns src/res/ocg.icns
		src/res/C4P.icns src/res/ocs.icns src/res/ocu.icns
		src/res/Mouse_Trans.png
		src/res/Cursor_Trans.png
		src/res/Brush_Trans.png
		src/res/Play_Trans.png
		src/res/Halt_Trans.png
		src/res/Dynamic_Trans.png
		src/res/Static_Trans.png
		src/res/Exact_Trans.png
		src/res/Line_Trans.png
		src/res/Rect_Trans.png
		src/res/Fill_Trans.png
		src/res/Picker_Trans.png
	)

	if (CMAKE_GENERATOR STREQUAL Xcode)
		list(APPEND OC_BUNDLE_RESOURCES
			src/res/MainMenu.xib
			src/res/FullScreen.xib
			src/res/EditorGUIWindow.xib
			src/res/EditorViewport.xib
			src/res/Editor.xib
		)
	else()
		list(APPEND OC_BUNDLE_RESOURCES
			src/res/nib/MainMenu.nib
			src/res/nib/FullScreen.nib
			src/res/nib/EditorGUIWindow.nib
			src/res/nib/EditorViewport.nib
			src/res/nib/Editor.nib
		)
	endif()

	# Add icon resources
	set_source_files_properties(
		${OC_BUNDLE_RESOURCES}
		PROPERTIES MACOSX_PACKAGE_LOCATION Resources
	)
endif()

############################################################################
# List target source files. Don't directly create a target since we condi-
# tionally add more sources depending on configuration options later.
############################################################################
# OC_CLONK_SOURCES: Sources for OpenClonk that are needed by every build.
# OC_SYSTEM_SOURCES: Sources for OpenClonk that are only needed by specific
# 	platforms.
# OC_GUI_SOURCES: Sources that are only needed by GUI applications.
set(OC_CLONK_SOURCES
	src/C4Globals.cpp
	src/c4group/C4Components.h
	src/c4group/C4Extra.cpp
	src/c4group/C4Extra.h
	src/c4group/C4Language.cpp
	src/c4group/C4Language.h
	src/C4Include.cpp
	src/C4Include.h
	src/C4Prototypes.h
	src/C4Version.h
	src/config/C4Config.cpp
	src/config/C4Config.h
	src/config/C4Constants.h
	src/config/C4Reloc.cpp
	src/config/C4Reloc.h
	src/control/C4Control.cpp
	src/control/C4Control.h
	src/control/C4GameControl.cpp
	src/control/C4GameControl.h
	src/control/C4GameParameters.cpp
	src/control/C4GameParameters.h
	src/control/C4GameSave.cpp
	src/control/C4GameSave.h
	src/control/C4PlayerControl.cpp
	src/control/C4PlayerControl.h
	src/control/C4PlayerInfoConflicts.cpp
	src/control/C4PlayerInfo.cpp
	src/control/C4PlayerInfo.h
	src/control/C4Record.cpp
	src/control/C4Record.h
	src/control/C4RoundResults.cpp
	src/control/C4RoundResults.h
	src/control/C4Teams.cpp
	src/control/C4Teams.h
	src/editor/C4Console.cpp
	src/editor/C4ConsoleGUICommon.h
	src/editor/C4ConsoleGUI.h
	src/editor/C4Console.h
	src/editor/C4EditCursor.cpp
	src/editor/C4EditCursor.h
	src/editor/C4ObjectListDlg.cpp
	src/editor/C4ObjectListDlg.h
	src/editor/C4ToolsDlg.cpp
	src/editor/C4ToolsDlg.h
	src/editor/C4ViewportWindow.cpp
	src/editor/C4ViewportWindow.h
	src/game/C4Application.cpp
	src/game/C4Application.h
	src/game/C4FullScreen.cpp
	src/game/C4FullScreen.h
	src/game/C4Game.cpp
	src/game/C4Game.h
	src/game/C4GameScript.cpp
	src/game/C4GameScript.h
	src/game/C4GameVersion.h
	src/game/C4GraphicsSystem.cpp
	src/game/C4GraphicsSystem.h
	src/game/C4Physics.h
	src/game/C4Viewport.cpp
	src/game/C4Viewport.h
	src/graphics/Bitmap256.cpp
	src/graphics/Bitmap256.h
	src/graphics/C4Draw.cpp
	src/graphics/C4DrawGL.cpp
	src/graphics/C4DrawGLCtx.cpp
	src/graphics/C4DrawGL.h
	src/graphics/C4DrawMeshGL.cpp
	src/graphics/C4DrawT.cpp
	src/graphics/C4DrawT.h
	src/graphics/C4Draw.h
	src/graphics/C4Facet.cpp
	src/graphics/C4FacetEx.cpp
	src/graphics/C4FacetEx.h
	src/graphics/C4Facet.h
	src/graphics/C4FontLoader.cpp
	src/graphics/C4FontLoader.h
	src/graphics/C4GraphicsResource.cpp
	src/graphics/C4GraphicsResource.h
	src/graphics/C4Shader.cpp
	src/graphics/C4Shader.h
	src/graphics/C4Surface.cpp
	src/graphics/C4Surface.h
	src/graphics/C4SurfaceLoaders.cpp
	src/graphics/CSurface8.cpp
	src/graphics/CSurface8.h
	src/graphics/StdPNG.cpp
	src/graphics/StdPNG.h
	src/gui/C4ChatDlg.cpp
	src/gui/C4ChatDlg.h
	src/gui/C4DownloadDlg.cpp
	src/gui/C4DownloadDlg.h
	src/gui/C4FileSelDlg.cpp
	src/gui/C4FileSelDlg.h
	src/gui/C4Folder.cpp
	src/gui/C4Folder.h
	src/gui/C4GameDialogs.cpp
	src/gui/C4GameDialogs.h
	src/gui/C4GameLobby.cpp
	src/gui/C4GameLobby.h
	src/gui/C4GameMessage.cpp
	src/gui/C4GameMessage.h
	src/gui/C4GameOptions.cpp
	src/gui/C4GameOptions.h
	src/gui/C4GameOverDlg.cpp
	src/gui/C4GameOverDlg.h
	src/gui/C4GfxErrorDlg.cpp
	src/gui/C4GfxErrorDlg.h
	src/gui/C4GuiButton.cpp
	src/gui/C4GuiCheckBox.cpp
	src/gui/C4GuiComboBox.cpp
	src/gui/C4GuiContainers.cpp
	src/gui/C4Gui.cpp
	src/gui/C4GuiDialogs.cpp
	src/gui/C4GuiEdit.cpp
	src/gui/C4Gui.h
	src/gui/C4GuiLabels.cpp
	src/gui/C4GuiListBox.cpp
	src/gui/C4GuiMenu.cpp
	src/gui/C4GuiTabular.cpp
	src/gui/C4ScriptGuiWindow.cpp
	src/gui/C4KeyboardInput.cpp
	src/gui/C4KeyboardInput.h
	src/gui/C4LoaderScreen.cpp
	src/gui/C4LoaderScreen.h
	src/gui/C4MainMenu.cpp
	src/gui/C4MainMenu.h
	src/gui/C4Menu.cpp
	src/gui/C4Menu.h
	src/gui/C4MessageBoard.cpp
	src/gui/C4MessageBoard.h
	src/gui/C4MessageInput.cpp
	src/gui/C4MessageInput.h
	src/gui/C4MouseControl.cpp
	src/gui/C4MouseControl.h
	src/gui/C4PlayerInfoListBox.cpp
	src/gui/C4PlayerInfoListBox.h
	src/gui/C4Scoreboard.cpp
	src/gui/C4Scoreboard.h
	src/gui/C4ScriptGuiWindow.cpp
	src/gui/C4ScriptGuiWindow.h
	src/gui/C4StartupAboutDlg.cpp
	src/gui/C4StartupAboutDlg.h
	src/gui/C4Startup.cpp
	src/gui/C4Startup.h
	src/gui/C4StartupMainDlg.cpp
	src/gui/C4StartupMainDlg.h
	src/gui/C4StartupNetDlg.cpp
	src/gui/C4StartupNetDlg.h
	src/gui/C4StartupOptionsDlg.cpp
	src/gui/C4StartupOptionsDlg.h
	src/gui/C4StartupPlrSelDlg.cpp
	src/gui/C4StartupPlrSelDlg.h
	src/gui/C4StartupScenSelDlg.cpp
	src/gui/C4StartupScenSelDlg.h
	src/gui/C4UpperBoard.cpp
	src/gui/C4UpperBoard.h
	src/landscape/fow/C4FoW.cpp
	src/landscape/fow/C4FoW.h
	src/landscape/fow/C4FoWAmbient.cpp
	src/landscape/fow/C4FoWAmbient.h
	src/landscape/fow/C4FoWBeam.cpp
	src/landscape/fow/C4FoWBeam.h
	src/landscape/fow/C4FoWLight.cpp
	src/landscape/fow/C4FoWLight.h
	src/landscape/fow/C4FoWLightSection.cpp
	src/landscape/fow/C4FoWLightSection.h
	src/landscape/fow/C4FoWRegion.cpp
	src/landscape/fow/C4FoWRegion.h
	src/landscape/fow/C4FoWDrawStrategy.cpp
	src/landscape/fow/C4FoWDrawStrategy.h
	src/landscape/fow/C4FoWBeamTriangle.h
	src/landscape/C4Landscape.cpp
	src/landscape/C4Landscape.h
	src/landscape/C4LandscapeRender.cpp
	src/landscape/C4LandscapeRender.h
	src/landscape/C4Map.cpp
	src/landscape/C4MapCreatorS2.cpp
	src/landscape/C4MapCreatorS2.h
	src/landscape/C4Map.h
	src/landscape/C4MapScript.cpp
	src/landscape/C4MapScriptAlgo.cpp
	src/landscape/C4MapScript.h
	src/landscape/C4MassMover.cpp
	src/landscape/C4MassMover.h
	src/landscape/C4Material.cpp
	src/landscape/C4Material.h
	src/landscape/C4MaterialList.cpp
	src/landscape/C4MaterialList.h
	src/landscape/C4Particles.cpp
	src/landscape/C4Particles.h
	src/landscape/C4PathFinder.cpp
	src/landscape/C4PathFinder.h
	src/landscape/C4PXS.cpp
	src/landscape/C4PXS.h
	src/landscape/C4Scenario.cpp
	src/landscape/C4ScenarioSection.cpp
	src/landscape/C4Scenario.h
	src/landscape/C4Sky.cpp
	src/landscape/C4Sky.h
	src/landscape/C4SolidMask.cpp
	src/landscape/C4SolidMask.h
	src/landscape/C4Texture.cpp
	src/landscape/C4Texture.h
	src/landscape/C4TextureShape.cpp
	src/landscape/C4TextureShape.h
	src/landscape/C4TransferZone.cpp
	src/landscape/C4TransferZone.h
	src/landscape/C4Weather.cpp
	src/landscape/C4Weather.h
	src/lib/C4LogBuf.cpp
	src/lib/C4LogBuf.h
	src/lib/C4Log.cpp
	src/lib/C4Log.h
	src/lib/C4NameList.cpp
	src/lib/C4NameList.h
	src/lib/C4Rect.cpp
	src/lib/C4Rect.h
	src/lib/C4Stat.cpp
	src/lib/C4Stat.h
	src/lib/StdAdaptors.h
	src/lib/StdColors.h
	src/lib/StdMesh.cpp
	src/lib/StdMesh.h
	src/lib/StdMeshLoaderBinaryChunks.cpp
	src/lib/StdMeshLoaderBinaryChunks.h
	src/lib/StdMeshLoaderBinary.cpp
	src/lib/StdMeshLoaderDataStream.h
	src/lib/StdMeshLoader.cpp
	src/lib/StdMeshLoader.h
	src/lib/StdMeshLoaderXml.cpp
	src/lib/StdMeshMaterial.cpp
	src/lib/StdMeshMaterial.h
	src/lib/StdMeshMath.cpp
	src/lib/StdMeshMath.h
	src/lib/StdMeshUpdate.cpp
	src/lib/StdMeshUpdate.h
	src/network/C4Client.cpp
	src/network/C4Client.h
	src/network/C4GameControlNetwork.cpp
	src/network/C4GameControlNetwork.h
	src/network/C4InteractiveThread.cpp
	src/network/C4InteractiveThread.h
	src/network/C4League.cpp
	src/network/C4League.h
	src/network/C4Network2Client.cpp
	src/network/C4Network2Client.h
	src/network/C4Network2.cpp
	src/network/C4Network2Dialogs.cpp
	src/network/C4Network2Dialogs.h
	src/network/C4Network2Discover.cpp
	src/network/C4Network2Discover.h
	src/network/C4Network2.h
	src/network/C4Network2IO.cpp
	src/network/C4Network2IO.h
	src/network/C4Network2IRC.cpp
	src/network/C4Network2IRC.h
	src/network/C4Network2Players.cpp
	src/network/C4Network2Players.h
	src/network/C4Network2Reference.cpp
	src/network/C4Network2Reference.h
	src/network/C4Network2Res.cpp
	src/network/C4Network2ResDlg.cpp
	src/network/C4Network2Res.h
	src/network/C4Network2Stats.cpp
	src/network/C4Network2Stats.h
	src/network/C4Network2UPnP.h
	src/network/C4Packet2.cpp
	src/network/C4PacketBase.h
	src/object/C4Action.cpp
	src/object/C4Command.cpp
	src/object/C4Command.h
	src/object/C4Def.cpp
	src/object/C4DefGraphics.cpp
	src/object/C4DefGraphics.h
	src/object/C4Def.h
	src/object/C4DefList.cpp
	src/object/C4DefList.h
	src/object/C4FindObject.cpp
	src/object/C4FindObject.h
	src/object/C4GameObjects.cpp
	src/object/C4GameObjects.h
	src/object/C4Id.cpp
	src/object/C4Id.h
	src/object/C4IDList.cpp
	src/object/C4IDList.h
	src/object/C4InfoCore.cpp
	src/object/C4InfoCore.h
	src/object/C4MeshAnimation.cpp
	src/object/C4MeshAnimation.h
	src/object/C4Movement.cpp
	src/object/C4ObjectCom.cpp
	src/object/C4ObjectCom.h
	src/object/C4Object.cpp
	src/object/C4Object.h
	src/object/C4ObjectInfo.cpp
	src/object/C4ObjectInfo.h
	src/object/C4ObjectInfoList.cpp
	src/object/C4ObjectInfoList.h
	src/object/C4ObjectList.cpp
	src/object/C4ObjectList.h
	src/object/C4ObjectMenu.cpp
	src/object/C4ObjectMenu.h
	src/object/C4ObjectPtr.cpp
	src/object/C4ObjectPtr.h
	src/object/C4ObjectScript.cpp
	src/object/C4Sector.cpp
	src/object/C4Sector.h
	src/object/C4Shape.cpp
	src/object/C4Shape.h
	src/platform/C4App.cpp
	src/platform/C4App.h
	src/platform/C4AppWin32Impl.h
	src/platform/C4FileMonitor.cpp
	src/platform/C4FileMonitor.h
	src/platform/C4GamePadCon.cpp
	src/platform/C4GamePadCon.h
	src/platform/C4MusicFile.cpp
	src/platform/C4MusicFile.h
	src/platform/C4MusicSystem.cpp
	src/platform/C4MusicSystem.h
	src/platform/C4SoundLoaders.cpp
	src/platform/C4SoundLoaders.h
	src/platform/C4SoundIncludes.h
	src/platform/C4SoundInstance.cpp
	src/platform/C4SoundInstance.h
	src/platform/C4SoundModifiers.cpp
	src/platform/C4SoundModifiers.h
	src/platform/C4SoundSystem.cpp
	src/platform/C4SoundSystem.h
	src/platform/C4TimeMilliseconds.cpp 
	src/platform/C4TimeMilliseconds.h
	src/platform/C4Window.h
	src/platform/C4windowswrapper.h
	src/platform/PlatformAbstraction.cpp
	src/platform/PlatformAbstraction.h
	src/platform/StdSync.h
	src/player/C4Player.cpp
	src/player/C4Player.h
	src/player/C4PlayerList.cpp
	src/player/C4PlayerList.h
	src/player/C4RankSystem.cpp
	src/player/C4RankSystem.h
	src/player/C4ScenarioParameters.cpp
	src/player/C4ScenarioParameters.h
	src/script/C4AulDebug.cpp
	src/script/C4AulDebug.h
	thirdparty/timsort/sort.h
)

if(WITH_QT_EDITOR)
  qt5_add_resources(qt_editor_resources "src/editor/resource.qrc")
  set(QT_EDITOR_SOURCES
	  src/editor/C4ConsoleQt.cpp
	  src/editor/C4ConsoleQt.h
	  src/editor/C4ConsoleQtState.cpp
	  src/editor/C4ConsoleQtState.h
	  src/editor/C4ConsoleQtShapes.cpp
	  src/editor/C4ConsoleQtShapes.h
	  src/editor/C4ConsoleQtPropListViewer.cpp
	  src/editor/C4ConsoleQtPropListViewer.h
	  src/editor/C4ConsoleQtObjectListViewer.cpp
	  src/editor/C4ConsoleQtObjectListViewer.h
	  src/editor/C4ConsoleQtDefinitionListViewer.cpp
	  src/editor/C4ConsoleQtDefinitionListViewer.h
	  src/editor/C4ConsoleQtViewport.cpp
	  src/editor/C4ConsoleQtViewport.h
	  src/editor/C4ConsoleQtNewScenario.cpp
	  src/editor/C4ConsoleQtNewScenario.h
	  src/editor/C4ConsoleQtNewScenario.ui
	  src/editor/C4ConsoleQtMainWindow.ui
	  src/editor/resource.qrc
	  ${qt_editor_resources}
  )
endif()

set(MAPE_BASE_SOURCES
	src/C4Include.cpp
	src/landscape/C4MapCreatorS2.cpp
	src/landscape/C4MapCreatorS2.h
	src/landscape/C4MapScriptAlgo.cpp
	src/landscape/C4MapScript.cpp
	src/landscape/C4MapScript.h
	src/landscape/C4Material.cpp
	src/landscape/C4Material.h
	src/landscape/C4Texture.cpp
	src/landscape/C4Texture.h
	src/landscape/C4Scenario.cpp
	src/landscape/C4Scenario.h
	src/graphics/Bitmap256.cpp
	src/graphics/Bitmap256.h
	src/graphics/CSurface8.cpp
	src/graphics/CSurface8.h
	src/lib/C4NameList.cpp
	src/lib/C4NameList.h
	src/lib/C4Rect.cpp
	src/lib/C4Rect.h
	src/object/C4Id.cpp
	src/object/C4Id.h
	src/script/C4ScriptStandaloneStubs.cpp
)

set(MAPE_SOURCES
	src/mape/cpp-handles/c4def-handle.cpp
	src/mape/cpp-handles/group-handle.h
	src/mape/cpp-handles/group-handle.cpp
	src/mape/cpp-handles/landscape-handle.cpp
	src/mape/cpp-handles/log-handle.h
	src/mape/cpp-handles/log-handle.cpp
	src/mape/cpp-handles/mapgen-handle.h
	src/mape/cpp-handles/mapgen-handle.cpp
	src/mape/cpp-handles/material-handle.h
	src/mape/cpp-handles/material-handle.cpp
	src/mape/cpp-handles/random-handle.h
	src/mape/cpp-handles/random-handle.cpp
	src/mape/cpp-handles/texture-handle.h
	src/mape/cpp-handles/texture-handle.cpp
	src/mape/cpp-handles/version-handle.h
	src/mape/cpp-handles/version-handle.cpp
	src/mape/cpp-handles/stub-handle.cpp
	src/mape/configfile.c
	src/mape/configfile.h
	src/mape/diskview.c
	src/mape/diskview.h
	src/mape/editview.c
	src/mape/editview.h
	src/mape/fileicon.c
	src/mape/fileicon.h
	src/mape/forward.h
	src/mape/group.c
	src/mape/group.h
	src/mape/header.c
	src/mape/header.h
	src/mape/iconview.c
	src/mape/iconview.h
	src/mape/mape.c
	src/mape/mapgen.c
	src/mape/mapgen.h
	src/mape/material.c
	src/mape/material.h
	src/mape/mattexview.c
	src/mape/mattexview.h
	src/mape/preferences.c
	src/mape/preferences.h
	src/mape/preferencesdialog.c
	src/mape/preferencesdialog.h
	src/mape/preview.c
	src/mape/preview.h
	src/mape/random.c
	src/mape/random.h
	src/mape/statusbar.c
	src/mape/statusbar.h
	src/mape/texture.c
	src/mape/texture.h
	src/mape/window.c
	src/mape/window.h
	mape-resource.c
)

# source files specific to an operating system
if(APPLE)
	list(APPEND OC_SYSTEM_SOURCES
		src/platform/C4AppMac.mm
		src/platform/C4FileMonitorMac.mm
		src/platform/C4AppDelegate.h
		src/platform/C4AppDelegate.mm
		src/platform/StdSchedulerMac.mm
		src/platform/ObjectiveCAssociated.h
	)
	list(APPEND OC_GUI_SOURCES ${OC_BUNDLE_RESOURCES})
else()
	list(APPEND OC_SYSTEM_SOURCES
		src/game/ClonkMain.cpp
	)
endif()
if(WIN32)
	list(APPEND OC_SYSTEM_SOURCES
		src/platform/C4CrashHandlerWin32.cpp
	)
	list(APPEND OC_GUI_SOURCES
		src/res/engine.rc
		src/res/resource.h
	)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/res/openclonk.manifest" "${CMAKE_CURRENT_BINARY_DIR}/openclonk.manifest" COPYONLY)
endif()

if(NOT UPNP_FOUND)
	list(APPEND OC_SYSTEM_SOURCES
		src/network/C4Network2UPnPDummy.cpp
	)
elseif(UPNP_STYLE STREQUAL "Win32")
	list(APPEND OC_SYSTEM_SOURCES
		src/network/C4Network2UPnPWin32.cpp
	)
elseif(UPNP_STYLE STREQUAL "miniupnpc")
	list(APPEND OC_SYSTEM_SOURCES
		src/network/C4Network2UPnPLinux.cpp
	)
endif()

# source files specific to a GUI library
if(USE_GTK)
	list(APPEND OC_GUI_SOURCES
		src/editor/C4ConsoleGTK.cpp
		src/editor/C4ConsoleGTKDlg.cpp
		src/editor/C4ConsoleGTKDlg.h
		src/platform/C4AppGTK.cpp
		src/platform/C4WindowGTK.cpp
		engine-resource.c
	)
elseif(USE_SDL_MAINLOOP)
	list(APPEND OC_GUI_SOURCES
		src/platform/C4AppSDL.cpp
		src/platform/C4WindowSDL.cpp
	)
	if(WITH_QT_EDITOR)
		list(APPEND OC_GUI_SOURCES ${QT_EDITOR_SOURCES})
	endif()
elseif(USE_WIN32_WINDOWS)
	if(WITH_QT_EDITOR)
		list(APPEND OC_GUI_SOURCES ${QT_EDITOR_SOURCES})
	else()
		list(APPEND OC_GUI_SOURCES
			src/editor/C4ConsoleWin32.cpp
		)
	endif()
	list(APPEND OC_GUI_SOURCES
		src/platform/C4WindowWin32.cpp
	)
elseif(USE_COCOA)
	list(APPEND OC_GUI_SOURCES
		src/editor/C4ConsoleCocoa.mm
		src/platform/C4WindowMac.mm
		src/platform/C4AppDelegate+MainMenuActions.h
		src/platform/C4AppDelegate+MainMenuActions.mm
		src/graphics/C4DrawGLMac.h
		src/graphics/C4DrawGLMac.mm
		src/platform/C4WindowController.h
		src/platform/C4WindowController.mm
		src/platform/CocoaKeycodeMap.h
		src/editor/C4EditorWindowController.h
		src/editor/C4EditorWindowController.mm
	)
endif()
if(WITH_AUTOMATIC_UPDATE)
	list(APPEND OC_GUI_SOURCES
		src/gui/C4UpdateDlg.cpp
		src/gui/C4UpdateDlg.h
	)
endif()

if(PROJECT_FOLDERS)
  source_group("Core" src/.*)
  source_group("Platform abstraction" src/platform/.*)
  source_group("Utility" src/lib/.*)
  source_group("C4Group" src/c4group/.*)
  source_group("Graphics" src/graphics/.*)
  source_group("GUI" src/gui/.*)
  source_group("Network" src/network/.*)
  source_group("Object" src/object/.*)
  source_group("Landscape" src/landscape/.*)
  source_group("Player" src/player/.*)
  source_group("Script" REGULAR_EXPRESSION .*script/.*)
  source_group("Config" src/config/.*)
  source_group("Control" src/control/.*)
  source_group("Editing" src/editor/.*)
  source_group("Mape" src/mape/.*)
endif()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Mark thirdparty as system headers so we don't get warnings from them
include_directories(SYSTEM
	${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

############################################################################
# Generate output files
############################################################################
add_definitions(-DHAVE_CONFIG_H)

add_library(libmisc
src/C4Include.cpp
src/c4group/C4Group.cpp
src/c4group/C4Group.h
src/c4group/C4Update.cpp
src/c4group/C4Update.h
src/c4group/CStdFile.cpp
src/c4group/CStdFile.h
src/lib/C4InputValidation.cpp
src/lib/C4InputValidation.h
src/lib/C4Markup.cpp
src/lib/C4Markup.h
src/lib/C4SimpleLog.cpp
src/lib/SHA1.h
src/lib/Standard.cpp
src/lib/Standard.h
src/lib/StdBuf.cpp
src/lib/StdBuf.h
src/lib/StdCompiler.cpp
src/lib/StdCompiler.h
src/lib/StdResStr2.cpp
src/network/C4NetIO.cpp
src/network/C4NetIO.h
src/platform/StdFile.cpp
src/platform/StdFile.h
src/platform/StdRegistry.cpp
src/platform/StdRegistry.h
src/platform/StdScheduler.cpp
src/platform/StdSchedulerWin32.cpp
src/platform/StdSchedulerPoll.cpp
src/platform/StdScheduler.h
src/platform/C4TimeMilliseconds.cpp 
src/platform/C4TimeMilliseconds.h
src/zlib/gzio.c
src/zlib/gzio.h
src/zlib/zutil.h

	# pcg is a header-only library which we're listing solely so MSVC shows
	# the sources in the solution explorer. We could use an INTERFACE library
	# but there is no point to that because we don't need it for non-IDE
	# generators and support on IDE-targetting generators is nonexistant.
	thirdparty/pcg/pcg_extras.hpp
	thirdparty/pcg/pcg_random.hpp
	thirdparty/pcg/pcg_uint128.hpp
)

target_link_libraries(libmisc ${ZLIB_LIBRARIES})
if (WIN32)
	target_link_libraries(libmisc winmm)
endif()
if(HAVE_PTHREAD)
	target_link_libraries(libmisc pthread)
endif()
if(UNIX AND NOT APPLE)
	target_link_libraries(libmisc rt)
endif()

add_library(libc4script
src/C4Include.cpp
src/c4group/C4GroupSet.cpp
src/c4group/C4GroupSet.h
src/c4group/C4ComponentHost.cpp
src/c4group/C4ComponentHost.h
src/c4group/C4LangStringTable.cpp
src/c4group/C4LangStringTable.h
src/lib/C4Real.cpp
src/lib/C4Real.h
src/lib/C4Random.cpp
src/lib/C4Random.h
src/script/C4Aul.cpp
src/script/C4AulCompiler.h
src/script/C4AulCompiler.cpp
src/script/C4AulDefFunc.h
src/script/C4AulExec.cpp
src/script/C4AulExec.h
src/script/C4AulFunc.cpp
src/script/C4AulFunc.h
src/script/C4Aul.h
src/script/C4AulLink.cpp
src/script/C4AulParse.cpp
src/script/C4AulParse.h
src/script/C4AulScriptFunc.cpp
src/script/C4AulScriptFunc.h
src/script/C4Effect.cpp
src/script/C4Effect.h
src/script/C4PropList.cpp
src/script/C4PropList.h
src/script/C4Script.cpp
src/script/C4ScriptHost.cpp
src/script/C4ScriptHost.h
src/script/C4StringTable.cpp
src/script/C4StringTable.h
src/script/C4ValueArray.cpp
src/script/C4ValueArray.h
src/script/C4Value.cpp
src/script/C4Value.h
src/script/C4ValueMap.cpp
src/script/C4ValueMap.h
)

target_link_libraries(libc4script libmisc)

add_executable(c4script
	include/c4script/c4script.h
	src/script/C4ScriptMain.cpp
	src/script/C4ScriptStandalone.cpp
	src/script/C4ScriptStandaloneStubs.cpp
)
set_property(TARGET c4script PROPERTY FOLDER "Utilities")

target_link_libraries(c4script
	libc4script
	libmisc
)

add_executable(openclonk WIN32 MACOSX_BUNDLE
	${OC_SYSTEM_SOURCES}
	${OC_GUI_SOURCES}
	${OC_CLONK_SOURCES}
)

target_link_libraries(openclonk
	${FREETYPE_LIBRARIES}
	${PNG_LIBRARIES}
	${JPEG_LIBRARIES}
	${EXECINFO_LIBRARY}
	${SDL2_LIBRARIES}
	${Audio_LIBRARIES}
	${GETOPT_LIBRARIES}
	${GLEW_LIBRARIES}
	${OPENGL_LIBRARIES}
	${TinyXML_LIBRARIES}
	${X11_LIBRARIES}
	${XRANDR_LIBRARIES}
	${DBGHELP_LIBRARIES}
	${UPNP_LIBRARIES}
	libc4script
	libmisc
)
if(USE_GTK)
	target_link_libraries(openclonk ${GTK3_LIBRARIES})
endif()
if(USE_COCOA)
	target_link_libraries(openclonk "-framework Cocoa -framework AppKit -framework Quartz -framework OpenAL -framework AudioToolbox")
endif()

if(WITH_QT_EDITOR)
	set_target_properties(openclonk PROPERTIES COMPILE_DEFINITIONS WITH_QT_EDITOR)
	qt5_use_modules(openclonk Widgets)
endif()

add_executable(openclonk-server
	${OC_SYSTEM_SOURCES}
	${OC_CLONK_SOURCES}
	src/platform/C4AppT.cpp
	src/platform/C4StdInProc.cpp
	src/platform/C4StdInProc.h
)
target_compile_definitions(openclonk-server PRIVATE "USE_CONSOLE")

target_link_libraries(openclonk-server
	${PNG_LIBRARIES}
	${JPEG_LIBRARIES}
	${EXECINFO_LIBRARY}
	${SDL2_LIBRARIES}
	${READLINE_LIBRARIES}
	${Audio_LIBRARIES}
	${GETOPT_LIBRARIES}
	${GTK3_glib_LIBRARIES}
	${TinyXML_LIBRARIES}
	${DBGHELP_LIBRARIES}
	${UPNP_LIBRARIES}
	libc4script
	libmisc
)
if(USE_COCOA)
	target_link_libraries(openclonk-server "-framework Cocoa")
endif()

if(GTK3_FOUND AND GTK3_gtksourceview_FOUND)
	add_executable(mape ${MAPE_BASE_SOURCES} ${MAPE_SOURCES})
	target_compile_options(mape PRIVATE ${GTK3_COMPILE_DEFINITIONS} ${GTK3_gtksourceview_COMPILE_DEFINITIONS})
	set_property(TARGET mape PROPERTY FOLDER "Utilities")
	target_include_directories(mape PRIVATE ${GTK3_INCLUDE_DIRS} ${GTK3_gtksourceview_INCLUDE_DIRS})
	target_link_libraries(mape
		${GTK3_LIBRARIES}
		${GTK3_gtksourceview_LIBRARIES}
		libc4script
		libmisc
	)
	# Suppress the console window for mape even though only a main() and not
	# a WinMain() entry point is provided.
	if(MINGW)
		set_target_properties(mape PROPERTIES LINK_FLAGS -mwindows)
	endif()
endif()

add_executable(c4group
	src/c4group/C4GroupMain.cpp
)
set_property(TARGET c4group PROPERTY FOLDER "Utilities")

target_link_libraries(c4group
	libmisc
)

if(WIN32)
	target_link_libraries(openclonk ws2_32)
	target_link_libraries(openclonk-server ws2_32)
	target_link_libraries(c4group ws2_32)
	target_link_libraries(c4script ws2_32)
endif()

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG>)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)

# This expands some variables in Info.plist as a side-effect. XCode might then 
# expand a second time, using the same syntax. Try not to get confused by this!
if (APPLE AND NOT CMAKE_GENERATOR STREQUAL "Xcode")
	# If cross-compiling for OS X, EXECUTABLE_NAME needs to be set so it's
	# expanded inside the Info.plist file. Otherwise you cannot rename the app
	# bundle later.
	# XXX: Make sure to fix this once we have more than one app bundle. No, I
	# have no idea how.
	set(EXECUTABLE_NAME openclonk)
endif()

set_target_properties(openclonk PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/res/Info.plist")

if(MSVC)
	# set target output filenames to a per-configuration value
	function(oc_set_target_names target_name)
		foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(TOUPPER "${CONFIG}" CONFIG_UPPER)
			string(TOLOWER "${CONFIG}" CONFIG_LOWER)
			set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_CURRENT_BINARY_DIR}")
			if(NOT "${CONFIG}" STREQUAL "RelWithDebInfo")
				set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_NAME_${CONFIG_UPPER} "${target_name}-${CONFIG_LOWER}")
			endif()
		endforeach()
	endfunction()
	oc_set_target_names(openclonk)
	oc_set_target_names(openclonk-server)
	oc_set_target_names(c4group)
	oc_set_target_names(c4script)

	# cmake does not support embedding arbitrary manifests,
	# so we add it to the resource file ourselves and tell
	# MSVC not to create its own.
	set_property(TARGET openclonk APPEND PROPERTY LINK_FLAGS "/MANIFEST:NO")
endif()

############################################################################
# Precompiled header support for MSVC
############################################################################
if(HAVE_PRECOMPILED_HEADERS)
	if(MSVC)
		get_property(OC_SOURCES TARGET openclonk PROPERTY SOURCES)
		foreach(FILE ${OC_SOURCES})
			get_filename_component(FILE_EXT ${FILE} EXT)
			get_filename_component(FILE_NAME_WE ${FILE} NAME_WE)
			if(FILE_EXT STREQUAL ".cpp" AND NOT FILE STREQUAL qt_editor_resources)
				# The check for .cpp is necessary since gzio.c gets compiled as C
				# code, which can't use C++ PCH. Also, the auto-generated file
				# qt_editor_resources does not include the PCH.
				set_property(SOURCE ${FILE} PROPERTY COMPILE_FLAGS /YuC4Include.h)
			endif()
		endforeach()
		set_property(SOURCE src/C4Include.cpp PROPERTY COMPILE_FLAGS /YcC4Include.h)
	endif()
endif()

############################################################################
# Some Xcode/OSX specific settings involving building with clang, precompiled headers...
############################################################################

if (APPLE)
	# for good measure - seems to be part of default OSX installation
	# linking with static freetype library requires it
	find_package("BZip2" REQUIRED)
	target_link_libraries(openclonk ${BZIP2_LIBRARIES})

	if(CMAKE_GENERATOR STREQUAL Xcode)
		set(CMAKE_XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES)
		set_target_properties(openclonk PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES)
		set_target_properties(openclonk PROPERTIES XCODE_ATTRIBUTE_GCC_PFE_FILE_C_DIALECTS "c++14 objective-c++14")
		set_target_properties(openclonk PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/C4Include.h")
		set_target_properties(c4group PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES)
		set_target_properties(c4group PROPERTIES XCODE_ATTRIBUTE_GCC_PFE_FILE_C_DIALECTS "c++14 objective-c++14")
		set_target_properties(c4group PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/C4Include.h")
		set_target_properties(libmisc PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES)
		set_target_properties(libmisc PROPERTIES XCODE_ATTRIBUTE_GCC_PFE_FILE_C_DIALECTS "c++14 objective-c++14")
		set_target_properties(libmisc PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/C4Include.h")
		set_target_properties(openclonk PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
		
		set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
	endif()

	add_custom_command(TARGET openclonk
		POST_BUILD COMMAND
		"${CMAKE_CURRENT_SOURCE_DIR}/tools/osx_bundle_libs"
		"$<$<BOOL:${CMAKE_SYSROOT}>:--sysroot=${CMAKE_SYSROOT}>"
		"$<TARGET_FILE:openclonk>"
	)
endif()

############################################################################
# Miscellaneous
############################################################################
add_subdirectory(tests)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# For MSVC 2015, add a Solution Items folder to the solution that contains
# a reference to the debugger visualization file.
if(CMAKE_GENERATOR MATCHES "^Visual Studio 14 2015")
	file(RELATIVE_PATH NATVIS_SOURCE_PATH "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/openclonk.vs14.natvis")
	file(TO_NATIVE_PATH "${NATVIS_SOURCE_PATH}" NATVIS_SOURCE_PATH)
	# We need to set the GUID of this to the cache, so CMake uses it for
	# dependency resolution. As a special bonus, this makes it so the
	# project has an actual version 4 GUID instead of the CMake-generated
	# almost-version-3 (actually just a bunch of MD5 hex output with dashes).
	set(VS_SolutionItems_GUID "9570CE35-9E26-4091-AF06-6B0C7F9D8F69")
	set("Solution Items_GUID_CMAKE" "${VS_SolutionItems_GUID}" CACHE INTERNAL "Stored GUID")
	# Unfortunately CMake doesn't allow us to add solution items directly, so
	# instead we abuse that it doesn't validate the TYPE parameter and instead
	# passes it straight through into the .sln file.
	include_external_msproject(
		"Solution Items" "Solution Items"
		TYPE "2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = \"Solution Items\", \"Solution Items\", \"{${VS_SolutionItems_GUID}}\"\n	ProjectSection(SolutionItems) = preProject\n		${NATVIS_SOURCE_PATH} = ${NATVIS_SOURCE_PATH}\n	EndProjectSection\n#"
	)
endif()

############################################################################
# Precompiled header support, gcc part (it needs the cxx flags)
############################################################################

if(CMAKE_COMPILER_IS_GNUCXX)
	include(GccPchSupport)
	option(USE_GCC_PCH "Use GCC precompiled headers" OFF)
endif()
if(USE_GCC_PCH)
	add_precompiled_header(libmisc src/C4Include.h)
	add_precompiled_header(libc4script src/C4Include.h)
	add_precompiled_header(openclonk src/C4Include.h)
	add_precompiled_header(openclonk-server src/C4Include.h)
endif()

############################################################################
# installation
############################################################################
# Don't allow installation for WITH_AUTOMATIC_UPDATE because otherwise
# the installed binary wouldn't find its game data in the system data path.
IF(WITH_AUTOMATIC_UPDATE)
	INSTALL(CODE "MESSAGE(SEND_ERROR \"Installation is only supported for WITH_AUTOMATIC_UPDATE disabled\")")
ENDIF()

# hack to build the data on install, see
# http://public.kitware.com/Bug/view.php?id=8438
add_custom_target(data)
set_property(TARGET data PROPERTY FOLDER "Setup")
install(
	CODE
	"execute_process(
		COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target data
	)"
)

if (NOT APPLE)
	install(
		FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/res/oc32.png
		DESTINATION share/icons/hicolor/32x32/apps
		RENAME openclonk.png
	)
	install(
		FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/res/oc64.png
		DESTINATION share/icons/hicolor/64x64/apps
		RENAME openclonk.png
	)
	install(
		FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/res/oc128.png
		DESTINATION share/icons/hicolor/128x128/apps
		RENAME openclonk.png
	)
	install(
		FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/res/oc256.png
		DESTINATION share/icons/hicolor/256x256/apps
		RENAME openclonk.png
	)
	install(
		FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/res/oc512.png
		DESTINATION share/icons/hicolor/512x512/apps
		RENAME openclonk.png
	)
endif()

# Export native tools for later use in cross-compile builds
IF(NOT CMAKE_CROSSCOMPILING)
	EXPORT(TARGETS c4group FILE ${CMAKE_BINARY_DIR}/NativeToolsExport.cmake NAMESPACE native- )
ENDIF()

# When cross-compiling, import c4group from a native build
IF(CMAKE_CROSSCOMPILING)
	SET(IMPORT_NATIVE_TOOLS "IMPORT_NATIVE_TOOLS-NOTFOUND" CACHE FILEPATH "Export file from a native build")
	INCLUDE(${IMPORT_NATIVE_TOOLS})
	SET(native_c4group native-c4group)
ELSE()
	SET(native_c4group c4group)
ENDIF()

# NOTE: The scripts that does the autobuilds and ultimately the automated
# releases as well do keep their own list of group files around currently.
# So if you change anything here, change it in the release scripts as well.
# See openclonk-release-scripts.git/groupcontent.py
set(OC_C4GROUPS
	Graphics.ocg
	Material.ocg
	Music.ocg
	Sound.ocg
	System.ocg
	Objects.ocd
	Decoration.ocd
	Arena.ocf
	Parkour.ocf
	Defense.ocf
	Missions.ocf
	Tutorials.ocf
	Worlds.ocf
)

foreach(group ${OC_C4GROUPS})
	if (APPLE)
		if (CMAKE_GENERATOR STREQUAL Xcode)
			add_custom_command(TARGET openclonk
				POST_BUILD COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tools/osx_pack_gamedata.sh"
				"$<TARGET_FILE:${native_c4group}>"
				"${CMAKE_CURRENT_SOURCE_DIR}/planet/${group}"
				# leave out third parameter here so the script can figure out Xcode-ish paths as usual
				DEPENDS "${native_c4group}"
			)
		else()
			add_custom_command(TARGET openclonk
				POST_BUILD COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tools/osx_pack_gamedata.sh"
				"$<TARGET_FILE:${native_c4group}>"
				"${CMAKE_CURRENT_SOURCE_DIR}/planet/${group}"
				"${CMAKE_CURRENT_BINARY_DIR}/openclonk.app/Contents/Resources"
				DEPENDS "${native_c4group}"
			)
		endif()
	else()
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${group}
			COMMAND "${native_c4group}" ARGS "${CMAKE_CURRENT_SOURCE_DIR}/planet/${group}" -t "${CMAKE_CURRENT_BINARY_DIR}/${group}"
			MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/planet/${group}
			DEPENDS "${native_c4group}"
			VERBATIM
		)
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${group} DESTINATION share/games/openclonk)
	endif()
endforeach()

if (NOT APPLE)
	add_custom_target(groups DEPENDS ${OC_C4GROUPS})
	set_property(TARGET groups PROPERTY FOLDER "Setup")
	add_dependencies(data groups)
	
	# Install new files
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/openclonk.desktop DESTINATION share/applications)
	# Update the MIME cache -- this makes the clonk:// protocol handler actually work
	install(CODE "execute_process(COMMAND update-desktop-database)")

	# Install binaries
	install(TARGETS openclonk DESTINATION games)
	install(TARGETS c4group DESTINATION bin)
else()
	install(TARGETS openclonk
		BUNDLE DESTINATION .
	)
	install(TARGETS c4group c4script
		RUNTIME DESTINATION .
	)
endif()

############################################################################
# setup_openclonk.exe
############################################################################

find_program(MAKENSIS makensis PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\NSIS])

add_custom_command(
	OUTPUT setup_openclonk.exe
	COMMAND ${MAKENSIS} -NOCD -DSRCDIR=${CMAKE_CURRENT_SOURCE_DIR} -DPROGRAMFILES=$PROGRAMFILES "-DPRODUCT_NAME=${C4ENGINENAME}" "-DPRODUCT_COMPANY=${C4PROJECT}" "-DCLONK=$<TARGET_FILE_DIR:openclonk>\\$<TARGET_FILE_NAME:openclonk>" "-DC4GROUP=$<TARGET_FILE_DIR:c4group>\\$<TARGET_FILE_NAME:c4group>" ${CMAKE_CURRENT_SOURCE_DIR}/tools/install/oc.nsi "-XOutFile setup_openclonk.exe"
	MAIN_DEPENDENCY
		${CMAKE_CURRENT_SOURCE_DIR}/tools/install/oc.nsi
	DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/tools/install/header.bmp
		${CMAKE_CURRENT_SOURCE_DIR}/tools/install/inst.ico
		${CMAKE_CURRENT_SOURCE_DIR}/tools/install/uninst.ico
		${OC_C4GROUPS} openclonk c4group
	VERBATIM
)

add_custom_target(setup
	DEPENDS setup_openclonk.exe groups
)
set_property(TARGET setup PROPERTY FOLDER "Setup")

############################################################################
# CPack support
############################################################################
set(CPACK_PACKAGE_NAME "openclonk")
set(CPACK_PACKAGE_VENDOR "${C4PROJECT_URL}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenClonk - A game mix out of tactic, strategy and Jump & Run")
set(CPACK_PACKAGE_VERSION "${C4XVER1}.${C4XVER2}")
set(CPACK_PACKAGE_VERSION_MAJOR "${C4XVER1}")
set(CPACK_PACKAGE_VERSION_MINOR "${C4XVER2}")
set(CPACK_PACKAGE_FILE_NAME "openclonk-${C4XVER1}.${C4XVER2}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "openclonk-src-${C4XVER1}.${C4XVER2}")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
include(CPack)
