/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2016, The OpenClonk Team and contributors
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

#include "platform/C4SoundIncludes.h"
#include "platform/C4SoundLoaders.h"

/* Base class */

class C4MusicFile
{
public:

	C4MusicFile() :
		pNext(nullptr), LastPlayed(-1), NoPlay(false), loop(false), SongExtracted(false), announced(false)
	{ }
	virtual ~C4MusicFile() { }

	// data
	char FileName[_MAX_FNAME +1];
	C4MusicFile *pNext;
	int LastPlayed;
	bool NoPlay;
	bool loop;
	bool announced;

	virtual bool Init(const char *strFile);
	virtual bool Play(bool loop = false, double max_resume_time = 0.0) = 0;
	virtual void Stop(int fadeout_ms = 0) = 0;
	virtual void CheckIfPlaying() = 0;
	virtual void SetVolume(int) = 0;
	virtual bool HasCategory(const char *szcat) const { return false; }
	virtual double GetRemainingTime() { return 0.0; }
	virtual bool HasResumePos() const { return false; }
	virtual void ClearResumePos() { }
	virtual C4TimeMilliseconds GetLastInterruptionTime() const { return C4TimeMilliseconds(); }

	virtual StdStrBuf GetDebugInfo() const { return StdStrBuf(FileName); }

	bool IsLooping() const { return loop; }

	bool HasBeenAnnounced() const { return announced; }
	void Announce();

protected:

	// helper: copy data to a (temp) file
	bool ExtractFile();
	bool RemTempFile(); // remove the temp file

	bool SongExtracted;

};

#if AUDIO_TK == AUDIO_TK_SDL_MIXER
typedef struct _Mix_Music Mix_Music;
class C4MusicFileSDL : public C4MusicFile
{
public:
	C4MusicFileSDL();
	~C4MusicFileSDL();
	bool Play(bool loop = false, double max_resume_time = 0.0);
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
	bool Play(bool loop = false, double max_resume_time = 0.0);
	void Stop(int fadeout_ms = 0);
	void CheckIfPlaying();
	void SetVolume(int);
	virtual bool HasCategory(const char *szcat) const;
	double GetRemainingTime();
	bool HasResumePos() const { return (last_playback_pos_sec > 0);  }
	void ClearResumePos() { last_playback_pos_sec = 0.0; last_interruption_time = C4TimeMilliseconds(); }
	C4TimeMilliseconds GetLastInterruptionTime() const { return last_interruption_time; }
	virtual StdStrBuf GetDebugInfo() const;
private:
	enum { num_buffers = 4, buffer_size = 160*1024 };
	::C4SoundLoaders::VorbisLoader::CompressedData data;
	::C4SoundLoaders::SoundInfo ogg_info;
	OggVorbis_File ogg_file;
	bool is_loading_from_file;
	CStdFile source_file;
	long last_source_file_pos; // remember position in source_file because it may be closed and re-opened
	bool playing, streaming_done, loaded;
	ALuint buffers[num_buffers];
	ALuint channel;
	double last_playback_pos_sec; // last playback position for resume when fading between pieces
	C4TimeMilliseconds last_interruption_time; // set to nonzero when song is interrupted
	int current_section;
	size_t byte_pos_total;
	float volume;
	std::vector<StdCopyStrBuf> categories; // cateogries stored in meta info

	bool FillBuffer(size_t idx);
	void Execute(); // fill processed buffers
	void UnprepareSourceFileReading(); // close file handle but remember buffer position for re-opening
	bool PrepareSourceFileReading(); // close file handle but remember buffer position for re-opening

	friend class C4SoundLoaders::VorbisLoader;
};

#endif

#endif
