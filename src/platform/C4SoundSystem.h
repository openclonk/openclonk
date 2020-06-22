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

/* Handles the sound bank and plays effects */

#ifndef INC_C4SoundSystem
#define INC_C4SoundSystem

#include "c4group/C4Group.h"
#include "platform/C4SoundModifiers.h"

const int32_t
	C4MaxSoundName=100,
	C4MaxSoundInstances=20,
	C4NearSoundRadius=50,
	C4AudibilityRadius=700;

const int32_t SoundUnloadTime=60, SoundMaxUnloadSize=100000;

class C4SoundInstance;
class C4SoundEffect;
class C4SoundModifier;

class C4SoundSystem
{
public:
	C4SoundSystem();
	~C4SoundSystem();
	void Clear();
	void Execute();
	int32_t LoadEffects(C4Group &hGroup, const char *namespace_prefix, bool group_is_root);
	C4SoundInstance *NewEffect(const char *szSound, bool fLoop = false, int32_t iVolume = 100, C4Object *pObj = nullptr, int32_t iCustomFalloffDistance = 0, int32_t iPitch = 0, C4SoundModifier *modifier = nullptr);
	C4SoundInstance *FindInstance(const char *szSound, C4Object *pObj);
	C4SoundInstance *GetFirstInstance() const;
	C4SoundInstance *GetNextInstance(C4SoundInstance *prev) const;
	bool Init();
	void ClearPointers(C4Object *pObj);
	C4SoundEffect *GetFirstSound() const { return FirstSound; }

	C4SoundModifierList Modifiers;
protected:
	C4Group SoundFile;
	C4SoundEffect *FirstSound{nullptr}; // TODO: Add a hash map for sound lookup. Also add a global list for all running sound instances.
	bool initialized{false};
	void ClearEffects();
	C4SoundEffect* GetEffect(const char *szSound);
	int32_t RemoveEffect(const char *szFilename);
};

C4SoundInstance *StartSoundEffect(const char *szSndName, bool fLoop = false, int32_t iVolume = 100, C4Object *pObj = nullptr, int32_t iCustomFalloffDistance = 0, int32_t iPitch = 0, C4SoundModifier *modifier = nullptr);
C4SoundInstance *StartSoundEffectAt(const char *szSndName, int32_t iX, int32_t iY, int32_t iVolume = 100, int32_t iCustomFallofDistance = 0, int32_t iPitch = 0, C4SoundModifier *modifier = nullptr);
C4SoundInstance *GetSoundInstance(const char *szSndName, C4Object *pObj);
void StopSoundEffect(const char *szSndName, C4Object *pObj);
void SoundLevel(const char *szSndName, C4Object *pObj, int32_t iLevel);
void SoundPan(const char *szSndName, C4Object *pObj, int32_t iPan);
void SoundPitch(const char *szSndName, C4Object *pObj, int32_t iPitch);
void SoundUpdate(C4SoundInstance *inst, int32_t iLevel, int32_t iPitch);

#endif
