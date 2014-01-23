/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */
/* Handles Music Files */

#ifndef INC_C4MusicFile
#define INC_C4MusicFile

#if defined HAVE_FMOD
#include <fmod.h>
#elif defined HAVE_LIBSDL_MIXER
#define USE_RWOPS
#include <SDL_mixer.h>
#undef USE_RWOPS
#endif
/* Base class */

class C4MusicFile
{
public:

	C4MusicFile() : LastPlayed(-1), NoPlay(false), SongExtracted(false) { }
	virtual ~C4MusicFile() { }

	// data
	char FileName[_MAX_FNAME +1];
	C4MusicFile *pNext;
	int LastPlayed;
	bool NoPlay;

	virtual bool Init(const char *strFile);
	virtual bool Play(bool loop = false) = 0;
	virtual void Stop(int fadeout_ms = 0) = 0;
	virtual void CheckIfPlaying() = 0;
	virtual void SetVolume(int) = 0;

protected:

	// helper: copy data to a (temp) file
	bool ExtractFile();
	bool RemTempFile(); // remove the temp file

	bool SongExtracted;

};
#if defined HAVE_FMOD
class C4MusicFileMID : public C4MusicFile
{
public:
	bool Play(bool loop = false);
	bool Extract();
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	//C4MusicFileMID();
	void SetVolume(int);
protected:
	FMUSIC_MODULE *mod;
};

/* MOD class */

class C4MusicFileMOD : public C4MusicFile
{
public:
	C4MusicFileMOD();
	~C4MusicFileMOD();
	bool Play(bool loop = false);
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
	bool Play(bool loop = false);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);
protected:
	FSOUND_STREAM *stream;
	char *Data;
	int Channel;
};

/* Ogg class */

class C4MusicFileOgg : public C4MusicFile
{
public:
	C4MusicFileOgg();
	~C4MusicFileOgg();
	bool Play(bool loop = false);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);

	static signed char __stdcall OnEnd(FSOUND_STREAM* stream, void* buff, int length, void* param);
protected:
	FSOUND_STREAM *stream;
	char *Data;
	int Channel;

	bool Playing;
};

#elif defined HAVE_LIBSDL_MIXER
typedef struct _Mix_Music Mix_Music;
class C4MusicFileSDL : public C4MusicFile
{
public:
	C4MusicFileSDL();
	~C4MusicFileSDL();
	bool Play(bool loop = false);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);
protected:
	char *Data;
	Mix_Music * Music;
};
#endif // HAVE_LIBSDL_MIXER

#endif
