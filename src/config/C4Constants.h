/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2005  Matthes Bender
 * Copyright (c) 2002, 2006-2007, 2009  Sven Eberhardt
 * Copyright (c) 2005, 2007  Günther Brammer
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

/* Lots of constants */

#ifndef INC_C4Constants
#define INC_C4Constants

//============================= Main =====================================================

const size_t C4MaxTitle = 512;
const int
	C4MaxDefString = 100,
	C4MaxMessage = 256,
	C4RetireDelay =  60,
	C4MaxColor = 12,
	C4MaxKey = 12,
	C4MaxKeyboardSet = 4,
	C4MaxControlSet = C4MaxKeyboardSet+4, // keyboard sets+gamepads
	C4MaxControlRate = 20,
	C4MaxGammaUserRamps = 8,
	C4MaxGammaRamps = C4MaxGammaUserRamps+1;

// gamma ramp indices
#define   C4GRI_SCENARIO  0
#define   C4GRI_SEASON    1
#define   C4GRI_RESERVED1 2
#define   C4GRI_DAYTIME   3
#define   C4GRI_RESERVED2 4
#define   C4GRI_LIGHTNING 5
#define   C4GRI_MAGIC     6
#define   C4GRI_RESERVED3 7
#define   C4GRI_USER      8

const int 
	C4M_MaxName = 15,
	C4M_MaxDefName = 2*C4M_MaxName+1,
	C4M_MaxTexIndex = 127; // last texture map index is reserved for diff

const int C4S_MaxPlayer = 4;

const int C4D_MaxVertex = 30;

const int 
	C4SymbolSize = 35,
	C4SymbolBorder = 5,
	C4UpperBoardHeight = 50,
	C4PictureSize = 64,
	C4MaxPictureSize = 150,
	C4MaxBigIconSize = 64;

const int C4P_MaxPosition = 4;

const int 
	C4P_Control_None = -1,
	C4P_Control_Keyboard1 = 0,
	C4P_Control_Keyboard2 = 1,
	C4P_Control_Keyboard3 = 2,
	C4P_Control_Keyboard4 = 3,
	C4P_Control_GamePad1 = 4,
	C4P_Control_GamePad2 = 5,
	C4P_Control_GamePad3 = 6,
	C4P_Control_GamePad4 = 7,
	C4P_Control_GamePadMax = C4P_Control_GamePad4;

const int C4ViewportScrollBorder = 40; // scrolling past landscape allowed at range of this border

//============================= Engine Return Values ======================================

const int 
	C4XRV_Completed = 0,
	C4XRV_Failure = 1,
	C4XRV_Aborted = 2;

//============================= Object Character Flags ====================================

const uint32_t	
	OCF_None = 0,
	OCF_All = ~OCF_None,
	OCF_Normal = 1,
	OCF_Construct = 1<<1,
	OCF_Grab = 1<<2,
	OCF_Carryable = 1<<3,
	OCF_OnFire = 1<<4,
	OCF_HitSpeed1 = 1<<5,
	OCF_FullCon = 1<<6,
	OCF_Inflammable = 1<<7,
	OCF_Chop = 1<<8,
	OCF_Rotate = 1<<9,
	OCF_Exclusive = 1<<10,
	OCF_Entrance = 1<<11,
	OCF_HitSpeed2 = 1<<12,
	OCF_HitSpeed3 = 1<<13,     
	OCF_Collection = 1<<14,
	OCF_Living = 1<<15,
	OCF_HitSpeed4 = 1<<16,
	OCF_LineConstruct = 1<<17,
	OCF_Prey = 1<<18,
	OCF_AttractLightning = 1<<19,
	OCF_NotContained = 1<<20,
	OCF_CrewMember = 1<<21,
	OCF_Edible = 1<<22,
	OCF_InLiquid = 1<<23,
	OCF_InSolid = 1<<24,
	OCF_InFree = 1<<25,
	OCF_Available = 1<<26,
	OCF_PowerConsumer = 1<<27,
	OCF_PowerSupply = 1<<28,
	OCF_Container = 1<<29,
	OCF_Alive = 1<<30;

//================================== Contact / Attachment ==============================================

const BYTE // Directional
	CNAT_None = 0,
	CNAT_Left = 1,
	CNAT_Right = 2,
	CNAT_Top = 4,
	CNAT_Bottom = 8,
	CNAT_Center = 16,
	// Additional flags
	CNAT_MultiAttach = 32, // new attachment behaviour; see C4Shape::Attach
	CNAT_NoCollision = 64; // turn off collision for this vertex

const BYTE CNAT_Flags = CNAT_MultiAttach | CNAT_NoCollision; // all attchment flags that can be combined with regular attachment

//=============================== Keyboard Input Controls =====================================================

const int C4DoubleClick = 10;

//=================================== Control Commands ======================================================

const BYTE 
	COM_Single   = 64,
	COM_Double   = 128;

const BYTE 
	COM_None = 0,
	COM_Left = 1,
	COM_Right = 2,
	COM_Up = 3,
	COM_Down = 4,
	COM_Throw = 5,
	COM_Dig = 6,

	COM_Special = 7,
	COM_Special2 = 8,

	COM_Contents = 9,

	COM_WheelUp = 10,
	COM_WheelDown= 11,

	COM_Left_R = COM_Left + 16,
	COM_Right_R = COM_Right + 16,
	COM_Up_R = COM_Up + 16,
	COM_Down_R = COM_Down + 16,
	COM_Throw_R = COM_Throw + 16,
	COM_Dig_R = COM_Dig + 16,
	COM_Special_R = COM_Special + 16,
	COM_Special2_R = COM_Special2 + 16,
	COM_ReleaseFirst = COM_Left_R,
	COM_ReleaseLast = COM_Special2_R,

	COM_Left_S = COM_Left | COM_Single,
	COM_Right_S = COM_Right | COM_Single,
	COM_Up_S = COM_Up | COM_Single,
	COM_Down_S = COM_Down | COM_Single,
	COM_Throw_S = COM_Throw | COM_Single,
	COM_Dig_S = COM_Dig | COM_Single,
	COM_Special_S = COM_Special | COM_Single,
	COM_Special2_S = COM_Special2 | COM_Single,

	COM_Left_D = COM_Left | COM_Double,
	COM_Right_D = COM_Right | COM_Double,
	COM_Up_D = COM_Up | COM_Double,
	COM_Down_D = COM_Down | COM_Double,
	COM_Throw_D = COM_Throw | COM_Double,
	COM_Dig_D = COM_Dig | COM_Double,
	COM_Special_D = COM_Special | COM_Double,
	COM_Special2_D = COM_Special2 | COM_Double;

const BYTE 
	COM_CursorLeft = 30,
	COM_CursorRight = 31;

const BYTE 
	COM_Help = 35,
	COM_PlayerMenu = 36,
	COM_Chat = 37;

const BYTE 
	COM_MenuEnter = 38,
	COM_MenuEnterAll = 39,
	COM_MenuClose = 40,
	COM_MenuShowText = 42,
	COM_MenuLeft = 52,
	COM_MenuRight = 53,
	COM_MenuUp = 54,
	COM_MenuDown = 55,
	COM_MenuSelect = 60,

	COM_MenuFirst = COM_MenuEnter,
	COM_MenuLast = COM_MenuSelect,

	COM_MenuNavigation1 = COM_MenuShowText,
	COM_MenuNavigation2 = COM_MenuSelect;

//=================================== SendCommand ========================================
const int32_t 
	C4P_Command_None = 0,
	C4P_Command_Set = 1,
	C4P_Command_Add = 2,
	C4P_Command_Append = 4,
	C4P_Command_Range = 8;

//=================================== Owners ==============================================

const int 
	NO_OWNER = -1,
	ANY_OWNER = -2,
	BY_OWNER = 10000,
	BY_HOSTILE_OWNER = 20000;

//=================================== League (escape those damn circular includes =========

enum C4LeagueDisconnectReason
{
	C4LDR_Unknown,
	C4LDR_ConnectionFailed,
	C4LDR_Desync
};

//=================================== Player (included by C4PlayerInfo and C4Player)

enum C4PlayerType
{
	C4PT_None = 0,
	C4PT_User = 1,     // Normal player
	C4PT_Script = 2    // AI players, etc.
};

//=================================== AllowPictureStack (DefCore value)

enum C4AllowPictureStack
{
	APS_Color = 1<<0,
	APS_Graphics = 1<<1,
	APS_Name = 1<<2,
	APS_Overlay = 1<<3
};

// Object size
const int32_t FullCon = 100000;

#endif // INC_C4Constants
