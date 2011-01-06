/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2004, 2008  Matthes Bender
 * Copyright (c) 2001, 2005-2006, 2008  Sven Eberhardt
 * Copyright (c) 2003-2005  Peter Wortmann
 * Copyright (c) 2005-2006, 2009  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Mortimer
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

/* Handles the sound bank and plays effects using DSoundX */

#include <C4Include.h>
#include <C4SoundSystem.h>

#include <C4Random.h>
#include <C4Object.h>
#include <C4Game.h>
#include <C4Config.h>
#include <C4Application.h>
#include <C4Viewport.h>
#include <C4SoundLoaders.h>

using namespace C4SoundLoaders;

C4SoundEffect::C4SoundEffect():
		UsageTime (0),
		Instances (0),
		Static (false),
		pSample (NULL),
		FirstInst (NULL),
		Next (NULL)
{
	Name[0]=0;
}

C4SoundEffect::~C4SoundEffect()
{
	Clear();
}

void C4SoundEffect::Clear()
{
	while (FirstInst) RemoveInst(FirstInst);
#ifdef HAVE_FMOD
	if (pSample) FSOUND_Sample_Free(pSample);
#endif
#ifdef HAVE_LIBSDL_MIXER
	if (pSample) Mix_FreeChunk(pSample);
#endif
#ifdef USE_OPEN_AL
	if (pSample) alDeleteBuffers(1, &pSample);
#endif
	pSample = NULL;
}

bool C4SoundEffect::Load(const char *szFileName, C4Group &hGroup, bool fStatic)
{
	// Sound check
	if (!Config.Sound.RXSound) return false;
	// Locate sound in file
	StdBuf WaveBuffer;
	if (!hGroup.LoadEntry(szFileName, WaveBuffer)) return false;
	// load it from mem
	if (!Load((BYTE*)WaveBuffer.getData(), WaveBuffer.getSize(), fStatic)) return false;
	// Set name
	SCopy(szFileName,Name,C4MaxSoundName);
	return true;
}

bool C4SoundEffect::Load(BYTE *pData, size_t iDataLen, bool fStatic, bool fRaw)
{
	// Sound check
	if (!Config.Sound.RXSound) return false;

	SoundInfo info;
	int32_t options = 0;
	if (fRaw)
		options |= SoundLoader::OPTION_Raw;
	for (SoundLoader* loader = SoundLoader::first_loader; loader; loader = loader->next)
	{
		if (loader->ReadInfo(&info, pData, iDataLen))
		{
			if (info.final_handle)
			{
				// loader supplied the handle specific to the sound system used; just assign to pSample
				pSample = info.final_handle;
			}
			else
			{
#ifdef USE_OPEN_AL
				Application.MusicSystem.SelectContext();
				alGenBuffers(1, &pSample);
				alBufferData(pSample, info.format, &info.sound_data[0], info.sound_data.size(), info.sample_rate);
#else
				Log("SoundLoader does not provide a ready-made handle");
#endif
			}
			SampleRate = info.sample_rate;
			Length = info.sample_length*1000;
			break;
		}
	}
	*Name = '\0';
	// Set usage time
	UsageTime=Game.Time;
	Static=fStatic;
	return !!pSample;
}

void C4SoundEffect::Execute()
{
	// check for instances that have stopped and volume changes
	for (C4SoundInstance *pInst = FirstInst; pInst; )
	{
		C4SoundInstance *pNext = pInst->pNext;
		if (!pInst->Playing())
			RemoveInst(pInst);
		else
			pInst->Execute();
		pInst = pNext;
	}
}

C4SoundInstance *C4SoundEffect::New(bool fLoop, int32_t iVolume, C4Object *pObj, int32_t iCustomFalloffDistance)
{
	// check: too many instances?
	if (!fLoop && Instances >= C4MaxSoundInstances) return NULL;
	// create & init sound instance
	C4SoundInstance *pInst = new C4SoundInstance();
	if (!pInst->Create(this, fLoop, iVolume, pObj, 0, iCustomFalloffDistance)) { delete pInst; return NULL; }
	// add to list
	AddInst(pInst);
	// return
	return pInst;
}

C4SoundInstance *C4SoundEffect::GetInstance(C4Object *pObj)
{
	for (C4SoundInstance *pInst = FirstInst; pInst; pInst = pInst->pNext)
		if (pInst->getObj() == pObj)
			return pInst;
	return NULL;
}

void C4SoundEffect::ClearPointers(C4Object *pObj)
{
	for (C4SoundInstance *pInst = FirstInst; pInst; pInst = pInst->pNext)
		pInst->ClearPointers(pObj);
}

int32_t C4SoundEffect::GetStartedInstanceCount(int32_t iX, int32_t iY, int32_t iRad)
{
	int32_t cnt = 0;
	for (C4SoundInstance *pInst = FirstInst; pInst; pInst = pInst->pNext)
		if (pInst->isStarted() && pInst->getObj() && pInst->Inside(iX, iY, iRad))
			cnt++;
	return cnt;
}

int32_t C4SoundEffect::GetStartedInstanceCount()
{
	int32_t cnt = 0;
	for (C4SoundInstance *pInst = FirstInst; pInst; pInst = pInst->pNext)
		if (pInst->isStarted() && pInst->Playing() && !pInst->getObj())
			cnt++;
	return cnt;
}

void C4SoundEffect::AddInst(C4SoundInstance *pInst)
{
	pInst->pNext = FirstInst;
	FirstInst = pInst;
	Instances++;
}
void C4SoundEffect::RemoveInst(C4SoundInstance *pInst)
{
	if (pInst == FirstInst)
		FirstInst = pInst->pNext;
	else
	{
		C4SoundInstance *pPos = FirstInst;
		while (pPos && pPos->pNext != pInst) pPos = pPos->pNext;
		if (pPos)
			pPos->pNext = pInst->pNext;
	}
	delete pInst;
	Instances--;
}


C4SoundInstance::C4SoundInstance():
		pEffect (NULL),
		iVolume (0), iPan (0),
		iChannel (-1),
		pNext (NULL)
{
}

C4SoundInstance::~C4SoundInstance()
{
	Clear();
}

void C4SoundInstance::Clear()
{
	Stop();
	iChannel = -1;
}

bool C4SoundInstance::Create(C4SoundEffect *pnEffect, bool fLoop, int32_t inVolume, C4Object *pnObj, int32_t inNearInstanceMax, int32_t iFalloffDistance)
{
	// Sound check
	if (!Config.Sound.RXSound || !pnEffect) return false;
	// Already playing? Stop
	if (Playing()) { Stop(); return false; }
	// Set effect
	pEffect = pnEffect;
	// Set
	iStarted = timeGetTime();
	iVolume = inVolume; iPan = 0; iChannel = -1;
	iNearInstanceMax = inNearInstanceMax;
	this->iFalloffDistance = iFalloffDistance;
	pObj = pnObj;
	fLooping = fLoop;
	// Start
	Execute();
	// Safe usage
	pEffect->UsageTime = Game.Time;
	return true;
}

bool C4SoundInstance::CheckStart()
{
	// already started?
	if (isStarted()) return true;
	// don't bother if half the time is up and the sound is not looping
	if (timeGetTime() > iStarted + pEffect->Length / 2 && !fLooping)
		return false;
	// do near-instances check
	int32_t iNearInstances = pObj ? pEffect->GetStartedInstanceCount(pObj->GetX(), pObj->GetY(), C4NearSoundRadius)
	                         : pEffect->GetStartedInstanceCount();
	// over maximum?
	if (iNearInstances > iNearInstanceMax) return false;
	// Start
	return Start();
}

bool C4SoundInstance::Start()
{
#ifdef HAVE_FMOD
	// Start
	if ((iChannel = FSOUND_PlaySound(FSOUND_FREE, pEffect->pSample)) == -1)
		return false;
	if (!FSOUND_SetLoopMode(iChannel, fLooping ? FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF))
		{ Stop(); return false; }
	// set position
	if (timeGetTime() > iStarted + 20)
	{
		assert(pEffect->Length > 0);
		int32_t iTime = (timeGetTime() - iStarted) % pEffect->Length;
		FSOUND_SetCurrentPosition(iChannel, iTime / 10 * pEffect->SampleRate / 100);
	}
#elif defined HAVE_LIBSDL_MIXER
	// Be paranoid about SDL_Mixer initialisation
	if (!Application.MusicSystem.MODInitialized) return false;
	if ((iChannel = Mix_PlayChannel(-1, pEffect->pSample, fLooping? -1 : 0)) == -1)
		return false;
#elif defined(USE_OPEN_AL)
	Application.MusicSystem.SelectContext();
	alGenSources(1, (ALuint*)&iChannel);
	alSourcei(iChannel, AL_BUFFER, pEffect->pSample);
	alSourcePlay(iChannel);
#else
	return false;
#endif
	// Update volume
	Execute();
	return true;
}

bool C4SoundInstance::Stop()
{
	if (!pEffect) return false;
	// Stop sound
	bool fRet = true;
#ifdef HAVE_FMOD
	if (Playing())
		fRet = !! FSOUND_StopSound(iChannel);
#endif
#ifdef HAVE_LIBSDL_MIXER
	// iChannel == -1 will halt all channels. Is that right?
	if (Playing())
		Mix_HaltChannel(iChannel);
#endif
#ifdef USE_OPEN_AL
	if (Playing())
		alSourceStop(iChannel);
#endif
	iChannel = -1;
	iStarted = 0;
	fLooping = false;
	return fRet;
}

bool C4SoundInstance::Playing()
{
	if (!pEffect) return false;
#ifdef HAVE_FMOD
	if (fLooping) return true;
	return isStarted() ? FSOUND_GetCurrentSample(iChannel) == pEffect->pSample
	       : timeGetTime() < iStarted + pEffect->Length;
#endif
#ifdef HAVE_LIBSDL_MIXER
	return Application.MusicSystem.MODInitialized && (iChannel != -1) && Mix_Playing(iChannel);
#endif
#ifdef USE_OPEN_AL
	if (iChannel == -1)
		return false;
	else
	{
		ALint state;
		alGetSourcei(iChannel, AL_SOURCE_STATE, &state);
		return state == AL_PLAYING;
	}
#endif
	return false;
}

void C4SoundInstance::Execute()
{
	// Object deleted?
	if (pObj && !pObj->Status) ClearPointers(pObj);
	// initial values
	int32_t iVol = iVolume * 256 * Config.Sound.SoundVolume / 100, iPan = C4SoundInstance::iPan;
	// bound to an object?
	if (pObj)
	{
		int iAudibility = pObj->Audible;
		// apply custom falloff distance
		if (iFalloffDistance)
		{
			iAudibility = BoundBy<int32_t>(100 + (iAudibility - 100) * C4AudibilityRadius / iFalloffDistance, 0,100);
		}
		iVol = iVol * iAudibility / 100;
		iPan += pObj->AudiblePan;
	}
	// sound off?
	if (!iVol)
	{
		// stop, if started
		if (isStarted())
		{
#ifdef HAVE_FMOD
			FSOUND_StopSound(iChannel);
#endif
#ifdef HAVE_LIBSDL_MIXER
			Mix_HaltChannel(iChannel);
#endif
#ifdef USE_OPEN_AL
			alDeleteSources(1, (ALuint*)&iChannel);
#endif
			iChannel = -1;
		}
	}
	else
	{
		// start
		if (!isStarted())
			if (!CheckStart())
				return;
		// set volume & panning
#ifdef HAVE_FMOD
		FSOUND_SetVolume(iChannel, BoundBy(iVol / 100, 0, 255));
		FSOUND_SetPan(iChannel, BoundBy(256*(iPan+100)/200,0,255));
#endif
#ifdef HAVE_LIBSDL_MIXER
		Mix_Volume(iChannel, (iVol * MIX_MAX_VOLUME) / (100 * 256));
		//Mix_SetPanning(iChannel, ((100 + iPan) * 256) / 200, ((100 - iPan) * 256) / 200);
		Mix_SetPanning(iChannel, BoundBy((100 - iPan) * 256 / 100, 0, 255), BoundBy((100 + iPan) * 256 / 100, 0, 255));
#endif
#ifdef USE_OPEN_AL
		alSource3f(iChannel, AL_POSITION, 0, 0, 0); // FIXME
		alSourcef(iChannel, AL_GAIN, iVol / 100);
#endif
	}
}

void C4SoundInstance::SetVolumeByPos(int32_t x, int32_t y)
{
	iVolume = ::Viewports.GetAudibility(x, y, &iPan);
}

void C4SoundInstance::ClearPointers(C4Object *pDelete)
{
	if (!Playing()) { Stop(); return; }
	if (pObj == pDelete)
	{
		// stop if looping (would most likely loop forever)
		if (fLooping)
			Stop();
		// otherwise: set volume by last position
		else
			SetVolumeByPos(pObj->GetX(), pObj->GetY());
		pObj = NULL;
	}
}

bool C4SoundInstance::Inside(int32_t iX, int32_t iY, int32_t iRad)
{
	return pObj &&
	       (pObj->GetX() - iX) * (pObj->GetX() - iX) + (pObj->GetY() - iY) * (pObj->GetY() - iY) <= iRad * iRad;
}


C4SoundSystem::C4SoundSystem():
		FirstSound (NULL)
{
}

C4SoundSystem::~C4SoundSystem()
{

}

bool C4SoundSystem::Init()
{
	if (!Application.MusicSystem.MODInitialized &&
	    !Application.MusicSystem.InitializeMOD())
		return false;

	// Might be reinitialisation
	ClearEffects();
	// Open sound file
	if (!SoundFile.IsOpen())
		if (!SoundFile.Open(Config.AtSystemDataPath(C4CFN_Sound))) return false;
#ifdef HAVE_LIBSDL_MIXER
	Mix_AllocateChannels(C4MaxSoundInstances);
#endif
	return true;
}

void C4SoundSystem::Clear()
{
	ClearEffects();
	// Close sound file
	SoundFile.Close();
}

void C4SoundSystem::ClearEffects()
{
	// Clear sound bank
	C4SoundEffect *csfx,*next;
	for (csfx=FirstSound; csfx; csfx=next)
	{
		next=csfx->Next;
		delete csfx;
	}
	FirstSound=NULL;
}

void C4SoundSystem::Execute()
{
#ifdef USE_OPEN_AL
	Application.MusicSystem.SelectContext();
#endif
	// Sound effect statistics & unload check
	C4SoundEffect *csfx,*next=NULL,*prev=NULL;
	for (csfx=FirstSound; csfx; csfx=next)
	{
		next=csfx->Next;
		// Instance removal check
		csfx->Execute();
		// Unload check
		if (!csfx->Static && Game.Time-csfx->UsageTime > SoundUnloadTime && !csfx->FirstInst)
		{
			if (prev) prev->Next=next;
			else FirstSound=next;
			delete csfx;
		}
		else
			prev=csfx;
	}
}

int32_t C4SoundSystem::EffectInBank(const char *szSound)
{
	int32_t iResult = 0;
	C4SoundEffect *pSfx;
	char szName[C4MaxSoundName+4+1];
	// Compose name (with extension)
	SCopy(szSound,szName,C4MaxSoundName);
	DefaultExtension(szName,"wav");
	// Count all matching sounds in bank
	for (pSfx=FirstSound; pSfx; pSfx=pSfx->Next)
		if (WildcardMatch(szName,pSfx->Name))
			iResult++;
	return iResult;
}

C4SoundEffect* C4SoundSystem::AddEffect(const char *szSoundName)
{
	C4SoundEffect *nsfx;
	// Allocate new bank entry
	if (!( nsfx=new C4SoundEffect )) return NULL;
	// Load sound to entry
	C4GRP_DISABLE_REWINDWARN // dynamic load; must rewind here :(
	if (!nsfx->Load(szSoundName,SoundFile,false))
		if (!nsfx->Load(szSoundName,Game.ScenarioFile,false))
			{ C4GRP_ENABLE_REWINDWARN delete nsfx; return NULL; }
	C4GRP_ENABLE_REWINDWARN
	// Add sound to bank
	nsfx->Next=FirstSound;
	FirstSound=nsfx;
	return nsfx;
}

C4SoundEffect* C4SoundSystem::GetEffect(const char *szSndName)
{
	C4SoundEffect *pSfx;
	char szName[C4MaxSoundName+4+1];
	int32_t iNumber;
	// Evaluate sound name
	SCopy(szSndName,szName,C4MaxSoundName);
	// Default extension
	DefaultExtension(szName,"wav");
	// Convert old style '*' wildcard to correct '?' wildcard
	// For sound effects, '*' is supposed to match single digits only
	SReplaceChar(szName, '*', '?');
	// Sound with a wildcard: determine number of available matches
	if (SCharCount('?',szName))
	{
		// Search global sound file
		if (!(iNumber=SoundFile.EntryCount(szName)))
			// Search scenario local files
			if (!(iNumber=Game.ScenarioFile.EntryCount(szName)))
				// Search bank loaded sounds
				if (!(iNumber=EffectInBank(szName)))
					// None found: failure
					return NULL;
		// Insert index to name
		iNumber=BoundBy(1+SafeRandom(iNumber),1,9);
		SReplaceChar(szName,'?','0'+iNumber);
	}
	// Find requested sound effect in bank
	for (pSfx=FirstSound; pSfx; pSfx=pSfx->Next)
		if (SEqualNoCase(szName,pSfx->Name))
			break;
	// Sound not in bank, try add
	if (!pSfx)
		if (!(pSfx = AddEffect(szName)))
			return NULL;
	return pSfx;
}

C4SoundInstance *C4SoundSystem::NewEffect(const char *szSndName, bool fLoop, int32_t iVolume, C4Object *pObj, int32_t iCustomFalloffDistance)
{
	// Sound not active
	if (!Config.Sound.RXSound) return false;
	// Get sound
	C4SoundEffect *csfx;
	if (!(csfx=GetEffect(szSndName))) return false;
	// Play
	return csfx->New(fLoop, iVolume, pObj, iCustomFalloffDistance);
}

C4SoundInstance *C4SoundSystem::FindInstance(const char *szSndName, C4Object *pObj)
{
	char szName[C4MaxSoundName+4+1];
	// Evaluate sound name (see GetEffect)
	SCopy(szSndName,szName,C4MaxSoundName);
	DefaultExtension(szName,"wav");
	SReplaceChar(szName, '*', '?');
	// Find an effect with a matching instance
	for (C4SoundEffect *csfx = FirstSound; csfx; csfx = csfx->Next)
		if (WildcardMatch(szName, csfx->Name))
		{
			C4SoundInstance *pInst = csfx->GetInstance(pObj);
			if (pInst) return pInst;
		}
	return NULL;
}

// LoadEffects will load all sound effects of all known sound types (i.e. *.wav and *.ogg)
// To play an ogg effect, however, you will have to specify the extension in the sound
// command (e.g. Sound("Hello.ogg")), because all playback functions will default to wav only.
// LoadEffects is currently used for static loading from object definitions only.

int32_t C4SoundSystem::LoadEffects(C4Group &hGroup, bool fStatic)
{
	int32_t iNum=0;
	char szFilename[_MAX_FNAME+1];
	char szFileType[_MAX_FNAME+1];
	C4SoundEffect *nsfx;
	// Process segmented list of file types
	for (int32_t i = 0; SCopySegment(C4CFN_SoundFiles, i, szFileType, '|', _MAX_FNAME); i++)
	{
		// Search all sound files in group
		hGroup.ResetSearch();
		while (hGroup.FindNextEntry(szFileType, szFilename))
			// Create and load effect
			if ((nsfx = new C4SoundEffect))
			{
				if (nsfx->Load(szFilename,hGroup,fStatic))
				{
					// Overload same name effects
					RemoveEffect(szFilename);
					// Add effect
					nsfx->Next=FirstSound;
					FirstSound=nsfx;
					iNum++;
				}
				else
					delete nsfx;
			}
	}
	return iNum;
}

int32_t C4SoundSystem::RemoveEffect(const char *szFilename)
{
	int32_t iResult=0;
	C4SoundEffect *pNext,*pPrev=NULL;
	for (C4SoundEffect *pSfx=FirstSound; pSfx; pSfx=pNext)
	{
		pNext=pSfx->Next;
		if (WildcardMatch(szFilename,pSfx->Name))
		{
			delete pSfx;
			if (pPrev) pPrev->Next=pNext;
			else FirstSound=pNext;
			iResult++;
		}
		else
			pPrev=pSfx;
	}
	return iResult;
}

void C4SoundSystem::ClearPointers(C4Object *pObj)
{
	for (C4SoundEffect *pEff=FirstSound; pEff; pEff=pEff->Next)
		pEff->ClearPointers(pObj);
}

C4SoundInstance *StartSoundEffect(const char *szSndName, bool fLoop, int32_t iVolume, C4Object *pObj, int32_t iCustomFalloffDistance)
{
	// Sound check
	if (!Config.Sound.RXSound) return false;
	// Start new
	return Application.SoundSystem.NewEffect(szSndName, fLoop, iVolume, pObj, iCustomFalloffDistance);
}

C4SoundInstance *StartSoundEffectAt(const char *szSndName, int32_t iX, int32_t iY, bool fLoop, int32_t iVolume)
{
	// Sound check
	if (!Config.Sound.RXSound) return false;
	// Create
	C4SoundInstance *pInst = StartSoundEffect(szSndName, fLoop, iVolume);
	// Set volume by position
	if (pInst) pInst->SetVolumeByPos(iX, iY);
	// Return
	return pInst;
}

C4SoundInstance *GetSoundInstance(const char *szSndName, C4Object *pObj)
{
	return Application.SoundSystem.FindInstance(szSndName, pObj);
}

void StopSoundEffect(const char *szSndName, C4Object *pObj)
{
	// Find instance
	C4SoundInstance *pInst = Application.SoundSystem.FindInstance(szSndName, pObj);
	if (!pInst) return;
	// Stop
	pInst->Stop();
}
void SoundLevel(const char *szSndName, C4Object *pObj, int32_t iLevel)
{
	// Sound level zero? Stop
	if (!iLevel) { StopSoundEffect(szSndName, pObj); return; }
	// Find or create instance
	C4SoundInstance *pInst = Application.SoundSystem.FindInstance(szSndName, pObj);
	if (!pInst) pInst = StartSoundEffect(szSndName, true, iLevel, pObj);
	if (!pInst) return;
	// Set volume
	pInst->SetVolume(iLevel);
	pInst->Execute();
}
void SoundPan(const char *szSndName, C4Object *pObj, int32_t iPan)
{
	// Find instance
	C4SoundInstance *pInst = Application.SoundSystem.FindInstance(szSndName, pObj);
	if (!pInst) return;
	// Set pan
	pInst->SetPan(iPan);
	pInst->Execute();
}
