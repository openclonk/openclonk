/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2015, The OpenClonk Team and contributors
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

/* Plays sound effects */

#ifndef INC_C4SoundModifiers
#define INC_C4SoundModifiers

#include <C4SoundIncludes.h>
#include <C4PropList.h>

class C4SoundModifier
{
public:
	enum Type
	{
		C4SMT_None = 0x0,
		C4SMT_Reverb = 0x1,
		C4SMT_Echo = 0x4,
		C4SMT_Equalizer = 0xc,
		C4SMT_Max = 0xc // value of largest type
	};

	C4SoundModifier(C4PropList *in_props);
	virtual ~C4SoundModifier();

private:
	// associated prop list for script interface
	C4Value props;

	// number of sound instances currently using the modifier
	int32_t instance_count;

	// set to true for sound modifiers released by script but with instance_count>0
	bool released;

#if AUDIO_TK == AUDIO_TK_OPENAL
protected:
	ALuint effect, slot;
#endif

protected:
	// get a property from the props proplist and divide by factor for float representation
	float GetFloatProp(C4PropertyName key, float ratio, float default_value);
	bool GetBoolProp(C4PropertyName key, bool default_value);

public:
	// update from props and mark as not released
	virtual void Update();

	// effect is deleted when marked for release and no instances are running
	void Release() {
		if (!instance_count) delete this; else { released = true; props.Set0(); }
	}
	void AddRef() { ++instance_count; }
	void DelRef() { if (!--instance_count && released) delete this; }
	int32_t GetRefCount() const { return instance_count; }

	const C4PropList *GetProps() const { return props._getPropList(); }

#if AUDIO_TK == AUDIO_TK_OPENAL
	// apply to AL buffer
	void ApplyTo(ALuint source);
#endif
};

// Reverb sound modifier: Adds effect of sound reflections in enclosed spaces
class C4SoundModifierReverb : public C4SoundModifier
{
public:
	C4SoundModifierReverb(C4PropList *in_props);

public:
	virtual void Update();
};

// Echo: Repeats dampened version of input signal
class C4SoundModifierEcho : public C4SoundModifier
{
public:
	C4SoundModifierEcho(C4PropList *in_props);

public:
	virtual void Update();
};

// Equalizer: Allows to specify low- mid- and high-frequency amplification and reduction
class C4SoundModifierEqualizer : public C4SoundModifier
{
public:
	C4SoundModifierEqualizer(C4PropList *in_props);

public:
	virtual void Update();
};

// member of C4SoundSystem: Handles modifier management and EFX initialization
class C4SoundModifierList
{
private:
	bool is_initialized;
	bool is_effect_available[C4SoundModifier::C4SMT_Max+1];
	std::list<C4SoundModifier *> sound_modifiers;
	std::vector<C4SoundModifier *> global_modifiers; // global modifiers indexed by player number+1. Global modifier for all players in index 0.
public:
	C4SoundModifierList();
	void Init();
	void Add(C4SoundModifier *new_modifier) { sound_modifiers.push_back(new_modifier);  }
	void Remove(C4SoundModifier *prev_modifier) { sound_modifiers.remove(prev_modifier); }
	C4SoundModifier *Get(class C4PropList *props, bool create_if_not_found);
	void Clear();
	void SetGlobalModifier(C4SoundModifier *new_modifier, int32_t player_index);
	C4SoundModifier *GetGlobalModifier(int32_t player_index) const;
	
};

#endif
