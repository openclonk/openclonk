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

/* Helper classes for individual sounds and effects in sound system. */

#ifndef INC_C4SoundInstance
#define INC_C4SoundInstance
#include "platform/C4SoundSystem.h"

class C4Object; 
class C4SoundModifier;

class C4SoundEffect
{
	friend class C4SoundInstance;
public:
	C4SoundEffect();
	~C4SoundEffect();
public:
	char Name[C4MaxSoundName+1];
	int32_t Instances{0};
	int32_t SampleRate, Length;
	C4SoundHandle pSample{0};
	C4SoundInstance *FirstInst{nullptr};
	C4SoundEffect *Next{nullptr};
public:
	void Clear();
	bool Load(const char *szFileName, C4Group &hGroup, const char *namespace_prefix);
	bool Load(BYTE *pData, size_t iDataLen, bool fRaw=false); // load directly from memory
	void Execute();
	C4SoundInstance *New(bool fLoop = false, int32_t iVolume = 100, C4Object *pObj = nullptr, int32_t iCustomFalloffDistance = 0, int32_t iPitch = 0, C4SoundModifier *modifier = nullptr);
	C4SoundInstance *GetInstance(C4Object *pObj);
	void ClearPointers(C4Object *pObj);
	int32_t GetStartedInstanceCount(int32_t iX, int32_t iY, int32_t iRad); // local
	int32_t GetStartedInstanceCount(); // global

	const char *GetFullName() const { return Name; }; // return full name including the path prefix
protected:
	void AddInst(C4SoundInstance *pInst);
	void RemoveInst(C4SoundInstance *pInst);
};

class C4SoundInstance
{
	friend class C4SoundEffect;
	friend class C4SoundSystem;
	friend class C4SoundModifier;
protected:
	C4SoundInstance();
public:
	~C4SoundInstance();
protected:
	C4SoundEffect *pEffect{nullptr};
	int32_t iVolume{0}, iPan{0}, iPitch{0}, iChannel{-1};
	bool pitch_dirty{false};
	C4TimeMilliseconds tStarted;
	int32_t iNearInstanceMax;
	bool fLooping;
	C4Object *pObj;
	int32_t iFalloffDistance;
	C4SoundModifier *modifier{nullptr};
	bool has_local_modifier{false};
	C4SoundInstance *pNext{nullptr};

	// NO_OWNER or a player number signifying which player owns the viewport in which the sound is heard (best)
	// Note that this does NOT correspond to the player number given in the Sound() script function
	// (i.e. sounds played only for one player). For example, a global GUI sound would have player==NO_OWNER even if
	// it is played for a specific player only.
	int32_t player;
public:
	C4Object *getObj() const { return pObj; }
	bool isStarted() const { return iChannel != -1; }
	void Clear();
	bool Create(C4SoundEffect *pEffect, bool fLoop = false, int32_t iVolume = 100, C4Object *pObj = nullptr, int32_t iNearInstanceMax = 0, int32_t iFalloffDistance = 0, int32_t inPitch = 0, C4SoundModifier *modifier = nullptr);
	bool CheckStart();
	bool Start();
	bool Stop();
	bool Playing();
	void Execute();
	void SetVolume(int32_t inVolume) { iVolume = inVolume; }
	void SetPan(int32_t inPan) { iPan = inPan; }
	void SetPitch(int32_t inPitch);
	void SetVolumeByPos(int32_t x, int32_t y, int32_t relative_volume = 100);
	void SetObj(C4Object *pnObj) { pObj = pnObj; }
	void ClearPointers(C4Object *pObj);
	bool Inside(int32_t iX, int32_t iY, int32_t iRad);
	C4SoundModifier *GetModifier() const { return modifier; }
	void SetModifier(C4SoundModifier *new_modifier, bool is_global);
	void SetPlayer(int32_t new_player);
};

#endif
