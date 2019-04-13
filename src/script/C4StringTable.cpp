/*
 * OpenClonk, http://www.openclonk.org
 *
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
/* string table: holds all strings used by script engine */

#include "C4Include.h"
#include "script/C4StringTable.h"


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
{
	// take string
	Data.Take(std::move(strString));
	Hash = Strings.Set.Hash(Data.getData());
	// reg
	Strings.Set.Add(this);
}

C4String::C4String() = default;

C4String::~C4String()
{
	// unreg
#ifdef _DEBUG
	static bool remove = false;
	assert(!remove); (void)remove;
	remove = true;
#endif
	Strings.Set.Remove(this);
#ifdef _DEBUG
	remove = false;
#endif
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
	P[P_Construction] = "Construction";
	P[P_Destruction] = "Destruction";
	P[P_Start] = "Start";
	P[P_Stop] = "Stop";
	P[P_Timer] = "Timer";
	P[P_Effect] = "Effect";
	P[P_Damage] = "Damage";
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
	P[P_x] = "x";
	P[P_y] = "y";
	P[P_Wdt] = "Wdt";
	P[P_Hgt] = "Hgt";
	P[P_wdt] = "wdt";
	P[P_hgt] = "hgt";
	P[P_Vertices] = "Vertices";
	P[P_Edges] = "Edges";
	P[P_LineWidth] = "LineWidth";
	P[P_OffX] = "OffX";
	P[P_OffY] = "OffY";
	P[P_Material] = "Material";
	P[P_Proplist] = "Proplist";
	P[P_proplist] = "proplist";
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
	P[P_BorderBound] = "BorderBound";
	P[P_ContactCalls] = "ContactCalls";
	P[P_SolidMaskPlane] = "SolidMaskPlane";
	P[P_Tooltip] = "Tooltip";
	P[P_Placement] = "Placement";
	P[P_ContainBlast] = "ContainBlast";
	P[P_BlastIncinerate] = "BlastIncinerate";
	P[P_ContactIncinerate] = "ContactIncinerate";
	P[P_MaterialIncinerate] = "MaterialIncinerate";
	P[P_Global] = "Global";
	P[P_Scenario] = "Scenario";
	P[P_JumpSpeed] = "JumpSpeed";
	P[P_BackgroundColor] = "BackgroundColor";
	P[P_Decoration] = "Decoration";
	P[P_Symbol] = "Symbol";
	P[P_Target] = "Target";
	P[P_Std] = "Std";
	P[P_Text] = "Text";
	P[P_GraphicsName] = "GraphicsName";
	P[P_OnClick] = "OnClick";
	P[P_OnMouseIn] = "OnMouseIn";
	P[P_OnMouseOut] = "OnMouseOut";
	P[P_OnClose] = "OnClose";
	P[P_ID] = "ID";
	P[P_Style] = "Style";
	P[P_Player] = "Player";
	P[P_Margin] = "Margin";
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
	P[P_CollisionDensity] = "CollisionDensity";
	P[P_OnCollision] = "OnCollision";
	P[P_Distance] = "Distance";
	P[P_Smoke] = "Smoke";
	P[P_Source] = "Source";
	P[P_Color] = "Color";
	P[P_EditCursorCommands] = "EditCursorCommands";
	P[P_IsPointContained] = "IsPointContained";
	P[P_GetRandomPoint] = "GetRandomPoint";
	P[P_Type] = "Type";
	P[P_Reverb_Density] = "Reverb_Density";
	P[P_Reverb_Diffusion] = "Reverb_Diffusion";
	P[P_Reverb_Gain] = "Reverb_Gain";
	P[P_Reverb_GainHF] = "Reverb_GainHF";
	P[P_Reverb_Decay_Time] = "Reverb_Decay_Time";
	P[P_Reverb_Decay_HFRatio] = "Reverb_Decay_HFRatio";
	P[P_Reverb_Reflections_Gain] = "Reverb_Reflections_Gain";
	P[P_Reverb_Reflections_Delay] = "Reverb_Reflections_Delay";
	P[P_Reverb_Late_Reverb_Gain] = "Reverb_Late_Reverb_Gain";
	P[P_Reverb_Late_Reverb_Delay] = "Reverb_Late_Reverb_Delay";
	P[P_Reverb_Air_Absorption_GainHF] = "Reverb_Air_Absorption_GainHF";
	P[P_Reverb_Room_Rolloff_Factor] = "Reverb_Room_Rolloff_Factor";
	P[P_Reverb_Decay_HFLimit] = "Reverb_Decay_HFLimit";
	P[P_Echo_Delay] = "Echo_Delay";
	P[P_Echo_LRDelay] = "Echo_LRDelay";
	P[P_Echo_Damping] = "Echo_Damping";
	P[P_Echo_Feedback] = "Echo_Feedback";
	P[P_Echo_Spread] = "Echo_Spread";
	P[P_Equalizer_Low_Gain] = "Equalizer_Low_Gain";
	P[P_Equalizer_Low_Cutoff] = "Equalizer_Low_Cutoff";
	P[P_Equalizer_Mid1_Gain] = "Equalizer_Mid1_Gain";
	P[P_Equalizer_Mid1_Center] = "Equalizer_Mid1_Center";
	P[P_Equalizer_Mid1_Width] = "Equalizer_Mid1_Width";
	P[P_Equalizer_Mid2_Gain] = "Equalizer_Mid2_Gain";
	P[P_Equalizer_Mid2_Center] = "Equalizer_Mid2_Center";
	P[P_Equalizer_Mid2_Width] = "Equalizer_Mid2_Width";
	P[P_Equalizer_High_Gain] = "Equalizer_High_Gain";
	P[P_Equalizer_High_Cutoff] = "Equalizer_High_Cutoff";
	P[P_LightOffset] = "LightOffset";
	P[P_PlayList] = "PlayList";
	P[P_MusicBreakMin] = "MusicBreakMin";
	P[P_MusicBreakMax] = "MusicBreakMax";
	P[P_MusicBreakChance] = "MusicBreakChance";
	P[P_MusicMaxPositionMemory] = "MusicMaxPositionMemory";
	P[P_InflameLandscape] = "InflameLandscape";
	P[P_OptionKey] = "OptionKey";
	P[P_ValueKey] = "ValueKey";
	P[P_Value] = "Value";
	P[P_DefaultValueFunction] = "DefaultValueFunction";
	P[P_Delegate] = "Delegate";
	P[P_VertexDelegate] = "VertexDelegate";
	P[P_EdgeDelegate] = "EdgeDelegate";
	P[P_HorizontalFix] = "HorizontalFix";
	P[P_VerticalFix] = "VerticalFix";
	P[P_StructureFix] = "StructureFix";
	P[P_OnUpdate] = "OnUpdate";
	P[P_EditorPropertyChanged] = "EditorPropertyChanged";
	P[P_Min] = "Min";
	P[P_Max] = "Max";
	P[P_Set] = "Set";
	P[P_SetGlobal] = "SetGlobal";
	P[P_SetRoot] = "SetRoot";
	P[P_Options] = "Options";
	P[P_Key] = "Key";
	P[P_AsyncGet] = "AsyncGet";
	P[P_Get] = "Get";
	P[P_Relative] = "Relative";
	P[P_CanMoveCenter] = "CanMoveCenter";
	P[P_StartFromObject] = "StartFromObject";
	P[P_Storage] = "Storage";
	P[P_Elements] = "Elements";
	P[P_EditOnSelection] = "EditOnSelection";
	P[P_EditorProps] = "EditorProps";
	P[P_DefaultEditorProp] = "DefaultEditorProp";
	P[P_EditorActions] = "EditorActions";
	P[P_CopyDefault] = "CopyDefault";
	P[P_Display] = "Display";
	P[P_DefaultValue] = "DefaultValue";
	P[P_DefinitionPriority] = "DefinitionPriority";
	P[P_Group] = "Group";
	P[P_Command] = "Command";
	P[P_Select] = "Select";
	P[P_DescendPath] = "DescendPath";
	P[P_EmptyName] = "EmptyName";
	P[P_ShortName] = "ShortName";
	P[P_EditorHelp] = "EditorHelp";
	P[P_Description] = "Description";
	P[P_AllowEditing] = "AllowEditing";
	P[P_EditorInitialize] = "EditorInitialize";
	P[P_EditorPlacementLimit] = "EditorPlacementLimit";
	P[P_EditorCollection] = "EditorCollection";
	P[P_Sorted] = "Sorted";
	P[P_Uniforms] = "Uniforms";
	P[P_ForceSerialization] = "ForceSerialization";
	P[P_DrawArrows] = "DrawArrows";
	P[P_SCENPAR] = "SCENPAR";
	P[P_Translatable] = "Translatable";
	P[P_Function] = "Function";
	P[P_Translate] = "Translate";
	P[DFA_WALK] = "WALK";
	P[DFA_FLIGHT] = "FLIGHT";
	P[DFA_KNEEL] = "KNEEL";
	P[DFA_SCALE] = "SCALE";
	P[DFA_HANGLE] = "HANGLE";
	P[DFA_DIG] = "DIG";
	P[DFA_SWIM] = "SWIM";
	P[DFA_THROW] = "THROW";
	P[DFA_PUSH] = "PUSH";
	P[DFA_LIFT] = "LIFT";
	P[DFA_FLOAT] = "FLOAT";
	P[DFA_ATTACH] = "ATTACH";
	P[DFA_CONNECT] = "CONNECT";
	P[DFA_PULL] = "PULL";
	// Prevent the individual strings from being deleted, they are not created with new
	for (auto & i : P)
	{
		assert(i.GetCStr()); // all strings should be assigned
		i.IncRef();
	}
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

C4String *C4StringTable::FindString(const char *strString) const
{
	return Set.Get(strString);
}
