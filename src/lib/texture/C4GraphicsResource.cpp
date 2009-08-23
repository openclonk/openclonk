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

#ifndef BIG_C4INCLUDE
#include <C4Gui.h>
#include <C4Log.h>
#include <C4Game.h>
#include <C4GraphicsSystem.h>
#include <C4Def.h>
#endif

#include <StdGL.h>

C4GraphicsResource::C4GraphicsResource()
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
	idPalGrp = 0;

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
	fctEnergyBars.Default();

	ZeroMem(GamePalette,3*256);
	ZeroMem(AlphaPalette,256);
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
	// GUI data
	C4GUI::Resource::Unload();

	sfcControl.Clear();
	idSfcControl = 0;
	idPalGrp = 0;

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
	fctArrow.Clear();
	fctExit.Clear();
	fctHand.Clear();
	fctGamepad.Clear();
	fctBuild.Clear();
	fctEnergyBars.Clear();

	// unhook deflist from font
	FontRegular.SetCustomImages(NULL);

	// closing the group set will also close the graphics.c4g
	// this is just for games that failed to init
	// normally, this is done after successful init anyway
	CloseFiles();
	}

bool C4GraphicsResource::InitFonts()
	{
	// update group set
	if (!RegisterMainGroups())
		{
		LogFatal(LoadResStr("IDS_ERR_GFX_REGISTERMAIN"));
		return false;
		}
	// reinit main font
	// this regards scenario-specific fonts or overloads in Extra.c4g
	const char *szFont;
	if (*Game.C4S.Head.Font) szFont = Game.C4S.Head.Font; else szFont = Config.General.RXFontName;
#ifndef USE_CONSOLE
	if (!Game.FontLoader.InitFont(FontRegular, szFont, C4FontLoader::C4FT_Main, Config.General.RXFontSize, &Files))
		return false;
	// assign def list as custom image source
	FontRegular.SetCustomImages(&::Definitions);
	// load additional fonts
	if (!Game.FontLoader.InitFont(FontTitle, szFont, C4FontLoader::C4FT_Title, Config.General.RXFontSize, &::GraphicsResource.Files)) return false;
	if (!Game.FontLoader.InitFont(FontCaption, szFont, C4FontLoader::C4FT_Caption, Config.General.RXFontSize, &::GraphicsResource.Files)) return false;
	if (!Game.FontLoader.InitFont(FontTiny, szFont, C4FontLoader::C4FT_Log, Config.General.RXFontSize, &::GraphicsResource.Files)) return false;
	if (!Game.FontLoader.InitFont(FontTooltip, szFont, C4FontLoader::C4FT_Main, Config.General.RXFontSize, &::GraphicsResource.Files, false)) return false;
#endif
	// done, success
	return true;
	}

bool C4GraphicsResource::Init(bool fInitGUI)
  {
	// Init fonts (double init will never if groups didnt change)
	if (!InitFonts())
		return false;
  // Game palette - could perhaps be eliminated...
	int32_t idNewPalGrp;
	C4Group *pPalGrp=Files.FindEntry("C4.pal", NULL, &idNewPalGrp);
	if (!pPalGrp) { LogF("%s: %s", LoadResStr("IDS_PRC_FILENOTFOUND"), "C4.pal"); return false; }
	if (idPalGrp != idNewPalGrp)
		{
		if (!pPalGrp->AccessEntry("C4.pal")) { LogFatal("Pal error!"); return false; }
		if (!pPalGrp->Read(GamePalette,256*3)) { LogFatal("Pal error!"); return false; }
		for (int32_t cnt=0; cnt<256*3; cnt++)	GamePalette[cnt]<<=2;
		for (int32_t cnt=0; cnt<256; cnt++)     AlphaPalette[cnt]=0xff;
		// Set default force field color
		GamePalette[191*3+0]=0;
		GamePalette[191*3+1]=0;
		GamePalette[191*3+2]=255;
		AlphaPalette[191]=127;
		// color 0 is transparent
		GamePalette[0]=GamePalette[1]=GamePalette[2]=0;
		AlphaPalette[0]=0;
		// update game pal
		if (!::GraphicsSystem.SetPalette()) { LogFatal("Pal error (2)!"); return false; }
		idPalGrp = idNewPalGrp;
		}

  // Control
	if (!LoadFile(sfcControl, "Control", Files, idSfcControl)) return false;
	fctKeyboard.Set(&sfcControl,0,0,80,36);
	fctCommand.Set(&sfcControl,0,36,32,32);
	fctKey.Set(&sfcControl,0,100,64,64);
	fctOKCancel.Set(&sfcControl,128,100,32,32);
	fctMouse.Set(&sfcControl,198,100,32,32);

  // Facet bitmap resources
	if (!LoadFile(fctFire,				"Fire",					Files, C4FCT_Height))	return false;
	if (!LoadFile(fctBackground,	"Background",		Files))								return false;
	if (!LoadFile(fctFlag,				"Flag",					Files))								return false;	// (new format)
	if (!LoadFile(fctCrew,				"Crew",					Files))								return false; // (new format)
	if (!LoadFile(fctScore,				"Score",				Files))								return false; // (new)
	if (!LoadFile(fctWealth,			"Wealth",				Files))								return false; // (new)
	if (!LoadFile(fctPlayer,			"Player",				Files))								return false; // (new format)
	if (!LoadFile(fctRank,				"Rank",					Files, C4FCT_Height))	return false;
	if (!LoadFile(fctCaptain,			"Captain",			Files))								return false;
	if (!LoadCursorGfx())                                               return false;
	if (!LoadFile(fctSelectMark,	"SelectMark",		Files, C4FCT_Height))	return false;
	if (!LoadFile(fctMenu,				"Menu",					Files, 35,35))				return false;
	if (!LoadFile(fctLogo,				"Logo",					Files))								return false;
	if (!LoadFile(fctConstruction,"Construction",	Files))								return false;	// (new)
	if (!LoadFile(fctEnergy,			"Energy",				Files))								return false;	// (new)
	if (!LoadFile(fctMagic,				"Magic",				Files))								return false;	// (new)
	if (!LoadFile(fctOptions,			"Options",			Files, C4FCT_Height))	return false;
	if (!LoadFile(fctUpperBoard,	"UpperBoard",		Files))								return false;
	if (!LoadFile(fctArrow,				"Arrow",				Files, C4FCT_Height))	return false;
	if (!LoadFile(fctExit,				"Exit",					Files))								return false;
	if (!LoadFile(fctHand,				"Hand",					Files, C4FCT_Height)) return false;
	if (!LoadFile(fctGamepad,			"Gamepad",			Files, 80)) return false;
	if (!LoadFile(fctBuild,				"Build",				Files)) return false;
	if (!LoadFile(fctEnergyBars,	"EnergyBars",		Files)) return false;
	// life bar facets
	if (fctEnergyBars.Surface)
		{
		int32_t bar_wdt = fctEnergyBars.Surface->Wdt/6;
		int32_t bar_hgt = fctEnergyBars.Surface->Hgt/3;
		if (!bar_wdt || !bar_hgt) { LogFatal("EnergyBars.png invalid or too small!"); return false; }
		fctEnergyBars.Set(fctEnergyBars.Surface, 0,0, bar_wdt, bar_hgt);
		}

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

	// load GUI files, if desired
	if (fInitGUI)
		{
		C4GUI::Resource *pRes = C4GUI::GetRes();
		if (!pRes) pRes = new C4GUI::Resource(FontCaption, FontTitle, FontRegular, FontTiny, FontTooltip);
		if (!pRes->Load(Files)) { delete pRes; return false; }
		}

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
	if (!LoadFile(fctMouseCursor,	"Cursor",				Files, C4FCT_Height, C4FCT_Full, true))
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
		if (!LoadFile(fctMouseCursor,	szCursorFilename,				Files, C4FCT_Height))
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

C4Group *FindSuitableFile(const char *szName, C4GroupSet &rGfxSet, char *szFileName, int32_t &rGroupID)
	{
	const char * const extensions[] = { "bmp", "jpeg", "jpg", "png" };

	C4Group *pGrp = 0;
	C4Group *pGrp2;
	int iPrio = -1;
	int iPrio2;
	int GroupID;
	char FileName[_MAX_FNAME];
	SCopy(szName, FileName);
	for (int i = 0; i < 4; ++i)
		{
		EnforceExtension(FileName, extensions[i]);
		pGrp2=rGfxSet.FindEntry(FileName, reinterpret_cast<int32_t *>(&iPrio2), reinterpret_cast<int32_t *>(&GroupID));
		if ((!pGrp || iPrio2 >= iPrio) && pGrp2)
			{
			rGroupID = GroupID;
			pGrp = pGrp2;
			SCopy(FileName, szFileName);
			}
		}
	// return found group, if any
	return pGrp;
	}

bool C4GraphicsResource::LoadFile(C4FacetID &fct, const char *szName, C4GroupSet &rGfxSet, int32_t iWdt, int32_t iHgt, bool fNoWarnIfNotFound)
	{
	char FileName[_MAX_FNAME]; int32_t ID;
	C4Group *pGrp = FindSuitableFile(szName, rGfxSet, FileName, ID);
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
	return true;
	}

bool C4GraphicsResource::LoadFile(C4Surface& sfc, const char *szName, C4GroupSet &rGfxSet, int32_t &ridCurrSfc)
	{
	// find
	char FileName[_MAX_FNAME]; int32_t ID;
	C4Group *pGrp = FindSuitableFile(szName, rGfxSet, FileName, ID);
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
	return true;
	}

int32_t C4GraphicsResource::GetColorIndex(int32_t iColor, bool fLast)
{
	// Returns first or last (hardcoded) index into the clonk color palette.

	// Not a valid index
	if (!Inside<int32_t>(iColor,0,C4MaxColor-1)) return 32;

	// Last index for this color
	if (fLast)
		// Colors with 8 shades
		if (iColor<10) return GetColorIndex(iColor,false)+7;
		// Colors with 4 shades
		else return GetColorIndex(iColor,false)+3;

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
	// reload any files that depend on the current resolution
	// reloads the cursor
	fctMouseCursor.idSourceGroup = 0;
	return LoadCursorGfx();
	}

C4GraphicsResource GraphicsResource;
