# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2018, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

# GCC6 doesn't work well with CMake while cross-compiling. See bugs:
# https://gitlab.kitware.com/cmake/cmake/issues/16291
# https://gitlab.kitware.com/cmake/cmake/issues/16919
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70129

function(HANDLE_GCC_CROSS_INCLUDE_PATHS _lang _gcc_lang_flag)
	set(_compiler "${CMAKE_${_lang}_COMPILER}")
	set(_compile_flags "${CMAKE_${_lang}_FLAGS}")
	separate_arguments(_compile_flags UNIX_COMMAND "${_compile_flags}")
	execute_process(
		COMMAND ${_compiler} ${_compile_flags} -v -E -x ${_gcc_lang_flag} /dev/null
		OUTPUT_QUIET
		ERROR_VARIABLE _compiler_output
		)
	if ("${_compiler_output}" MATCHES "#include <\\.\\.\\.> search starts here:\n *(.*)\nEnd of search list\\.")
		string(REGEX REPLACE "[\n ]+" " " _search_list "${CMAKE_MATCH_1}")
		separate_arguments(_search_list)
		foreach(_directory ${_search_list})
			get_filename_component(_fixed_component "${_directory}" REALPATH)
			set(_resolved_list ${_resolved_list} "${_fixed_component}")
		endforeach()
		set(CMAKE_${_lang}_IMPLICIT_INCLUDE_DIRECTORIES ${CMAKE_${_lang}_IMPLICIT_INCLUDE_DIRECTORIES} ${_resolved_list} PARENT_SCOPE)
	endif()
endfunction()
