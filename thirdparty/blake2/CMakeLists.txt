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

add_library(blake2 STATIC
	blake2.h
)

target_sources(blake2
	PRIVATE
	blake2b.c
	blake2bp.c
	blake2s.c
	blake2sp.c
	blake2xb.c
	blake2xs.c
	blake2b-load-sse2.h
	blake2b-load-sse41.h
	blake2b-round.h
	blake2s-load-sse2.h
	blake2s-load-sse41.h
	blake2s-load-xop.h
	blake2s-round.h
	blake2-config.h
	blake2-impl.h
)

include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG("-msse2" HAVE_CFLAG_MSSE2)
target_compile_definitions(blake2 PRIVATE -DHAVE_SSE2)
if(HAVE_CFLAG_MSSE2)
	target_compile_options(blake2 PUBLIC -msse2)
endif()
target_include_directories(blake2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
