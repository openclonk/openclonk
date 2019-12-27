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
#include "platform/C4SoundSystem.h"

#include "lib/C4Random.h"
#include "game/C4Application.h"
#include "game/C4Viewport.h"
#include "object/C4Object.h"
#include "platform/C4SoundIncludes.h"
#include "platform/C4SoundInstance.h"
#include "platform/C4SoundLoaders.h"

C4SoundSystem::C4SoundSystem() = default;

C4SoundSystem::~C4SoundSystem() = default;

bool C4SoundSystem::Init()
{
	if (!Application.MusicSystem.MODInitialized &&
	    !Application.MusicSystem.InitializeMOD())
		return false;

	// Might be reinitialisation
	ClearEffects();
	// (re)init EFX
	Modifiers.Init();
	// Open sound file
	if (!SoundFile.IsOpen())
		if (!Reloc.Open(SoundFile, C4CFN_Sound)) return false;
	// Load static sound from Sound.ocg
	LoadEffects(SoundFile, nullptr, false);
#if AUDIO_TK == AUDIO_TK_SDL_MIXER
	Mix_AllocateChannels(C4MaxSoundInstances);
#endif
	initialized = true;
	return true;
}

void C4SoundSystem::Clear()
{
	initialized = false;
	ClearEffects();
	Modifiers.Clear();
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
	FirstSound=nullptr;
}

void C4SoundSystem::Execute()
{
#if AUDIO_TK == AUDIO_TK_OPENAL
	Application.MusicSystem.SelectContext();
#endif
	C4SoundEffect *csfx;
	for (csfx=FirstSound; csfx; csfx=csfx->Next)
	{
		// Instance removal check
		csfx->Execute();
	}
}

C4SoundEffect* C4SoundSystem::GetEffect(const char *szSndName)
{
	// Remember wildcards before adding .* extension - if there are 2 versions with different file extensions, play the last added
	bool bRandomSound = IsWildcardString(szSndName);
	// Evaluate sound name
	char szName[C4MaxSoundName+2+1];
	SCopy(szSndName,szName,C4MaxSoundName);
	// Any extension accepted
	DefaultExtension(szName,"*");
	// Play nth Sound. Standard: 1
	int32_t iNumber = 1;
	// Sound with a wildcard: determine number of available matches
	if (bRandomSound)
	{
		iNumber = 0;
		// Count matching sounds
		for (C4SoundEffect *pSfx=FirstSound; pSfx; pSfx=pSfx->Next)
			if (WildcardMatch(szName,pSfx->Name))
				++iNumber;
		// Nothing found? Abort
		if(iNumber == 0)
			return nullptr;
		iNumber=UnsyncedRandom(iNumber)+1;
	}
	// Find requested sound effect in bank
	C4SoundEffect *pSfx;
	for (pSfx=FirstSound; pSfx; pSfx=pSfx->Next)
		if (WildcardMatch(szName,pSfx->Name))
			if(!--iNumber)
				break;
	return pSfx; // Is still nullptr if nothing is found
}

C4SoundInstance *C4SoundSystem::NewEffect(const char *szSndName, bool fLoop, int32_t iVolume, C4Object *pObj, int32_t iCustomFalloffDistance, int32_t iPitch, C4SoundModifier *modifier)
{
	// Sound not active
	if (!initialized || !Config.Sound.RXSound) return nullptr;
	// Get sound
	C4SoundEffect *csfx;
	if (!(csfx = GetEffect(szSndName)))
	{
		// Warn about missing or incorrectly spelled sound to allow finding mistakes earlier.
		DebugLogF("Warning: could not find sound matching '%s'", szSndName);
		return nullptr;
	}
	// Play
	return csfx->New(fLoop, iVolume, pObj, iCustomFalloffDistance, iPitch, modifier);
}

C4SoundInstance *C4SoundSystem::FindInstance(const char *szSndName, C4Object *pObj)
{
	char szName[C4MaxSoundName+2+1];
	// Evaluate sound name (see GetEffect)
	SCopy(szSndName,szName,C4MaxSoundName);
	DefaultExtension(szName,"*");
	// Find an effect with a matching instance
	for (C4SoundEffect *csfx = FirstSound; csfx; csfx = csfx->Next)
		if (WildcardMatch(szName, csfx->Name))
		{
			C4SoundInstance *pInst = csfx->GetInstance(pObj);
			if (pInst) return pInst;
		}
	return nullptr;
}

// LoadEffects will load all sound effects of all known sound types (i.e. *.wav and *.ogg) as defined in C4CFN_SoundFiles

int32_t C4SoundSystem::LoadEffects(C4Group &hGroup, const char *namespace_prefix, bool group_is_root)
{
	// Local definition sounds: If there is a Sound.ocg in the group, load the sound from there
	if(group_is_root && hGroup.FindEntry(C4CFN_Sound))
	{
		C4Group g;
		g.OpenAsChild(&hGroup, C4CFN_Sound, false, false);
		return LoadEffects(g, namespace_prefix, false);
	}
	int32_t iNum=0;
	char szFilename[_MAX_FNAME_LEN];
	char szFileType[_MAX_FNAME_LEN];
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
				if (nsfx->Load(szFilename, hGroup, namespace_prefix))
				{
					// Overload same name effects
					RemoveEffect(nsfx->Name);
					// Add effect
					nsfx->Next=FirstSound;
					FirstSound=nsfx;
					iNum++;
				}
				else
					delete nsfx;
			}
	}
	// Load subgroups from Sound.ocg and other subgroups
	if (!group_is_root)
	{
		hGroup.ResetSearch();
		while (hGroup.FindNextEntry(C4CFN_SoundSubgroups, szFilename))
		{
			// Load from subgroup as a sub-namespace
			// get namespace name
			StdStrBuf sub_namespace;
			if (namespace_prefix)
			{
				sub_namespace.Copy(namespace_prefix);
				sub_namespace.Append("::");
			}
			sub_namespace.Append(szFilename, strlen(szFilename) - strlen(C4CFN_SoundSubgroups) + 1);
			// load from child group
			C4Group subgroup;
			if (subgroup.OpenAsChild(&hGroup, szFilename, false, false))
			{
				iNum += LoadEffects(subgroup, sub_namespace.getData(), false);
			}
		}
	}
	return iNum;
}

int32_t C4SoundSystem::RemoveEffect(const char *szFilename)
{
	int32_t iResult=0;
	C4SoundEffect *pNext,*pPrev=nullptr;
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

C4SoundInstance *C4SoundSystem::GetFirstInstance() const
{
	// Return by searching through effect linked list.
	for (C4SoundEffect *pSfx = FirstSound; pSfx; pSfx = pSfx->Next)
		if (pSfx->FirstInst) return pSfx->FirstInst;
	return nullptr;
}

C4SoundInstance *C4SoundSystem::GetNextInstance(C4SoundInstance *prev) const
{
	// Return by searching through instance linked list and parent linked list of effects
	assert(prev && prev->pEffect);
	if (prev->pNext) return prev->pNext;
	for (C4SoundEffect *pSfx = prev->pEffect->Next; pSfx; pSfx = pSfx->Next)
		if (pSfx->FirstInst) return pSfx->FirstInst;
	return nullptr;
}

C4SoundInstance *StartSoundEffect(const char *szSndName, bool fLoop, int32_t iVolume, C4Object *pObj, int32_t iCustomFalloffDistance, int32_t iPitch, C4SoundModifier *modifier)
{
	// Sound check
	if (!Config.Sound.RXSound) return nullptr;
	// Start new
	return Application.SoundSystem.NewEffect(szSndName, fLoop, iVolume, pObj, iCustomFalloffDistance, iPitch, modifier);
}

C4SoundInstance *StartSoundEffectAt(const char *szSndName, int32_t iX, int32_t iY, int32_t iVolume, int32_t iCustomFallofDistance, int32_t iPitch, C4SoundModifier *modifier)
{
	// Sound check
	if (!Config.Sound.RXSound) return nullptr;
	// Create
	C4SoundInstance *pInst = StartSoundEffect(szSndName, false, iVolume, nullptr, iCustomFallofDistance, iPitch, modifier);
	// Set volume by position
	if (pInst) pInst->SetVolumeByPos(iX, iY, iVolume);
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

void SoundPitch(const char *szSndName, C4Object *pObj, int32_t iPitch)
{
	// Find instance
	C4SoundInstance *pInst = Application.SoundSystem.FindInstance(szSndName, pObj);
	if (!pInst) return;
	// Set pitch
	pInst->SetPitch(iPitch);
	pInst->Execute();
}

void SoundUpdate(C4SoundInstance *pInst, int32_t iLevel, int32_t iPitch)
{
	// Set sound data
	pInst->SetVolume(iLevel);
	pInst->SetPitch(iPitch);
	// Ensure it's reflected in audio engine
	pInst->Execute();
}
