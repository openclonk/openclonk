/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010-2012, The OpenClonk Team and contributors
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

#define K(k,s,a) {k+CocoaKeycodeOffset,s,a}
const C4KeyCodeMapEntry KeyCodeMap [] =
{
	{ K_BACK            , "Backspace"     , NULL },
	{ K_TAB             , "Tab"           , NULL },
	//{ VK_CLEAR          , "Clear"         , NULL },
	{ K_RETURN          , "Return"        , NULL },

	/*K( VK_SHIFT          , "KeyShift"      , "Shift" ),
	K( VK_CONTROL        , "KeyControl"    , "Control" ),
	K( VK_MENU           , "Menu"          , NULL ),
	K( VK_PAUSE          , "Pause"         , NULL ),*/

/*	K( VK_CAPITAL        , "Capital"       , NULL ),
	K( VK_KANA           , "Kana"          , NULL ),
	K( VK_HANGEUL        , "Hangeul"       , NULL ),
	K( VK_HANGUL         , "Hangul"        , NULL ),
	K( VK_JUNJA          , "Junja"         , NULL ),
	K( VK_FINAL          , "Final"         , NULL ),
	K( VK_HANJA          , "Hanja"         , NULL ),
	K( VK_KANJI          , "Kanji"         , NULL ),
	K( VK_ESCAPE         , "Escape"        , "Esc" ),
	K( VK_ESCAPE         , "Esc"           ,NULL ),
	K( VK_CONVERT        , "Convert"       , NULL ),
	K( VK_NONCONVERT     , "Noconvert"     , NULL ),
	K( VK_ACCEPT         , "Accept"        , NULL ),
	K( VK_MODECHANGE     , "Modechange"    , NULL ),*/

	{ K_SPACE           , "Space"         , "Sp" },

	{ K_END             , "End"           , NULL },
	{ K_HOME            , "Home"          , NULL },
	{ K_LEFT            , "Left"          , NULL },
	{ K_UP              , "Up"            , NULL },
	{ K_RIGHT           , "Right"         , NULL },
	{ K_DOWN            , "Down"          , NULL },
	/*{ VK_SELECT         , "Select"        , NULL },
	{ VK_PRINT          , "Print"         , NULL },
	{ VK_EXECUTE        , "Execute"       , NULL },
	{ VK_SNAPSHOT       , "Snapshot"      , NULL },
	{ VK_INSERT         , "Insert"        , "Ins" },*/
	{ K_DELETE          , "Delete"        , "Del" },
//	{ VK_HELP           , "Help"          , NULL },
	
	K( 29                , "0"         , NULL ),
	K( 18                , "1"         , NULL ),
	K( 19                , "2"         , NULL ),
	K( 20                , "3"         , NULL ),
	K( 21                , "4"         , NULL ),
	K( 23                , "5"         , NULL ),
	K( 22                , "6"         , NULL ),
	K( 26                , "7"         , NULL ),
	K( 28                , "8"         , NULL ),
	K( 25                , "9"         , NULL ),

	K( 0                 , "A"         , NULL ),
	K( 11                , "B"         , NULL ),
	K( 8                 , "C"         , NULL ),
	K( 2                 , "D"         , NULL ),
	K( 14                , "E"         , NULL ),
	K( 3                 , "F"         , NULL ),
	K( 5                 , "G"         , NULL ),
	K( 4                 , "H"         , NULL ),
	K( 34                , "I"         , NULL ),
	K( 38                , "J"         , NULL ),
	K( 40                , "K"         , NULL ),
	K( 37                , "L"         , NULL ),
	K( 46                , "M"         , NULL ),
	K( 45                , "N"         , NULL ),
	K( 31                , "O"         , NULL ),
	K( 35                , "P"         , NULL ),
	K( 12                , "Q"         , NULL ),
	K( 15                , "R"         , NULL ),
	K( 1                 , "S"         , NULL ),
	K( 17                , "T"         , NULL ),
	K( 32                , "U"         , NULL ),
	K( 9                 , "V"         , NULL ),
	K( 13                , "W"         , NULL ),
	K( 7                 , "X"         , NULL ),
	K( 6                 , "Y"         , NULL ),
	K( 16                , "Z"         , NULL ),
	K( 43                , "Comma"     , NULL ),
	K( 47                , "Period"    , NULL ),
	K( 43                , "Apostrophe", NULL ),
	K( 44                , "Backslash" , NULL ),
	K( 43                , "Comma_US"  , NULL ),
	K( 49                , "Less"      , NULL ),

	/*K( VK_LWIN           , "WinLeft"      , NULL ),
	K( VK_RWIN           , "WinRight"     , NULL ),*/
	//K( VK_APPS           , "Apps"         , NULL ),

	K( 82                , "Num0"         , "N0" ),
	K( 83                , "Num1"         , "N1" ),
	K( 84                , "Num2"         , "N2" ),
	K( 85                , "Num3"         , "N3" ),
	K( 86                , "Num4"         , "N4" ),
	K( 87                , "Num5"         , "N5" ),
	K( 88                , "Num6"         , "N6" ),
	K( 89                , "Num7"         , "N7" ),
	K( 91                , "Num8"         , "N8" ),
	K( 92                , "Num9"         , "N9" ),
	K( 67                , "Multiply"     , "N*" ),
	K( 69                , "Add"          , "N+" ),
	//K( 65                , "Separator"    , "NSep" ),
	K( 78                , "Subtract"     , "N-" ),
	K( 65                , "Decimal"      , "N," ),
	K( 75                , "Divide"       , "N/" ),
	{ K_F1             , "F1"           , NULL },
	{ K_F2             , "F2"           , NULL },
	{ K_F3             , "F3"           , NULL },
	{ K_F4             , "F4"           , NULL },
	{ K_F5             , "F5"           , NULL },
	{ K_F6             , "F6"           , NULL },
	{ K_F7             , "F7"           , NULL },
	{ K_F8             , "F8"           , NULL },
	{ K_F9             , "F9"           , NULL },
	{ K_F10            , "F10"          , NULL },
	{ K_F11            , "F11"          , NULL },
	{ K_F12            , "F12"          , NULL },
	/*
	{ K_F13            , "F13"          , NULL },
	{ K_F14            , "F14"          , NULL },
	{ K_F15            , "F15"          , NULL },
	{ K_F16            , "F16"          , NULL },
	{ K_F17            , "F17"          , NULL },
	{ K_F18            , "F18"          , NULL },
	{ K_F19            , "F19"          , NULL },
	{ K_F20            , "F20"          , NULL },
	{ K_F21            , "F21"          , NULL },
	{ K_F22            , "F22"          , NULL },
	{ K_F23            , "F23"          , NULL },
	{ K_F24            , "F24"          , NULL },*/
	K( 71               , "NumLock"      , "NLock" ),
	//K( K_SCROLL         , "Scroll"        , NULL ),

	//K( VK_PROCESSKEY     , "PROCESSKEY"   , NULL ),

/*#if defined VK_SLEEP && defined VK_OEM_NEC_EQUAL
	K( VK_SLEEP          , "Sleep"        , NULL ),

	K( VK_OEM_NEC_EQUAL  , "OEM_NEC_EQUAL"     , NULL ),

	K( VK_OEM_FJ_JISHO   , "OEM_FJ_JISHO"      , NULL ),
	K( VK_OEM_FJ_MASSHOU , "OEM_FJ_MASSHOU"    , NULL ),
	K( VK_OEM_FJ_TOUROKU , "OEM_FJ_TOUROKU"    , NULL ),
	K( VK_OEM_FJ_LOYA    , "OEM_FJ_LOYA"       , NULL ),
	K( VK_OEM_FJ_ROYA    , "OEM_FJ_ROYA"       , NULL ),

	K( VK_BROWSER_BACK        , "BROWSER_BACK"         , NULL ),
	K( VK_BROWSER_FORWARD     , "BROWSER_FORWARD"      , NULL ),
	K( VK_BROWSER_REFRESH     , "BROWSER_REFRESH"      , NULL ),
	K( VK_BROWSER_STOP        , "BROWSER_STOP"         , NULL ),
	K( VK_BROWSER_SEARCH      , "BROWSER_SEARCH"       , NULL ),
	K( VK_BROWSER_FAVORITES   , "BROWSER_FAVORITES"    , NULL ),
	K( VK_BROWSER_HOME        , "BROWSER_HOME"         , NULL ),

	K( VK_VOLUME_MUTE         , "VOLUME_MUTE"          , NULL ),
	K( VK_VOLUME_DOWN         , "VOLUME_DOWN"          , NULL ),
	K( VK_VOLUME_UP           , "VOLUME_UP"            , NULL ),
	K( VK_MEDIA_NEXT_TRACK    , "MEDIA_NEXT_TRACK"     , NULL ),
	K( VK_MEDIA_PREV_TRACK    , "MEDIA_PREV_TRACK"     , NULL ),
	K( VK_MEDIA_STOP          , "MEDIA_STOP"           , NULL ),
	K( VK_MEDIA_PLAY_PAUSE    , "MEDIA_PLAY_PAUSE"     , NULL ),
	K( VK_LAUNCH_MAIL         , "LAUNCH_MAIL"          , NULL ),
	K( VK_LAUNCH_MEDIA_SELECT , "LAUNCH_MEDIA_SELECT"  , NULL ),
	K( VK_LAUNCH_APP1         , "LAUNCH_APP1"          , NULL ),
	K( VK_LAUNCH_APP2         , "LAUNCH_APP2"          , NULL ),

	K( VK_OEM_1          , "OEM Ü"    , "Ü" ), // German hax
	K( VK_OEM_PLUS       , "OEM +"   , "+" ),
	K( VK_OEM_COMMA      , "OEM ,"   , "," ),
	K( VK_OEM_MINUS      , "OEM -"   , "-" ),
	K( VK_OEM_PERIOD     , "OEM ."   , "." ),
	K( VK_OEM_2          , "OEM 2"    , "2" ),
	K( VK_OEM_3          , "OEM Ö"    , "Ö" ), // German hax
	K( VK_OEM_4          , "OEM 4"    , "4" ),
	K( VK_OEM_5          , "OEM 5"    , "5" ),
	K( VK_OEM_6          , "OEM 6"    , "6" ),
	K( VK_OEM_7          , "OEM Ä"    , "Ä" ), // German hax
	K( VK_OEM_8          , "OEM 8"   , "8" ),
	K( VK_OEM_AX         , "AX"      , "AX" ),
	K( VK_OEM_102        , "< > |"    , "<" ), // German hax
	K( VK_ICO_HELP       , "Help"    , "Help" ),
	K( VK_ICO_00         , "ICO_00"   , "00" ),

	K( VK_ICO_CLEAR      , "ICO_CLEAR"     , NULL ),

	K( VK_PACKET         , "PACKET"        , NULL ),

	K( VK_OEM_RESET      , "OEM_RESET"     , NULL ),
	K( VK_OEM_JUMP       , "OEM_JUMP"      , NULL ),
	K( VK_OEM_PA1        , "OEM_PA1"       , NULL ),
	K( VK_OEM_PA2        , "OEM_PA2"       , NULL ),
	K( VK_OEM_PA3        , "OEM_PA3"       , NULL ),
	K( VK_OEM_WSCTRL     , "OEM_WSCTRL"    , NULL ),
	K( VK_OEM_CUSEL      , "OEM_CUSEL"     , NULL ),
	K( VK_OEM_ATTN       , "OEM_ATTN"      , NULL ),
	K( VK_OEM_FINISH     , "OEM_FINISH"    , NULL ),
	K( VK_OEM_COPY       , "OEM_COPY"      , NULL ),
	K( VK_OEM_AUTO       , "OEM_AUTO"      , NULL ),
	K( VK_OEM_ENLW       , "OEM_ENLW"      , NULL ),
	K( VK_OEM_BACKTAB    , "OEM_BACKTAB"   , NULL ),
#endif

	K( VK_ATTN           , "ATTN"          , NULL ),
	K( VK_CRSEL          , "CRSEL"         , NULL ),
	K( VK_EXSEL          , "EXSEL"         , NULL ),
	K( VK_EREOF          , "EREOF"         , NULL ),
	K( VK_PLAY           , "PLAY"          , NULL ),
	K( VK_ZOOM           , "ZOOM"          , NULL ),
	K( VK_NONAME         , "NONAME"        , NULL ),
	K( VK_PA1            , "PA1"           , NULL ),
	K( VK_OEM_CLEAR      , "OEM_CLEAR"     , NULL ),*/

	{ KEY_Any, "Any"     , NULL},
	{ KEY_Default, "None", NULL},
	{ KEY_Undefined, NULL, NULL}
};
#undef K
