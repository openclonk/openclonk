# OpenClonk, http://www.openclonk.org
#
# Copyright (c) 2009-2014, The OpenClonk Team and contributors
#
# Distributed under the terms of the ISC license; see accompanying file
# "COPYING" for details.
#
# "Clonk" is a registered trademark of Matthes Bender, used with permission.
# See accompanying file "TRADEMARK" for details.
#
# To redistribute this file separately, substitute the full license texts
# for the above references.

# This module chooses an audio provider for use in OpenClonk.

macro(__FINDAUDIO_FINDOPENAL)
	find_package(PkgConfig QUIET)
	if(PKG_CONFIG_FOUND AND NOT(APPLE))
		pkg_check_modules(OpenAL "openal>=1.15.1")
		pkg_check_modules(Alut "freealut>=1.1.0")
		pkg_check_modules(OggVorbis "vorbisfile>=1.3.2" "vorbis>=1.3.2" "ogg>=1.3.0")
	else()
		if(MSVC OR APPLE)
			find_path(OpenAL_INCLUDE_DIRS al.h PATH_SUFFIXES include/AL include/OpenAL include OpenAL)
			find_path(Vorbis_INCLUDE_DIRS vorbis/codec.h vorbis/vorbisfile.h PATH_SUFFIXES include)
			find_library(Ogg_LIBRARY NAMES libogg_static libogg ogg)
			find_library(Vorbis_LIBRARY NAMES libvorbis_static libvorbis vorbis)
			find_library(Vorbisfile_LIBRARY NAMES libvorbisfile_static libvorbisfile vorbisfile)
			if(OpenAL_INCLUDE_DIRS)
				set(OpenAL_FOUND TRUE)
			endif()
			if(Vorbis_INCLUDE_DIRS AND Ogg_LIBRARY AND Vorbis_LIBRARY AND Vorbisfile_LIBRARY)
				set(OggVorbis_FOUND TRUE)
				set(OggVorbis_LIBRARIES ${Vorbisfile_LIBRARY} ${Vorbis_LIBRARY} ${Ogg_LIBRARY})
				set(OggVorbis_INCLUDE_DIRS(${Vorbis_INCLUDE_DIRS}))
			endif()
			find_path(Alut_INCLUDE_DIRS alut.h PATH_SUFFIXES include/AL include/OpenAL include)
			find_library(Alut_LIBRARY NAMES alut_static alut)
			if (MSVC)
				if(${FIND_LIBRARY_USE_LIB64_PATHS})
					find_library(OpenAL_LIBRARIES NAMES OpenAL64)
				else()
					find_library(OpenAL_LIBRARIES NAMES OpenAL32)
				endif()
				if(NOT OpenAL_LIBRARIES)
					set(OpenAL_FOUND FALSE)
				endif()
			endif()
			if(Alut_INCLUDE_DIRS AND Alut_LIBRARY)
				set(Alut_FOUND TRUE)
				set(Alut_LIBRARIES ${Alut_LIBRARY})
			endif()
		endif()
	endif()
endmacro()

macro(__FINDAUDIO_FINDSDLMIXER)
	find_package(PkgConfig QUIET)
	if(PKG_CONFIG_FOUND)
		pkg_check_modules(SDLMixer "SDL_mixer>=1.2.12")
	endif()
endmacro()

if(Audio_TK)
	# Already chosen, don't do anything
elseif(USE_CONSOLE)
	# Dedicated servers don't need audio output
	set(Audio_TK "none")
	set(Audio_FOUND TRUE)
else()
	# Test for OpenAL
	__FINDAUDIO_FINDOPENAL()
	__FINDAUDIO_FINDSDLMIXER()
	find_package("FMod")

	if(OpenAL_FOUND AND Alut_FOUND AND OggVorbis_FOUND)
		# Prefer OpenAL
		set(Audio_TK "OpenAL")
	elseif(SDLMixer_FOUND)
		set(Audio_TK "SDL_Mixer")
	elseif(FMOD_FOUND)
		set(Audio_TK "FMod")
	endif()
endif()

# Search for the libraries again. If the provider was selected automatically, this will be
# answered from cache; otherwise (because the user manually selected a provider) it will
# make sure the provider is available.
if(Audio_TK STREQUAL "OpenAL")
	__FINDAUDIO_FINDOPENAL()
	if(OpenAL_FOUND AND (APPLE OR Alut_FOUND) AND OggVorbis_FOUND)
		set(Audio_FOUND TRUE)
		set(Audio_LIBRARIES ${OpenAL_LIBRARIES} ${OggVorbis_LIBRARIES})
		set(Audio_INCLUDE_DIRS ${OpenAL_INCLUDE_DIRS} ${OggVorbis_INCLUDE_DIRS})
		set(Audio_LIBRARIES ${Audio_LIBRARIES} ${Alut_LIBRARIES})
		set(Audio_INCLUDE_DIRS ${Audio_INCLUDE_DIRS} ${Alut_INCLUDE_DIRS})
	endif()
elseif(Audio_TK STREQUAL "SDL_Mixer")
	__FINDAUDIO_FINDSDLMIXER()
	if(SDLMixer_FOUND)
		set(Audio_FOUND TRUE)
		set(Audio_LIBRARIES ${SDLMixer_LIBRARIES})
		set(Audio_INCLUDE_DIRS ${SDLMixer_INCLUDE_DIRS})
	endif()
elseif(Audio_TK STREQUAL "FMod")
	find_package("FMod")
	if(FMOD_FOUND)
		set(Audio_FOUND TRUE)
		set(Audio_LIBRARIES ${FMOD_LIBRARIES})
		set(Audio_INCLUDE_DIRS ${FMOD_INCLUDE_DIR})
	endif()
elseif(Audio_TK STREQUAL "none")
	set(Audio_FOUND TRUE)
	set(Audio_LIBRARIES "")
	set(Audio_INCLUDE_DIRS "")
endif()

if(Audio_FOUND)
	string(TOUPPER "${Audio_TK}" Audio_TK_UPPER)
	if(NOT Audio_FIND_QUIETLY)
		message(STATUS "Using Audio toolkit: ${Audio_TK}")
	endif()
elseif(Audio_TK)
	message(FATAL_ERROR "User-requested audio provider not available.")
else()
	set(Audio_FOUND FALSE)
	set(Audio_TK "none")
	string(TOUPPER "${Audio_TK}" Audio_TK_UPPER)
	set(Audio_LIBRARIES "")
	set(Audio_INCLUDE_DIRS "")

	if(Audio_FIND_REQUIRED)
		message(FATAL_ERROR "No audio provider was found")
	else()
		message(STATUS "Not enabling audio output.")
	endif()
endif()
