/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
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

/* Handles Music.ocg and randomly plays songs */

#include "C4Include.h"
#include "platform/C4MusicSystem.h"

#include "game/C4Application.h"
#include "game/C4GraphicsSystem.h"
#include "lib/C4Random.h"
#include "platform/C4MusicFile.h"
#include "platform/C4Window.h"

C4MusicSystem::C4MusicSystem():
		playlist(),
		music_break_min(DefaultMusicBreak), music_break_max(DefaultMusicBreak),
		wait_time_end()
{
}

C4MusicSystem::~C4MusicSystem()
{
	Clear();
}

#if AUDIO_TK == AUDIO_TK_OPENAL
void C4MusicSystem::SelectContext()
{
	alcMakeContextCurrent(alcContext);
}
#endif

bool C4MusicSystem::InitializeMOD()
{
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
	SDL_version compile_version;
	const SDL_version * link_version;
	MIX_VERSION(&compile_version);
	link_version=Mix_Linked_Version();
	LogF("SDL_mixer runtime version is %d.%d.%d (compiled with %d.%d.%d)",
	     link_version->major, link_version->minor, link_version->patch,
	     compile_version.major, compile_version.minor, compile_version.patch);
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		LogF("SDL_InitSubSystem(SDL_INIT_AUDIO): %s", SDL_GetError());
		return false;
	}
	//frequency, format, stereo, chunksize
	if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024))
	{
		LogF("SDL_mixer: %s", SDL_GetError());
		return false;
	}
	MODInitialized = true;
	return true;
#elif AUDIO_TK == AUDIO_TK_OPENAL
	alcDevice = alcOpenDevice(nullptr);
	if (!alcDevice)
	{
		LogF("Sound system: OpenAL create context error");
		return false;
	}
	alcContext = alcCreateContext(alcDevice, nullptr);
	if (!alcContext)
	{
		LogF("Sound system: OpenAL create context error");
		return false;
	}
#ifndef __APPLE__
	if (!alutInitWithoutContext(nullptr, nullptr))
	{
		LogF("Sound system: ALUT init error");
		return false;
	}
#endif
	MODInitialized = true;
	return true;
#endif
	return false;
}

void C4MusicSystem::DeinitializeMOD()
{
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
	Mix_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
#elif AUDIO_TK == AUDIO_TK_OPENAL
#ifndef __APPLE__
	alutExit();
#endif
	alcDestroyContext(alcContext);
	alcCloseDevice(alcDevice);
	alcContext = nullptr;
	alcDevice = nullptr;
#endif
	MODInitialized = false;
}

bool C4MusicSystem::Init(const char * PlayList)
{
	// init mod
	if (!MODInitialized && !InitializeMOD()) return false;
	// Might be reinitialisation
	ClearSongs();
	// Global music file
	LoadDir(Config.AtSystemDataPath(C4CFN_Music));
	// User music file
	LoadDir(Config.AtUserDataPath(C4CFN_Music));
	// read MoreMusic.txt
	LoadMoreMusic();
	// set play list
	SCounter = 0;
	if (PlayList) SetPlayList(PlayList); else SetPlayList(nullptr);
	// set initial volume
	UpdateVolume();
	// ok
	return true;
}

bool C4MusicSystem::InitForScenario(C4Group & hGroup)
{
	// check if the scenario contains music
	bool fLocalMusic = false;
	StdStrBuf MusicDir;
	if (GrpContainsMusic(hGroup))
	{
		// clear global songs
		ClearSongs();
		fLocalMusic = true;
		// add songs
		MusicDir.Take(Game.ScenarioFile.GetFullName());
		LoadDir(MusicDir.getData());
		// log
		LogF(LoadResStr("IDS_PRC_LOCALMUSIC"), MusicDir.getData());
	}
	// check for music folders in group set
	C4Group *pMusicFolder = nullptr;
	while ((pMusicFolder = Game.GroupSet.FindGroup(C4GSCnt_Music, pMusicFolder)))
	{
		if (!fLocalMusic)
		{
			// clear global songs
			ClearSongs();
			fLocalMusic = true;
		}
		// add songs
		MusicDir.Take(pMusicFolder->GetFullName());
		MusicDir.AppendChar(DirectorySeparator);
		MusicDir.Append(C4CFN_Music);
		LoadDir(MusicDir.getData());
		// log
		LogF(LoadResStr("IDS_PRC_LOCALMUSIC"), MusicDir.getData());
	}
	// no music?
	if (!SongCount) return false;
	// set play list
	SetPlayList(nullptr);
	// ok
	return true;
}

void C4MusicSystem::Load(const char *szFile)
{
	// safety
	if (!szFile || !*szFile) return;
	C4MusicFile *NewSong=nullptr;
#if AUDIO_TK == AUDIO_TK_OPENAL
	// openal: Only ogg supported
	const char *szExt = GetExtension(szFile);
	if (SEqualNoCase(szExt, "ogg")) NewSong = new C4MusicFileOgg;
#elif AUDIO_TK == AUDIO_TK_SDL_MIXER
	if (GetMusicFileTypeByExtension(GetExtension(szFile)) == MUSICTYPE_UNKNOWN) return;
	NewSong = new C4MusicFileSDL;
#endif
	// unrecognized type/mod not initialized?
	if (!NewSong) return;
	// init music file
	NewSong->Init(szFile);
	// add song to list (push back)
	C4MusicFile *pCurr = Songs;
	while (pCurr && pCurr->pNext) pCurr = pCurr->pNext;
	if (pCurr) pCurr->pNext = NewSong; else Songs = NewSong;
	NewSong->pNext = nullptr;
	// count songs
	SongCount++;
	playlist_valid = false;
}

void C4MusicSystem::LoadDir(const char *szPath)
{
	char Path[_MAX_FNAME_LEN], File[_MAX_FNAME_LEN];
	C4Group *pDirGroup = nullptr;
	// split path
	SCopy(szPath, Path, _MAX_FNAME);
	char *pFileName = GetFilename(Path);
	SCopy(pFileName, File);
	*(pFileName - 1) = 0;
	// no file name?
	if (!File[0])
		// -> add the whole directory
		SCopy("*", File);
	// no wildcard match?
	else if (!SSearch(File, "*?"))
	{
		// then it's either a file or a directory - do the test with C4Group
		pDirGroup = new C4Group();
		if (!pDirGroup->Open(szPath))
		{
			// so it must be a file
			if (!pDirGroup->Open(Path))
			{
				// -> file/dir doesn't exist
				LogF("Music File not found: %s", szPath);
				delete pDirGroup;
				return;
			}
			// mother group is open... proceed with normal handling
		}
		else
		{
			// ok, set wildcard (load the whole directory)
			SCopy(szPath, Path);
			SCopy("*", File);
		}
	}
	// open directory group, if not already done so
	if (!pDirGroup)
	{
		pDirGroup = new C4Group();
		if (!pDirGroup->Open(Path))
		{
			LogF("Music File not found: %s", szPath);
			delete pDirGroup;
			return;
		}
	}
	// search file(s)
	char szFile[_MAX_FNAME_LEN];
	pDirGroup->ResetSearch();
	while (pDirGroup->FindNextEntry(File, szFile))
	{
		char strFullPath[_MAX_FNAME_LEN];
		sprintf(strFullPath, "%s%c%s", Path, DirectorySeparator, szFile);
		Load(strFullPath);
	}
	// free it
	delete pDirGroup;
}

void C4MusicSystem::LoadMoreMusic()
{
	StdStrBuf MoreMusicFile;
	// load MoreMusic.txt
	if (!MoreMusicFile.LoadFromFile(Config.AtUserDataPath(C4CFN_MoreMusic))) return;
	// read contents
	char *pPos = MoreMusicFile.getMData();
	while (pPos && *pPos)
	{
		// get line
		char szLine[1024 + 1];
		SCopyUntil(pPos, szLine, '\n', 1024);
		pPos = strchr(pPos, '\n'); if (pPos) pPos++;
		// remove leading whitespace
		char *pLine = szLine;
		while (*pLine == ' ' || *pLine == '\t' || *pLine == '\r') pLine++;
		// and whitespace at end
		char *p = pLine + strlen(pLine) - 1;
		while (*p == ' ' || *p == '\t' || *p == '\r') { *p = 0; --p; }
		// comment?
		if (*pLine == '#')
		{
			// might be a "directive"
			if (SEqual(pLine, "#clear"))
				ClearSongs();
			continue;
		}
		// try to load file(s)
		LoadDir(pLine);
	}
}

void C4MusicSystem::ClearSongs()
{
	Stop();
	while (Songs)
	{
		C4MusicFile *pFile = Songs;
		Songs = pFile->pNext;
		delete pFile;
	}
	SongCount = 0;
	FadeMusicFile = upcoming_music_file = PlayMusicFile = nullptr;
	playlist_valid = false;
}

void C4MusicSystem::Clear()
{
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
	// Stop a fadeout
	Mix_HaltMusic();
#endif
	ClearSongs();
	if (MODInitialized) { DeinitializeMOD(); }
}

void C4MusicSystem::ClearGame()
{
	game_music_level = 100;
	music_break_min = music_break_max = DefaultMusicBreak;
	music_break_chance = DefaultMusicBreakChance;
	music_max_position_memory = DefaultMusicMaxPositionMemory;
	SetPlayList(nullptr);
	is_waiting = false;
	upcoming_music_file = nullptr;
}

void C4MusicSystem::Execute(bool force_song_execution)
{
	// Execute music fading
	if (FadeMusicFile)
	{
		C4TimeMilliseconds tNow = C4TimeMilliseconds::Now();
		// Fading done?
		if (tNow >= FadeTimeEnd)
		{
			FadeMusicFile->Stop();
			FadeMusicFile = nullptr;
			if (PlayMusicFile)
			{
				PlayMusicFile->SetVolume(Volume);
			}
			else if (upcoming_music_file)
			{
				// Fade end -> start desired next immediately
				force_song_execution = true;
			}
		}
		else
		{
			// Fade process
			int fade_volume = 1000 * (tNow - FadeTimeStart) / (FadeTimeEnd - FadeTimeStart);
			FadeMusicFile->SetVolume(Volume * (1000 - fade_volume) / 1000);
			if (PlayMusicFile) PlayMusicFile->SetVolume(Volume * fade_volume / 1000);
		}
	}
	// Ensure a piece is played
#if AUDIO_TK != AUDIO_TK_SDL_MIXER
	if (!::Game.iTick35 || !::Game.IsRunning || force_song_execution || ::Game.IsPaused())
#else
	(void) force_song_execution;
#endif
	{
		if (!PlayMusicFile)
		{
			if (!is_waiting || (C4TimeMilliseconds::Now() >= wait_time_end))
			{
				// Play a song if no longer in silence mode and nothing is playing right now
				C4MusicFile *next_file = upcoming_music_file;
				is_waiting = false;
				upcoming_music_file = nullptr;
				if (next_file)
					Play(next_file, false, 0.0);
				else
					Play();
			}
		}
		else
		{
			// Calls NotifySuccess if a new piece had been selected.
			PlayMusicFile->CheckIfPlaying();
		}
	}
}

bool C4MusicSystem::Play(const char *szSongname, bool fLoop, int fadetime_ms, double max_resume_time, bool allow_break)
{
	// pause is done
	is_waiting = false;
	upcoming_music_file = nullptr;

	// music off?
	if (Game.IsRunning ? !Config.Sound.RXMusic : !Config.Sound.FEMusic)
		return false;

	// info
	if (::Config.Sound.Verbose)
	{
		LogF(R"(MusicSystem: Play("%s", %s, %d, %.3lf, %s))", szSongname ? szSongname : "(null)", fLoop ? "true" : "false", fadetime_ms, max_resume_time, allow_break ? "true" : "false");
	}

	C4MusicFile* NewFile = nullptr;

	// Specified song name
	if (szSongname && szSongname[0])
	{
		// Search in list
		for (NewFile = Songs; NewFile; NewFile = NewFile->pNext)
		{
			char songname[_MAX_FNAME_LEN];
			SCopy(szSongname, songname); DefaultExtension(songname, "mid");
			if (SEqual(GetFilename(NewFile->FileName), songname))
				break;
			SCopy(szSongname, songname); DefaultExtension(songname, "ogg");
			if (SEqual(GetFilename(NewFile->FileName), songname))
				break;
		}
	}
	else
	{
		// When resuming, prefer songs that were interrupted before
		if (max_resume_time > 0)
		{
			C4TimeMilliseconds t_now = C4TimeMilliseconds::Now();
			for (C4MusicFile *check_file = Songs; check_file; check_file = check_file->pNext)
				if (!check_file->NoPlay)
				{
					if (check_file->HasResumePos() && check_file->GetRemainingTime() > max_resume_time)
						if (!music_max_position_memory || (t_now - check_file->GetLastInterruptionTime() <= music_max_position_memory*1000))
							if (!NewFile || NewFile->LastPlayed < check_file->LastPlayed)
								NewFile = check_file;
				}
		}

		// Random song
		if (!NewFile)
		{
			// Intead of a new song, is a break also allowed?
			if (allow_break) ScheduleWaitTime();
			if (!is_waiting)
			{
				if (::Config.Sound.Verbose) LogF("  ASongCount=%d SCounter=%d", ASongCount, SCounter);
				// try to find random song
				int32_t new_file_playability = 0, new_file_num_rolls = 0;
				for (C4MusicFile *check_file = Songs; check_file; check_file = check_file->pNext)
				{
					if (!check_file->NoPlay)
					{
						// Categorize song playability:
						// 0 = no song found yet
						// 1 = song was played recently
						// 2 = song not played recently
						// 3 = song was not played yet
						int32_t check_file_playability = (check_file->LastPlayed < 0) ? 3 : (SCounter - check_file->LastPlayed <= ASongCount / 2) ? 1 : 2;
						if (::Config.Sound.Verbose) LogF("  Song LastPlayed %d [%d] (%s)", int(check_file->LastPlayed), int(check_file_playability), check_file->GetDebugInfo().getData());
						if (check_file_playability > new_file_playability)
						{
							// Found much better fit. Play this and reset number of songs found in same plyability
							new_file_num_rolls = 1;
							NewFile = check_file;
							new_file_playability = check_file_playability;
						}
						else if (check_file_playability == new_file_playability)
						{
							// Found a fit in the same playability category: Roll for it
							if (!UnsyncedRandom(++new_file_num_rolls)) NewFile = check_file;
						}
						else
						{
							// Worse playability - ignore this song
						}
					}
				}
			}

		}
	}

	// File (or wait time) found?
	if (!NewFile && !is_waiting)
		return false;

	// Stop/Fade out old music
	bool is_fading = (fadetime_ms && NewFile != PlayMusicFile && PlayMusicFile);
	if (!is_fading)
	{
		Stop();
	}
	else
	{
		C4TimeMilliseconds tNow = C4TimeMilliseconds::Now();
		if (FadeMusicFile)
		{
			if (FadeMusicFile == NewFile && FadeMusicFile->IsLooping() == fLoop && tNow < FadeTimeEnd)
			{
				// Fading back to a song while it wasn't fully faded out yet. Just swap our pointers and fix timings for that.
				FadeMusicFile = PlayMusicFile;
				PlayMusicFile = NewFile;
				FadeTimeEnd = tNow + fadetime_ms * (tNow - FadeTimeStart) / (FadeTimeEnd - FadeTimeStart);
				FadeTimeStart = FadeTimeEnd - fadetime_ms;
				return true;
			}
			else
			{
				// Fading to a third song while the previous wasn't faded out yet
				// That's pretty chaotic anyway, so just cancel the last song
				// Also happens if fading should already be done, in which case it won't harm to stop now
				// (It would stop on next call to Execute() anyway)
				// Also happens when fading back to the same song but loop status changes, but that should be really uncommon.
				FadeMusicFile->Stop();
			}

		}
		FadeMusicFile = PlayMusicFile;
		PlayMusicFile = nullptr;
		FadeTimeStart = tNow;
		FadeTimeEnd = FadeTimeStart + fadetime_ms;
	}

	// Waiting?
	if (!NewFile) return false;

	// If the old file is being faded out and a new file would just start, start delayed and without fading
	// so the beginning of a song isn't faded unnecesserily (because our songs often start very abruptly)
	if (is_fading && (!NewFile->HasResumePos() || NewFile->GetRemainingTime() <= max_resume_time))
	{
		upcoming_music_file = NewFile;
		is_waiting = true;
		wait_time_end = FadeTimeEnd;
		return false;
	}

	if (!Play(NewFile, fLoop, max_resume_time)) return false;

	if (is_fading) PlayMusicFile->SetVolume(0);

	return true;
}

bool C4MusicSystem::Play(C4MusicFile *NewFile, bool fLoop, double max_resume_time)
{
	// info
	if (::Config.Sound.Verbose)
	{
		LogF(R"(MusicSystem: PlaySong("%s", %s, %.3lf))", NewFile->GetDebugInfo().getData(), fLoop ? "true" : "false", max_resume_time);
	}
	// Play new song directly
	if (!NewFile->Play(fLoop, max_resume_time)) return false;
	PlayMusicFile = NewFile;
	NewFile->LastPlayed = SCounter++;
	Loop = fLoop;

	// Set volume
	PlayMusicFile->SetVolume(Volume);

	// Message first time a piece is played
	if (!NewFile->HasBeenAnnounced())
		NewFile->Announce();

	return true;
}

void C4MusicSystem::NotifySuccess()
{
	// nothing played?
	if (!PlayMusicFile) return;
	// loop?
	if (Loop)
		if (PlayMusicFile->Play())
			return;
	// clear last played piece
	Stop();
	// force a wait time after this song?
	ScheduleWaitTime();
}

bool C4MusicSystem::ScheduleWaitTime()
{
	// Roll for scheduling a break after the next piece.
	if (SCounter < 3) return false; // But not right away.
	if (int32_t(UnsyncedRandom(100)) >= music_break_chance) return false;
	if (music_break_max > 0)
	{
		int32_t music_break = music_break_min;
		if (music_break_max > music_break_min) music_break += UnsyncedRandom(music_break_max - music_break_min); // note that UnsyncedRandom has limited range
		if (music_break > 0)
		{
			is_waiting = true;
			wait_time_end = C4TimeMilliseconds::Now() + music_break;
			if (::Config.Sound.Verbose)
			{
				LogF("MusicSystem: Pause (%d msecs)", (int)music_break);
			}
			// After wait, do not resume previously started songs
			for (C4MusicFile *check_file = Songs; check_file; check_file = check_file->pNext)
				check_file->ClearResumePos();
		}
	}
	return is_waiting;
}

void C4MusicSystem::FadeOut(int fadeout_ms)
{
	// Kill any previous fading music and schedule current piece to fade
	if (PlayMusicFile)
	{
		if (FadeMusicFile) FadeMusicFile->Stop();
		FadeMusicFile = PlayMusicFile;
		PlayMusicFile = nullptr;
		FadeTimeStart = C4TimeMilliseconds::Now();
		FadeTimeEnd = FadeTimeStart + fadeout_ms;
	}
}

bool C4MusicSystem::Stop()
{
	if (PlayMusicFile)
	{
		PlayMusicFile->Stop();
		PlayMusicFile=nullptr;
	}
	if (FadeMusicFile)
	{
		FadeMusicFile->Stop();
		FadeMusicFile = nullptr;
	}
	return true;
}

void C4MusicSystem::UpdateVolume()
{
	// Save volume for next file
	int32_t config_volume = Clamp<int32_t>(Config.Sound.MusicVolume, 0, 100);
	Volume = config_volume * game_music_level / 100;
	// Tell it to the act file
	if (PlayMusicFile)
		PlayMusicFile->SetVolume(Volume);
}

MusicType GetMusicFileTypeByExtension(const char* ext)
{
	if (SEqualNoCase(ext, "mid"))
		return MUSICTYPE_MID;
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
	else if (SEqualNoCase(ext, "xm") || SEqualNoCase(ext, "it") || SEqualNoCase(ext, "s3m") || SEqualNoCase(ext, "mod"))
		return MUSICTYPE_MOD;
#ifdef USE_MP3
	else if (SEqualNoCase(ext, "mp3"))
		return MUSICTYPE_MP3;
#endif
#endif
	else if (SEqualNoCase(ext, "ogg"))
		return MUSICTYPE_OGG;
	return MUSICTYPE_UNKNOWN;
}

bool C4MusicSystem::GrpContainsMusic(C4Group &rGrp)
{
	// search for known file extensions
	return           rGrp.FindEntry("*.mid")
#ifdef USE_MP3
	                 || rGrp.FindEntry("*.mp3")
#endif
	                 || rGrp.FindEntry("*.xm")
	                 || rGrp.FindEntry("*.it")
	                 || rGrp.FindEntry("*.s3m")
	                 || rGrp.FindEntry("*.mod")
	                 || rGrp.FindEntry("*.ogg");
}

int C4MusicSystem::SetPlayList(const char *szPlayList, bool fForceSwitch, int fadetime_ms, double max_resume_time)
{
	// Shortcut if no change
	if (playlist_valid && playlist == szPlayList) return 0;
	// info
	if (::Config.Sound.Verbose)
	{
		LogF(R"(MusicSystem: SetPlayList("%s", %s, %d, %.3lf))", szPlayList ? szPlayList : "(null)", fForceSwitch ? "true" : "false", fadetime_ms, max_resume_time);
	}
	// reset
	C4MusicFile *pFile;
	for (pFile = Songs; pFile; pFile = pFile->pNext)
	{
		pFile->NoPlay = true;
	}
	ASongCount = 0;
	if (szPlayList && *szPlayList)
	{
		// match
		char szFileName[_MAX_FNAME_LEN];
		for (int cnt = 0; SGetModule(szPlayList, cnt, szFileName, _MAX_FNAME); cnt++)
			for (pFile = Songs; pFile; pFile = pFile->pNext) if (pFile->NoPlay)
				if (WildcardMatch(szFileName, GetFilename(pFile->FileName)) || pFile->HasCategory(szFileName))
				{
					ASongCount++;
					pFile->NoPlay = false;
				}
	}
	else
	{
		// default: all files except the ones beginning with an at ('@')
		// Ignore frontend and credits music
		for (pFile = Songs; pFile; pFile = pFile->pNext)
			if (*GetFilename(pFile->FileName) != '@' &&
			    !pFile->HasCategory("frontend") &&
			    !pFile->HasCategory("credits"))
			{
				ASongCount++;
				pFile->NoPlay = false;
			}
	}
	// Force switch of music if currently playing piece is not in list or idle because no music file matched
	if (fForceSwitch)
	{
		if (PlayMusicFile)
		{
			fForceSwitch = PlayMusicFile->NoPlay;
		}
		else
		{
			fForceSwitch = (!is_waiting || C4TimeMilliseconds::Now() >= wait_time_end);
		}
		if (fForceSwitch)
		{
			// Switch music. Switching to a break is also allowed, but won't be done if there is a piece to resume
			// Otherwise breaks would never occur if the playlist changes often.
			Play(nullptr, false, fadetime_ms, max_resume_time, PlayMusicFile != nullptr);
		}
	}
	// Remember setting (e.g. to be saved in savegames)
	playlist.Copy(szPlayList);
	playlist_valid = true; // do not re-calculate available song if playlist is reset to same value in the future
	return ASongCount;
}

bool C4MusicSystem::ToggleOnOff()
{
	// // command key for music toggle pressed
	// use different settings for game/menu (lobby also counts as "menu", so go by Game.IsRunning-flag rather than startup)
	if (Game.IsRunning)
	{
		// game music
		Config.Sound.RXMusic = !Config.Sound.RXMusic;
		if (!Config.Sound.RXMusic) Stop(); else Play();
		::GraphicsSystem.FlashMessageOnOff(LoadResStr("IDS_CTL_MUSIC"), !!Config.Sound.RXMusic);
	}
	else
	{
		// game menu
		Config.Sound.FEMusic = !Config.Sound.FEMusic;
		if (!Config.Sound.FEMusic) Stop(); else Play();
	}
	// key processed
	return true;
}

void C4MusicSystem::CompileFunc(StdCompiler *comp)
{
	comp->Value(mkNamingAdapt(playlist, "PlayList", StdCopyStrBuf()));
	comp->Value(mkNamingAdapt(game_music_level, "Volume", 100));
	comp->Value(mkNamingAdapt(music_break_min, "MusicBreakMin", DefaultMusicBreak));
	comp->Value(mkNamingAdapt(music_break_max, "MusicBreakMax", DefaultMusicBreak));
	comp->Value(mkNamingAdapt(music_break_chance, "MusicBreakChance", DefaultMusicBreakChance));
	comp->Value(mkNamingAdapt(music_max_position_memory, "MusicMaxPositionMemory", DefaultMusicMaxPositionMemory));
	// Wait time is not saved - begin savegame resume with a fresh song!
	// Reflect loaded values immediately
	if (comp->isDeserializer())
	{
		SetGameMusicLevel(game_music_level);
		SetPlayList(playlist.getData());
	}
}

int32_t C4MusicSystem::SetGameMusicLevel(int32_t volume_percent)
{
	game_music_level = Clamp<int32_t>(volume_percent, 0, 500); // allow max 5x the user setting
	UpdateVolume();
	return game_music_level;
}

const int32_t C4MusicSystem::DefaultMusicBreak = 120000; // two minutes default music break time
const int32_t C4MusicSystem::DefaultMusicBreakChance = 50; // ...with a 50% chance
const int32_t C4MusicSystem::DefaultMusicMaxPositionMemory = 420; // after this time (in seconds) a piece is no longer continued at the position where it was interrupted
