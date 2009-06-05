/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006  Sven Eberhardt
 * Copyright (c) 2007  Günther Brammer
 * Copyright (c) 2006-2009, RedWolf Design GmbH, http://www.clonk.de
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
// InGame sequence video playback support

#include <C4Include.h>
#include <C4VideoPlayback.h>

#ifndef BIG_C4INCLUDE
#include <C4FullScreen.h>
#include <C4Game.h>
#include <C4Log.h>
#endif

#ifdef HAVE_LIBSMPEG
#include <smpeg/smpeg.h>
#include <SDL.h>
#include <SDL_mixer.h>
#endif // HAVE_LIBSMPEG

void C4VideoFile::Clear()
	{
	if (sFilename.getLength())
		{
		if (fIsTemp) if (!EraseFile(sFilename.getData()))
			{
			LogF("C4VideoFile::Clear: Error deleting temporary video file %s", (const char *) sFilename.getData());
			}
		sFilename.Clear();
		}
	fIsTemp = false;
	}

bool C4VideoFile::Load(const char *szFilename, bool fTemp)
	{
	// clear previous
	Clear();
	// some simple sanity check
	if (!FileExists(szFilename)) return false;
	// simply link to file
	sFilename.Copy(szFilename);
	fIsTemp = fTemp;
	return true;
	}

bool C4VideoFile::Load(class C4Group &hGrp, const char *szFilename)
	{
	// unpacked group?
	if (!hGrp.IsPacked())
		{
		// then direct file registration is possible
		StdStrBuf sPath; sPath.Take(hGrp.GetFullName());
		sFilename.AppendChar(DirectorySeparator);
		sFilename.Append(szFilename);
		return Load(sPath.getData());
		}
	// packed group: Extract to temp space
	char szTempFn[_MAX_PATH+1];
	SCopy(Config.AtTempPath(::GetFilename(szFilename)), szTempFn, _MAX_PATH);
	MakeTempFilename(szTempFn);
	if (!hGrp.ExtractEntry(szFilename, szTempFn))
		return false;
	return Load(szTempFn, true);
	}



// C4VideoShowDialog

bool C4VideoShowDialog::LoadVideo(C4VideoFile *pVideoFile)
	{
#ifdef _WIN32
	// load video file
	if (!AVIFile.OpenFile(pVideoFile->GetFilename(), FullScreen.hWindow, lpDDraw->GetByteCnt()*8)) return false;
	// prepare surface for display
	if (!fctBuffer.Create(AVIFile.GetWdt(), AVIFile.GetHgt(), C4FCT_Full, C4FCT_Full)) return false;
	iStartFrameTime = 0; // no frame shown yet
	// play audio
	if (Config.Sound.RXSound && AVIFile.OpenAudioStream())
		{
		size_t iAudioDataSize=0;
		BYTE *pAudioData = AVIFile.GetAudioStreamData(&iAudioDataSize);
		if (pAudioData)
			{
			if (pAudioTrack) delete pAudioTrack;
			pAudioTrack = new C4SoundEffect();
			if (pAudioTrack->Load(pAudioData, iAudioDataSize, FALSE, false))
				{
				C4SoundInstance *pSoundInst = pAudioTrack->New();
				if (pSoundInst) pSoundInst->Start();
				}
			}
		AVIFile.CloseAudioStream();
		}
	return true;
#else
#ifdef HAVE_LIBSMPEG
	// FIXME
	return false;
	mpeg_info = new SMPEG_Info;
	mpeg = SMPEG_new(pVideoFile->GetFilename(), mpeg_info, 0);
	if (SMPEG_error(mpeg)) {
		LogF("smpeg: %s", SMPEG_error(mpeg));
		return false;
	}
	if (!fctBuffer.Create(mpeg_info->width, mpeg_info->height, C4FCT_Full, C4FCT_Full)) return false;
	surface = SDL_AllocSurface(SDL_SWSURFACE,
		mpeg_info->width,
		mpeg_info->height,
		32,
		0x000000FF,
		0x0000FF00,
		0x00FF0000,
		0xFF000000);
	if (!surface) {
		LogF("smpeg: %s", SDL_GetError());
		return false;
	}
	SMPEG_setdisplay(mpeg, surface, NULL, /*glmpeg_update*/NULL);

        /* Play the movie, using SDL_mixer for audio */
        //SMPEG_enableaudio(mpeg, 0);
	SDL_AudioSpec audiofmt;
	Uint16 format;
	int freq, channels;

	/* Tell SMPEG what the audio format is */
	Mix_QuerySpec(&freq, &format, &channels);
	audiofmt.format = format;
	audiofmt.freq = freq;
	audiofmt.channels = channels;
	SMPEG_actualSpec(mpeg, &audiofmt);

        SMPEG_play(mpeg);
	/* Hook in the MPEG music mixer */
	Mix_HookMusic(SMPEG_playAudioSDL, mpeg);
	//SMPEG_enableaudio(mpeg, 1);

	return true;
#endif // HAVE_LIBSMPEG
#endif // _WIN32
	return false;
	}

C4VideoShowDialog::~C4VideoShowDialog()
	{
#ifdef _WIN32
	if (pAudioTrack) delete pAudioTrack;
#else
#ifdef HAVE_LIBSMPEG
	// FIXME
	return;
        /* Stop the movie and unhook SMPEG from the mixer */
        SMPEG_stop(mpeg);
	SMPEG_delete(mpeg);
        Mix_HookMusic(NULL, NULL);
	SDL_FreeSurface(surface);
	delete mpeg_info;
#endif // HAVE_LIBSMPEG
#endif // _WIN32
	}

void C4VideoShowDialog::VideoDone()
	{
	// finished playback
	if (IsShown()) Close(true);
	}

void C4VideoShowDialog::DrawElement(C4TargetFacet &cgo)
	{
	// draw current video frame
#ifdef _WIN32
	// get frame to be drawn
	time_t iCurrFrameTime = timeGetTime();
	int32_t iGetFrame;
	if (!iStartFrameTime) iStartFrameTime = iCurrFrameTime;
	if (!AVIFile.GetFrameByTime(iCurrFrameTime - iStartFrameTime, &iGetFrame))
		{
		// no frame available: Video playback done!
		// 2do: This will always show the last frame two gfx frames?
		VideoDone();
		}
	else
		{
		// get available frame
		AVIFile.GrabFrame(iGetFrame, &(fctBuffer.GetFace()));
		}
	// draw the found frame
	fctBuffer.Draw(cgo, FALSE);
#else
#ifdef HAVE_LIBSMPEG
	// FIXME
	return;
	CSurface * sfc = &fctBuffer.GetFace();
	sfc->Lock();
	sfc->CopyBytes((BYTE*)surface->pixels);
	sfc->Unlock();
	fctBuffer.Draw(cgo, FALSE);
	if (SMPEG_status(mpeg) != SMPEG_PLAYING)
		VideoDone();
#endif // HAVE_LIBSMPEG
#endif // _WIN32
	}


// C4VideoPlayer

void C4VideoPlayer::Clear()
	{
	// delete all loaded videos
	C4VideoFile *pDelFile;
	while (pDelFile = pFirstVideo)
		{
		pFirstVideo = pDelFile->GetNext();
		delete pDelFile;
		}
	}

bool C4VideoPlayer::PreloadVideos(class C4Group &rFromGroup)
	{
	// preload all videos from group file
	rFromGroup.ResetSearch();
	char szFilename[_MAX_PATH+1];
	int32_t iNumLoaded = 0; bool fAnyLoaded=false;
	while (rFromGroup.FindNextEntry(C4CFN_AnimationFiles, szFilename))
		{
		if (!fAnyLoaded)
			{
			// log message: Only if there are videos, because in 99.99% of the cases, there are none
			LogF(LoadResStr("IDS_PRC_PRELOADVIDEOS"), rFromGroup.GetName());
			fAnyLoaded = true;
			}
		C4VideoFile *pVideoFile = new C4VideoFile();
		if (!pVideoFile->Load(rFromGroup, szFilename))
			{
			LogF("C4VideoPlayer::PreloadVideos: Error preloading video %s from group %s.", szFilename, rFromGroup.GetFullName().getData());
			delete pVideoFile;
			continue;
			}
		if (pFirstVideo) pVideoFile->SetNext(pFirstVideo);
		pFirstVideo = pVideoFile;
		++iNumLoaded;
		}
	// done loading
	return true;
	}

bool C4VideoPlayer::PlayVideo(const char *szVideoFilename)
	{
	// video speicifed by filename - uses preload database if possible; otherwise gets video from root video collection
	// search video file in loaded list
	C4VideoFile *pVidFile;
	for (pVidFile = pFirstVideo; pVidFile; pVidFile = pVidFile->GetNext())
		if (ItemIdentical(pVidFile->GetFilename(), szVideoFilename))
			break;
	bool fTempLoaded = false;
	if (!pVidFile)
		{
		// video not found: Try loading it from game root
		pVidFile = new C4VideoFile();
		if (SCharPos('/', szVideoFilename)>=0 || SCharPos('\\', szVideoFilename)>=0)
			{
			// subfolders involved: UseC4Group to load the video
			char szParentPath[_MAX_PATH+1];
			if (!GetParentPath(szVideoFilename, szParentPath)) { delete pVidFile; return false; }
			C4Group Grp;
			if (!Grp.Open(szParentPath)) { delete pVidFile; return false; }
			if (!pVidFile->Load(Grp, GetFilename(szVideoFilename))) { delete pVidFile; return false; }
			}
		else
			{
			// Direct filename in Clonk directory
			if (!pVidFile->Load(szVideoFilename)) { delete pVidFile; return false; }
			}
		fTempLoaded = true;
		}
	// OK, play that video
	bool fSuccess = PlayVideo(pVidFile);
	// cleanup
	if (fTempLoaded) delete pVidFile;
	return fSuccess;
	}

bool C4VideoPlayer::PlayVideo(C4VideoFile *pVideoFile)
	{
	// safety
	if (!pVideoFile) return false;
	// plays the specified video and returns when finished or skipped by user (blocking call)
	// cannot play in console mode
	if (!FullScreen.Active) return false;
	// videos are played in a fullscreen GUI dialog
	if (!::pGUI) return false;
	C4VideoShowDialog *pVideoDlg = new C4VideoShowDialog();
	if (!pVideoDlg->LoadVideo(pVideoFile))
		{
		DebugLogF("Error playing video file: %s", pVideoFile->GetFilename());
		delete pVideoDlg;
		return false;
		}
	++Game.HaltCount;
	::pGUI->ShowModalDlg(pVideoDlg, true); // ignore result; even an aborted video was shown successfully
	--Game.HaltCount;
	return true;
	}

