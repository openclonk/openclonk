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

/* Loads all standard graphics from Graphics.ocg */

#include "C4Include.h"
#include "C4ForbidLibraryCompilation.h"
#include "graphics/C4GraphicsResource.h"

#include "object/C4DefList.h"
#include "graphics/C4FontLoader.h"
#include "lib/C4Log.h"
#include "game/C4Game.h"
#include "c4group/C4Components.h"
#include "graphics/C4DrawGL.h"

/* C4GraphicsResource */

C4GraphicsResource::C4GraphicsResource():
	idSfcCaption(0), idSfcButton(0), idSfcButtonD(0), idSfcScroll(0), idSfcContext(0),
	CaptionFont(FontCaption), TitleFont(FontTitle), TextFont(FontRegular), MiniFont(FontTiny), TooltipFont(FontTooltip)
{
	Default();
}

C4GraphicsResource::~C4GraphicsResource()
{
	Clear();
}

void C4GraphicsResource::Default()
{
	fInitialized = false;

	sfcControl.Default();
	idSfcControl = 0;
	sfcClonkSkins.Default();
	idSfcClonkSkins = 0;

	fctPlayer.Default();
	fctFlag.Default();
	fctClonkSkin.Default();
	fctCrew.Default();
	fctWealth.Default();
	fctRank.Default();
	fctFire.Default();
	fctBackground.Default();
	fctCaptain.Default();
	fctMouseCursor.Default();
	fctSelectMark.Default();
	fctMenu.Default();
	fctUpperBoard.Default();
	fctLogo.Default();
	fctConstruction.Default();
	fctEnergy.Default();
	fctArrow.Default();
	fctExit.Default();
	fctHand.Default();
	fctGamepad.Default();
	fctBuild.Default();

	fctCrewClr.Default();
	fctFlagClr.Default();
	fctPlayerClr.Default();

	fctKeyboard.Default();
	fctGamepad.Default();
	fctCommand.Default();
	fctKey.Default();
	fctOKCancel.Default();
	fctMouse.Default();

	fctTransformKnob.Default();

	iNumRanks=1;
	idRegisteredMainGroupSetFiles=-1;
}

void C4GraphicsResource::Clear()
{
	fInitialized = false;

	sfcControl.Clear();
	idSfcControl = 0;
	sfcClonkSkins.Clear();
	idSfcClonkSkins = 0;

	fctCrewClr.Clear();
	fctFlagClr.Clear();
	fctPlayerClr.Clear();

	fctPlayer.Clear();
	fctFlag.Clear();
	fctCrew.Clear();
	fctWealth.Clear();
	fctRank.Clear();
	fctFire.Clear();
	fctBackground.Clear();
	fctCaptain.Clear();
	fctMouseCursor.Clear();
	fctSelectMark.Clear();
	fctMenu.Clear();
	fctUpperBoard.Clear();
	fctLogo.Clear();
	fctConstruction.Clear();
	fctEnergy.Clear();
	fctOptions.Clear();
	fctArrow.Clear();
	fctExit.Clear();
	fctHand.Clear();
	fctGamepad.Clear();
	fctBuild.Clear();
	fctTransformKnob.Clear();
	// GUI data
	sfcCaption.Clear(); sfcButton.Clear(); sfcButtonD.Clear(); sfcScroll.Clear(); sfcContext.Clear();
	idSfcCaption = idSfcButton = idSfcButtonD = idSfcScroll = idSfcContext = 0;
	barCaption.Clear(); barButton.Clear(); barButtonD.Clear();
	fctButtonHighlight.Clear(); fctIcons.Clear(); fctIconsEx.Clear();
	fctControllerIcons.Clear();
	fctButtonHighlightRound.Clear();
	fctSubmenu.Clear();
	fctCheckbox.Clear();
	fctBigArrows.Clear();
	fctProgressBar.Clear();
	fctContext.Default();


	// unhook deflist from font
	FontRegular.SetCustomImages(nullptr);

	Achievements.Clear();

	// closing the group set will also close the graphics.ocg
	// this is just for games that failed to init
	// normally, this is done after successful init anyway
	CloseFiles();
}

bool C4GraphicsResource::InitFonts()
{
	// this regards scenario-specific fonts or overloads in Extra.ocg
	const char *szFont;
	if (!Game.C4S.Head.Font.empty()) szFont = Game.C4S.Head.Font.c_str(); else szFont = Config.General.RXFontName;
	if (!::FontLoader.InitFont(&FontRegular, szFont, C4FontLoader::C4FT_Main, Config.General.RXFontSize, &Files)) return false;
	Game.SetInitProgress(ProgressStart); ProgressStart += ProgressIncrement;
	if (!::FontLoader.InitFont(&FontTiny, szFont, C4FontLoader::C4FT_Log, Config.General.RXFontSize, &Files)) return false;
	Game.SetInitProgress(ProgressStart); ProgressStart += ProgressIncrement;
	if (!::FontLoader.InitFont(&FontTitle, szFont, C4FontLoader::C4FT_Title, Config.General.RXFontSize, &Files)) return false;
	Game.SetInitProgress(ProgressStart); ProgressStart += ProgressIncrement;
	if (!::FontLoader.InitFont(&FontCaption, szFont, C4FontLoader::C4FT_Caption, Config.General.RXFontSize, &Files)) return false;
	Game.SetInitProgress(ProgressStart); ProgressStart += ProgressIncrement;
	if (!::FontLoader.InitFont(&FontTooltip, szFont, C4FontLoader::C4FT_Main, Config.General.RXFontSize, &Files, false)) return false;
	Game.SetInitProgress(ProgressStart); ProgressStart += ProgressIncrement;
	// assign def list as custom image source
	FontRegular.SetCustomImages(&::Definitions);
	return true;
}

void C4GraphicsResource::ClearFonts()
{
	FontRegular.Clear();
	FontTitle.Clear();
	FontCaption.Clear();
	FontTiny.Clear();
	FontTooltip.Clear();
}

bool C4GraphicsResource::Init()
{
	if (!RegisterGlobalGraphics())
		return false;
	// update group set
	if (!RegisterMainGroups())
	{
		LogFatal(LoadResStr("IDS_ERR_GFX_REGISTERMAIN"));
		return false;
	}

#ifndef USE_CONSOLE
	// Pre-load all shader files
	Files.PreCacheEntries(C4CFN_ShaderFiles);
	if (!pGL->InitShaders(&Files))
	{
		LogFatal(LoadResStr("IDS_ERR_GFX_INITSHADERS"));
		return false;
	}
#endif

	Game.SetInitProgress(11.0f);
	ProgressStart = 12.0f; ProgressIncrement = 0.35f; // TODO: This should be changed so that it stops at 25%, no matter how many graphics we load.
	// The progress bar is the only graphic besides the background that is
	// used during startup, so load it early
	if (!LoadFile(fctProgressBar, "GUIProgress", Files, C4FCT_Full, C4FCT_Full, false, 0)) return false;
	fctProgressBar.Set(fctProgressBar.Surface, 1,0, fctProgressBar.Wdt-2, fctProgressBar.Hgt);

	if (!InitFonts()) return false;

	// load GUI files
	if (!LoadFile(sfcCaption, "GUICaption", Files, idSfcCaption, 0)) return false;
	barCaption.SetHorizontal(sfcCaption, sfcCaption.Hgt, 32);
	if (!LoadFile(sfcButton, "GUIButton", Files, idSfcButton, 0)) return false;
	barButton.SetHorizontal(sfcButton);
	if (!LoadFile(sfcButtonD, "GUIButtonDown", Files, idSfcButtonD, 0)) return false;
	barButtonD.SetHorizontal(sfcButtonD);
	if (!LoadFile(fctButtonHighlight, "GUIButtonHighlight", Files, C4FCT_Full, C4FCT_Full, false, 0)) return false;
	if (!LoadFile(fctButtonHighlightRound, "GUIButtonHighlightRound", Files, C4FCT_Full, C4FCT_Full, false, 0)) return false;
	if (!LoadFile(fctIcons, "GUIIcons", Files, C4FCT_Full, C4FCT_Full, false, 0)) return false;
	fctIcons.Set(fctIcons.Surface,0,0,C4GUI_IconWdt,C4GUI_IconHgt);
	if (!LoadFile(fctIconsEx, "GUIIcons2", Files, C4FCT_Full, C4FCT_Full, false, 0)) return false;
	fctIconsEx.Set(fctIconsEx.Surface,0,0,C4GUI_IconExWdt,C4GUI_IconExHgt);
	if (!LoadFile(fctControllerIcons, "ControllerIcons", Files, C4FCT_Full, C4FCT_Full, false, 0)) return false;
	fctControllerIcons.Set(fctControllerIcons.Surface,0,0,C4GUI_ControllerIconWdt,C4GUI_ControllerIconHgt);
	if (!LoadFile(sfcScroll, "GUIScroll", Files, idSfcScroll, 0)) return false;
	sfctScroll.Set(C4Facet(&sfcScroll,0,0,32,32));
	if (!LoadFile(sfcContext, "GUIContext", Files, idSfcContext, 0)) return false;
	fctContext.Set(&sfcContext,0,0,16,16);
	if (!LoadFile(fctSubmenu, "GUISubmenu", Files, C4FCT_Full, C4FCT_Full, false, 0)) return false;
	if (!LoadFile(fctCheckbox, "GUICheckbox", Files, C4FCT_Full, C4FCT_Full, false, 0)) return false;
	fctCheckbox.Set(fctCheckbox.Surface, 0,0,fctCheckbox.Hgt,fctCheckbox.Hgt);
	if (!LoadFile(fctBigArrows, "GUIBigArrows", Files, C4FCT_Full, C4FCT_Full, false, 0)) return false;
	fctBigArrows.Set(fctBigArrows.Surface, 0,0, fctBigArrows.Wdt/4, fctBigArrows.Hgt);

	// Control
	if (!LoadFile(sfcControl, "Control", Files, idSfcControl, 0)) return false;
	fctKeyboard.Set(&sfcControl,0,0,80,36);
	fctCommand.Set(&sfcControl,0,36,32,32);
	fctKey.Set(&sfcControl,0,100,64,64);
	fctOKCancel.Set(&sfcControl,128,100,32,32);
	fctMouse.Set(&sfcControl,198,100,32,32);

	// Clonk style selection
	if (!LoadFile(sfcClonkSkins, "ClonkSkins",  Files, idSfcClonkSkins, 0)) return false;
	fctClonkSkin.Set(&sfcClonkSkins,0,0,64,64);

	// Facet bitmap resources
	if (!LoadFile(fctFire,        "Fire",         Files, C4FCT_Height, C4FCT_Full, false, 0))             return false;
	if (!LoadFile(fctBackground,  "Background",   Files, C4FCT_Full,   C4FCT_Full, false, C4SF_Tileable)) return false; // tileable
	if (!LoadFile(fctFlag,        "Flag",         Files, C4FCT_Full,   C4FCT_Full, false, 0))             return false; // (new format)
	if (!LoadFile(fctCrew,        "Crew",         Files, C4FCT_Full,   C4FCT_Full, false, 0))             return false; // (new format)
	if (!LoadFile(fctWealth,      "Wealth",       Files, C4FCT_Full,   C4FCT_Full, false, 0))             return false; // (new)
	if (!LoadFile(fctPlayer,      "Player",       Files, C4FCT_Full,   C4FCT_Full, false, 0))             return false; // (new format)
	if (!LoadFile(fctRank,        "Rank",         Files, C4FCT_Height, C4FCT_Full, false, 0))             return false;
	if (!LoadFile(fctCaptain,     "Captain",      Files, C4FCT_Full,   C4FCT_Full, false, 0))             return false;
	if (!LoadCursorGfx())                                                                                 return false;
	if (!LoadFile(fctSelectMark,  "SelectMark",   Files, C4FCT_Height, C4FCT_Full, false, 0))             return false;
	if (!LoadFile(fctMenu,        "Menu",         Files, 35,           35,         false, 0))             return false;
	if (!LoadFile(fctLogo,        "Logo",         Files, C4FCT_Full,   C4FCT_Full, false, 0))             return false;
	if (!LoadFile(fctConstruction,"Construction", Files, C4FCT_Full,   C4FCT_Full, false, 0))             return false; // (new)
	if (!LoadFile(fctEnergy,      "Energy",       Files, C4FCT_Full,   C4FCT_Full, false, 0))             return false; // (new)
	if (!LoadFile(fctOptions,     "Options",      Files, C4FCT_Height, C4FCT_Full, false, 0))             return false;
	if (!LoadFile(fctUpperBoard,  "UpperBoard",   Files, C4FCT_Full,   C4FCT_Full, false, C4SF_Tileable)) return false; // tileable
	if (!LoadFile(fctArrow,       "Arrow",        Files, C4FCT_Height, C4FCT_Full, false, 0))             return false;
	if (!LoadFile(fctExit,        "Exit",         Files, C4FCT_Full,   C4FCT_Full, false, 0))             return false;
	if (!LoadFile(fctHand,        "Hand",         Files, C4FCT_Height, C4FCT_Full, false, 0))             return false;
	if (!LoadFile(fctGamepad,     "Gamepad",      Files, 80,           C4FCT_Full, false, 0))             return false;
	if (!LoadFile(fctBuild,       "Build",        Files, C4FCT_Full,   C4FCT_Full, false, 0))             return false;
	if (!LoadFile(fctTransformKnob,"TransformKnob",Files,C4FCT_Full,   C4FCT_Full, false, 0))             return false;

	// achievements
	if (!Achievements.Init(Files)) return false;

	// create ColorByOwner overlay surfaces
	if (fctCrew.idSourceGroup != fctCrewClr.idSourceGroup)
	{
		if (!fctCrewClr.CreateClrByOwner(fctCrew.Surface)) { LogFatal("ClrByOwner error! (1)"); return false; }
		fctCrewClr.Wdt=fctCrew.Wdt;
		fctCrewClr.Hgt=fctCrew.Hgt;
		fctCrewClr.idSourceGroup = fctCrew.idSourceGroup;
	}
	if (fctFlag.idSourceGroup != fctFlagClr.idSourceGroup)
	{
		if (!fctFlagClr.CreateClrByOwner(fctFlag.Surface)) { LogFatal("ClrByOwner error! (1)"); return false; }
		fctFlagClr.Wdt=fctFlag.Wdt;
		fctFlagClr.Hgt=fctFlag.Hgt;
		fctFlagClr.idSourceGroup = fctFlag.idSourceGroup;
	}
	if (fctPlayer.idSourceGroup != fctPlayerClr.idSourceGroup)
	{
		if (!fctPlayerClr.CreateClrByOwner(fctPlayer.Surface)) { LogFatal("ClrByOwner error! (1)"); return false; }
		fctPlayerClr.Wdt=fctPlayer.Wdt;
		fctPlayerClr.Hgt=fctPlayer.Hgt;
		fctPlayerClr.idSourceGroup = fctPlayer.idSourceGroup;
	}

	// get number of ranks
	int32_t Q; fctRank.GetPhaseNum(iNumRanks, Q);
	if (!iNumRanks) iNumRanks=1;

	// CloseFiles() must not be called now:
	// The sky still needs to be loaded from the global graphics
	// group in C4Game::InitGame -> C4Sky::Init so we need to keep the group(s) open.
	// In activated NETWORK2, the files mustn't be closed either, because there will be
	// multiple calls to this function to allow overloadings

	// mark initialized
	fInitialized = true;

	return true;
}

bool C4GraphicsResource::LoadCursorGfx()
{
	// Determine appropriate GFX file by screen resolution
	const char *szCursorFilename;
	szCursorFilename = "Cursor";
	if (!LoadFile(fctMouseCursor, szCursorFilename, Files, C4FCT_Height, C4FCT_Full, false, 0))
		return false;
	return true;
}

bool C4GraphicsResource::RegisterGlobalGraphics()
{
	// Create main gfx group - register with fixed ID 1, to prevent unnecessary font reloading.
	// FontLoader-initializations always check whether the font has already been initialized
	// with the same parameters. If the game is simply reloaded in console-mode, this means
	// that fonts are not reinitialized. This will also apply for InGame-scenario switches yet
	// to be implemented.
	// Bitmap fonts from other groups are always reloaded, because the group indices of the gfx
	// group set are not reset, and will then differ for subsequent group registrations.
	// Resetting the group index of the gfx group set at game reset would cause problems if a
	// scenario with its own font face is being closed, and then another scenario with another,
	// overloaded font face is opened. The group indices could match and the old font would
	// then be kept.
	// The cleanest alternative would be to reinit all the fonts whenever a scenario is reloaded
	// FIXME: Test whether vector fonts from a scenario are correctly reloaded
	C4Group *pMainGfxGrp = new C4Group();
	if (!Reloc.Open(*pMainGfxGrp, C4CFN_Graphics) || !Files.RegisterGroup(*pMainGfxGrp, true, C4GSPrio_Base, C4GSCnt_Graphics, 1))
	{
		// error
		LogFatal(FormatString(LoadResStr("IDS_PRC_NOGFXFILE"),C4CFN_Graphics,pMainGfxGrp->GetError()).getData());
		delete pMainGfxGrp;
		return false;
	}
	return true;
}

bool C4GraphicsResource::RegisterMainGroups()
{
	// register main groups
	Files.RegisterGroups(Game.GroupSet, C4GSCnt_Graphics, C4CFN_Graphics, idRegisteredMainGroupSetFiles);
	idRegisteredMainGroupSetFiles = Game.GroupSet.GetLastID();
	return true;
}

void C4GraphicsResource::CloseFiles()
{
	// closes main gfx group; releases dependencies into game group set
	Files.Clear();
	idRegisteredMainGroupSetFiles=-1;
}

static C4Group *FindSuitableFile(const char *szName, C4GroupSet &rGfxSet, char *szFileName, int32_t * pID)
{
	const char * const extensions[] = { "bmp", "jpeg", "jpg", "png", nullptr };

	return rGfxSet.FindSuitableFile(szName, extensions, szFileName, pID);
}

bool C4GraphicsResource::LoadFile(C4FacetID &fct, const char *szName, C4GroupSet &rGfxSet, int32_t iWdt, int32_t iHgt, bool fNoWarnIfNotFound, int iFlags)
{
	char FileName[_MAX_FNAME]; int32_t ID = 0;
	C4Group *pGrp = FindSuitableFile(szName, rGfxSet, FileName, &ID);
	if (!pGrp)
	{
		// FIXME: Use LogFatal here
		if (!fNoWarnIfNotFound)
		{
			LogF(LoadResStr("IDS_PRC_NOGFXFILE"), szName, LoadResStr("IDS_PRC_FILENOTFOUND"));
		}
		return false;
	}
	// check group
	if (fct.idSourceGroup == ID)
		// already up-to-date
		return true;
	// load
	if (!fct.Load(*pGrp, FileName, iWdt, iHgt, false, iFlags))
	{
		LogF(LoadResStr("IDS_PRC_NOGFXFILE"), FileName, LoadResStr("IDS_ERR_NOFILE"));
		return false;
	}
	fct.idSourceGroup = ID;
	Game.SetInitProgress(ProgressStart);
	ProgressStart += ProgressIncrement;
	return true;
}

bool C4GraphicsResource::LoadFile(C4Surface& sfc, const char *szName, C4GroupSet &rGfxSet, int32_t &ridCurrSfc, int iFlags)
{
	// find
	char FileName[_MAX_FNAME]; int32_t ID = 0;
	C4Group *pGrp = FindSuitableFile(szName, rGfxSet, FileName, &ID);
	if (!pGrp)
	{
		LogF(LoadResStr("IDS_PRC_NOGFXFILE"), szName, LoadResStr("IDS_PRC_FILENOTFOUND"));
		return false;
	}
	// check group
	if (ID == ridCurrSfc)
		// already up-to-date
		return true;
	// load
	if (!sfc.Load(*pGrp, FileName, false, false, iFlags))
	{
		LogF(LoadResStr("IDS_PRC_NOGFXFILE"), FileName, LoadResStr("IDS_ERR_NOFILE"));
		return false;
	}
	ridCurrSfc = ID;
	Game.SetInitProgress(ProgressStart);
	ProgressStart += ProgressIncrement;
	return true;
}

CStdFont &C4GraphicsResource::GetFontByHeight(int32_t iHgt, float *pfZoom)
{
	// get optimal font for given control size
	CStdFont *pUseFont;
	if (iHgt <= MiniFont.GetLineHeight()) pUseFont = &MiniFont;
	else if (iHgt <= TextFont.GetLineHeight()) pUseFont = &TextFont;
	else if (iHgt <= CaptionFont.GetLineHeight()) pUseFont = &CaptionFont;
	else pUseFont = &TitleFont;
	// determine zoom
	if (pfZoom)
	{
		int32_t iLineHgt = pUseFont->GetLineHeight();
		if (iLineHgt)
			*pfZoom = (float) iHgt / (float) iLineHgt;
		else
			*pfZoom = 1.0f; // error
	}
	return *pUseFont;
}

bool C4GraphicsResource::ReloadResolutionDependantFiles()
{
	if(!fInitialized) return false;
	// reload any files that depend on the current resolution
	// reloads the cursor

	// Re-open the graphics files if they are not open anymore -- this
	// happens when the game is running.
	// Note also that at the moment there are no resolution dependent
	// graphics files...
	const bool hadGroupsRegistered = (idRegisteredMainGroupSetFiles != -1);
	if(!hadGroupsRegistered)
	{
		RegisterGlobalGraphics();
		RegisterMainGroups();
	}

	fctMouseCursor.idSourceGroup = 0;
	const bool result = true;
	
	if(!hadGroupsRegistered)
		CloseFiles();

	return result;
}

C4GraphicsResource GraphicsResource;
