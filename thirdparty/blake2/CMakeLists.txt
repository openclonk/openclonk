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

include(CheckCSourceCompiles)

CHECK_C_SOURCE_COMPILES("
#if __x86_64 || __x86_64__ || __amd64 || __AMD64 || _M_X64
// x86-64 target system
#else
#error Not building for x86-64
#endif
int main() {}
" HAVE_X86_64)

option(BLAKE2B_USE_SSE2 "Use SSE2 instructions for BLAKE2b" ${HAVE_X86_64})

if (BLAKE2B_USE_SSE2)
	add_library(blake2 STATIC
		sse/blake2.h
	)

	target_sources(blake2
		PRIVATE
		sse/blake2b.c
		sse/blake2bp.c
		sse/blake2s.c
		sse/blake2sp.c
		sse/blake2xb.c
		sse/blake2xs.c
		sse/blake2b-load-sse2.h
		sse/blake2b-load-sse41.h
		sse/blake2b-round.h
		sse/blake2s-load-sse2.h
		sse/blake2s-load-sse41.h
		sse/blake2s-load-xop.h
		sse/blake2s-round.h
		sse/blake2-config.h
		sse/blake2-impl.h
	)

	include(CheckCCompilerFlag)
	CHECK_C_COMPILER_FLAG("-msse2" HAVE_CFLAG_MSSE2)
	target_compile_definitions(blake2 PRIVATE -DHAVE_SSE2)
	if(HAVE_CFLAG_MSSE2)
		target_compile_options(blake2 PUBLIC -msse2)
	endif()
	target_include_directories(blake2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/sse)
else()
	add_library(blake2 STATIC
		ref/blake2.h
	)

	target_sources(blake2
		PRIVATE
		ref/blake2bp-ref.c
		ref/blake2b-ref.c
		ref/blake2sp-ref.c
		ref/blake2s-ref.c
		ref/blake2xb-ref.c
		ref/blake2xs-ref.c
		ref/blake2-impl.h
	)
	target_include_directories(blake2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ref)
endif()
