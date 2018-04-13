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

project (c4group CXX C)

# CMP0054: Only interpret if() arguments as variables or keywords when unquoted
# (since CMake 3.1)
if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

set(OC_TOP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../")
set(CMAKE_MODULE_PATH "${OC_TOP_SOURCE_DIR}cmake")


include(sourcesfiles)
include(Version)

set(CMAKE_ADDITIONAL_DEPS_PATH "${CMAKE_CURRENT_BINARY_DIR}/deps" CACHE PATH "Additional directory to search for libraries and headers")
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_ADDITIONAL_DEPS_PATH})

############################################################################
# User selectable options
############################################################################
include(CMakeDependentOption)
option(PROJECT_FOLDERS "Put source files into subfolders in project file" ON)
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

	# Enable LTCG for release builds
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ob2 /GL")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /Ob2 /GL")
	add_linker_flags(optimized MODULES exe shared static FLAGS "/LTCG:incremental")

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

if(WIN32 AND MINGW)
	# Activate DEP and ASLR
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--nxcompat -Wl,--dynamicbase")
endif()

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-arc -headerpad_max_install_names")
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
CHECK_CXX_SOURCE_COMPILES("#include <getopt.h>\nint main(int argc, char * argv[]) { getopt_long(argc, argv, \"\", 0, 0); }" HAVE_GETOPT_H)

############################################################################
# Locate libraries
############################################################################
find_package(ZLIB REQUIRED)
include_directories(
	${ZLIB_INCLUDE_DIR})
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
	include_directories(SYSTEM ${OC_TOP_SOURCE_DIR}thirdparty/getopt)
	add_subdirectory(thirdparty/getopt ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/getopt)
	set_property(TARGET getopt PROPERTY FOLDER "Third-party libraries")
	set(GETOPT_LIBRARIES getopt)
endif()

############################################################################
# Mac OS bundle setup
############################################################################

if(PROJECT_FOLDERS)
  source_group("Core" src/.*)
  source_group("Platform abstraction" src/platform/.*)
  source_group("Utility" src/lib/.*)
  source_group("C4Group" src/c4group/.*)
  source_group("Network" src/network/.*)
  source_group("Object" src/object/.*)
  source_group("Script" REGULAR_EXPRESSION .*script/.*)
  source_group("Config" src/config/.*)
  source_group("Control" src/control/.*)
endif()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${OC_TOP_SOURCE_DIR}src
)

# Mark thirdparty as system headers so we don't get warnings from them
include_directories(SYSTEM
	${OC_TOP_SOURCE_DIR}thirdparty
)

############################################################################
# Generate output files
############################################################################
add_definitions(-DHAVE_CONFIG_H)

add_library(libmisc STATIC
	${OC_LIBMISC_SOURCES}
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
	${OC_LIBC4SCRIPT_SOURCES}
)

add_subdirectory(../thirdparty/blake2 ${CMAKE_CURRENT_BINARY_DIR}/thirdparty/blake2)
set_property(TARGET blake2 PROPERTY FOLDER "Third-party libraries")
target_link_libraries(libc4script blake2 libmisc)

target_compile_definitions(libmisc PRIVATE "COMPILED_AS_C4LIBRARY")
target_compile_definitions(libc4script PRIVATE "COMPILED_AS_C4LIBRARY")

add_executable(c4script
	${OC_TOP_SOURCE_DIR}include/c4script/c4script.h
	${OC_TOP_SOURCE_DIR}src/script/C4ScriptMain.cpp
	${OC_TOP_SOURCE_DIR}src/script/C4ScriptStandalone.cpp
	${OC_TOP_SOURCE_DIR}src/script/C4ScriptStandaloneStubs.cpp
)
set_property(TARGET c4script PROPERTY FOLDER "Utilities")

target_compile_definitions(c4script PRIVATE "USE_CONSOLE")

target_link_libraries(c4script
	libmisc
	libc4script
	${GETOPT_LIBRARIES}
)

add_executable(c4group
	${OC_TOP_SOURCE_DIR}src/c4group/C4GroupMain.cpp
)
set_property(TARGET c4group PROPERTY FOLDER "Utilities")

target_link_libraries(c4group
	libmisc
)

add_executable(netpuncher EXCLUDE_FROM_ALL
	${OC_TOP_SOURCE_DIR}src/netpuncher/C4PuncherHash.h
	${OC_TOP_SOURCE_DIR}src/netpuncher/main.cpp
)

target_link_libraries(netpuncher
	libmisc
)

if(WIN32)
	target_link_libraries(libmisc iphlpapi ws2_32)
endif()

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:_DEBUG>)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)

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
	if(CMAKE_GENERATOR STREQUAL Xcode)
		set(CMAKE_XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES)
		set_target_properties(c4group PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES)
		set_target_properties(c4group PROPERTIES XCODE_ATTRIBUTE_GCC_PFE_FILE_C_DIALECTS "c++14 objective-c++14")
		set_target_properties(c4group PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/C4Include.h")
		set_target_properties(libmisc PROPERTIES XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES)
		set_target_properties(libmisc PROPERTIES XCODE_ATTRIBUTE_GCC_PFE_FILE_C_DIALECTS "c++14 objective-c++14")
		set_target_properties(libmisc PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/C4Include.h")
		
		set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
	endif()
endif()

############################################################################
# Miscellaneous
############################################################################

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# For MSVC 2015, add a Solution Items folder to the solution that contains
# a reference to the debugger visualization file.
if(CMAKE_GENERATOR MATCHES "^Visual Studio 14 2015")
	file(RELATIVE_PATH NATVIS_SOURCE_PATH "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/openclonk.vs14.natvis")
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
endif()

############################################################################
# installation
############################################################################

if(NOT APPLE)
	# Install binaries
	install(TARGETS c4group DESTINATION bin)
else()
	install(TARGETS c4group c4script
		RUNTIME DESTINATION .
	)
endif()

