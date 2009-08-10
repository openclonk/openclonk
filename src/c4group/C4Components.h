/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2003-2004, 2008  Matthes Bender
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2009  Armin Burgmeier
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

/* Known component file names */

#ifndef INC_C4Components
#define INC_C4Components

//========================= Component File Names ============================================

#define C4CFN_Engine					"Clonk.exe"
#define C4CFN_Editor    			"Editor.exe"

#define C4CFN_Material				"Material.c4g"
#define C4CFN_Sound						"Sound.c4g"
#define C4CFN_Graphics				"Graphics.c4g"
#define C4CFN_System				  "System.c4g"
#define C4CFN_Music						"Music.c4g"
#define C4CFN_Extra						"Extra.c4g"
#define C4CFN_Languages				"Language.c4g"

#define C4CFN_ScenarioSections "Sect*.c4g"

#define C4CFN_Objects					"Objects.c4d"

#define C4CFN_Mouse						"Mouse.c4f"
#define C4CFN_Keyboard				"Keyboard.c4f"
#define C4CFN_Easy						"Easy.c4f"

#define C4CFN_ScenarioCore		"Scenario.txt"
#define C4CFN_FolderCore   		"Folder.txt"
#define C4CFN_PlayerInfoCore	"Player.txt"
#define C4CFN_DefCore					"DefCore.txt"
#define C4CFN_ObjectInfoCore	"ObjectInfo.txt"
#define C4CFN_ParticleCore		"Particle.txt"
#define C4CFN_LinkCore				"Link.txt"
#define C4CFN_UpdateCore			"AutoUpdate.txt"
#define C4CFN_UpdateEntries		"GRPUP_Entries.txt"

#ifdef __x86_64
#define C4CFN_UpdateProgram		"c4group64"
#else
#define C4CFN_UpdateProgram		"c4group"
#endif

#define C4CFN_Map		      		"Map.bmp"
#define C4CFN_Landscape				"Landscape.bmp"
#define C4CFN_LandscapePNG		"Landscape.png"
#define C4CFN_DiffLandscape		"DiffLandscape.bmp"
#define C4CFN_Sky							"Sky"
#define C4CFN_Script					"Script.c|Script%s.c|C4Script%s.c"
#define C4CFN_ScriptStringTbl	"StringTbl.txt|StringTbl%s.txt"
#define C4CFN_Info						"Info.txt"
#define C4CFN_Author					"Author.txt"
#define C4CFN_Version 				"Version.txt"
#define C4CFN_Game						"Game.txt"
#define C4CFN_PXS							"PXS.c4b"
#define C4CFN_MassMover				"MassMover.c4b"
#define C4CFN_CtrlRec					"CtrlRec.c4b"
#define C4CFN_CtrlRecText			"CtrlRec.txt"
#define C4CFN_TexMap					"TexMap.txt"
#define C4CFN_MatMap					"MatMap.txt"
#define C4CFN_Title						"Title%s.txt|Title.txt"
#define C4CFN_WriteTitle			"Title.txt" // file that is generated if a title is set automatically
#define C4CFN_ScenarioTitle		"Title.bmp"
#define C4CFN_ScenarioTitlePNG "Title.png"
#define C4CFN_ScenarioIcon		"Icon.bmp"
#define C4CFN_IconPNG         "Icon.png"
#define C4CFN_ScenarioObjects	"Objects.txt"
#define C4CFN_ScenarioDesc		"Desc%s.rtf"
#define C4CFN_DefGraphics			"Graphics.bmp"
#define C4CFN_DefGraphicsPNG	"Graphics.png"
#define C4CFN_ClrByOwnerPNG		"Overlay.png"
#define C4CFN_DefGraphicsEx		"Graphics*.bmp"
#define C4CFN_DefGraphicsExPNG "Graphics*.png"
#define C4CFN_DefGraphicsScaled		 "Graphics.*.bmp"
#define C4CFN_DefGraphicsScaledPNG "Graphics.*.png"
#define C4CFN_ClrByOwnerExPNG	"Overlay*.png"
#define C4CFN_DefActMap				"ActMap.txt"
#define C4CFN_DefDesc					"Desc%s.txt"
#define C4CFN_BigIcon 				"BigIcon.png"
#define C4CFN_Portrait				"Portrait.png"
#define C4CFN_PortraitOverlay	"PortraitOverlay.png"
#define C4CFN_Portrait_Old		"Portrait.bmp"
#define C4CFN_Portraits       "Portrait*.*"
#define C4CFN_UpperBoard			"UpperBoard"
#define C4CFN_Logo						"Logo"
#define C4CFN_MoreMusic				"MoreMusic.txt"
#define C4CFN_DynLandscape		"Landscape.txt"
#define C4CFN_ClonkNames			"ClonkNames%s.txt|ClonkNames.txt"
#define C4CFN_ClonkNameFiles	"ClonkNames*.txt"
#define C4CFN_RankNames				"Rank%s.txt|Rank.txt"
#define C4CFN_RankNameFiles		"Rank*.txt"
#define C4CFN_RankFaces				"Rank.bmp"
#define C4CFN_RankFacesPNG		"Rank.png"
#define C4CFN_ClonkRank				"Rank.png" // custom rank in info file: One rank image only
#define C4CFN_Strings					"Strings.txt"
#define C4CFN_LeagueInfo      "League.txt" // read by frontend only
#define C4CFN_FontDefs        "Fonts.txt"
#define C4CFN_PlayerInfos     "PlayerInfos.txt"
#define C4CFN_SavePlayerInfos "SavePlayerInfos.txt"
#define C4CFN_RecPlayerInfos  "RecPlayerInfos.txt"
#define C4CFN_Teams           "Teams.txt"
#define C4CFN_Parameters      "Parameters.txt"
#define C4CFN_RoundResults    "RoundResults.txt"

#define C4CFN_MapFolderData   "FolderMap.txt"
#define C4CFN_MapFolderBG     "FolderMap"

#define C4CFN_Language        "Language*.txt"
#define C4CFN_KeyConfig       "KeyConfig.txt"

#define C4CFN_Log							"Clonk.log"
#define C4CFN_LogEx						"Clonk%d.log" // created if regular logfile is in use
#define C4CFN_Intro						"Clonk4.avi"
#define C4CFN_Names						"Names.txt"
#define C4CFN_Titles					"Title*.txt|Title.txt"
#define C4CFN_DefNameFiles    "Names*.txt|Names.txt"

#define C4CFN_Splash					"Splash.c4v"

#define C4CFN_TempMusic				"~Music.tmp"
#define C4CFN_TempMusic2			"~Music2.tmp"
#define C4CFN_TempSky					"~Sky.tmp"
#define C4CFN_TempMap         "~Map.tmp"
#define C4CFN_TempLandscape		"~Landscape.tmp"
#define C4CFN_TempLandscapePNG "~Landscape2.tmp"
#define C4CFN_TempPXS					"~PXS.tmp"
#define C4CFN_TempTitle				"~Title.tmp"
#define C4CFN_TempCtrlRec			"~CtrlRec.tmp"
#define C4CFN_TempReSync			"~ReSync.tmp"
#define C4CFN_TempPlayer			"~plr.tmp"
#define C4CFN_TempRoundResults "~C4Results.tmp"
#define C4CFN_TempLeagueInfo  "~league.tmp"

#define C4CFN_DefFiles				"*.c4d"
#define C4CFN_PlayerFiles			"*.c4p"
#define C4CFN_MaterialFiles		"*.c4m"
#define C4CFN_ObjectInfoFiles	"*.c4i"
#define C4CFN_MusicFiles			"*.mid"
#define C4CFN_SoundFiles			"*.wav|*.ogg"
#define C4CFN_PNGFiles				"*.png"
#define C4CFN_BitmapFiles			"*.bmp"
#define C4CFN_ScenarioFiles   "*.c4s"
#define C4CFN_FolderFiles			"*.c4f"
#define C4CFN_QueueFiles			"*.c4q"
#define C4CFN_AnimationFiles	"*.c4v"
#define C4CFN_KeyFiles				"*.c4k"
#define C4CFN_ScriptFiles			"*.c"
#define C4CFN_ImageFiles			"*.png|*.bmp|*.jpeg|*.jpg"
#define C4CFN_FontFiles       "*.fon|*.fnt|*.ttf|*.ttc|*.fot|*.otf"

//================================= File Load Sequences ================================================

// TODO: proper sorting of scaled def graphics (once we know what order we might load them in...)

#define C4FLS_Scenario	"Loader*.bmp|Loader*.png|Loader*.jpeg|Loader*.jpg|Fonts.txt|Scenario.txt|Title*.txt|Info.txt|Desc*.rtf|Icon.png|Icon.bmp|Game.txt|StringTbl*.txt|Teams.txt|Parameters.txt|Info.txt|Sect*.c4g|Music.c4g|*.mid|*.wav|Desc*.rtf|Title.bmp|Title.png|*.c4d|Material.c4g|MatMap.txt|Landscape.bmp|Landscape.png|" C4CFN_DiffLandscape "|Sky.bmp|Sky.png|Sky.jpeg|Sky.jpg|PXS.c4b|MassMover.c4b|CtrlRec.c4b|Strings.txt|Objects.txt|RoundResults.txt|Author.txt|Version.txt|Names.txt|*.c4d|Script.c|Script*.c|System.c4g"
#define C4FLS_Section   "Scenario.txt|Game.txt|Landscape.bmp|Landscape.png|Sky.bmp|Sky.png|Sky.jpeg|Sky.jpg|PXS.c4b|MassMover.c4b|CtrlRec.c4b|Strings.txt|Objects.txt"
#define C4FLS_SectionLandscape "Scenario.txt|Landscape.bmp|Landscape.png|PXS.c4b|MassMover.c4b"
#define C4FLS_SectionObjects   "Strings.txt|Objects.txt"
#define C4FLS_Def				"Particle.txt|DefCore.txt|Graphics.bmp|Graphics.png|Overlay.png|Graphics*.png|Overlay*.png|Portrait*.png|Portrait*.bmp|ActMap.txt|Script.c|Script*.c|C4Script.c|StringTbl*.txt|Names*.txt|Title*.txt|ClonkNames.txt|Rank.txt|Rank.bmp|Rank.png|Desc*.txt|Overlay.png|Title.bmp|Title.png|Icon.bmp|Author.txt|Version.txt|*.wav|*.c4d"
#define C4FLS_Player		"Player.txt|Portrait.png|Portrait.bmp|*.c4i"
#define C4FLS_Object		"ObjectInfo.txt|Portrait.png|Portrait.bmp"
#define C4FLS_Folder		"Folder.txt|Title*.txt|Info.txt|Desc*.rtf|Title.png|Title.bmp|Icon.png|Icon.bmp|Author.txt|Version.txt|*.c4s|Loader*.bmp|Loader*.png|Loader*.jpeg|Loader*.jpg|FolderMap.txt|FolderMap.png"
#define C4FLS_Material	"TexMap.txt|*.bmp|*.png|*.c4m"
#define C4FLS_Graphics	"Loader*.bmp|Loader*.png|Loader*.jpeg|Loader*.jpg|FontEndeavour12.png|FontEndeavour24.png|FontEndeavour16.png|FontEndeavour10.png|Font*.png"\
            "|*.pal|Control.png|Fire.png|Background.png|Flag.png|Crew.png|Score.png|Wealth.png|Player.png|Rank.png|Entry.png|Captain.png|Cursor.png|CursorSmall.png|CursorMedium.png|CursorLarge.png|SelectMark.png|MenuSymbol.png|Menu.png|Logo.png|Construction.png|Energy.png|Magic.png|Options.png|UpperBoard.png|Arrow.png|Exit.png|Hand.png|Gamepad.png|Build.png|EnergyBars.png|Liquid.png"\
						"|GUICaption.png|GUIButton.png|GUIButtonDown.png|GUIButtonHighlight.png|GUIIcons.png|GUIIcons2.png|GUIScroll.png|GUIContext.png|GUISubmenu.png|GUICheckBox.png|GUIBigArrows.png|GUIProgress.png"\
            "|StartupScenSelBG.*|StartupPlrSelBG.*|StartupPlrPropBG.*|StartupNetworkBG.*|StartupAboutBG.*|StartupBigButton.png|StartupBigButtonDown.png|StartupBookScroll.png|StartupContext.png|StartupScenSelIcons.png|StartupScenSelTitleOv.png|StartupPlrCtrlType.png|StartupDlgPaper.png|StartupOptionIcons.png|StartupTabClip.png|StartupNetGetRef.png"
#define C4FLS_Objects		"Names*.txt|Desc*.txt|*.c4d"
#define C4FLS_Mouse			"*.txt|*.rtf|Title.bmp|Title.png|Icon.bmp|Tutorial01.c4s|Tutorial02.c4s|Tutorial03.c4s|Objects.c4d"
#define C4FLS_Keyboard	"*.txt|*.rtf|Title.bmp|Title.png|Icon.bmp|Tutorial01.c4s|Tutorial02.c4s|Tutorial03.c4s|Tutorial04.c4s|Tutorial05.c4s|Tutorial06.c4s|Tutorial07.c4s|Tutorial08.c4s|Tutorial09.c4s|Tutorial10.c4s"
#define C4FLS_Easy			"*.txt|*.rtf|Title.bmp|Title.png|Icon.bmp|Goldmine.c4s|Monsterkill.c4s|Economy.c4s|Melee.c4s|Lake.c4s|Castle.c4s"
#define C4FLS_System		"*.hlp|*.cnt|Language*.txt|*.fon|*.fnt|*.ttf|*.ttc|*.fot|*.otf|Fonts.txt|Alchem.c|StringTbl*.txt|*.c|Names.txt"
#define C4FLS_Music 		"Frontend.*|Credits.*"
// western foldermap hardcoded...
#define C4FLS_Western   C4FLS_Folder "|ScenGCBase.png|ScenGC.png|ScenDMVBase.png|ScenDMV.png|ScenFSBase.png|ScenFS.png|ScenCTFBase.png|ScenCTF.png|ScenLHBase.png|ScenLH.png|ScenMCBase.png|ScenMC.png|ScenMWBase.png|ScenMW.png|ScenBRBase.png|ScenBR.png|ScenTHBase.png|ScenTH.png|ScenGRBase.png|ScenGR.png|ScenSTSBase.png|ScenSTS.png|ScenNWBase.png|ScenNW.png|AccLH.png|AccFS.png|AccGC.png|AccGR.png|AccMW.png|AccNW.png"

#endif
