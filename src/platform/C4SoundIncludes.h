// Put all the terrible walls of includes for sound toolkits here

#ifndef INC_C4SoundIncludes
#define INC_C4SoundIncludes

#if AUDIO_TK == AUDIO_TK_FMOD
#	include <fmod.h>
	typedef FSOUND_SAMPLE* C4SoundHandle;
#	include <fmod_errors.h>
#elif AUDIO_TK == AUDIO_TK_SDL_MIXER
#	define USE_RWOPS
#	include <SDL_mixer.h>
#	undef USE_RWOPS
 	typedef struct Mix_Chunk* C4SoundHandle;
#	include <SDL.h>
#elif AUDIO_TK == AUDIO_TK_OPENAL
#	ifdef __APPLE__
#		include <OpenAL/al.h>
#	else
#		include <al.h>
#	endif
 	typedef ALuint C4SoundHandle;
#	ifdef _WIN32
 		// This is an ugly hack to make FreeALUT not dllimport everything.
#		define _XBOX
#	endif
#	include <alut.h>
#	undef _XBOX
#	if defined(__APPLE__)
#		import <CoreFoundation/CoreFoundation.h>
#		import <AudioToolbox/AudioToolbox.h>
#	endif
#	include <vorbis/codec.h>
#	include <vorbis/vorbisfile.h>
#	include <ogg/os_types.h>
#else
	typedef void* C4SoundHandle;
#endif

#endif