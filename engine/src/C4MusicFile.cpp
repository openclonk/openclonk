/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2006  Sven Eberhardt
 * Copyright (c) 2001  Michael Käser
 * Copyright (c) 2002-2004  Peter Wortmann
 * Copyright (c) 2004  Armin Burgmeier
 * Copyright (c) 2005-2006, 2008  Günther Brammer
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */
/* Handles Music Files */

#include <C4Include.h>
#include <C4MusicFile.h>

#ifndef BIG_C4INCLUDE
#include <C4Application.h>
#include <C4Log.h>
#endif

#ifdef USE_FMOD
#include <fmod_errors.h>
#endif
#include <Midi.h>

/* helpers */

BOOL C4MusicFile::ExtractFile()
	{
	// safety
	if(SongExtracted) return TRUE;
	// extract entry
	if(!C4Group_CopyItem(FileName, Config.AtTempPath(C4CFN_TempMusic2))) return FALSE;
	// ok
	SongExtracted = TRUE;
	return TRUE;
	}

BOOL C4MusicFile::RemTempFile()
	{
	if(!SongExtracted) return TRUE;
	// delete it
	remove(Config.AtTempPath(C4CFN_TempMusic2));
	SongExtracted = FALSE;
	return TRUE;
	}

BOOL C4MusicFile::Init(const char *szFile)
	{
	SCopy(szFile, FileName);
	return TRUE;
	}

/* midi */

#ifdef USE_WINDOWS_MIDI
BOOL C4MusicFileMID::Play(BOOL loop)
	{

	// check existance
	if(!FileExists(FileName))
		// try extracting it
		if(!ExtractFile())
			// doesn't exist - or file is corrupt
			return FALSE;

	// Play Song
	PlayMidi(SongExtracted ? Config.AtTempPath(C4CFN_TempMusic2) : FileName, Application.hWindow);

	return TRUE;
	}

void C4MusicFileMID::Stop(int fadeout_ms)
	{
	// stop song
	StopMidi();
	// delete temp file
	RemTempFile();
	}

void C4MusicFileMID::CheckIfPlaying()
	{
	// windows will send the message
	}

void C4MusicFileMID::SetVolume(int iLevel)
	{
	// FIXME
	}
#endif

#ifdef USE_FMOD
BOOL C4MusicFileMID::Play(BOOL loop)
	{
	// check existance
	if(!FileExists(FileName))
		// try extracting it
		if(!ExtractFile())
			// doesn't exist - or file is corrupt
			return FALSE;

	// init fmusic
	mod = FMUSIC_LoadSong(SongExtracted ? Config.AtTempPath(C4CFN_TempMusic2) : FileName);

	if (!mod)
		{
		LogF("FMod: %s", FMOD_ErrorString(FSOUND_GetError()));
		return FALSE;
		}

	// Play Song
	FMUSIC_PlaySong(mod);

	return TRUE;
	}

void C4MusicFileMID::Stop(int fadeout_ms)
	{
	if(mod)
		{
		FMUSIC_StopSong(mod);
		FMUSIC_FreeSong(mod);
		mod = NULL;
		}
	RemTempFile();
	}

void C4MusicFileMID::CheckIfPlaying()
	{
	if (FMUSIC_IsFinished(mod))
		Application.MusicSystem.NotifySuccess();
	}

void C4MusicFileMID::SetVolume(int iLevel)
	{
		FMUSIC_SetMasterVolume(mod, BoundBy((iLevel * 256) / 100, 0, 255));
	}

/* MOD */

C4MusicFileMOD::C4MusicFileMOD()
: Data(NULL), mod(NULL)
	{

	}

C4MusicFileMOD::~C4MusicFileMOD()
	{
	Stop();
	}

BOOL C4MusicFileMOD::Play(BOOL loop)
	{
	// Load Song
	size_t iFileSize;
	if(!C4Group_ReadFile(FileName, &Data, &iFileSize))
		return FALSE;

	// init fmusic
	mod = FMUSIC_LoadSongEx(Data, 0, iFileSize, FSOUND_LOADMEMORY, 0, 0);

	if (!mod)
		{
		LogF("FMod: %s", FMOD_ErrorString(FSOUND_GetError()));
		return FALSE;
		}

	// Play Song
	FMUSIC_PlaySong(mod);

	return TRUE;
	}

void C4MusicFileMOD::Stop(int fadeout_ms)
	{
	if(mod)
		{
		FMUSIC_StopSong(mod);
		FMUSIC_FreeSong(mod);
		mod = NULL;
		}
	if(Data) { delete[] Data; Data = NULL; }
	}

void C4MusicFileMOD::CheckIfPlaying()
	{
	if (FMUSIC_IsFinished(mod))
		Application.MusicSystem.NotifySuccess();
	}

void C4MusicFileMOD::SetVolume(int iLevel)
	{
		FMUSIC_SetMasterVolume(mod, (int) ((iLevel * 255) / 100));
	}

/* MP3 */

C4MusicFileMP3::C4MusicFileMP3()
: Data(NULL), stream(NULL), Channel(-1)
	{

	}

C4MusicFileMP3::~C4MusicFileMP3()
	{
	Stop();
	}

BOOL C4MusicFileMP3::Play(BOOL loop)
	{
#ifndef USE_MP3
	return FALSE;
#endif

	// Load Song
	size_t iFileSize;
	if(!C4Group_ReadFile(FileName, &Data, &iFileSize))
		return FALSE;

	// init fsound
	int loop_flag = loop ? FSOUND_LOOP_NORMAL : 0;
	stream = FSOUND_Stream_Open(Data, FSOUND_LOADMEMORY | FSOUND_NORMAL | FSOUND_2D | loop_flag, 0, iFileSize);

	if(!stream) return FALSE;

	// Play Song
	Channel = FSOUND_Stream_Play(FSOUND_FREE, stream);
  if(Channel == -1) return FALSE;

  // Set highest priority
  if(!FSOUND_SetPriority(Channel, 255))
    return FALSE;

	return TRUE;
	}

void C4MusicFileMP3::Stop(int fadeout_ms)
	{
	if(stream)
		{
		FSOUND_Stream_Close(stream);
		stream = NULL;
		}
	if(Data) { delete[] Data; Data = NULL; }
	}

void C4MusicFileMP3::CheckIfPlaying()
	{
	if(FSOUND_Stream_GetPosition(stream) >= (unsigned) FSOUND_Stream_GetLength(stream))
		Application.MusicSystem.NotifySuccess();
	}

void C4MusicFileMP3::SetVolume(int iLevel)
	{
		FSOUND_SetVolume(Channel, (int) ((iLevel * 255) / 100));
	}

/* Ogg Vobis */

C4MusicFileOgg::C4MusicFileOgg()
: Data(NULL), stream(NULL), Channel(-1), Playing(FALSE)
	{

	}

C4MusicFileOgg::~C4MusicFileOgg()
	{
	Stop();
	}

BOOL C4MusicFileOgg::Play(BOOL loop)
	{
	// Load Song
	size_t iFileSize;
	if(!C4Group_ReadFile(FileName, &Data, &iFileSize))
		return FALSE;

	// init fsound
	int loop_flag = loop ? FSOUND_LOOP_NORMAL : 0;
	stream = FSOUND_Stream_Open(Data, FSOUND_LOADMEMORY | FSOUND_NORMAL | FSOUND_2D | loop_flag, 0, iFileSize);

	if(!stream) return FALSE;

	// Play Song
	Channel = FSOUND_Stream_Play(FSOUND_FREE, stream);
  if(Channel == -1) return FALSE;

  // Set highest priority
  if(!FSOUND_SetPriority(Channel, 255))
    return FALSE;

  Playing = TRUE;

  FSOUND_Stream_SetEndCallback(stream, &C4MusicFileOgg::OnEnd, this);

	return TRUE;
	}

// End Callback
signed char __stdcall C4MusicFileOgg::OnEnd(FSOUND_STREAM* stream, void* buff, int length, void *param)
{
  C4MusicFileOgg* pFile = static_cast<C4MusicFileOgg*>(param);
  pFile->Playing = FALSE;
  return 0;
}

void C4MusicFileOgg::Stop(int fadeout_ms)
	{
	if(stream)
		{
		FSOUND_Stream_Close(stream);
		stream = NULL;
		}
	if(Data) { delete[] Data; Data = NULL; }
  Playing = FALSE;
	}

void C4MusicFileOgg::CheckIfPlaying()
  {

  if(!Playing)
	//if(FSOUND_Stream_GetPosition(stream) >= (unsigned) FSOUND_Stream_GetLength(stream))
		Application.MusicSystem.NotifySuccess();
	}

void C4MusicFileOgg::SetVolume(int iLevel)
	{
		FSOUND_SetVolume(Channel, (int) ((iLevel * 255) / 100));
	}
#endif

#ifdef HAVE_LIBSDL_MIXER
C4MusicFileSDL::C4MusicFileSDL():
	Data(NULL),
	Music(NULL)
	{
	}

C4MusicFileSDL::~C4MusicFileSDL()
	{
	Stop();
	}

BOOL C4MusicFileSDL::Play(BOOL loop)
	{
	const SDL_version * link_version = Mix_Linked_Version();
	if (link_version->major < 1
	    || (link_version->major == 1 && link_version->minor < 2)
	    || (link_version->major == 1 && link_version->minor == 2 && link_version->patch < 7))
		{
		// Check existance and try extracting it
		if(!FileExists(FileName))	if(!ExtractFile())
			// Doesn't exist - or file is corrupt
			{
			LogF("Error reading %s", FileName);
			return FALSE;
			}
		// Load
		Music = Mix_LoadMUS(SongExtracted ? Config.AtTempPath(C4CFN_TempMusic2) : FileName);
		// Load failed
		if (!Music)
			{
			LogF("SDL_mixer: %s", SDL_GetError());
			return FALSE;
			}
		// Play Song
		if (Mix_PlayMusic(Music, loop? -1 : 1) == -1)
			{
			LogF("SDL_mixer: %s", SDL_GetError());
			return FALSE;
			}
		}
	else
		{
		// Load Song
		// Fixme: Try loading this from the group incrementally for less lag
		size_t filesize;
		if(!C4Group_ReadFile(FileName, &Data, &filesize))
			{
			LogF("Error reading %s", FileName);
			return FALSE;
			}
		// Mix_FreeMusic frees the RWop
		Music = Mix_LoadMUS_RW(SDL_RWFromConstMem(Data, filesize));
		if (!Music)
			{
			LogF("SDL_mixer: %s", SDL_GetError());
			return FALSE;
			}
		if (Mix_PlayMusic(Music, loop? -1 : 1) == -1)
			{
			LogF("SDL_mixer: %s", SDL_GetError());
			return FALSE;
			}
		}
	return TRUE;
	}

void C4MusicFileSDL::Stop(int fadeout_ms)
	{
	if (fadeout_ms && Music)
		{
		// Don't really stop yet
		Mix_FadeOutMusic(fadeout_ms);
		return;
		}
	if(Music)
		{
		Mix_FreeMusic(Music);
		Music = NULL;
		}
	RemTempFile();
	if(Data)
		{
		delete[] Data;
		Data = NULL;
		}
	}

void C4MusicFileSDL::CheckIfPlaying()
	{
	if (!Mix_PlayingMusic())
		Application.MusicSystem.NotifySuccess();
	}

void C4MusicFileSDL::SetVolume(int iLevel)
	{
	Mix_VolumeMusic((int) ((iLevel * MIX_MAX_VOLUME) / 100));
	}

#endif
