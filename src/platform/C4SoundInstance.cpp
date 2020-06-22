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

/* Handles the sound bank and plays effects using DSoundX */

#include "C4Include.h"
#include "platform/C4SoundInstance.h"

#include "game/C4Application.h"
#include "game/C4Viewport.h"
#include "lib/C4Random.h"
#include "object/C4Object.h"
#include "platform/C4SoundIncludes.h"
#include "platform/C4SoundLoaders.h"

using namespace C4SoundLoaders;

C4SoundEffect::C4SoundEffect()
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
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
	if (pSample) Mix_FreeChunk(pSample);
#elif AUDIO_TK == AUDIO_TK_OPENAL
	if (pSample) alDeleteBuffers(1, &pSample);
#endif
	pSample = 0;
}

bool C4SoundEffect::Load(const char *szFileName, C4Group &hGroup, const char *namespace_prefix)
{
	// Sound check
	if (!Config.Sound.RXSound) return false;
	// Locate sound in file
	StdBuf WaveBuffer;
	if (!hGroup.LoadEntry(szFileName, &WaveBuffer)) return false;
	// load it from mem
	if (!Load((BYTE*)WaveBuffer.getMData(), WaveBuffer.getSize())) return false;
	// Set name
	if (namespace_prefix)
	{
		// Local sound name
		SAppend(namespace_prefix, Name, C4MaxSoundName);
		SAppend("::", Name, C4MaxSoundName);
		SAppend(szFileName, Name, C4MaxSoundName);
	}
	else
	{
		// Global sound name
		SCopy(szFileName, Name, C4MaxSoundName);
	}
	return true;
}

bool C4SoundEffect::Load(BYTE *pData, size_t iDataLen, bool fRaw)
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
#if AUDIO_TK == AUDIO_TK_OPENAL
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

C4SoundInstance *C4SoundEffect::New(bool fLoop, int32_t iVolume, C4Object *pObj, int32_t iCustomFalloffDistance, int32_t iPitch, C4SoundModifier *modifier)
{
	// check: too many instances?
	if (!fLoop && Instances >= C4MaxSoundInstances) return nullptr;
	// create & init sound instance
	C4SoundInstance *pInst = new C4SoundInstance();
	if (!pInst->Create(this, fLoop, iVolume, pObj, 0, iCustomFalloffDistance, iPitch, modifier)) { delete pInst; return nullptr; }
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
	return nullptr;
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
		player(NO_OWNER)
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
	if (modifier)
	{
		modifier->DelRef();
		modifier = nullptr;
		has_local_modifier = false;
	}
}

bool C4SoundInstance::Create(C4SoundEffect *pnEffect, bool fLoop, int32_t inVolume, C4Object *pnObj, int32_t inNearInstanceMax, int32_t iFalloffDistance, int32_t inPitch, C4SoundModifier *modifier)
{
	// Sound check
	if (!Config.Sound.RXSound || !pnEffect) return false;
	// Already playing? Stop
	if (Playing()) { Stop(); return false; }
	// Set effect
	pEffect = pnEffect;
	// Set
	tStarted = C4TimeMilliseconds::Now();
	iVolume = inVolume; iPan = 0; iChannel = -1;
	iPitch = inPitch; pitch_dirty = (iPitch != 0);
	iNearInstanceMax = inNearInstanceMax;
	this->iFalloffDistance = iFalloffDistance;
	pObj = pnObj;
	fLooping = fLoop;
	if ((this->modifier = modifier))
	{
		modifier->AddRef();
		has_local_modifier = true;
	}
	SetPlayer(NO_OWNER); // may be updated on first execution
	// Start
	Execute();
	return true;
}

void C4SoundInstance::SetPitch(int32_t inPitch)
{
	// set pitch and update on next call to Execute
	iPitch = inPitch;
	pitch_dirty = true;
}

bool C4SoundInstance::CheckStart()
{
	// already started?
	if (isStarted()) return true;
	// don't bother if half the time is up and the sound is not looping
	if (C4TimeMilliseconds::Now() > tStarted + pEffect->Length / 2 && !fLooping)
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
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
	// Be paranoid about SDL_Mixer initialisation
	if (!Application.MusicSystem.MODInitialized) return false;
	if ((iChannel = Mix_PlayChannel(-1, pEffect->pSample, fLooping? -1 : 0)) == -1)
		return false;
#elif AUDIO_TK == AUDIO_TK_OPENAL
	Application.MusicSystem.SelectContext();
	alGenSources(1, (ALuint*)&iChannel);
	alSourcei(iChannel, AL_BUFFER, pEffect->pSample);
	alSourcei(iChannel, AL_LOOPING,  fLooping ? AL_TRUE : AL_FALSE);
	if (modifier) modifier->ApplyTo(iChannel);
	alSourcePlay(iChannel);
#else
	return false;
#endif
	// Safety: Don't execute if start failed, or Execute() would try to start again
	if (!isStarted()) return false;
	// Update volume
	Execute();
	return true;
}

bool C4SoundInstance::Stop()
{
	if (!pEffect) return false;
	// Stop sound
	bool fRet = true;
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
	// iChannel == -1 will halt all channels. Is that right?
	if (Playing())
		Mix_HaltChannel(iChannel);
#elif AUDIO_TK == AUDIO_TK_OPENAL
	if (iChannel != -1)
	{
		if (Playing()) alSourceStop(iChannel);
		ALuint c = iChannel;
		alDeleteSources(1, &c);
	}
#endif
	iChannel = -1;
	tStarted = 0;
	fLooping = false;
	return fRet;
}

bool C4SoundInstance::Playing()
{
	if (!pEffect) return false;
	if (fLooping) return true;
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
	return Application.MusicSystem.MODInitialized && (iChannel != -1) && Mix_Playing(iChannel);
#elif AUDIO_TK == AUDIO_TK_OPENAL
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
		if (pObj->AudiblePlayer != player) SetPlayer(pObj->AudiblePlayer);
		// apply custom falloff distance
		if (iFalloffDistance)
		{
			iAudibility = Clamp<int32_t>(100 + (iAudibility - 100) * C4AudibilityRadius / iFalloffDistance, 0,100);
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
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
			Mix_HaltChannel(iChannel);
#elif AUDIO_TK == AUDIO_TK_OPENAL
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
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
		Mix_Volume(iChannel, (iVol * MIX_MAX_VOLUME) / (100 * 256));
		Mix_SetPanning(iChannel, Clamp((100 - iPan) * 256 / 100, 0, 255), Clamp((100 + iPan) * 256 / 100, 0, 255));
#elif AUDIO_TK == AUDIO_TK_OPENAL
		alSource3f(iChannel, AL_POSITION, Clamp<float>(float(iPan)/100.0f, -1.0f, +1.0f), 0, -0.7f);
		alSourcef(iChannel, AL_GAIN, float(iVol) / (100.0f*256.0f));
		if (pitch_dirty)
		{
			// set pitch; map -90..+100 range to 0.1f..2.0f
			alSourcef(iChannel, AL_PITCH, std::max(float(iPitch + 100) / 100.0f, 0.1f));
			pitch_dirty = false;
		}
#endif
	}
}

void C4SoundInstance::SetVolumeByPos(int32_t x, int32_t y, int32_t relative_vol)
{
	int32_t vol_player = NO_OWNER;
	iVolume = ::Viewports.GetAudibility(x, y, &iPan, 0, &vol_player) * relative_vol / 100.0f;
	if (vol_player != player) SetPlayer(vol_player);
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
		pObj = nullptr;
	}
}

bool C4SoundInstance::Inside(int32_t iX, int32_t iY, int32_t iRad)
{
	return pObj &&
	       (pObj->GetX() - iX) * (pObj->GetX() - iX) + (pObj->GetY() - iY) * (pObj->GetY() - iY) <= iRad * iRad;
}

void C4SoundInstance::SetModifier(C4SoundModifier *new_modifier, bool is_global)
{
	// do not overwrite local modifier with global one
	if (is_global)
	{
		if (has_local_modifier) return;
	}
	else
	{
		// this sound has its own modifier now and doesn't care for global ones
		has_local_modifier = (new_modifier != nullptr);
	}
	if (new_modifier != modifier)
	{
		// update modifier and ref-count
		C4SoundModifier *old_modifier = modifier;
		modifier = new_modifier;
		if (modifier) modifier->AddRef();
		if (old_modifier) old_modifier->DelRef();
		// Apply new modifier
		if (isStarted())
		{
			if (modifier)
			{
#if AUDIO_TK == AUDIO_TK_OPENAL
				modifier->ApplyTo(iChannel);
#endif
			}
			else
			{
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
				alSource3i(iChannel, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
#endif
			}
		}
	}
}

void C4SoundInstance::SetPlayer(int32_t new_player)
{
	// update player and associated sound modifier
	player = new_player;
	SetModifier(::Application.SoundSystem.Modifiers.GetGlobalModifier(player), true);
}
