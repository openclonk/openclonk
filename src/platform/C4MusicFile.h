/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2001  Michael Käser
 * Copyright (c) 2002-2003  Peter Wortmann
 * Copyright (c) 2005, 2008  Günther Brammer
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

#ifndef INC_C4MusicFile
#define INC_C4MusicFile

#ifdef USE_FMOD
#include <fmod.h>
#endif
#ifdef HAVE_LIBSDL_MIXER
#define USE_RWOPS
#include <SDL_mixer.h>
#undef USE_RWOPS
#endif
/* Base class */

class C4MusicFile
{
public:

	C4MusicFile() : LastPlayed(-1), NoPlay(FALSE), SongExtracted(FALSE) { }
	virtual ~C4MusicFile() { }

	// data
	char FileName[_MAX_FNAME +1];
	C4MusicFile *pNext;
	int LastPlayed;
	BOOL NoPlay;

	virtual BOOL Init(const char *strFile);
	virtual BOOL Play(BOOL loop = FALSE) = 0;
	virtual void Stop(int fadeout_ms = 0) = 0;
	virtual void CheckIfPlaying() = 0;
	virtual void SetVolume(int) = 0;

protected:

	// helper: copy data to a (temp) file
	BOOL ExtractFile();
	BOOL RemTempFile(); // remove the temp file

	BOOL SongExtracted;

};
#if defined(USE_FMOD) || defined(USE_WINDOWS_MIDI)
class C4MusicFileMID : public C4MusicFile
	{
	public:
	  BOOL Play(BOOL loop = FALSE);
		BOOL Extract();
		void Stop(int fadeout_ms = 0);
		void CheckIfPlaying();
		//C4MusicFileMID();
		void SetVolume(int);
#ifdef USE_FMOD
	protected:
		FMUSIC_MODULE *mod;
#endif
	};
#endif

#ifdef USE_FMOD
/* MOD class */

class C4MusicFileMOD : public C4MusicFile
{
public:
	C4MusicFileMOD();
	~C4MusicFileMOD();
  BOOL Play(BOOL loop = FALSE);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);
protected:
	FMUSIC_MODULE *mod;
	char *Data;
};

/* MP3 class */

class C4MusicFileMP3 : public C4MusicFile
{
public:
	C4MusicFileMP3();
	~C4MusicFileMP3();
  BOOL Play(BOOL loop = FALSE);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);
protected:
	FSOUND_STREAM	*stream;
	char *Data;
	int Channel;
};

/* Ogg class */

class C4MusicFileOgg : public C4MusicFile
{
public:
	C4MusicFileOgg();
	~C4MusicFileOgg();
  BOOL Play(BOOL loop = FALSE);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);

  static signed char __stdcall OnEnd(FSOUND_STREAM* stream, void* buff, int length, void* param);
protected:
	FSOUND_STREAM	*stream;
	char *Data;
	int Channel;

  BOOL Playing;
};
#endif

#ifdef HAVE_LIBSDL_MIXER
typedef struct _Mix_Music Mix_Music;
class C4MusicFileSDL : public C4MusicFile
{
public:
	C4MusicFileSDL();
	~C4MusicFileSDL();
  BOOL Play(BOOL loop = FALSE);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);
protected:
	char *Data;
	Mix_Music * Music;
};
#endif // HAVE_LIBSDL_MIXER

#endif
