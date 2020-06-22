# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2012-2016, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

function(git_get_changeset_id VAR)
	find_package(Git QUIET)
	if (GIT_FOUND)
		execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			COMMAND "${GIT_EXECUTABLE}" "rev-parse" "HEAD"
			RESULT_VARIABLE GIT_RESULT
			OUTPUT_VARIABLE C4REVISION
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET)

		if(GIT_RESULT EQUAL 0)
			string(SUBSTRING "${C4REVISION}" 0 12 C4REVISION)
			execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				COMMAND "${GIT_EXECUTABLE}" "status" "--porcelain"
				OUTPUT_VARIABLE GIT_STATUS
			)
			string(REGEX MATCH "^[MADRC ][MD ]" WORKDIR_DIRTY "${GIT_STATUS}")
			execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				COMMAND "${GIT_EXECUTABLE}" "rev-parse" "--git-path" "index"
				OUTPUT_VARIABLE GIT_INDEX
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)
			set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
				APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
				"${GIT_INDEX}"
			)
			execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				COMMAND "${GIT_EXECUTABLE}" "show" "--format=%ci" "-s" "HEAD"
				OUTPUT_VARIABLE GIT_TIMESTAMP
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)
			if(DEFINED ENV{C4REVISION_BRANCH})
				set(GIT_BRANCH "$ENV{C4REVISION_BRANCH}")
			else()
				execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
					COMMAND "${GIT_EXECUTABLE}" "symbolic-ref" "--short" "HEAD"
					RESULT_VARIABLE GIT_RESULT
					OUTPUT_VARIABLE GIT_BRANCH
					OUTPUT_STRIP_TRAILING_WHITESPACE
				)
				if(NOT GIT_RESULT EQUAL 0)
					set(GIT_BRANCH "unknown")
				endif()
			endif()
		endif()
	endif()
	if (NOT C4REVISION)
		# Git not found or not a git workdir
		file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.git_archival" C4REVISION
			LIMIT_COUNT 1
			REGEX "node: [0-9a-f]+"
		)
		string(LENGTH "${C4REVISION}" revlength)
		if(revlength LESS 18)
			set(C4REVISION "unknown")
			message(WARNING "Could not retrieve git revision. Please set GIT_EXECUTABLE!")
		else()
			string(SUBSTRING "${C4REVISION}" 6 12 C4REVISION)
		endif()
		unset(revlength)

		file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.git_archival" C4REVISION_TS
			LIMIT_COUNT 1
			REGEX "date: .+"
		)
		string(SUBSTRING "${C4REVISION_TS}" 6 -1 GIT_TIMESTAMP)
		set(GIT_BRANCH "unknown")
	endif()
	if(WORKDIR_DIRTY)
		set(WORKDIR_DIRTY 1)
	endif()
	set(${VAR} "${C4REVISION}" PARENT_SCOPE)
	set(${VAR}_DIRTY ${WORKDIR_DIRTY} PARENT_SCOPE)
	set(${VAR}_TS "${GIT_TIMESTAMP}" PARENT_SCOPE)
	set(${VAR}_BRANCH "${GIT_BRANCH}" PARENT_SCOPE)
endfunction()
