/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
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
/* string table: holds all strings used by script engine */

#include <C4Include.h>
#include <C4StringTable.h>


// *** C4Set
template<> template<>
unsigned int C4Set<C4String *>::Hash<const char *>(const char * const & s)
{
	// Fowler/Noll/Vo hash
	unsigned int h = 2166136261u;
	const char * p = s;
	while (*p)
		h = (h ^ *(p++)) * 16777619;
	return h;
}

template<> template<>
bool C4Set<C4String *>::Equals<const char *>(C4String * const & a, const char * const & b)
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
	P[P_Touchable] = "Touchable";
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
	P[P_LineMaxDistance] = "LineMaxDistance";
	P[P_MouseDrag] = "MouseDrag";
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
	P[P_Plane] = "Plane";
	P[P_SolidMaskPlane] = "SolidMaskPlane";
	P[P_Tooltip] = "Tooltip";
	P[P_Placement] = "Placement";
	P[P_ContainBlast] = "ContainBlast";
	P[P_BlastIncinerate] = "BlastIncinerate";
	P[P_ContactIncinerate] = "ContactIncinerate";
	P[P_Global] = "Global";
	P[P_Scenario] = "Scenario";
	P[P_JumpSpeed] = "JumpSpeed";
	P[P_Algo] = "Algo";
	P[P_Layer] = "Layer";
	P[P_Seed] = "Seed";
	P[P_Ratio] = "Ratio";
	P[P_FixedOffset] = "FixedOffset";
	P[P_Op] = "Op";
	P[P_R] = "R";
	P[P_Scale] = "Scale";
	P[P_Amplitude] = "Amplitude";
	P[P_Iterations] = "Iterations";
	P[P_Empty] = "Empty";
	P[P_Open] = "Open";
	P[P_Left] = "Left";
	P[P_Top] = "Top";
	P[P_Right] = "Right";
	P[P_Bottom] = "Bottom";
	P[P_Filter] = "Filter";
	P[P_ForceX] = "ForceX";
	P[P_ForceY] = "ForceY";
	P[P_G] = "G";
	P[P_B] = "B";
	P[P_Alpha] = "Alpha";
	P[P_DampingX] = "DampingX";
	P[P_DampingY] = "DampingY";
	P[P_Size] = "Size";
	P[P_Rotation] = "Rotation";
	P[P_BlitMode] = "BlitMode";
	P[P_Phase] = "Phase";
	P[P_Stretch] = "Stretch";
	P[P_CollisionVertex] = "CollisionVertex";
	P[P_OnCollision] = "OnCollision";
	P[P_Distance] = "Distance";
	P[P_Smoke] = "Smoke";
	P[P_Source] = "Source";
	P[P_Color] = "Color";
	P[P_EditCursorCommands] = "EditCursorCommands";
	P[DFA_WALK] = "WALK";
	P[DFA_FLIGHT] = "FLIGHT";
	P[DFA_KNEEL] = "KNEEL";
	P[DFA_SCALE] = "SCALE";
	P[DFA_HANGLE] = "HANGLE";
	P[DFA_DIG] = "DIG";
	P[DFA_SWIM] = "SWIM";
	P[DFA_THROW] = "THROW";
	P[DFA_BRIDGE] = "BRIDGE";
	P[DFA_PUSH] = "PUSH";
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
			if (*s >= &Strings.P[0] && *s < &Strings.P[P_LAST])
			{
				if ((*s)->RefCnt != 1)
#ifdef _WIN32
					OutputDebugString(FormatString(" \"%s\" %d\n", (*s)->GetCStr(), (*s)->RefCnt).GetWideChar());
#else
					fprintf(stderr, " \"%s\" %d\n", (*s)->GetCStr(), (*s)->RefCnt);
#endif
			}
			else
#ifdef _WIN32
				OutputDebugString(FormatString("\"%s\" %d\n", (*s)->GetCStr(), (*s)->RefCnt).GetWideChar());
#else
				fprintf(stderr, "\"%s\" %d\n", (*s)->GetCStr(), (*s)->RefCnt);
#endif
		}
	}
#endif
	assert(Set.GetSize() == P_LAST);
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
