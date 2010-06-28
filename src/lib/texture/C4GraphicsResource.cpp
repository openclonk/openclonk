/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2001, 2003, 2006  Matthes Bender
 * Copyright (c) 2002, 2004-2009  Sven Eberhardt
 * Copyright (c) 2005  Peter Wortmann
 * Copyright (c) 2005-2007  Günther Brammer
 * Copyright (c) 2008  Armin Burgmeier
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

/* Loads all standard graphics from Graphics.c4g */

#include <C4Include.h>
#include <C4GraphicsResource.h>

#include <C4Log.h>
#include <C4Game.h>
#include <C4GraphicsSystem.h>
#include <C4Def.h>
#include <C4Fonts.h>

#include <StdGL.h>

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

	fctPlayer.Default();
	fctFlag.Default();
	fctCrew.Default();
	fctScore.Default();
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
	fctMagic.Default();
	fctArrow.Default();
	fctExit.Default();
	fctHand.Default();
	fctGamepad.Default();
	fctBuild.Default();

	fctCrewClr.Default();
	fctFlagClr.Default();
	fctPlayerClr.Default();

	fctCursor.Default();
	fctDropTarget.Default();
	fctInsideSymbol.Default();
	fctKeyboard.Default();
	fctGamepad.Default();
	fctCommand.Default();
	fctKey.Default();
	fctOKCancel.Default();
	fctMouse.Default();

	iNumRanks=1;
	idRegisteredMainGroupSetFiles=-1;
	fOldStyleCursor = false;
}

void C4GraphicsResource::Clear()
{
	fInitialized = false;

	sfcControl.Clear();
	idSfcControl = 0;

	fctCrewClr.Clear();
	fctFlagClr.Clear();
	fctPlayerClr.Clear();
	fctPlayerGray.Clear();

	fctPlayer.Clear();
	fctFlag.Clear();
	fctCrew.Clear();
	fctScore.Clear();
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
	fctMagic.Clear();
	fctOptions.Clear();
	fctArrow.Clear();
	fctExit.Clear();
	fctHand.Clear();
	fctGamepad.Clear();
	fctBuild.Clear();
	// GUI data
	sfcCaption.Clear(); sfcButton.Clear(); sfcButtonD.Clear(); sfcScroll.Clear(); sfcContext.Clear();
	idSfcCaption = idSfcButton = idSfcButtonD = idSfcScroll = idSfcContext = 0;
	barCaption.Clear(); barButton.Clear(); barButtonD.Clear();
	fctButtonHighlight.Clear(); fctIcons.Clear(); fctIconsEx.Clear();
	fctSubmenu.Clear();
	fctCheckbox.Clear();
	fctBigArrows.Clear();
	fctProgressBar.Clear();
	fctContext.Default();

	// unhook deflist from font
	FontRegular.SetCustomImages(NULL);

	// closing the group set will also close the graphics.c4g
	// this is just for games that failed to init
	// normally, this is done after successful init anyway
	CloseFiles();
}

bool C4GraphicsResource::InitFonts()
{
	// this regards scenario-specific fonts or overloads in Extra.c4g
	const char *szFont;
	if (*Game.C4S.Head.Font) szFont = Game.C4S.Head.Font; else szFont = Config.General.RXFontName;
	if (!::FontLoader.InitFont(FontRegular, szFont, C4FontLoader::C4FT_Main, Config.General.RXFontSize, &Files)) return false;
	Game.SetInitProgress(ProgressStart); ProgressStart += ProgressIncrement;
	if (!::FontLoader.InitFont(FontTiny, szFont, C4FontLoader::C4FT_Log, Config.General.RXFontSize, &Files)) return false;
	Game.SetInitProgress(ProgressStart); ProgressStart += ProgressIncrement;
	if (!::FontLoader.InitFont(FontTitle, szFont, C4FontLoader::C4FT_Title, Config.General.RXFontSize, &Files)) return false;
	Game.SetInitProgress(ProgressStart); ProgressStart += ProgressIncrement;
	if (!::FontLoader.InitFont(FontCaption, szFont, C4FontLoader::C4FT_Caption, Config.General.RXFontSize, &Files)) return false;
	Game.SetInitProgress(ProgressStart); ProgressStart += ProgressIncrement;
	if (!::FontLoader.InitFont(FontTooltip, szFont, C4FontLoader::C4FT_Main, Config.General.RXFontSize, &Files, false)) return false;
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

	Game.SetInitProgress(11.0f);
	ProgressStart = 12; ProgressIncrement = 0.4;
	// The progress bar is the only graphic besides the background that is
	// used during startup, so load it early
	if (!LoadFile(fctProgressBar, "GUIProgress", Files)) return false;
	fctProgressBar.Set(fctProgressBar.Surface, 1,0, fctProgressBar.Wdt-2, fctProgressBar.Hgt);

	if (!InitFonts()) return false;

	// load GUI files
	if (!LoadFile(sfcCaption, "GUICaption", Files, idSfcCaption)) return false;
	barCaption.SetHorizontal(sfcCaption, sfcCaption.Hgt, 32);
	if (!LoadFile(sfcButton, "GUIButton", Files, idSfcButton)) return false;
	barButton.SetHorizontal(sfcButton);
	if (!LoadFile(sfcButtonD, "GUIButtonDown", Files, idSfcButtonD)) return false;
	barButtonD.SetHorizontal(sfcButtonD);
	if (!LoadFile(fctButtonHighlight, "GUIButtonHighlight", Files)) return false;
	if (!LoadFile(fctIcons, "GUIIcons", Files)) return false;
	fctIcons.Set(fctIcons.Surface,0,0,C4GUI_IconWdt,C4GUI_IconHgt);
	if (!LoadFile(fctIconsEx, "GUIIcons2", Files)) return false;
	fctIconsEx.Set(fctIconsEx.Surface,0,0,C4GUI_IconExWdt,C4GUI_IconExHgt);
	if (!LoadFile(sfcScroll, "GUIScroll", Files, idSfcScroll)) return false;
	sfctScroll.Set(C4Facet(&sfcScroll,0,0,32,32));
	if (!LoadFile(sfcContext, "GUIContext", Files, idSfcContext)) return false;
	fctContext.Set(&sfcContext,0,0,16,16);
	if (!LoadFile(fctSubmenu, "GUISubmenu", Files)) return false;
	if (!LoadFile(fctCheckbox, "GUICheckbox", Files)) return false;
	fctCheckbox.Set(fctCheckbox.Surface, 0,0,fctCheckbox.Hgt,fctCheckbox.Hgt);
	if (!LoadFile(fctBigArrows, "GUIBigArrows", Files)) return false;
	fctBigArrows.Set(fctBigArrows.Surface, 0,0, fctBigArrows.Wdt/4, fctBigArrows.Hgt);

	// Control
	if (!LoadFile(sfcControl, "Control", Files, idSfcControl)) return false;
	fctKeyboard.Set(&sfcControl,0,0,80,36);
	fctCommand.Set(&sfcControl,0,36,32,32);
	fctKey.Set(&sfcControl,0,100,64,64);
	fctOKCancel.Set(&sfcControl,128,100,32,32);
	fctMouse.Set(&sfcControl,198,100,32,32);

	// Facet bitmap resources
	if (!LoadFile(fctFire,        "Fire",         Files, C4FCT_Height)) return false;
	if (!LoadFile(fctBackground,  "Background",   Files))               return false;
	if (!LoadFile(fctFlag,        "Flag",         Files))               return false; // (new format)
	if (!LoadFile(fctCrew,        "Crew",         Files))               return false; // (new format)
	if (!LoadFile(fctScore,       "Score",        Files))               return false; // (new)
	if (!LoadFile(fctWealth,      "Wealth",       Files))               return false; // (new)
	if (!LoadFile(fctPlayer,      "Player",       Files))               return false; // (new format)
	if (!LoadFile(fctRank,        "Rank",         Files, C4FCT_Height)) return false;
	if (!LoadFile(fctCaptain,     "Captain",      Files))               return false;
	if (!LoadCursorGfx())                                               return false;
	if (!LoadFile(fctSelectMark,  "SelectMark",   Files, C4FCT_Height)) return false;
	if (!LoadFile(fctMenu,        "Menu",         Files, 35,35))        return false;
	if (!LoadFile(fctLogo,        "Logo",         Files))               return false;
	if (!LoadFile(fctConstruction,"Construction", Files))               return false; // (new)
	if (!LoadFile(fctEnergy,      "Energy",       Files))               return false; // (new)
	if (!LoadFile(fctMagic,       "Magic",        Files))               return false; // (new)
	if (!LoadFile(fctOptions,     "Options",      Files, C4FCT_Height)) return false;
	if (!LoadFile(fctUpperBoard,  "UpperBoard",   Files))               return false;
	if (!LoadFile(fctArrow,       "Arrow",        Files, C4FCT_Height)) return false;
	if (!LoadFile(fctExit,        "Exit",         Files))               return false;
	if (!LoadFile(fctHand,        "Hand",         Files, C4FCT_Height)) return false;
	if (!LoadFile(fctGamepad,     "Gamepad",      Files, 80)) return false;
	if (!LoadFile(fctBuild,       "Build",        Files)) return false;

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
	if (fctPlayer.idSourceGroup != fctPlayerGray.idSourceGroup)
	{
		fctPlayerGray.Create(fctPlayer.Wdt, fctPlayer.Hgt);
		fctPlayer.Draw(fctPlayerGray);
		fctPlayerGray.Grayscale(30);
		fctPlayerGray.idSourceGroup = fctPlayer.idSourceGroup;
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
	// old-style cursor file overloads new-stye, because old scenarios might want to have their own cursors
	if (!LoadFile(fctMouseCursor, "Cursor",       Files, C4FCT_Height, C4FCT_Full, true))
	{
		// no old-style overload present: Determine appropriate GFX file by screen resolution
		const char *szCursorFilename;
		if (C4GUI::GetScreenWdt() >= 1280)
			szCursorFilename = "CursorLarge";
		else if (C4GUI::GetScreenWdt() >= 800)
			szCursorFilename = "CursorMedium";
		else
			szCursorFilename = "CursorSmall";
		// always fallback to regular cursor file
		if (!LoadFile(fctMouseCursor, szCursorFilename,       Files, C4FCT_Height))
			return false;
	}
	// adjust dependant faces
	int32_t iCursorSize = fctMouseCursor.Hgt;
	if (iCursorSize == 13)
	{
		fctCursor.Set(fctMouseCursor.Surface, 455, 0, 13, 13);
		fOldStyleCursor = true;
	}
	else
	{
		fctCursor.Set(fctMouseCursor.Surface, 35*iCursorSize, 0, iCursorSize, iCursorSize);
		fOldStyleCursor = false;
	}
	if (iCursorSize == 13)
	{
		fctInsideSymbol.Set(fctMouseCursor.Surface, 468, 0, 13, 13);
		fctDropTarget.Set(fctMouseCursor.Surface, 494, 0, 13, 13);
	}
	else
	{
		fctInsideSymbol.Set(fctMouseCursor.Surface, 36*iCursorSize, 0, iCursorSize, iCursorSize);
		fctDropTarget.Set(fctMouseCursor.Surface, 38*iCursorSize, 0, iCursorSize, iCursorSize);
	}
	// done
	return true;
}

bool C4GraphicsResource::RegisterGlobalGraphics()
{
	// Create main gfx group - register with fixed ID 1, to prevent unnecessary font reloading.
	// FontLoader-initializations always check whether the font has already been initialized
	// with the same parameters. If the game is simply reloaded in console-mode, this means
	// that non-bitmap-fonts are not reinitialized. This will also apply for InGame-scenario
	// switches yet to be implemented.
	// Bitmap fonts from other groups are always reloaded, because the group indices of the gfx
	// group set are not reset, and will then differ for subsequent group registrations.
	// Resetting the group index of the gfx group set at game reset would cause problems if a
	// scenario with its own font face is being closed, and then another scenario with another,
	// overloaded font face is opened. The group indices could match and the old font would
	// then be kept.
	// The cleanest alternative would be to reinit all the fonts whenever a scenario is reloaded
	C4Group *pMainGfxGrp = new C4Group();
	if (!pMainGfxGrp->Open(C4CFN_Graphics) || !Files.RegisterGroup(*pMainGfxGrp, true, C4GSPrio_Base, C4GSCnt_Graphics, 1))
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
	const char * const extensions[] = { "bmp", "jpeg", "jpg", "png", NULL };

	return rGfxSet.FindSuitableFile(szName, extensions, szFileName, pID);
}

bool C4GraphicsResource::LoadFile(C4FacetID &fct, const char *szName, C4GroupSet &rGfxSet, int32_t iWdt, int32_t iHgt, bool fNoWarnIfNotFound)
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
	if (!fct.Load(*pGrp, FileName, iWdt, iHgt))
	{
		LogF(LoadResStr("IDS_PRC_NOGFXFILE"), FileName, LoadResStr("IDS_ERR_NOFILE"));
		return false;
	}
	fct.idSourceGroup = ID;
	Game.SetInitProgress(ProgressStart);
	ProgressStart += ProgressIncrement;
	return true;
}

bool C4GraphicsResource::LoadFile(C4Surface& sfc, const char *szName, C4GroupSet &rGfxSet, int32_t &ridCurrSfc)
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
	if (!sfc.Load(*pGrp, FileName))
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

int32_t C4GraphicsResource::GetColorIndex(int32_t iColor, bool fLast)
{
	// Returns first or last (hardcoded) index into the clonk color palette.

	// Not a valid index
	if (!Inside<int32_t>(iColor,0,C4MaxColor-1)) return 32;

	// Last index for this color
	if (fLast)
	{
		// Colors with 8 shades
		if (iColor<10) return GetColorIndex(iColor,false)+7;
		// Colors with 4 shades
		else return GetColorIndex(iColor,false)+3;
	}

	// First index for this color
	switch (iColor)
	{
		// Blue, red, green, yellow, light brown, dark brown, red brown, orange
	case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
		return 32+8*iColor;
		// Black, white
	case 8: case 9:
		return 16+8*(iColor-8);
		// Cyan, purple
	case 10: case 11:
		return 96+4*(iColor-10);
	}

	// Unreachable code
	return 0;
}

bool C4GraphicsResource::ReloadResolutionDependantFiles()
{
	if(!fInitialized) return false;
	// reload any files that depend on the current resolution
	// reloads the cursor
	fctMouseCursor.idSourceGroup = 0;
	return LoadCursorGfx();
}

C4GraphicsResource GraphicsResource;
