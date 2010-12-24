/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2005  Peter Wortmann
 * Copyright (c) 2006, 2009-2010  Günther Brammer
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
/* string table: holds all strings used by script engine */

#include <C4Include.h>
#include <C4StringTable.h>

#include <C4Group.h>
#include <C4Components.h>
//#include <C4Aul.h>

// *** C4Set
template<> template<>
unsigned int C4Set<C4String *>::Hash<const char *>(const char * s)
{
	// Fowler/Noll/Vo hash
	unsigned int h = 2166136261u;
	while (*s)
		h = (h ^ *(s++)) * 16777619;
	return h;
}

template<> template<>
bool C4Set<C4String *>::Equals<const char *>(C4String * a, const char * b)
{
	return a->GetData() == b;
}

// *** C4String

C4String::C4String(StdStrBuf strString)
: RefCnt(0)
{
	// take string
	Data.Take(std::move(strString));
	Hash = Strings.Set.Hash(Data.getData());
	// reg
	Strings.Set.Add(this);
}

C4String::C4String()
: RefCnt(0)
{
}

C4String::~C4String()
{
	// unreg
	static bool remove = false;
	assert(!remove);
	remove = true;
	Strings.Set.Remove(this);
	remove = false;
}

void C4String::operator=(const char * s)
{
	assert(!RefCnt);
	assert(!Data);
	// ref string
	Data.Ref(s);
	Hash = Strings.Set.Hash(Data.getData());
	// reg
	Strings.Set.Add(this);
}

// *** C4StringTable

C4StringTable::C4StringTable()
{
	P[P_Prototype] = "Prototype";
	P[P_Name] = "Name";
	P[P_Priority] = "Priority";
	P[P_Interval] = "Interval";
	P[P_CommandTarget] = "CommandTarget";
	P[P_Time] = "Time";
	P[P_Collectible] = "Collectible";
	P[P_ActMap] = "ActMap";
	P[P_Procedure] = "Procedure";
	P[P_Speed] = "Speed";
	P[P_Accel] = "Accel";
	P[P_Decel] = "Decel";
	P[P_Attach] = "Attach";
	P[P_Directions] = "Directions";
	P[P_FlipDir] = "FlipDir";
	P[P_Length] = "Length";
	P[P_Delay] = "Delay";
	P[P_X] = "X";
	P[P_Y] = "Y";
	P[P_Wdt] = "Wdt";
	P[P_Hgt] = "Hgt";
	P[P_OffX] = "OffX";
	P[P_OffY] = "OffY";
	P[P_FacetBase] = "FacetBase";
	P[P_FacetTopFace] = "FacetTopFace";
	P[P_FacetTargetStretch] = "FacetTargetStretch";
	P[P_NextAction] = "NextAction";
	P[P_Hold] = "Hold";
	P[P_Idle] = "Idle";
	P[P_NoOtherAction] = "NoOtherAction";
	P[P_StartCall] = "StartCall";
	P[P_EndCall] = "EndCall";
	P[P_AbortCall] = "AbortCall";
	P[P_PhaseCall] = "PhaseCall";
	P[P_Sound] = "Sound";
	P[P_ObjectDisabled] = "ObjectDisabled";
	P[P_DigFree] = "DigFree";
	P[P_InLiquidAction] = "InLiquidAction";
	P[P_TurnAction] = "TurnAction";
	P[P_Reverse] = "Reverse";
	P[P_Step] = "Step";
	P[P_Animation] = "Animation";
	P[P_Action] = "Action";
	P[P_Visibility] = "Visibility";
	P[P_Parallaxity] = "Parallaxity";
	P[P_LineColors] = "LineColors";
	P[P_LineAttach] = "LineAttach";
	P[P_MouseDragImage] = "MouseDragImage";
	P[P_PictureTransformation] = "PictureTransformation";
	P[P_MeshTransformation] = "MeshTransformation";
	P[P_BreatheWater] = "BreatheWater";
	P[P_CorrosionResist] = "CorrosionResist";
	P[P_MaxEnergy] = "MaxEnergy";
	P[P_MaxBreath] = "MaxBreath";
	P[P_ThrowSpeed] = "ThrowSpeed";
	P[P_Mode] = "Mode";
	P[P_CausedBy] = "CausedBy";
	P[P_Blasted] = "Blasted";
	P[P_IncineratingObj] = "IncineratingObj";
	P[DFA_WALK] = "WALK";
	P[DFA_FLIGHT] = "FLIGHT";
	P[DFA_KNEEL] = "KNEEL";
	P[DFA_SCALE] = "SCALE";
	P[DFA_HANGLE] = "HANGLE";
	P[DFA_DIG] = "DIG";
	P[DFA_SWIM] = "SWIM";
	P[DFA_THROW] = "THROW";
	P[DFA_BRIDGE] = "BRIDGE";
	P[DFA_BUILD] = "BUILD";
	P[DFA_PUSH] = "PUSH";
	P[DFA_CHOP] = "CHOP";
	P[DFA_LIFT] = "LIFT";
	P[DFA_FLOAT] = "FLOAT";
	P[DFA_ATTACH] = "ATTACH";
	P[DFA_CONNECT] = "CONNECT";
	P[DFA_PULL] = "PULL";
	// Prevent the individual strings from being deleted, they are not created with new
	for (unsigned int i = 0; i < P_LAST; ++i) P[i].IncRef();
}

C4StringTable::~C4StringTable()
{
#ifdef _DEBUG
	if(Set.GetSize() != P_LAST)
	{
		for (C4String * const * s = Set.First(); s; s = Set.Next(s))
		{
			fprintf(stderr, "\"%s\"\n", (*s)->GetCStr());
		}
	}
#endif
	assert(Set.GetSize() == P_LAST);
	for (unsigned int i = 0; i < P_LAST; ++i) P[i].Data.Clear();
}

C4String *C4StringTable::RegString(StdStrBuf String)
{
	C4String * s = FindString(String.getData());
	if (s)
		return s;
	else
		return new C4String(String);
}

C4String *C4StringTable::FindString(const char *strString)
{
	return Set.Get(strString);
}

C4StringTable Strings;
