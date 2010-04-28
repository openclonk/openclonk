/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2001, 2005, 2007  Sven Eberhardt
 * Copyright (c) 2001  Carlo Teubner
 * Copyright (c) 2001  Michael Käser
 * Copyright (c) 2002-2003  Peter Wortmann
 * Copyright (c) 2005-2006, 2008  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
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

/* Handles Music.c4g and randomly plays songs */

#include <C4Include.h>
#include <C4MusicSystem.h>

#include <C4MusicFile.h>
#include <C4Application.h>
#include <C4Random.h>
#include <C4Log.h>
#include <C4Game.h>
#include <C4GraphicsSystem.h>

#if defined HAVE_FMOD
#include <fmod_errors.h>
#elif defined HAVE_LIBSDL_MIXER
#include <SDL.h>
#endif

// helper
const char *SGetRelativePath(const char *strPath)
{
	const char *strEXEPath = Config.AtExePath("");
	while (*strEXEPath == *strPath) { strEXEPath++; strPath++; }
	return strPath;
}


C4MusicSystem::C4MusicSystem():
		Songs(NULL),
		SongCount(0),
		PlayMusicFile(NULL),
		Volume(100)
{
}

C4MusicSystem::~C4MusicSystem()
{
	Clear();
}

bool C4MusicSystem::InitializeMOD()
{
#if defined HAVE_FMOD
#ifdef _WIN32
	// Debug code
	switch (Config.Sound.FMMode)
	{
	case 0:
		FSOUND_SetOutput(FSOUND_OUTPUT_WINMM);
		break;
	case 1:
		FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
		FSOUND_SetHWND(Application.pWindow->hWindow);
		break;
	case 2:
		FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
		FSOUND_SetDriver(0);
		break;
	}
	FSOUND_SetMixer(FSOUND_MIXER_QUALITY_AUTODETECT);
#endif
	if (FSOUND_GetVersion() < FMOD_VERSION)
	{
		LogF("FMod: You are using the wrong DLL version!  You should be using %.02f", FMOD_VERSION);
		return false;
	}
	if (!FSOUND_Init(44100, 32, 0))
	{
		LogF("FMod: %s", FMOD_ErrorString(FSOUND_GetError()));
		return false;
	}
	// ok
	MODInitialized = true;
	return true;
#elif defined HAVE_LIBSDL_MIXER
	SDL_version compile_version;
	const SDL_version * link_version;
	MIX_VERSION(&compile_version);
	link_version=Mix_Linked_Version();
	LogF("SDL_mixer runtime version is %d.%d.%d (compiled with %d.%d.%d)",
	     link_version->major, link_version->minor, link_version->patch,
	     compile_version.major, compile_version.minor, compile_version.patch);
	if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_Init(SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE))
	{
		LogF("SDL: %s", SDL_GetError());
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
#endif
	return false;
}

void C4MusicSystem::DeinitializeMOD()
{
#if defined HAVE_FMOD
	FSOUND_StopSound(FSOUND_ALL); /* to prevent some hangs in FMOD */
#ifdef DEBUG
	Sleep(0);
#endif
	FSOUND_Close();
#elif defined HAVE_LIBSDL_MIXER
	Mix_CloseAudio();
	SDL_Quit();
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
	if (PlayList) SetPlayList(PlayList); else SetPlayList(0);
	// set initial volume
	SetVolume(Config.Sound.MusicVolume);

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
		LogF(LoadResStr("IDS_PRC_LOCALMUSIC"), SGetRelativePath(MusicDir.getData()));
	}
	// check for music folders in group set
	C4Group *pMusicFolder = NULL;
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
		LogF(LoadResStr("IDS_PRC_LOCALMUSIC"), SGetRelativePath(MusicDir.getData()));
	}
	// no music?
	if (!SongCount) return false;
	// set play list
	SetPlayList(0);
	// ok
	return true;
}

void C4MusicSystem::Load(const char *szFile)
{
	// safety
	if (!szFile || !*szFile) return;
	C4MusicFile *NewSong=NULL;
	// get extension
#if defined HAVE_FMOD
	const char *szExt = GetExtension(szFile);
	// get type
	switch (GetMusicFileTypeByExtension(GetExtension(szFile)))
	{
	case MUSICTYPE_MOD:
		if (MODInitialized) NewSong = new C4MusicFileMOD;
		break;
	case MUSICTYPE_MP3:
		if (MODInitialized) NewSong = new C4MusicFileMP3;
		break;
	case MUSICTYPE_OGG:
		if (MODInitialized) NewSong = new C4MusicFileOgg;
		break;

	case MUSICTYPE_MID:
		if (MODInitialized)
			NewSong = new C4MusicFileMID;
		break;
	default: return; // safety
	}
#elif defined HAVE_LIBSDL_MIXER
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
	NewSong->pNext = NULL;
	// count songs
	SongCount++;
}

void C4MusicSystem::LoadDir(const char *szPath)
{
	char Path[_MAX_FNAME + 1], File[_MAX_FNAME + 1];
	C4Group *pDirGroup = NULL;
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
	char szFile[_MAX_FNAME + 1];
	pDirGroup->ResetSearch();
	while (pDirGroup->FindNextEntry(File, szFile))
	{
		char strFullPath[_MAX_FNAME + 1];
		sprintf(strFullPath, "%s%c%s", Path, DirectorySeparator, szFile);
		Load(strFullPath);
	}
	// free it
	delete pDirGroup;
}

void C4MusicSystem::LoadMoreMusic()
{
	BYTE *szMoreMusic;
	CStdFile MoreMusicFile;
	// load MoreMusic.txt
	if (!MoreMusicFile.Load(Config.AtUserDataPath(C4CFN_MoreMusic), &szMoreMusic, NULL, 1)) return;
	// read contents
	char *pPos = reinterpret_cast<char *>(szMoreMusic);
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
	delete [] szMoreMusic;
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
}

void C4MusicSystem::Clear()
{
#ifdef HAVE_LIBSDL_MIXER
	// Stop a fadeout
	Mix_HaltMusic();
#endif
	ClearSongs();
	if (MODInitialized) { DeinitializeMOD(); }
}

void C4MusicSystem::Execute()
{
#ifndef HAVE_LIBSDL_MIXER
	if (!::Game.iTick35)
#endif
	{
		if (!PlayMusicFile)
			Play();
		else
			PlayMusicFile->CheckIfPlaying();
	}
}

bool C4MusicSystem::Play(const char *szSongname, bool fLoop)
{
	if (Game.IsRunning ? !Config.Sound.RXMusic : !Config.Sound.FEMusic)
		return false;

	C4MusicFile* NewFile = NULL;

	// Specified song name
	if (szSongname && szSongname[0])
	{
		// Search in list
		for (NewFile=Songs; NewFile; NewFile = NewFile->pNext)
		{
			char songname[_MAX_FNAME+1];
			SCopy(szSongname, songname); DefaultExtension(songname, "mid");
			if (SEqual(GetFilename(NewFile->FileName), songname))
				break;
			SCopy(szSongname, songname); DefaultExtension(songname, "ogg");
			if (SEqual(GetFilename(NewFile->FileName), songname))
				break;
		}
	}

	// Random song
	else
	{
		// try to find random song
		for (int i = 0; i <= 1000; i++)
		{
			int nmb = SafeRandom(Max(ASongCount / 2 + ASongCount % 2, ASongCount - SCounter));
			int j;
			for (j = 0, NewFile = Songs; NewFile; NewFile = NewFile->pNext)
				if (!NewFile->NoPlay)
					if (NewFile->LastPlayed == -1 || NewFile->LastPlayed < SCounter - ASongCount / 2)
					{
						j++;
						if (j > nmb) break;
					}
			if (NewFile) break;
		}

	}

	// File found?
	if (!NewFile)
		return false;

	// Stop old music
	Stop();

	LogF(LoadResStr("IDS_PRC_PLAYMUSIC"), GetFilename(NewFile->FileName));

	// Play new song
	if (!NewFile->Play(fLoop)) return false;
	PlayMusicFile = NewFile;
	NewFile->LastPlayed = SCounter++;
	Loop = fLoop;

	// Set volume
	PlayMusicFile->SetVolume(Volume);

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
	// stop
	Stop();
}

void C4MusicSystem::FadeOut(int fadeout_ms)
{
	if (PlayMusicFile)
	{
		PlayMusicFile->Stop(fadeout_ms);
	}
}

bool C4MusicSystem::Stop()
{
	if (PlayMusicFile)
	{
		PlayMusicFile->Stop();
		PlayMusicFile=NULL;
	}
	return true;
}

int C4MusicSystem::SetVolume(int iLevel)
{
	if (iLevel > 100) iLevel = 100;
	if (iLevel < 0) iLevel = 0;
	// Save volume for next file
	Volume = iLevel;
	// Tell it to the act file
	if (PlayMusicFile)
		PlayMusicFile->SetVolume(iLevel);
	return iLevel;
}

MusicType GetMusicFileTypeByExtension(const char* ext)
{
	if (SEqualNoCase(ext, "mid"))
		return MUSICTYPE_MID;
#if defined HAVE_FMOD || defined HAVE_LIBSDL_MIXER
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

int C4MusicSystem::SetPlayList(const char *szPlayList)
{
	// reset
	C4MusicFile *pFile;
	for (pFile = Songs; pFile; pFile = pFile->pNext)
	{
		pFile->NoPlay = true;
		pFile->LastPlayed = -1;
	}
	ASongCount = 0;
	SCounter = 0;
	if (szPlayList && *szPlayList)
	{
		// match
		char szFileName[_MAX_FNAME + 1];
		for (int cnt = 0; SGetModule(szPlayList, cnt, szFileName, _MAX_FNAME); cnt++)
			for (pFile = Songs; pFile; pFile = pFile->pNext)
				if (pFile->NoPlay && WildcardMatch(szFileName, GetFilename(pFile->FileName)))
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
			    !SEqual2(GetFilename(pFile->FileName), "Credits.") &&
			    !SEqual2(GetFilename(pFile->FileName), "Frontend."))
			{
				ASongCount++;
				pFile->NoPlay = false;
			}
	}
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
