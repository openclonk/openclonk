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

/* Known component file names */

#ifndef INC_C4Components
#define INC_C4Components

//========================= Component File Names ============================================

#define C4CFN_Material        "Material.ocg"
#define C4CFN_Sound           "Sound.ocg"
#define C4CFN_SoundSubgroups  "*.ocg"
#define C4CFN_Graphics        "Graphics.ocg"
#define C4CFN_System          "System.ocg"
#define C4CFN_Music           "Music.ocg"
#define C4CFN_Extra           "Extra.ocg"
#define C4CFN_Languages       "Language.ocg"
#define C4CFN_Template        "Template.ocg"

#define C4CFN_Savegames       "Savegames.ocf"
#define C4CFN_Records         "Records.ocf"

#define C4CFN_ScenarioSections "Sect*.ocg"

#define C4CFN_Objects         "Objects.ocd"

#define C4CFN_ScenarioCore    "Scenario.txt"
#define C4CFN_ScenarioParameterDefs "ParameterDefs.txt"
#define C4CFN_FolderCore      "Folder.txt"
#define C4CFN_PlayerInfoCore  "Player.txt"
#define C4CFN_DefCore         "DefCore.txt"
#define C4CFN_ObjectInfoCore  "ObjectInfo.txt"
#define C4CFN_ParticleCore    "Particle.txt"
#define C4CFN_LinkCore        "Link.txt"
#define C4CFN_UpdateCore      "AutoUpdate.txt"
#define C4CFN_UpdateEntries   "GRPUP_Entries.txt"

#define C4CFN_UpdateGroupExtension ".ocu"
#define C4CFN_UpdateProgram   "c4group"
#define C4CFN_UpdateProgramLibs "*.dll"

#define C4CFN_Map             "Map.bmp"
#define C4CFN_MapFg           "MapFg.bmp"
#define C4CFN_MapBg           "MapBg.bmp"
#define C4CFN_Landscape       "Landscape.bmp"
#define C4CFN_LandscapeFg     "LandscapeFg.bmp"
#define C4CFN_LandscapeBg     "LandscapeBg.bmp"
#define C4CFN_DiffLandscape   "DiffLandscape.bmp"
#define C4CFN_DiffLandscapeBkg "DiffLandscapeBkg.bmp"
#define C4CFN_Sky             "Sky"
#define C4CFN_Script          "Script.c|Script%s.c|C4Script%s.c"
#define C4CFN_MapScript       "Map.c"
#define C4CFN_ScriptStringTbl "StringTbl.txt|StringTbl%s.txt"
#define C4CFN_AnyScriptStringTbl "StringTbl*.txt"
#define C4CFN_Info            "Info.txt"
#define C4CFN_Author          "Author.txt"
#define C4CFN_Version         "Version.txt"
#define C4CFN_Game            "Game.txt"
#define C4CFN_ScenarioObjectsScript "Objects.c"
#define C4CFN_PXS             "PXS.ocb"
#define C4CFN_MassMover       "MassMover.ocb"
#define C4CFN_CtrlRec         "CtrlRec.ocb"
#define C4CFN_CtrlRecText     "CtrlRec.txt"
#define C4CFN_LogRec          "Record.log"
#define C4CFN_TexMap          "TexMap.txt"
#define C4CFN_MatMap          "MatMap.txt"
#define C4CFN_Title           "Title%s.txt|Title.txt"
#define C4CFN_WriteTitle      "Title.txt" // file that is generated if a title is set automatically
#define C4CFN_ScenarioTitle   "Title"
#define C4CFN_ScenarioIcon    "Icon.bmp"
#define C4CFN_IconPNG         "Icon.png"
#define C4CFN_ScenarioObjects "Objects.txt"
#define C4CFN_ScenarioDesc    "Desc%s.txt"
#define C4CFN_DefMaterials    "*.material"
#define C4CFN_Achievements    "Achv*.png"

#define C4CFN_DefMesh              "Graphics.mesh"
#define C4CFN_DefMeshXml           C4CFN_DefMesh ".xml"
#define C4CFN_DefSkeleton          "*.skeleton"
#define C4CFN_DefSkeletonXml       C4CFN_DefSkeleton ".xml"
#define C4CFN_DefGraphicsExMesh    "Graphics*.mesh"
#define C4CFN_DefGraphicsExMeshXml C4CFN_DefGraphicsExMesh ".xml"

#define C4CFN_DefGraphics          "Graphics.png"
#define C4CFN_ClrByOwner           "Overlay.png"
#define C4CFN_NormalMap            "Normal.png"
#define C4CFN_DefGraphicsEx        "Graphics*.png"
#define C4CFN_ClrByOwnerEx         "Overlay*.png"
#define C4CFN_NormalMapEx          "Normal*.png"

#define C4CFN_DefGraphicsScaled    "Graphics.*.png"
#define C4CFN_ClrByOwnerScaled     "Graphics.*.png"
#define C4CFN_NormalMapScaled      "Normal.*.png"

#define C4CFN_DefDesc         "Desc%s.txt"
#define C4CFN_BigIcon         "BigIcon.png"
#define C4CFN_UpperBoard      "UpperBoard"
#define C4CFN_Logo            "Logo"
#define C4CFN_MoreMusic       "MoreMusic.txt"
#define C4CFN_DynLandscape    "Landscape.txt"
#define C4CFN_ClonkNames      "ClonkNames%s.txt|ClonkNames.txt"
#define C4CFN_ClonkNameFiles  "ClonkNames*.txt"
#define C4CFN_RankNames       "Rank%s.txt|Rank.txt"
#define C4CFN_RankNameFiles   "Rank*.txt"
#define C4CFN_RankFacesPNG    "Rank.png"
#define C4CFN_ClonkRank       "Rank.png" // custom rank in info file: One rank image only
#define C4CFN_SolidMask       "SolidMask.png"
#define C4CFN_LeagueInfo      "League.txt" // read by frontend only
#define C4CFN_PlayerInfos     "PlayerInfos.txt"
#define C4CFN_SavePlayerInfos "SavePlayerInfos.txt"
#define C4CFN_RecPlayerInfos  "RecPlayerInfos.txt"
#define C4CFN_Teams           "Teams.txt"
#define C4CFN_Parameters      "Parameters.txt"
#define C4CFN_RoundResults    "RoundResults.txt"
#define C4CFN_PlayerControls  "PlayerControls.txt"
#define C4CFN_LandscapeShader "LandscapeShader.c"
#define C4CFN_LandscapeScaler "Scaler.png"
#define C4CFN_MaterialShapeFiles "_Shape.png"

#define C4CFN_MapFolderData   "FolderMap.txt"
#define C4CFN_MapFolderBG     "FolderMap"

#define C4CFN_Language        "Language*.txt"
#define C4CFN_KeyConfig       "KeyConfig.txt"

#define C4CFN_Log             "OpenClonk.log"
#define C4CFN_LogEx           "OpenClonk%d.log" // created if regular logfile is in use
#define C4CFN_LogShader       "OpenClonkShaders.log" // created in editor mode to dump shader code
#define C4CFN_Intro           "Clonk4.avi"
#define C4CFN_Names           "Names.txt"
#define C4CFN_Titles          "Title*.txt|Title.txt"
#define C4CFN_DefNameFiles    "Names*.txt|Names.txt"
#define C4CFN_EditorGeometry  "Editor.geometry"
#define C4CFN_DefaultScenarioTemplate "Empty.ocs"

#define C4CFN_TempMusic       "~Music.tmp"
#define C4CFN_TempMusic2      "~Music2.tmp"
#define C4CFN_TempSky         "~Sky.tmp"
#define C4CFN_TempMapFg       "~MapFg.tmp"
#define C4CFN_TempMapBg       "~MapBg.tmp"
#define C4CFN_TempLandscape   "~Landscape.tmp"
#define C4CFN_TempLandscapeBkg "~LandscapeBkg.tmp"
#define C4CFN_TempPXS         "~PXS.tmp"
#define C4CFN_TempTitle       "~Title.tmp"
#define C4CFN_TempCtrlRec     "~CtrlRec.tmp"
#define C4CFN_TempReSync      "~ReSync.tmp"
#define C4CFN_TempPlayer      "~plr.tmp"
#define C4CFN_TempRoundResults "~C4Results.tmp"
#define C4CFN_TempLeagueInfo  "~league.tmp"

#define C4CFN_DefFiles        "*.ocd"
#define C4CFN_PlayerFiles     "*.ocp"
#define C4CFN_MaterialFiles   "*.ocm"
#define C4CFN_ObjectInfoFiles "*.oci"
#define C4CFN_MusicFiles      "*.ogg"
#define C4CFN_SoundFiles      "*.wav|*.ogg"
#define C4CFN_PNGFiles        "*.png"
#define C4CFN_BitmapFiles     "*.bmp"
#define C4CFN_ScenarioFiles   "*.ocs"
#define C4CFN_FolderFiles     "*.ocf"
#define C4CFN_QueueFiles      "*.c4q"
#define C4CFN_AnimationFiles  "*.ocv"
#define C4CFN_KeyFiles        "*.c4k"
#define C4CFN_ScriptFiles     "*.c"
#define C4CFN_ImageFiles      "*.png|*.bmp|*.jpeg|*.jpg"
#define C4CFN_FontFiles       "*.fon|*.fnt|*.ttf|*.ttc|*.fot|*.otf"
#define C4CFN_ShaderFiles     "*.glsl"

//================================= File Load Sequences ================================================

// TODO: proper sorting of scaled def graphics (once we know what order we might load them in...)

#define C4FLS_Scenario  "Loader*.bmp|Loader*.png|Loader*.jpeg|Loader*.jpg|Fonts.txt|Scenario.txt|Title*.txt|Info.txt|Desc*.txt|Icon.png|Icon.bmp|Achv*.png|Game.txt|StringTbl*.txt|ParameterDefs.txt|Teams.txt|Parameters.txt|Info.txt|Sect*.ocg|Music.ocg|*.mid|*.wav|Desc*.txt|Title.png|Title.jpg|*.ocd|Script.c|Script*.c|Map.c|Objects.c|System.ocg|Material.ocg|MatMap.txt|Map.bmp|MapFg.bmp|MapBg.bmp|Landscape.bmp|LandscapeFg.bmp|LandscapeBg.bmp|" C4CFN_DiffLandscape "|" C4CFN_DiffLandscapeBkg "|Sky.bmp|Sky.png|Sky.jpeg|Sky.jpg|PXS.ocb|MassMover.ocb|CtrlRec.ocb|Strings.txt|Objects.txt|RoundResults.txt|Author.txt|Version.txt|Names.txt"
#define C4FLS_Section   "Scenario.txt|Game.txt|Map.bmp|MapFg.bmp|MapBg.bmp|Landscape.bmp|LandscapeFg.bmp|LandscapeBg.bmp|Sky.bmp|Sky.png|Sky.jpeg|Sky.jpg|PXS.ocb|MassMover.ocb|CtrlRec.ocb|Strings.txt|Objects.txt|Objects.c"
#define C4FLS_SectionLandscape "Scenario.txt|Map.bmp|MapFg.bmp|MapBg.bmp|Landscape.bmp|LandscapeFg.bmp|LandscapeBg.bmp|PXS.ocb|MassMover.ocb"
#define C4FLS_SectionObjects   "Strings.txt|Objects.txt|Objects.c"
#define C4FLS_Def       "*.glsl|*.png|*.bmp|*.jpeg|*.jpg|*.material|Particle.txt|DefCore.txt|*.wav|*.ogg|*.skeleton|Graphics.mesh|*.mesh|StringTbl*.txt|Script.c|Script*.c|C4Script.c|Names*.txt|Title*.txt|ClonkNames.txt|Rank.txt|Rank*.txt|Desc*.txt|Author.txt|Version.txt|*.ocd"
#define C4FLS_Player    "Player.txt|BigIcon.png|*.oci"
#define C4FLS_Object    "ObjectInfo.txt"
#define C4FLS_Folder    "Folder.txt|Title*.txt|Info.txt|Desc*.txt|Title.png|Title.jpg|Icon.png|Icon.bmp|Author.txt|Version.txt|StringTbl*.txt|ParameterDefs.txt|Achv*.png|*.ocs|Loader*.bmp|Loader*.png|Loader*.jpeg|Loader*.jpg|FolderMap.txt|FolderMap.png"
#define C4FLS_Material  "TexMap.txt|*.ocm|*.jpeg|*.jpg|*.bmp|*.png"
#define C4FLS_Graphics  "Loader*.bmp|Loader*.png|Loader*.jpeg|Loader*.jpg|*.glsl|Font*.png"\
            "|GUIProgress.png|Endeavour.ttf|GUICaption.png|GUIButton.png|GUIButtonDown.png|GUIButtonHighlight.png|GUIButtonHighlightRound.png|GUIIcons.png|GUIIcons2.png|GUIScroll.png|GUIContext.png|GUISubmenu.png|GUICheckBox.png|GUIBigArrows.png"\
            "|Control.png|ClonkSkins.png|Fire.png|Background.png|Flag.png|Crew.png|Wealth.png|Player.png|Rank.png|Captain.png|Cursor.png|SelectMark.png|MenuSymbol.png|Menu.png|Logo.png|Construction.png|Energy.png|Options.png|UpperBoard.png|Arrow.png|Exit.png|Hand.png|Gamepad.png|Build.png|TransformKnob.png|Achv*.png"\
            "|StartupMainMenuBG.*|StartupScenSelBG.*|StartupPlrSelBG.*|StartupPlrPropBG.*|StartupNetworkBG.*|StartupAboutBG.*|StartupBigButton.png|StartupBigButtonDown.png|StartupBookScroll.png|StartupContext.png|StartupScenSelIcons.png|StartupScenSelTitleOv.png|StartupDlgPaper.png|StartupOptionIcons.png|StartupTabClip.png|StartupNetGetRef.png|StartupLogo.png"
#define C4FLS_Objects   "Names*.txt|Desc*.txt|*.ocd"
#define C4FLS_System    "*.hlp|*.cnt|Language*.txt|*.fon|*.fnt|*.ttf|*.ttc|*.fot|*.otf|Fonts.txt|StringTbl*.txt|PlayerControls.txt|*.c|Names.txt"
#define C4FLS_Sound     C4CFN_SoundFiles "|" C4CFN_SoundSubgroups
#define C4FLS_Music     C4CFN_MusicFiles

#endif
