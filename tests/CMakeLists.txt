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

# Look for GTest and GMock
# GMock's source package ships GTest too, so look for it first
find_path(GMOCK_ROOT
    NAMES "gmock-all.cc" "src/gmock-all.cc" "gmock/gmock-all.cc"
    PATHS "/usr/src/gmock"
    )
find_path(GMOCK_INCLUDE_DIR
    NAMES "gmock/gmock.h"
    HINTS "${GMOCK_ROOT}"
    PATH_SUFFIXES "include"
    )
find_file(GMOCK_SOURCE_ALL
    NAMES "gmock-all.cc"
    HINTS "${GMOCK_ROOT}"
    PATH_SUFFIXES "src" "gtest"
    NO_DEFAULT_PATH
    )

find_path(GTEST_ROOT
    NAMES "src/gtest-all.cc" "gtest/gtest-all.cc"
    PATHS "${GMOCK_ROOT}" "/usr/src/gtest"
    )
find_path(GTEST_INCLUDE_DIR
    NAMES "gtest/gtest.h"
    HINTS "${GTEST_ROOT}"
    PATH_SUFFIXES "include"
    )
find_file(GTEST_SOURCE_ALL
    NAMES "gtest-all.cc"
    HINTS "${GTEST_ROOT}"
    PATH_SUFFIXES "src" "gtest"
    NO_DEFAULT_PATH
    )

if (GMOCK_SOURCE_ALL AND GMOCK_INCLUDE_DIR)
    add_library(gmock STATIC
        "${GMOCK_SOURCE_ALL}"
        "${GMOCK_INCLUDE_DIR}/gmock/gmock.h"
        )
	set_property(TARGET gmock PROPERTY FOLDER "Third-party libraries")
    if (GMOCK_ROOT)
        include_directories("${GMOCK_ROOT}")
    endif()
    include_directories("${GMOCK_INCLUDE_DIR}")
    set(GMOCK_FOUND 1)
endif()

if (GTEST_SOURCE_ALL AND GTEST_INCLUDE_DIR)
    add_library(gtest STATIC
        "${GTEST_SOURCE_ALL}"
        "${GTEST_INCLUDE_DIR}/gtest/gtest.h"
        )
	set_property(TARGET gtest PROPERTY FOLDER "Third-party libraries")
    if (GTEST_ROOT)
        include_directories("${GTEST_ROOT}")
    endif()
     if(HAVE_PTHREAD)
        target_link_libraries(gtest pthread)
    endif()
    include_directories("${GTEST_INCLUDE_DIR}")
    set(GTEST_FOUND 1)
endif()

include(CMakeParseArguments)
function(create_test testName)
    set(oneValueArgs "")
    set(multiValueArgs SOURCES LIBRARIES)
    CMAKE_PARSE_ARGUMENTS(CT "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    add_executable("${testName}" EXCLUDE_FROM_ALL
		${CT_SOURCES}
		TestLog.cpp
		TestLog.h
		main.cpp
	)
	target_include_directories("${testName}" PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
	set_property(TARGET "${testName}" PROPERTY FOLDER "Testing")
    target_link_libraries("${testName}" gtest gmock ${CT_LIBRARIES})
    add_test(NAME "${testName}" COMMAND "${testName}")
endfunction()

if (GTEST_FOUND AND GMOCK_FOUND)
    enable_testing()

    create_test(StdMeshMath
        SOURCES
        "../src/lib/StdMeshMath.cpp"
        "../src/lib/StdMeshMath.h"
        "math/StdMeshVectorTest.cpp"
        "math/StdMeshQuaternionTest.cpp"
        )

    AUX_SOURCE_DIRECTORY("${CMAKE_CURRENT_LIST_DIR}" TESTS_SOURCES)
    add_executable(tests EXCLUDE_FROM_ALL ${TESTS_SOURCES} ${C4SCRIPT_SOURCES})
	set_property(TARGET "tests" PROPERTY FOLDER "Testing")
    target_link_libraries(tests gtest gmock libc4script libmisc)
    if(UNIX AND NOT APPLE)
	    target_link_libraries(tests rt)
    endif()
    if(WIN32)
        target_link_libraries(tests winmm)
    endif()

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
else()
    set(_gtest_missing "")
    if (NOT GTEST_INCLUDE_DIR)
        set(_gtest_missing "${_gtest_missing} gtest/gtest.h")
    endif()
    if (NOT GTEST_SOURCE_ALL)
        set(_gtest_missing "${_gtest_missing} gtest-all.cc")
    endif()
    if (NOT GMOCK_INCLUDE_DIR)
        set(_gtest_missing "${_gtest_missing} gmock/gmock.h")
    endif()
    if (NOT GMOCK_SOURCE_ALL)
        set(_gtest_missing "${_gtest_missing} gmock-all.cc")
    endif()
    message(STATUS "Could NOT find GTest/GMock (missing:${_gtest_missing})")
endif()
