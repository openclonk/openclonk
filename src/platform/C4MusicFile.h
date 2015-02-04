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

#include <C4SoundIncludes.h>
#include <C4SoundLoaders.h>

/* Base class */

class C4MusicFile
{
public:

	C4MusicFile() :
		pNext(NULL), LastPlayed(-1), NoPlay(false), loop(false), SongExtracted(false)
	{ }
	virtual ~C4MusicFile() { }

	// data
	char FileName[_MAX_FNAME +1];
	C4MusicFile *pNext;
	int LastPlayed;
	bool NoPlay;
	bool loop;

	virtual bool Init(const char *strFile);
	virtual bool Play(bool loop = false) = 0;
	virtual void Stop(int fadeout_ms = 0) = 0;
	virtual void CheckIfPlaying() = 0;
	virtual void SetVolume(int) = 0;
	virtual bool HasCategory(const char *szcat) const { return false; }

	bool IsLooping() const { return loop; }

protected:

	// helper: copy data to a (temp) file
	bool ExtractFile();
	bool RemTempFile(); // remove the temp file

	bool SongExtracted;

};
#if AUDIO_TK == AUDIO_TK_FMOD
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

#elif AUDIO_TK == AUDIO_TK_SDL_MIXER
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

#elif AUDIO_TK == AUDIO_TK_OPENAL

class C4MusicFileOgg : public C4MusicFile
{
public:
	C4MusicFileOgg();
	~C4MusicFileOgg();
	void Clear();
	virtual bool Init(const char *strFile);
	bool Play(bool loop = false);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);
	virtual bool HasCategory(const char *szcat) const;
private:
	enum { num_buffers = 4, buffer_size = 160*1024 };
	::C4SoundLoaders::VorbisLoader::CompressedData data;
	::C4SoundLoaders::SoundInfo ogg_info;
	OggVorbis_File ogg_file;
	bool playing, streaming_done, loaded;
	ALuint buffers[num_buffers];
	ALuint channel;
	int current_section;
	size_t byte_pos_total;
	float volume;
	std::vector<StdCopyStrBuf> categories; // cateogries stored in meta info

	bool FillBuffer(size_t idx);
	void Execute(); // fill processed buffers
};

#endif

#endif
