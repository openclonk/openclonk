/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2010-2016, The OpenClonk Team and contributors
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
	{ K_BACK            , "Backspace"     , nullptr },
	{ K_TAB             , "Tab"           , nullptr },
	//{ VK_CLEAR          , "Clear"         , nullptr },
	{ K_RETURN          , "Return"        , nullptr },

	{ K_SHIFT_L        , "LeftShift"     , "LShift" },
	{ K_SHIFT_R        , "RightShift"    , "RShift" },
	{ K_CONTROL_L      , "LeftControl"   , "LCtrl" },
	{ K_CONTROL_R      , "RightControl"  , "RCtrl" },
	{ K_ALT_L          , "LeftAlt"       , "LAlt" },
	{ K_ALT_R          , "RightAlt"      , "RAlt" },
	{ K_PAUSE          , "Pause"         , nullptr },

/*	K( VK_CAPITAL        , "Capital"       , nullptr ),
	K( VK_KANA           , "Kana"          , nullptr ),
	K( VK_HANGEUL        , "Hangeul"       , nullptr ),
	K( VK_HANGUL         , "Hangul"        , nullptr ),
	K( VK_JUNJA          , "Junja"         , nullptr ),
	K( VK_FINAL          , "Final"         , nullptr ),
	K( VK_HANJA          , "Hanja"         , nullptr ),
	K( VK_KANJI          , "Kanji"         , nullptr ),
	K( VK_ESCAPE         , "Escape"        , "Esc" ),
	K( VK_ESCAPE         , "Esc"           ,nullptr ),
	K( VK_CONVERT        , "Convert"       , nullptr ),
	K( VK_NONCONVERT     , "Noconvert"     , nullptr ),
	K( VK_ACCEPT         , "Accept"        , nullptr ),
	K( VK_MODECHANGE     , "Modechange"    , nullptr ),*/

	{ K_SPACE           , "Space"         , "Sp" },

	{ K_END             , "End"           , nullptr },
	{ K_HOME            , "Home"          , nullptr },
	{ K_LEFT            , "Left"          , nullptr },
	{ K_UP              , "Up"            , nullptr },
	{ K_RIGHT           , "Right"         , nullptr },
	{ K_DOWN            , "Down"          , nullptr },
	/*{ VK_SELECT         , "Select"        , nullptr },
	{ VK_PRINT          , "Print"         , nullptr },
	{ VK_EXECUTE        , "Execute"       , nullptr },
	{ VK_SNAPSHOT       , "Snapshot"      , nullptr },
	{ VK_INSERT         , "Insert"        , "Ins" },*/
	{ K_DELETE          , "Delete"        , "Del" },
//	{ VK_HELP           , "Help"          , nullptr },
	
	K( 29                , "0"         , nullptr ),
	K( 18                , "1"         , nullptr ),
	K( 19                , "2"         , nullptr ),
	K( 20                , "3"         , nullptr ),
	K( 21                , "4"         , nullptr ),
	K( 23                , "5"         , nullptr ),
	K( 22                , "6"         , nullptr ),
	K( 26                , "7"         , nullptr ),
	K( 28                , "8"         , nullptr ),
	K( 25                , "9"         , nullptr ),

	K( 0                 , "A"         , nullptr ),
	K( 11                , "B"         , nullptr ),
	K( 8                 , "C"         , nullptr ),
	K( 2                 , "D"         , nullptr ),
	K( 14                , "E"         , nullptr ),
	K( 3                 , "F"         , nullptr ),
	K( 5                 , "G"         , nullptr ),
	K( 4                 , "H"         , nullptr ),
	K( 34                , "I"         , nullptr ),
	K( 38                , "J"         , nullptr ),
	K( 40                , "K"         , nullptr ),
	K( 37                , "L"         , nullptr ),
	K( 46                , "M"         , nullptr ),
	K( 45                , "N"         , nullptr ),
	K( 31                , "O"         , nullptr ),
	K( 35                , "P"         , nullptr ),
	K( 12                , "Q"         , nullptr ),
	K( 15                , "R"         , nullptr ),
	K( 1                 , "S"         , nullptr ),
	K( 17                , "T"         , nullptr ),
	K( 32                , "U"         , nullptr ),
	K( 9                 , "V"         , nullptr ),
	K( 13                , "W"         , nullptr ),
	K( 7                 , "X"         , nullptr ),
	K( 6                 , "Y"         , nullptr ),
	K( 16                , "Z"         , nullptr ),
	K( 43                , "Comma"     , nullptr ),
	K( 47                , "Period"    , nullptr ),
	K( 43                , "Apostrophe", nullptr ),
	K( 44                , "Backslash" , nullptr ),
	K( 43                , "Comma_US"  , nullptr ),
	K( 49                , "Less"      , nullptr ),

	/*K( VK_LWIN           , "WinLeft"      , nullptr ),
	K( VK_RWIN           , "WinRight"     , nullptr ),*/
	//K( VK_APPS           , "Apps"         , nullptr ),

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
	{ K_F1             , "F1"           , nullptr },
	{ K_F2             , "F2"           , nullptr },
	{ K_F3             , "F3"           , nullptr },
	{ K_F4             , "F4"           , nullptr },
	{ K_F5             , "F5"           , nullptr },
	{ K_F6             , "F6"           , nullptr },
	{ K_F7             , "F7"           , nullptr },
	{ K_F8             , "F8"           , nullptr },
	{ K_F9             , "F9"           , nullptr },
	{ K_F10            , "F10"          , nullptr },
	{ K_F11            , "F11"          , nullptr },
	{ K_F12            , "F12"          , nullptr },
	/*
	{ K_F13            , "F13"          , nullptr },
	{ K_F14            , "F14"          , nullptr },
	{ K_F15            , "F15"          , nullptr },
	{ K_F16            , "F16"          , nullptr },
	{ K_F17            , "F17"          , nullptr },
	{ K_F18            , "F18"          , nullptr },
	{ K_F19            , "F19"          , nullptr },
	{ K_F20            , "F20"          , nullptr },
	{ K_F21            , "F21"          , nullptr },
	{ K_F22            , "F22"          , nullptr },
	{ K_F23            , "F23"          , nullptr },
	{ K_F24            , "F24"          , nullptr },*/
	K( 71               , "NumLock"      , "NLock" ),
	//K( K_SCROLL         , "Scroll"        , nullptr ),

	//K( VK_PROCESSKEY     , "PROCESSKEY"   , nullptr ),

/*#if defined VK_SLEEP && defined VK_OEM_NEC_EQUAL
	K( VK_SLEEP          , "Sleep"        , nullptr ),

	K( VK_OEM_NEC_EQUAL  , "OEM_NEC_EQUAL"     , nullptr ),

	K( VK_OEM_FJ_JISHO   , "OEM_FJ_JISHO"      , nullptr ),
	K( VK_OEM_FJ_MASSHOU , "OEM_FJ_MASSHOU"    , nullptr ),
	K( VK_OEM_FJ_TOUROKU , "OEM_FJ_TOUROKU"    , nullptr ),
	K( VK_OEM_FJ_LOYA    , "OEM_FJ_LOYA"       , nullptr ),
	K( VK_OEM_FJ_ROYA    , "OEM_FJ_ROYA"       , nullptr ),

	K( VK_BROWSER_BACK        , "BROWSER_BACK"         , nullptr ),
	K( VK_BROWSER_FORWARD     , "BROWSER_FORWARD"      , nullptr ),
	K( VK_BROWSER_REFRESH     , "BROWSER_REFRESH"      , nullptr ),
	K( VK_BROWSER_STOP        , "BROWSER_STOP"         , nullptr ),
	K( VK_BROWSER_SEARCH      , "BROWSER_SEARCH"       , nullptr ),
	K( VK_BROWSER_FAVORITES   , "BROWSER_FAVORITES"    , nullptr ),
	K( VK_BROWSER_HOME        , "BROWSER_HOME"         , nullptr ),

	K( VK_VOLUME_MUTE         , "VOLUME_MUTE"          , nullptr ),
	K( VK_VOLUME_DOWN         , "VOLUME_DOWN"          , nullptr ),
	K( VK_VOLUME_UP           , "VOLUME_UP"            , nullptr ),
	K( VK_MEDIA_NEXT_TRACK    , "MEDIA_NEXT_TRACK"     , nullptr ),
	K( VK_MEDIA_PREV_TRACK    , "MEDIA_PREV_TRACK"     , nullptr ),
	K( VK_MEDIA_STOP          , "MEDIA_STOP"           , nullptr ),
	K( VK_MEDIA_PLAY_PAUSE    , "MEDIA_PLAY_PAUSE"     , nullptr ),
	K( VK_LAUNCH_MAIL         , "LAUNCH_MAIL"          , nullptr ),
	K( VK_LAUNCH_MEDIA_SELECT , "LAUNCH_MEDIA_SELECT"  , nullptr ),
	K( VK_LAUNCH_APP1         , "LAUNCH_APP1"          , nullptr ),
	K( VK_LAUNCH_APP2         , "LAUNCH_APP2"          , nullptr ),

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

	K( VK_ICO_CLEAR      , "ICO_CLEAR"     , nullptr ),

	K( VK_PACKET         , "PACKET"        , nullptr ),

	K( VK_OEM_RESET      , "OEM_RESET"     , nullptr ),
	K( VK_OEM_JUMP       , "OEM_JUMP"      , nullptr ),
	K( VK_OEM_PA1        , "OEM_PA1"       , nullptr ),
	K( VK_OEM_PA2        , "OEM_PA2"       , nullptr ),
	K( VK_OEM_PA3        , "OEM_PA3"       , nullptr ),
	K( VK_OEM_WSCTRL     , "OEM_WSCTRL"    , nullptr ),
	K( VK_OEM_CUSEL      , "OEM_CUSEL"     , nullptr ),
	K( VK_OEM_ATTN       , "OEM_ATTN"      , nullptr ),
	K( VK_OEM_FINISH     , "OEM_FINISH"    , nullptr ),
	K( VK_OEM_COPY       , "OEM_COPY"      , nullptr ),
	K( VK_OEM_AUTO       , "OEM_AUTO"      , nullptr ),
	K( VK_OEM_ENLW       , "OEM_ENLW"      , nullptr ),
	K( VK_OEM_BACKTAB    , "OEM_BACKTAB"   , nullptr ),
#endif

	K( VK_ATTN           , "ATTN"          , nullptr ),
	K( VK_CRSEL          , "CRSEL"         , nullptr ),
	K( VK_EXSEL          , "EXSEL"         , nullptr ),
	K( VK_EREOF          , "EREOF"         , nullptr ),
	K( VK_PLAY           , "PLAY"          , nullptr ),
	K( VK_ZOOM           , "ZOOM"          , nullptr ),
	K( VK_NONAME         , "NONAME"        , nullptr ),
	K( VK_PA1            , "PA1"           , nullptr ),
	K( VK_OEM_CLEAR      , "OEM_CLEAR"     , nullptr ),*/

	{ KEY_Any, "Any"     , nullptr},
	{ KEY_Default, "None", nullptr},
	{ KEY_Undefined, nullptr, nullptr}
};
#undef K
