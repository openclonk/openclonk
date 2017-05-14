# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2013-2015, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

# used by c4script and by the unit tests
set(C4SCRIPT_SOURCES
	../include/c4script/c4script.h
	../src/C4Include.cpp
	../src/script/C4ScriptStandalone.cpp
	../src/script/C4ScriptStandaloneStubs.cpp
)

include(DownloadProject)
download_project(
	PROJ     googletest
	URL      https://github.com/google/googletest/archive/release-1.8.0.tar.gz
	URL_HASH SHA256=58a6f4277ca2bc8565222b3bbd58a177609e9c488e8a72649359ba51450db7d8
)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})

include(CMakeParseArguments)
function(create_test testName)
	set(oneValueArgs "")
	set(multiValueArgs SOURCES LIBRARIES)
	CMAKE_PARSE_ARGUMENTS(CT "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	add_executable("${testName}"
		${CT_SOURCES}
		TestLog.cpp
		TestLog.h
		main.cpp
	)
	target_include_directories("${testName}" PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
	set_property(TARGET "${testName}" PROPERTY FOLDER "Testing")
	target_link_libraries("${testName}" gtest gmock ${CT_LIBRARIES})
	add_test(NAME "${testName}" COMMAND "${testName}")
	set_tests_properties(${testName} PROPERTIES ENVIRONMENT "PATH=${CMAKE_ADDITIONAL_DEPS_PATH}/bin;$ENV{PATH}")
endfunction()

create_test(StdMeshMath
	SOURCES
		../src/lib/StdMeshMath.cpp
		../src/lib/StdMeshMath.h
		math/StdMeshVectorTest.cpp
		math/StdMeshQuaternionTest.cpp
)

if(UNIX AND NOT APPLE)
	set(tests_LIBRARIES rt)
endif()
if(WIN32)
	set(tests_LIBRARIES winmm)
endif()

create_test(tests
	SOURCES
		C4NetIOTest.cpp
		C4StringTableTest.cpp
		C4ValueTest.cpp
		DirectExecTest.cpp
		UnicodeHandlingTest.cpp
		${C4SCRIPT_SOURCES}
	LIBRARIES
		libc4script
		libmisc
		${tests_LIBRARIES}
)

create_test(aul_test
	SOURCES
		aul/AulTest.cpp
		aul/AulTest.h
		aul/AulMathTest.cpp
		aul/AulPredefinedFunctionTest.cpp
		aul/AulDeathTest.cpp
		aul/AulDiagnosticsTest.cpp
		aul/AulSyntaxTest.cpp
		aul/AulSyntaxTestDetail.h
		aul/ErrorHandler.h
		../src/script/C4ScriptStandaloneStubs.cpp
		../src/script/C4ScriptStandalone.cpp
	LIBRARIES
		libmisc
		libc4script)
