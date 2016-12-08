/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2015-2016, The OpenClonk Team and contributors
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
#include "platform/C4SoundModifiers.h"
#include "platform/C4SoundSystem.h"
#include "platform/C4SoundInstance.h"
#include "platform/C4SoundIncludes.h"
#include "game/C4Application.h"
#include "script/C4Value.h"

#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
static LPALGENEFFECTS alGenEffects;
static LPALDELETEEFFECTS alDeleteEffects;
static LPALISEFFECT alIsEffect;
static LPALEFFECTI alEffecti;
static LPALEFFECTIV alEffectiv;
static LPALEFFECTF alEffectf;
static LPALEFFECTFV alEffectfv;
static LPALGETEFFECTI alGetEffecti;
static LPALGETEFFECTIV alGetEffectiv;
static LPALGETEFFECTF alGetEffectf;
static LPALGETEFFECTFV alGetEffectfv;
static LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
static LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
static LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
static LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
static LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
static LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
static LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
static LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
static LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
static LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;
#endif

C4SoundModifier::C4SoundModifier(C4PropList *in_props) : instance_count(0)
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
	, effect(0u), slot(0u)
#endif
{
	props.SetPropList(in_props);
	Application.SoundSystem.Modifiers.Add(this);
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
	Application.MusicSystem.SelectContext();
	alGenEffects(1, &effect);
	alGenAuxiliaryEffectSlots(1, &slot);
#endif
}

C4SoundModifier::~C4SoundModifier()
{
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
	// failsafe effect removal
	if (alIsEffect(effect))
		alDeleteEffects(1, &effect);
	if (alIsAuxiliaryEffectSlot(slot))
		alDeleteAuxiliaryEffectSlots(1, &slot);
#endif
	Application.SoundSystem.Modifiers.Remove(this);
}

void C4SoundModifier::Update()
{
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
	// update AL effect slot
	if (slot)
	{
		alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect);
	}
#endif
	// update from props and mark as not released
	released = false;
}

#if AUDIO_TK == AUDIO_TK_OPENAL
void C4SoundModifier::ApplyTo(ALuint source)
{
	// apply slot to source if valid
#if defined(HAVE_ALEXT)
	if (slot) alSource3i(source, AL_AUXILIARY_SEND_FILTER, slot, 0, AL_FILTER_NULL);
#endif
}
#endif

float C4SoundModifier::GetFloatProp(C4PropertyName key, float ratio, float default_value)
{
	// get scaled property and fill in default value
	C4PropList *p = props._getPropList();
	return float(p->GetPropertyInt(key, int32_t(default_value * ratio))) / ratio;
}

bool C4SoundModifier::GetBoolProp(C4PropertyName key, bool default_value)
{
	// get scaled property and fill in default value
	C4PropList *p = props._getPropList();
	return (p->GetPropertyInt(key, int32_t(default_value ? 1 : 0)) != 0);
}

C4SoundModifierReverb::C4SoundModifierReverb(C4PropList *in_props)
	: C4SoundModifier(in_props)
{
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
	alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);
#endif
}

void C4SoundModifierReverb::Update()
{
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
	// use the cave preset as default for the reverb modifier
	Application.MusicSystem.SelectContext();
	alEffectf(effect, AL_REVERB_DENSITY, GetFloatProp(P_Reverb_Density, 1000, 1.0f));
	alEffectf(effect, AL_REVERB_DIFFUSION, GetFloatProp(P_Reverb_Diffusion, 1000, 1.0f));
	alEffectf(effect, AL_REVERB_GAIN, GetFloatProp(P_Reverb_Gain, 1000, 0.316f));
	alEffectf(effect, AL_REVERB_GAINHF, GetFloatProp(P_Reverb_GainHF, 1000, 1.0f));
	alEffectf(effect, AL_REVERB_DECAY_TIME, GetFloatProp(P_Reverb_Decay_Time, 1000, 2.91f));
	alEffectf(effect, AL_REVERB_DECAY_HFRATIO, GetFloatProp(P_Reverb_Decay_HFRatio, 1000, 1.3f));
	alEffectf(effect, AL_REVERB_REFLECTIONS_GAIN, GetFloatProp(P_Reverb_Reflections_Gain, 1000, 0.5f));
	alEffectf(effect, AL_REVERB_REFLECTIONS_DELAY, GetFloatProp(P_Reverb_Reflections_Delay, 1000, 0.015f));
	alEffectf(effect, AL_REVERB_LATE_REVERB_GAIN, GetFloatProp(P_Reverb_Late_Reverb_Gain, 1000, 0.706f));
	alEffectf(effect, AL_REVERB_LATE_REVERB_DELAY, GetFloatProp(P_Reverb_Late_Reverb_Delay, 1000, 0.022f));
	alEffectf(effect, AL_REVERB_AIR_ABSORPTION_GAINHF, GetFloatProp(P_Reverb_Air_Absorption_GainHF, 1000, 0.994f));
	alEffectf(effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, GetFloatProp(P_Reverb_Room_Rolloff_Factor, 1000, 0.0f));
	alEffecti(effect, AL_REVERB_DECAY_HFLIMIT, GetBoolProp(P_Reverb_Decay_HFLimit, true) ? 1 : 0);
#endif
	C4SoundModifier::Update();
}

C4SoundModifierEcho::C4SoundModifierEcho(C4PropList *in_props)
	: C4SoundModifier(in_props)
{
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
	alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);
#endif
}

void C4SoundModifierEcho::Update()
{
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
	// use default OpenAL echo preset
	Application.MusicSystem.SelectContext();
	alEffectf(effect, AL_ECHO_DELAY, GetFloatProp(P_Echo_Delay, 1000, 0.1f));
	alEffectf(effect, AL_ECHO_LRDELAY, GetFloatProp(P_Echo_LRDelay, 1000, 0.1f));
	alEffectf(effect, AL_ECHO_DAMPING, GetFloatProp(P_Echo_Damping, 1000, 0.5f));
	alEffectf(effect, AL_ECHO_FEEDBACK, GetFloatProp(P_Echo_Feedback, 1000, 0.5f));
	alEffectf(effect, AL_ECHO_SPREAD, GetFloatProp(P_Echo_Spread, 1000, -1.0f));
#endif
	C4SoundModifier::Update();
}

C4SoundModifierEqualizer::C4SoundModifierEqualizer(C4PropList *in_props)
	: C4SoundModifier(in_props)
{
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
	alEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EQUALIZER);
#endif
}

void C4SoundModifierEqualizer::Update()
{
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
	// use default OpenAL equalizer preset
	Application.MusicSystem.SelectContext();
	alEffectf(effect, AL_EQUALIZER_LOW_GAIN, GetFloatProp(P_Equalizer_Low_Gain, 1000, 1.0f));
	alEffectf(effect, AL_EQUALIZER_LOW_CUTOFF, GetFloatProp(P_Equalizer_Low_Cutoff, 1000, 200.0f));
	alEffectf(effect, AL_EQUALIZER_MID1_GAIN, GetFloatProp(P_Equalizer_Mid1_Gain, 1000, 1.0f));
	alEffectf(effect, AL_EQUALIZER_MID1_CENTER, GetFloatProp(P_Equalizer_Mid1_Center, 1000, 500.0f));
	alEffectf(effect, AL_EQUALIZER_MID1_WIDTH, GetFloatProp(P_Equalizer_Mid1_Width, 1000, 1.0f));
	alEffectf(effect, AL_EQUALIZER_MID2_GAIN, GetFloatProp(P_Equalizer_Mid2_Gain, 1000, 1.0f));
	alEffectf(effect, AL_EQUALIZER_MID2_CENTER, GetFloatProp(P_Equalizer_Mid2_Center, 1000, 3000.0f));
	alEffectf(effect, AL_EQUALIZER_MID2_WIDTH, GetFloatProp(P_Equalizer_Mid2_Width, 1000, 1.0f));
	alEffectf(effect, AL_EQUALIZER_HIGH_GAIN, GetFloatProp(P_Equalizer_High_Gain, 1000, 1.0f));
	alEffectf(effect, AL_EQUALIZER_HIGH_CUTOFF, GetFloatProp(P_Equalizer_High_Cutoff, 1000, 6000.0f));
#endif
	C4SoundModifier::Update();
}

C4SoundModifierList::C4SoundModifierList()
{
	for (int32_t i = 0; i <= C4SoundModifier::C4SMT_Max; ++i)
		is_effect_available[i] = false;
}

void C4SoundModifierList::Init()
{
	is_initialized = false;
#if (AUDIO_TK == AUDIO_TK_OPENAL) && defined(HAVE_ALEXT)
#define LOAD_ALPROC(x)  ((void *&)(x) = alGetProcAddress(#x))
	Application.MusicSystem.SelectContext();
	if (!alcIsExtensionPresent(Application.MusicSystem.GetDevice(), "ALC_EXT_EFX"))
	{
		LogF("ALExt: No efx extensions available. Sound modifications disabled.");
		return;
	}
	LOAD_ALPROC(alGenEffects);
	LOAD_ALPROC(alDeleteEffects);
	LOAD_ALPROC(alIsEffect);
	LOAD_ALPROC(alEffecti);
	LOAD_ALPROC(alEffectiv);
	LOAD_ALPROC(alEffectf);
	LOAD_ALPROC(alEffectfv);
	LOAD_ALPROC(alGetEffecti);
	LOAD_ALPROC(alGetEffectiv);
	LOAD_ALPROC(alGetEffectf);
	LOAD_ALPROC(alGetEffectfv);
	LOAD_ALPROC(alGenAuxiliaryEffectSlots);
	LOAD_ALPROC(alDeleteAuxiliaryEffectSlots);
	LOAD_ALPROC(alIsAuxiliaryEffectSlot);
	LOAD_ALPROC(alAuxiliaryEffectSloti);
	LOAD_ALPROC(alAuxiliaryEffectSlotiv);
	LOAD_ALPROC(alAuxiliaryEffectSlotf);
	LOAD_ALPROC(alAuxiliaryEffectSlotfv);
	LOAD_ALPROC(alGetAuxiliaryEffectSloti);
	LOAD_ALPROC(alGetAuxiliaryEffectSlotiv);
	LOAD_ALPROC(alGetAuxiliaryEffectSlotf);
	LOAD_ALPROC(alGetAuxiliaryEffectSlotfv);
	if (!alGenEffects)
	{
		LogF("ALExt: Error loading efx extensions. Sound modifications disabled.");
		return; // safety
	}
	StdStrBuf sAvailableEffects("");
	StdStrBuf sUnavailableEffects("");
	static const struct {
		ALenum effect;
		const char *name;
	} test_effects[] = {
#define AL_TEST_EFFECT(effect) { effect, #effect }
		AL_TEST_EFFECT(AL_EFFECT_REVERB),
		AL_TEST_EFFECT(AL_EFFECT_ECHO),
		AL_TEST_EFFECT(AL_EFFECT_EQUALIZER),
#undef AL_TEST_EFFECT
	};
	ALuint effect = 0u;
	alGenEffects(1, &effect);
	ALenum err = alGetError();
	if (err != AL_NO_ERROR)
	{
		LogF("OpenAL alGenEffects Error: %s", alGetString(err));
	}
	for (auto test_effect : test_effects)
	{
		alEffecti(effect, AL_EFFECT_TYPE, test_effect.effect);
		err = alGetError();
		bool is_ok = (err == AL_NO_ERROR);
		is_effect_available[test_effect.effect] = is_ok;
		StdStrBuf *target_string;
		if (!is_ok) target_string = &sUnavailableEffects; else target_string = &sAvailableEffects;
		if (target_string->getLength()) target_string->Append(", ");
		target_string->Append(test_effect.name);
		if (!is_ok) target_string->AppendFormat(" (%s)", alGetString(err));
	}
	if (alIsEffect(effect)) alDeleteEffects(1, &effect);
	if (sAvailableEffects.getLength() == 0) sAvailableEffects = "(none)";
	if (sUnavailableEffects.getLength() == 0) sUnavailableEffects = "(none)";
	LogF("OpenAL extensions loaded. Available: %s. Unavailable: %s.", sAvailableEffects.getData(), sUnavailableEffects.getData());
#undef LOAD_ALPROC
#else
	// modifiers not supported
	return;
#endif
	is_initialized = true;
}

void C4SoundModifierList::Clear()
{
	// Release all sounds. ref count should be zero now because sound instances should have been cleared before this call.
	for (auto iter = sound_modifiers.begin(); iter != sound_modifiers.end(); )
	{
		C4SoundModifier *modifier = *iter;
		++iter;
		// release after iterator increase because deletion will modify the list
		assert(modifier->GetRefCount() == 0);
		modifier->Release();
	}
}

C4SoundModifier *C4SoundModifierList::Get(class C4PropList *props, bool create_if_not_found)
{
	// find odifier by prop list
	auto iter = std::find_if(sound_modifiers.begin(), sound_modifiers.end(),
		[props](const C4SoundModifier *mod) { return mod->GetProps() == props; });
	if (iter != sound_modifiers.end()) return *iter;
	// if not found, create if desired
	if (!create_if_not_found) return nullptr;
	C4SoundModifier *modifier;
	int32_t effect_type = props->GetPropertyInt(P_Type);
	if (!is_effect_available[effect_type]) return nullptr; // Not supported D:
	switch (effect_type)
	{
	case C4SoundModifier::C4SMT_Reverb:
		modifier = new C4SoundModifierReverb(props);
		break;
	case C4SoundModifier::C4SMT_Echo:
		modifier = new C4SoundModifierEcho(props);
		break;
	case C4SoundModifier::C4SMT_Equalizer:
		modifier = new C4SoundModifierEqualizer(props);
		break;
	default:
		// invalid modifier
		return nullptr;
	}
	// initial parameter settings
	modifier->Update();
	return modifier;
}

void C4SoundModifierList::SetGlobalModifier(C4SoundModifier *new_modifier, int32_t player_number)
{
	// -1 based array access for NO_OWNER player number
	int32_t global_modifier_index = player_number + 1;
	if (global_modifier_index < 0) return; // safety
	size_t index = static_cast<size_t>(global_modifier_index);
	if (index >= global_modifiers.size()) global_modifiers.resize(index+1);
	if (new_modifier != global_modifiers[index])
	{
		// Run new effects with new modifier
		C4SoundModifier *old_modifier = global_modifiers[index];
		global_modifiers[index] = new_modifier;
		// update existing sound effects first
		for (C4SoundInstance *pInst = ::Application.SoundSystem.GetFirstInstance(); pInst; pInst = ::Application.SoundSystem.GetNextInstance(pInst))
			if (pInst->GetModifier() == old_modifier) // that check is not 100% correct but should cover all realistic use-cases
				pInst->SetModifier(new_modifier, true);
		// Reference counting
		if (new_modifier) new_modifier->AddRef();
		if (old_modifier) old_modifier->DelRef();
	}
}

C4SoundModifier *C4SoundModifierList::GetGlobalModifier(int32_t player_index) const
{
	// safety
	int32_t list_index = player_index + 1;
	if (list_index < 0 || static_cast<size_t>(list_index) >= global_modifiers.size()) return nullptr;
	// get from player and fall back to global list
	C4SoundModifier *result = global_modifiers[list_index];
	if (!result && list_index) result = global_modifiers[0];
	return result;
}
