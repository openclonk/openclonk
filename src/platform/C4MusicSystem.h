/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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

/* Handles Music.ocg and randomly plays songs */

#ifndef INC_C4MusicSystem
#define INC_C4MusicSystem

#include "c4group/C4Group.h"
#include "platform/C4SoundIncludes.h"

class C4MusicFileInfoNode;
class C4MusicFile;

class C4MusicSystem
{
	friend class C4SoundEffect;
	friend class C4SoundInstance;
	friend class C4SoundSystem;
public:
	C4MusicSystem();
	~C4MusicSystem();
	void Clear();
	void ClearGame();
	void UpdateVolume(); // compute volume from game + config data
	void Execute(bool force_buffer_checks = false);
	void NotifySuccess();
	bool Init(const char * PlayList = nullptr);
	bool InitForScenario(C4Group & hGroup);
	bool Play(const char *szSongname = nullptr, bool fLoop = false, int fadetime_ms = 0, double max_resume_time = 0.0, bool allow_break = false);
	bool Play(C4MusicFile *NewFile, bool fLoop, double max_resume_time);
	bool Stop();
	void FadeOut(int fadeout_ms);

	int SetPlayList(const char *szPlayList, bool fForceSwitch = false, int fadetime_ms = 0, double max_resume_time = 0.0);

	bool ToggleOnOff(); // keyboard callback

protected:
	// song list
	C4MusicFile* Songs;
	int SongCount, ASongCount, SCounter;

	// play
	C4MusicFile *PlayMusicFile;
	int Volume; bool Loop;

	// fading between two songs
	C4MusicFile *FadeMusicFile, *upcoming_music_file;
	C4TimeMilliseconds FadeTimeStart, FadeTimeEnd;

	// Wait time until next song
	bool is_waiting;
	C4TimeMilliseconds wait_time_end;

	void LoadDir(const char *szPath); // load some music files (by wildcard / directory)
	void Load(const char *szFile); // load a music file
	void LoadMoreMusic(); // load music file names from MoreMusic.txt
	void ClearSongs();

	bool GrpContainsMusic(C4Group &rGrp); // return whether this group contains music files

	bool ScheduleWaitTime();

	// SDL_mixer / OpenAL
	bool MODInitialized;
	bool InitializeMOD();
	void DeinitializeMOD();
#if AUDIO_TK == AUDIO_TK_OPENAL
private:
	ALCdevice* alcDevice;
	ALCcontext* alcContext;
public:
	void SelectContext();
	ALCcontext *GetContext() const { return alcContext; }
	ALCdevice *GetDevice() const { return alcDevice; }
#endif
public:
	inline bool IsMODInitialized() {return MODInitialized;}

private:
	// scenario-defined music level
	int32_t game_music_level;
	// current play list
	StdCopyStrBuf playlist;
	bool playlist_valid;
	// Set to nonzero to allow pauses between songs
	int32_t music_break_min, music_break_max, music_break_chance;
	// Maximum time (in seconds) last position in a song is remembered until it would just be restarted from the beginning
	int32_t music_max_position_memory;

	static const int32_t DefaultMusicBreak;
	static const int32_t DefaultMusicBreakChance;
	static const int32_t DefaultMusicMaxPositionMemory;

public:
	void CompileFunc(class StdCompiler *comp);

	void SetMusicBreakMin(int32_t val) { music_break_min = std::max<int32_t>(val, 0); }
	void SetMusicBreakMax(int32_t val) { music_break_max = std::max<int32_t>(val, 0); }
	void SetMusicBreakChance(int32_t val) { music_break_chance = Clamp<int32_t>(val, 0, 100); }
	void SetMusicMaxPositionMemory(int32_t val) { music_max_position_memory = val; }
	int32_t SetGameMusicLevel(int32_t val);
};


// --- helper stuff --- //

enum MusicType { MUSICTYPE_MID, MUSICTYPE_MOD, MUSICTYPE_MP3, MUSICTYPE_OGG, MUSICTYPE_UNKNOWN };

class C4MusicFileInfoNode // We need this for the MoreMusic.txt stuff
{
public:
	C4MusicFileInfoNode() { next=nullptr; str=nullptr; };
	~C4MusicFileInfoNode() { if (str) delete [] str; }
	char* str;
	MusicType type;
	C4MusicFileInfoNode *next;
};

MusicType GetMusicFileTypeByExtension(const char* ext);

#endif
