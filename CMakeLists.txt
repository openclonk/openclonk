# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2009-2018, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

cmake_minimum_required (VERSION 3.5.1)

# Don't allow people to build "Release" builds because there's no reason to do that.
# Use one of RelWithDebInfo or MinSizeRel instead.
set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo MinSizeRel CACHE STRING "List of supported configuration types." FORCE)

# Set a default build type as the empty build type is without either debug
# symbols or optimizations and thus completely useless.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if(POLICY CMP0067)
	cmake_policy(SET CMP0067 NEW)
else()
	# Wrap try_compile such that it correctly passes the standard flags to the
	# compiler on CMake versions below 3.8. Remove when you update the minimum
	# required version to 3.8 or later.
	macro(try_compile)
		set(_tc_langs)
		foreach(_tc_lang CXX C)
			if(DEFINED CMAKE_${_tc_lang}_STANDARD)
				list(APPEND _tc_langs ${_tc_lang})
			endif()
		endforeach()
		foreach(_tc_lang ${_tc_langs})
			if (${CMAKE_${_tc_lang}_EXTENSIONS})
				set(_tc_ext EXTENSION)
			else()
				set(_tc_ext STANDARD)
			endif()
			set(_backup_${_tc_lang}_flags "${CMAKE_${_tc_lang}_FLAGS}")
			set(CMAKE_${_tc_lang}_FLAGS "${CMAKE_${_tc_lang}_FLAGS} ${CMAKE_${_tc_lang}${CMAKE_${_tc_lang}_STANDARD}_${_tc_ext}_COMPILE_OPTION}")
		endforeach()

		_try_compile(${ARGN})
	
		foreach(_tc_lang ${_tc_langs})
			set(CMAKE_${_tc_lang}_FLAGS "${_backup_${_tc_lang}_flags}")
		endforeach()
	endmacro()
endif()

project (openclonk CXX C)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Version)

if(DEFINED CMAKE_ADDITIONAL_DEPS_PATH)
	message(WARNING "CMAKE_ADDITIONAL_DEPS_PATH is deprecated and will be removed in a future version. Set CMAKE_PREFIX_PATH instead.")
	set(CMAKE_PREFIX_PATH ${CMAKE_ADDITIONAL_DEPS_PATH})
endif()

############################################################################
# User selectable options
############################################################################
include(CMakeDependentOption)
option(PROJECT_FOLDERS "Put source files into subfolders in project file" ON)
option(DEPLOY_QT_LIBRARIES "Deploy Qt libraries to executable path" OFF)
CMAKE_DEPENDENT_OPTION(USE_COCOA         "Use Apple Cocoa widgets." ON "APPLE" OFF)
CMAKE_DEPENDENT_OPTION(USE_WIN32_WINDOWS "Use Microsoft Desktop App User Interface widgets." ON "WIN32" OFF)
CMAKE_DEPENDENT_OPTION(USE_SDL_MAINLOOP  "Use SDL to create windows etc. Qt editor." ON "NOT USE_COCOA AND NOT USE_WIN32_WINDOWS AND NOT HEADLESS_ONLY" OFF)
option(WITH_AUTOMATIC_UPDATE "Automatic updates are downloaded from the project website." OFF)
CMAKE_DEPENDENT_OPTION(WITH_APPDIR_INSTALLATION "Install into an AppDir" OFF "UNIX AND NOT APPLE AND WITH_AUTOMATIC_UPDATE" ON)
option(HEADLESS_ONLY "Only build headless parts. Somewhat reduces dependencies. (still needs libpng because that one's small and hard to remove.) Only tested with make/gcc/linux." OFF)
option(C4GROUP_TOOL_ONLY "Only build c4group binary." OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ${PROJECT_FOLDERS})

############################################################################
# Assemble compiler flags
############################################################################
include(CheckCXXCompilerFlag)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to enable link-time optimization / link-time code generation /
# interprocedural optimization for release builds.
if(POLICY CMP0069)
	cmake_policy(SET CMP0069 NEW)

	include(CheckIPOSupported)
	check_ipo_supported(RESULT result OUTPUT output)
	if(result)
		foreach(_config MINSIZEREL RELWITHDEBINFO RELEASE)
			set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_${_config} TRUE)
		endforeach()
	else()
		message(WARNING "IPO is not supported: ${output}")
	endif()
else()
	message(WARNING "IPO is not supported because CMake is too old")
endif()

if(MSVC)
	if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
		# Set up some additional flags only if we're using the MS compiler,
		# and not the clang-based toolchain

		# Disable non-standard conversion from string literal to (nonconst) char*
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:strictStrings")

		# Activate edit-and-continue
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /ZI /Gy")
	endif()

	# Enable multi-core builds
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

	# Fix inlining option that CMake overrides per default.
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ob2")    
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /Ob2")

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
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wredundant-decls -Wendif-labels -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Winit-self -Wsign-promo -Wno-reorder -Wno-unused-parameter -Wnon-virtual-dtor -Woverloaded-virtual -Wformat-security")
endif()

# Deleting a pointer to an incomplete type is undefined behavior in almost all cases, so we don't want that to happen
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /we4150")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=delete-incomplete")
endif()

if(WIN32 AND MINGW)
	# Activate DEP and ASLR
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--nxcompat -Wl,--dynamicbase")
endif()

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -headerpad_max_install_names")
endif()

############################################################################
# Check for compiler quirks and features
############################################################################
if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CROSSCOMPILING)
	include(HandleGccCrossIncludePaths)
	HANDLE_GCC_CROSS_INCLUDE_PATHS(CXX c++)
	HANDLE_GCC_CROSS_INCLUDE_PATHS(C c)
endif()

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

############################################################################
# Check for required system headers
############################################################################
include(CheckIncludeFileCXX)
include(CheckIncludeFilesCXX)
CHECK_INCLUDE_FILE_CXX(io.h HAVE_IO_H)
CHECK_INCLUDE_FILE_CXX(share.h HAVE_SHARE_H)
CHECK_INCLUDE_FILE_CXX(signal.h HAVE_SIGNAL_H)
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
SET(JPEG_NAMES ${JPEG_NAMES} libjpeg jpeg-static)
if(NOT HEADLESS_ONLY AND NOT C4GROUP_TOOL_ONLY)
	find_package(Freetype REQUIRED)
	include_directories(
		${FREETYPE_INCLUDE_DIRS})
	link_directories(
		${FREETYPE_LIBRARY_DIRS})
endif()

# Needed for c4group
find_package(ZLIB REQUIRED)

if (NOT C4GROUP_TOOL_ONLY)
	find_package(JPEG REQUIRED)
	find_package(PNG REQUIRED)
	include_directories(
		${JPEG_INCLUDE_DIR}
		${PNG_INCLUDE_DIR}
		${ZLIB_INCLUDE_DIR})
	if(CMAKE_SYSTEM MATCHES "Windows")
		message(STATUS "Using Win32 threading.")
	else()
		find_package("Threads" REQUIRED)
		SET(HAVE_PTHREAD ${CMAKE_USE_PTHREADS_INIT} CACHE INTERNAL "libpthread available")
	endif()
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

if (NOT C4GROUP_TOOL_ONLY)
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
endif()

if(NOT HEADLESS_ONLY AND NOT C4GROUP_TOOL_ONLY)
	find_package(OpenGL)
	find_package(Epoxy REQUIRED)
	include_directories(${Epoxy_INCLUDE_DIRS})
endif()

if(NOT C4GROUP_TOOL_ONLY)
	if(UNIX AND NOT APPLE)
		FINDLIB(X11_LIBRARIES X11)

		set(Backward_DIR thirdparty/backward-cpp)
		find_package(Backward)
		if(Backward_FOUND)
			set(HAVE_BACKWARD 1)
		endif()

		if(NOT DEFINED PROC_SELF_EXE)
			macro(FIND_PROC_SELF_EXE path)
				if(EXISTS ${path})
					set(PROC_SELF_EXE ${path} CACHE FILEPATH "Path to /proc/self/exe (Linux) or equivalent")
				endif()
			endmacro()
			FIND_PROC_SELF_EXE(/proc/self/exe)     # Linux
			FIND_PROC_SELF_EXE(/proc/curproc/file) # FreeBSD
			FIND_PROC_SELF_EXE(/proc/curproc/exe)  # NetBSD
		endif()
	endif()
endif()

if (WIN32)
	find_package(DbgHelp)
	if(DBGHELP_FOUND)
		include_directories(${DBGHELP_INCLUDE_DIR})
	endif()
	set(HAVE_DBGHELP ${DBGHELP_FOUND})
endif()

if(NOT C4GROUP_TOOL_ONLY)
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
	find_package(Qt5 5.4 CONFIG COMPONENTS Core Widgets)
	if(TARGET Qt5::Core)
		SET(WITH_QT_EDITOR ON "Qt editor dialogues available")
		set(CMAKE_AUTOMOC ON)
		set(CMAKE_AUTOUIC ON)
		# As moc files are generated in the binary dir, tell CMake
		# to always look for includes there:
		set(CMAKE_INCLUDE_CURRENT_DIR ON)
	else()
		message("Qt5Widgets not found. Building openclonk without editor.")		
		UNSET(WITH_QT_EDITOR)
	endif()
endif()

############################################################################
# native tools
############################################################################
IF(CMAKE_CROSSCOMPILING)
	SET(IMPORT_NATIVE_TOOLS "IMPORT_NATIVE_TOOLS-NOTFOUND" CACHE FILEPATH "Export file from a native build")
	INCLUDE(${IMPORT_NATIVE_TOOLS})
	SET(native_c4group native-c4group)
	SET(native_oc-licenses-into-code native-oc-licenses-into-code)
ELSE()
	SET(native_c4group c4group)
	SET(native_oc-licenses-into-code oc-licenses-into-code)
ENDIF()

############################################################################
# generated source files
############################################################################
if(NOT C4GROUP_TOOL_ONLY)
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
endif()

IF(NOT CMAKE_CROSSCOMPILING)
	add_executable(oc-licenses-into-code tools/generate_license_headers.cpp)
	if (NOT C4GROUP_TOOL_ONLY)
		set_target_properties(oc-licenses-into-code PROPERTIES EXCLUDE_FROM_ALL On)
	endif()
endif()
set(OC_LICENSE_LIST
	TRADEMARK "Clonk Trademark"
	COPYING "OpenClonk Engine Engine License"
	planet/COPYING "OpenClonk Game Content License"
	licenses/xiph.txt "OGG Library"
	licenses/getopt.txt "Getopt Library"
	licenses/apache.txt "Various Libraries"
	licenses/LGPL.txt "Various Libraries"
	thirdparty/pcg/pcg_random.hpp "PCG Random number generator"
	thirdparty/timsort/README.rst "Timsort"
)
if(HAVE_BACKWARD)
	set(OC_LICENSE_LIST ${OC_LICENSE_LIST} thirdparty/backward-cpp/LICENSE.txt "backward-cpp Stack Trace Pretty Printer")
endif()
set(OC_LICENSES ${CMAKE_CURRENT_BINARY_DIR}/OCLicenses.cpp)
add_custom_command(
	OUTPUT ${OC_LICENSES}
	COMMENT "Generating OCLicenses.cpp"
	COMMAND ${native_oc-licenses-into-code} ${OC_LICENSES} ${OC_LICENSE_LIST}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS COPYING TRADEMARK planet/COPYING ${native_oc-licenses-into-code}
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
	src/gui/C4ChatDlg.cpp
	src/gui/C4ChatDlg.h
	src/gui/C4DownloadDlg.cpp
	src/gui/C4DownloadDlg.h
	src/gui/C4FileSelDlg.cpp
	src/gui/C4FileSelDlg.h
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
	src/gui/C4StartupLegalDlg.cpp
	src/gui/C4StartupLegalDlg.h
	src/gui/C4Startup.cpp
	src/gui/C4Startup.h
	src/gui/C4StartupMainDlg.cpp
	src/gui/C4StartupMainDlg.h
	src/gui/C4StartupNetDlg.cpp
	src/gui/C4StartupNetDlg.h
	src/gui/C4StartupModsDlg.cpp
	src/gui/C4StartupModsDlg.h
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
	src/landscape/C4MapScript.h
	src/landscape/C4Material.cpp
	src/landscape/C4Material.h
	src/landscape/C4Particles.cpp
	src/landscape/C4Particles.h
	src/landscape/C4PathFinder.cpp
	src/landscape/C4PathFinder.h
	src/landscape/C4PXS.cpp
	src/landscape/C4PXS.h
	src/landscape/C4Sky.cpp
	src/landscape/C4Sky.h
	src/landscape/C4SolidMask.cpp
	src/landscape/C4SolidMask.h
	src/landscape/C4Texture.cpp
	src/landscape/C4Texture.h
	src/landscape/C4TransferZone.cpp
	src/landscape/C4TransferZone.h
	src/landscape/C4Weather.cpp
	src/landscape/C4Weather.h
	src/lib/C4Rect.cpp
	src/lib/C4Rect.h
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
	src/object/C4Command.cpp
	src/object/C4Command.h
	src/object/C4Def.cpp
	src/object/C4DefGraphics.cpp
	src/object/C4DefGraphics.h
	src/object/C4Def.h
	src/object/C4DefList.cpp
	src/object/C4DefList.h
	src/object/C4GameObjects.cpp
	src/object/C4GameObjects.h
	src/object/C4IDList.cpp
	src/object/C4IDList.h
	src/object/C4MeshAnimation.cpp
	src/object/C4MeshAnimation.h
	src/object/C4MeshDenumerator.cpp
	src/object/C4MeshDenumerator.h
	src/object/C4ObjectCom.cpp
	src/object/C4ObjectCom.h
	src/object/C4ObjectCommand.cpp
	src/object/C4ObjectContents.cpp
	src/object/C4Object.cpp
	src/object/C4Object.h
	src/object/C4ObjectAction.cpp
	src/object/C4ObjectGraphics.cpp
	src/object/C4ObjectInfo.cpp
	src/object/C4ObjectInfo.h
	src/object/C4ObjectInfoList.cpp
	src/object/C4ObjectInfoList.h
	src/object/C4ObjectList.cpp
	src/object/C4ObjectList.h
	src/object/C4ObjectMenu.cpp
	src/object/C4ObjectMenu.h
	src/object/C4ObjectMenuLogic.cpp
	src/object/C4ObjectOCF.cpp
	src/object/C4ObjectPhysics.cpp
	src/object/C4ObjectRank.cpp
	src/object/C4ObjectScript.cpp
	src/object/C4ObjectVisibility.cpp
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
	src/platform/C4Window.h
	src/platform/C4windowswrapper.h
	src/platform/PlatformAbstraction.cpp
	src/platform/PlatformAbstraction.h
	src/platform/StdSync.h
	src/player/C4Achievement.cpp
	src/player/C4Achievement.h
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
	src/lib/C4LogBuf.cpp
	src/lib/C4LogBuf.h
	src/lib/C4Log.cpp
	src/lib/C4Log.h
)

if(WITH_QT_EDITOR)
  qt5_add_resources(qt_editor_resources "src/editor/resource.qrc")
  set(QT_EDITOR_SOURCES
	  src/editor/C4PropertyPath.cpp
	  src/editor/C4PropertyPath.h
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
	  src/editor/C4ConsoleQtLocalizeString.cpp
	  src/editor/C4ConsoleQtLocalizeString.h
	  src/editor/C4ConsoleQtLocalizeString.ui
	  src/editor/C4ConsoleQtLocalizeOverview.cpp
	  src/editor/C4ConsoleQtLocalizeOverview.h
	  src/editor/C4ConsoleQtLocalizeOverview.ui
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
set_property(SOURCE mape-resource.c PROPERTY SKIP_AUTOGEN ON)

# source files specific to an operating system
if(APPLE)
	list(APPEND OC_SYSTEM_SOURCES
		src/platform/C4AppMac.mm
		src/platform/C4FileMonitorMac.mm
		src/platform/C4AppDelegate.h
		src/platform/C4AppDelegate.mm
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
if(USE_SDL_MAINLOOP)
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
		src/platform/C4WindowMac.mm
		src/platform/C4AppDelegate+MainMenuActions.h
		src/platform/C4AppDelegate+MainMenuActions.mm
		src/graphics/C4DrawGLMac.h
		src/graphics/C4DrawGLMac.mm
		src/platform/C4WindowController.h
		src/platform/C4WindowController.mm
		src/platform/CocoaKeycodeMap.h
	)
	if(WITH_QT_EDITOR)
		list(APPEND OC_GUI_SOURCES ${QT_EDITOR_SOURCES})
	endif()
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

add_library(libmisc STATIC
src/C4Include.cpp
src/c4group/C4Group.cpp
src/c4group/C4Group.h
src/c4group/C4Update.cpp
src/c4group/C4Update.h
src/c4group/CStdFile.cpp
src/c4group/CStdFile.h
src/graphics/C4BltTransform.cpp
src/graphics/C4BltTransform.h
src/lib/C4InputValidation.cpp
src/lib/C4InputValidation.h
src/lib/C4Markup.cpp
src/lib/C4Markup.h
src/lib/C4Random.cpp
src/lib/C4Random.h
src/lib/C4SimpleLog.cpp
src/lib/SHA1.h
src/lib/Standard.cpp
src/lib/Standard.h
src/lib/StdBuf.cpp
src/lib/StdBuf.h
src/lib/StdCompiler.cpp
src/lib/StdCompiler.h
src/lib/StdResStr2.cpp
src/netpuncher/C4PuncherPacket.cpp
src/netpuncher/C4PuncherPacket.h
src/network/C4NetIO.cpp
src/network/C4NetIO.h
src/network/C4Network2Address.cpp
src/network/C4Network2Address.h
src/platform/StdFile.cpp
src/platform/StdFile.h
src/platform/StdRegistry.cpp
src/platform/StdRegistry.h
src/platform/StdScheduler.cpp
$<$<BOOL:${APPLE}>:src/platform/StdSchedulerMac.mm>
src/platform/StdSchedulerWin32.cpp
src/platform/StdSchedulerPoll.cpp
src/platform/StdScheduler.h
src/platform/C4TimeMilliseconds.cpp 
src/platform/C4TimeMilliseconds.h
src/zlib/gzio.c
src/zlib/gzio.h
src/zlib/zutil.h
${OC_LICENSES}

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

add_library(libc4script STATIC
src/C4Include.cpp
src/c4group/C4ComponentHost.cpp
src/c4group/C4ComponentHost.h
src/c4group/C4GroupSet.cpp
src/c4group/C4GroupSet.h
src/c4group/C4LangStringTable.cpp
src/c4group/C4LangStringTable.h
src/lib/C4RandomRecord.cpp
src/lib/C4Real.cpp
src/lib/C4Real.h
src/script/C4Aul.cpp
src/script/C4Aul.h
src/script/C4AulAST.h
src/script/C4AulCompiler.cpp
src/script/C4AulCompiler.h
src/script/C4AulDefFunc.h
src/script/C4AulExec.cpp
src/script/C4AulExec.h
src/script/C4AulFunc.cpp
src/script/C4AulFunc.h
src/script/C4AulLink.cpp
src/script/C4AulParse.cpp
src/script/C4AulParse.h
src/script/C4AulScriptFunc.cpp
src/script/C4AulScriptFunc.h
src/script/C4AulWarnings.h
src/script/C4Effect.cpp
src/script/C4Effect.h
src/script/C4PropList.cpp
src/script/C4PropList.h
src/script/C4Script.cpp
src/script/C4ScriptHost.cpp
src/script/C4ScriptHost.h
src/script/C4ScriptLibraries.cpp
src/script/C4ScriptLibraries.h
src/script/C4StringTable.cpp
src/script/C4StringTable.h
src/script/C4Value.cpp
src/script/C4Value.h
src/script/C4ValueArray.cpp
src/script/C4ValueArray.h
src/script/C4ValueMap.cpp
src/script/C4ValueMap.h
)
add_subdirectory(thirdparty/blake2)
set_property(TARGET blake2 PROPERTY FOLDER "Third-party libraries")
target_link_libraries(libc4script blake2 libmisc)

add_library(libopenclonk STATIC
	src/c4group/C4Extra.cpp
	src/c4group/C4Extra.h
	src/control/C4PlayerInfoConflicts.cpp
	src/graphics/Bitmap256.cpp
	src/graphics/Bitmap256.h
	src/graphics/CSurface8.cpp
	src/graphics/CSurface8.h
	src/graphics/StdPNG.cpp
	src/graphics/StdPNG.h
	src/gui/C4Folder.cpp
	src/gui/C4Folder.h
	src/landscape/C4MapScriptAlgo.cpp
	src/landscape/C4MassMover.cpp
	src/landscape/C4MassMover.h
	src/landscape/C4MaterialList.cpp
	src/landscape/C4MaterialList.h
	src/landscape/C4Scenario.cpp
	src/landscape/C4Scenario.h
	src/landscape/C4ScenarioSection.cpp
	src/landscape/C4TextureShape.cpp
	src/landscape/C4TextureShape.h
	src/lib/C4NameList.cpp
	src/lib/C4NameList.h
	src/lib/C4Stat.cpp
	src/lib/C4Stat.h
	src/lib/StdMeshMath.cpp
	src/lib/StdMeshMath.h
	src/network/C4Network2Discover.cpp
	src/network/C4Network2Discover.h
	src/object/C4Action.cpp
	src/object/C4FindObject.cpp
	src/object/C4FindObject.h
	src/object/C4Id.cpp
	src/object/C4Id.h
	src/object/C4InfoCore.cpp
	src/object/C4InfoCore.h
	src/object/C4Movement.cpp
	src/object/C4ObjectPtr.cpp
	src/object/C4ObjectPtr.h
	src/object/C4Sector.cpp
	src/object/C4Sector.h
	src/object/C4Shape.cpp
	src/object/C4Shape.h
	src/platform/C4TimeMilliseconds.cpp 
	src/platform/C4TimeMilliseconds.h
)

target_link_libraries(libopenclonk ${PNG_LIBRARIES})

target_compile_definitions(libmisc PRIVATE "COMPILED_AS_C4LIBRARY")
target_compile_definitions(libc4script PRIVATE "COMPILED_AS_C4LIBRARY")
target_compile_definitions(libopenclonk PRIVATE "COMPILED_AS_C4LIBRARY")

add_executable(c4script
	include/c4script/c4script.h
	src/script/C4ScriptMain.cpp
	src/script/C4ScriptStandalone.cpp
	src/script/C4ScriptStandaloneStubs.cpp
)
set_property(TARGET c4script PROPERTY FOLDER "Utilities")

target_compile_definitions(c4script PRIVATE "USE_CONSOLE")

target_link_libraries(c4script
	libmisc
	libc4script
	${GETOPT_LIBRARIES}
)

if(NOT HEADLESS_ONLY AND NOT C4GROUP_TOOL_ONLY)
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
		${Epoxy_LIBRARIES}
		${OPENGL_LIBRARIES}
		${TinyXML_LIBRARIES}
		${X11_LIBRARIES}
		${XRANDR_LIBRARIES}
		${DBGHELP_LIBRARIES}
		${UPNP_LIBRARIES}
		libmisc
		libc4script
		libopenclonk
	)
	if(USE_COCOA)
		target_link_libraries(openclonk "-framework Cocoa -framework AppKit -framework Quartz -framework OpenAL -framework AudioToolbox")
	endif()

	if(WITH_QT_EDITOR)
		set_target_properties(openclonk PROPERTIES COMPILE_DEFINITIONS WITH_QT_EDITOR)
		target_link_libraries(openclonk Qt5::Widgets)

		# Deploy dependency libraries and platforms/ folder
		if(DEPLOY_QT_LIBRARIES)
			if(WIN32)
				include(DeployQt)
				windeployqt(openclonk)
			elseif(APPLE)
				include(DeployQt)
				macdeployqt(openclonk)
			endif()
		endif()
	endif()

	if(HAVE_BACKWARD)
		target_link_libraries(openclonk Backward::Backward)
	endif()
endif()

if(NOT C4GROUP_TOOL_ONLY)
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
		${READLINE_LIBRARIES}
		${GETOPT_LIBRARIES}
		${TinyXML_LIBRARIES}
		${DBGHELP_LIBRARIES}
		libmisc
		libc4script
		libopenclonk
	)
	if(UPNP_FOUND)
		target_link_libraries(openclonk-server ${UPNP_LIBRARIES})
	endif()
	if(USE_COCOA)
		target_link_libraries(openclonk-server "-framework Cocoa")
	endif()
	if(HAVE_BACKWARD)
		target_link_libraries(openclonk-server Backward::Backward)
	endif()
	install(TARGETS openclonk-server DESTINATION bin)
endif()

if(GTK3_FOUND AND GTK3_gtksourceview_FOUND AND NOT HEADLESS_ONLY AND NOT C4GROUP_TOOL_ONLY)
	add_executable(mape ${MAPE_BASE_SOURCES} ${MAPE_SOURCES})
	target_compile_options(mape PRIVATE ${GTK3_COMPILE_DEFINITIONS} ${GTK3_gtksourceview_COMPILE_DEFINITIONS})
	set_property(TARGET mape PROPERTY FOLDER "Utilities")
	target_include_directories(mape PRIVATE ${GTK3_INCLUDE_DIRS} ${GTK3_gtksourceview_INCLUDE_DIRS})
	target_link_directories(mape PRIVATE
		${GTK3_LIBRARY_DIRS}
		${GTK3_gtksourceview_LIBRARY_DIRS}
	)
	target_link_libraries(mape
		${GTK3_LIBRARIES}
		${GTK3_gtksourceview_LIBRARIES}
		libmisc
		libc4script
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

if(NOT C4GROUP_TOOL_ONLY)
	add_executable(netpuncher EXCLUDE_FROM_ALL
		src/netpuncher/C4PuncherHash.h
		src/netpuncher/main.cpp
	)

	target_link_libraries(netpuncher
		libmisc
	)

	if(WIN32)
		target_link_libraries(libmisc iphlpapi ws2_32)
	endif()
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

if(NOT HEADLESS_ONLY AND NOT C4GROUP_TOOL_ONLY)
	set_target_properties(openclonk PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/res/Info.plist")
endif()

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
	if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
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
if(NOT C4GROUP_TOOL_ONLY)
	add_subdirectory(tests)
endif()

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
# installation
############################################################################
# Don't allow installation for WITH_AUTOMATIC_UPDATE because otherwise
# the installed binary wouldn't find its game data in the system data path.
IF(WITH_AUTOMATIC_UPDATE)
	if(WITH_APPDIR_INSTALLATION)
		INSTALL(CODE "MESSAGE(\"WITH_APPDIR_INSTALLATION is enabled, installation will create an AppDir\")")
	else()
		INSTALL(CODE "MESSAGE(SEND_ERROR \"Installation is only supported for WITH_AUTOMATIC_UPDATE disabled\")")
	endif()
ENDIF()

if(NOT C4GROUP_TOOL_ONLY)
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
endif()

if (NOT APPLE AND NOT C4GROUP_TOOL_ONLY)
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
	EXPORT(TARGETS c4group oc-licenses-into-code FILE ${CMAKE_BINARY_DIR}/NativeToolsExport.cmake NAMESPACE native- )
ENDIF()

if(NOT C4GROUP_TOOL_ONLY)
	# NOTE: The scripts that does the autobuilds and ultimately the automated
	# releases as well do keep their own list of group files around currently.
	# So if you change anything here, change it in the release scripts as well.
	# See openclonk-release-scripts.git/groupcontent.py
	set(OC_C4GROUPS
		Graphics.ocg
		Material.ocg
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
	if(APPLE)
		list(APPEND OC_C4GROUPS Music.ocg)
	endif()

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
				USES_TERMINAL # Hack: prevent parallel execution (for ninja), c4group tends to fail otherwise
			)
			install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${group} DESTINATION share/games/openclonk)
		endif()
	endforeach()
endif()

if(NOT APPLE)
	if(NOT HEADLESS_ONLY)
		install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/planet/Music.ocg DESTINATION share/games/openclonk)
	endif()
	if(NOT C4GROUP_TOOL_ONLY)
		# group files (game data)
		add_custom_target(groups DEPENDS ${OC_C4GROUPS})
		set_property(TARGET groups PROPERTY FOLDER "Setup")
		add_dependencies(data groups)
	endif()
	# Install binaries
	install(TARGETS c4group DESTINATION bin)
endif()
if(NOT HEADLESS_ONLY AND NOT C4GROUP_TOOL_ONLY)
	if(NOT APPLE)
		# Install new files
		install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/openclonk.desktop DESTINATION share/applications)
		# Update the MIME cache -- this makes the clonk:// protocol handler actually work
		install(CODE "message(\"Note: Run update-desktop-database to install clonk:// protocol handler\")")

		# Install software center metadata
		install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/openclonk.appdata.xml DESTINATION share/appdata)

		# Install binaries
		if(WITH_APPDIR_INSTALLATION)
			# AppDir expects binaries in /usr/bin
			install(TARGETS openclonk DESTINATION bin)
		else()
			install(TARGETS openclonk DESTINATION games)
		endif()
	else()
		install(TARGETS openclonk
			BUNDLE DESTINATION .
		)
		install(TARGETS c4group c4script
			RUNTIME DESTINATION .
		)
	endif()
endif()

############################################################################
# setup_openclonk.exe
############################################################################

if(NOT C4GROUP_TOOL_ONLY)
	find_program(MAKENSIS makensis PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\NSIS])

	if(NOT HEADLESS_ONLY)
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
	endif()
endif()

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
