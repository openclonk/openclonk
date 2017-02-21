#Copied from http://www.mail-archive.com/cmake@cmake.org/msg04394.html which copied it from the rosengarden project
#see also: http://gcc.gnu.org/onlinedocs/gcc-4.0.4/gcc/Precompiled-Headers.html

MACRO(ADD_PRECOMPILED_HEADER _targetName _input)

	# Prepare environment
	GET_FILENAME_COMPONENT(_name ${_input} NAME)
	SET(_pchdir "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_targetName}_pch.dir")
	MAKE_DIRECTORY(${_pchdir})
	SET(_source "${CMAKE_CURRENT_SOURCE_DIR}/${_input}")
	SET(_includeput "${_pchdir}/${_name}")
	SET(_output "${_includeput}.gch")

	# Assemble the compiler command with which future stuff will be built
	STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name) # Don't worry: CMake won't use any of the R/D/RWDI/MSR vars if the build type is not defined
	if(CMAKE_CXX_COMPILER_ARG1) # please, know what you're doing if you rely on this...
		STRING(STRIP ${CMAKE_CXX_COMPILER_ARG1} _ccache_tweak)
		SET(_ccache_tweak "\"${_ccache_tweak}\"")
	endif()
	SET(_compiler_FLAGS ${_ccache_tweak} ${CMAKE_CXX_FLAGS} ${${_flags_var_name}})
	if(CMAKE_BUILD_TYPE)
        if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
            LIST(APPEND _compiler_FLAGS "-D_DEBUG")
        endif()
	endif()
	GET_TARGET_PROPERTY(_compile_defines ${_targetName} COMPILE_DEFINITIONS)
	if(_compile_defines)
		FOREACH(item ${_compile_defines})
			STRING(REPLACE "\"" "\\\"" item ${item})
			LIST(APPEND _compiler_FLAGS "-D${item}")
		ENDFOREACH(item)
	endif()
	GET_TARGET_PROPERTY(_compile_defines ${_targetName} COMPILE_DEFINITIONS_${CMAKE_BUILD_TYPE})
	if(_compile_defines)
		FOREACH(item ${_compile_defines})
			STRING(REPLACE "\"" "\\\"" item ${item})
			LIST(APPEND _compiler_FLAGS "$<$<NOT:$<STREQUAL:,${item}>>:-D${item}>")
		ENDFOREACH(item)
	endif()
	GET_DIRECTORY_PROPERTY(_compile_defines COMPILE_DEFINITIONS)
	if(_compile_defines)
		FOREACH(item ${_compile_defines})
			STRING(REPLACE "\"" "\\\"" item ${item})
			LIST(APPEND _compiler_FLAGS "$<$<NOT:$<STREQUAL:,${item}>>:-D${item}>")
		ENDFOREACH(item)
	endif()
	GET_DIRECTORY_PROPERTY(_compile_defines COMPILE_DEFINITIONS_${CMAKE_BUILD_TYPE})
	if(_compile_defines)
		FOREACH(item ${_compile_defines})
			STRING(REPLACE "\"" "\\\"" item ${item})
			LIST(APPEND _compiler_FLAGS "$<$<NOT:$<STREQUAL:,${item}>>:-D${item}>")
		ENDFOREACH(item)
	endif()
	GET_DIRECTORY_PROPERTY(_directory_flags INCLUDE_DIRECTORIES)
	FOREACH(item ${_directory_flags})
		LIST(APPEND _compiler_FLAGS "-I\"${item}\"")
	ENDFOREACH(item)
	SEPARATE_ARGUMENTS(_compiler_FLAGS)
	list(REMOVE_DUPLICATES _compiler_FLAGS)
	# Add a target with the pch
	ADD_CUSTOM_COMMAND(
		OUTPUT ${_output}
		COMMAND "${CMAKE_CXX_COMPILER}"
			${_compiler_FLAGS} -x c++-header -o ${_output} ${_source}
		IMPLICIT_DEPENDS CXX ${_source}
	)
	FILE(WRITE "${_pchdir}/${_name}" "#ifdef __cplusplus\n#warning Precompiled header not used. Turn off or fix!\n#endif") # This file is added so the cc-units don't stumble over not being able to include the file
	ADD_CUSTOM_TARGET(${_targetName}_pch DEPENDS ${_output})
	ADD_DEPENDENCIES(${_targetName} ${_targetName}_pch)
	SET_TARGET_PROPERTIES(${_targetName} PROPERTIES	COMPILE_FLAGS "-include ${_includeput}")

ENDMACRO(ADD_PRECOMPILED_HEADER)
