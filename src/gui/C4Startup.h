/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2007  Sven Eberhardt
 * Copyright (c) 2008  Günther Brammer
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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
// Startup screen for non-parameterized engine start

#ifndef INC_C4Startup
#define INC_C4Startup

#include "C4Gui.h"

#define C4CFN_StartupBackgroundMain    "StartupMainMenuBG"

// special colors for startup designs
const int32_t C4StartupFontClr         = 0xff000000,
    C4StartupFontClrDisabled = 0xff7f7f7f,
                               C4StartupEditBGColor     = 0x00000000,
                                                          C4StartupEditBorderColor = 0xffa4947a,
                                                                                     C4StartupBtnFontClr      = 0xff202020,
                                                                                                                C4StartupBtnBorderColor1 = 0xffccc3b4,
                                                                                                                                           C4StartupBtnBorderColor2 = 0xff94846a;

// graphics needed only by startup
class C4StartupGraphics
{
private:
	bool LoadFile(C4FacetID &rToFct, const char *szFilename);

public:
	// backgrounds
	C4FacetID fctScenSelBG; // for scenario selection screen
	C4FacetID fctPlrSelBG;  // for player selection screen
	C4FacetID fctPlrPropBG; // for player property subpage
	C4FacetID fctNetBG;     // for network screen
	C4FacetID fctAboutBG;   // for about screen

	C4FacetID fctStartupLogo; // logo

	// big buttons used in main menu
	C4FacetID fctMainButtons,fctMainButtonsDown;
	C4GUI::DynBarFacet barMainButtons,barMainButtonsDown;

	// scroll bars in book
	C4FacetID fctBookScroll;
	C4GUI::ScrollBarFacets sfctBookScroll, sfctBookScrollR, sfctBookScrollG, sfctBookScrollB;

	// color preview
	C4FacetID fctCrew, fctCrewClr; // ColorByOwner-surface of fctCrew

	// scenario selection: Scenario and folder icons
	C4FacetID fctScenSelIcons;
	// scenario selection: Title overlay
	C4FacetID fctScenSelTitleOverlay;
	// scenario selection and player selection book fonts
	CStdFont BookFontCapt, BookFont, BookFontTitle, BookSmallFont;

	// context button for combo boxes on white
	C4FacetID fctContext;

	// options dlg gfx
	C4FacetID fctOptionsDlgPaper, fctOptionsIcons, fctOptionsTabClip;

	// net dlg gfx
	C4FacetID fctNetGetRef;

	bool Init();
	bool InitFonts();

	CStdFont &GetBlackFontByHeight(int32_t iHgt, float *pfZoom); // get optimal font for given control size
};

// base class for all startup dialogs
class C4StartupDlg : public C4GUI::FullscreenDialog
{
public:
	C4StartupDlg(const char *szTitle) : C4GUI::FullscreenDialog(szTitle, NULL) {}
};

class C4Startup
{
public:
	C4Startup();
	~C4Startup();

public:
	C4StartupGraphics Graphics;

	enum DialogID { SDID_Main=0, SDID_ScenSel, SDID_ScenSelNetwork, SDID_NetJoin, SDID_Options, SDID_About, SDID_PlrSel, SDID_Back };

private:
	bool fInStartup, fAborted, fLastDlgWasBack;
	static C4Startup *pInstance; // singleton instance
	static DialogID eLastDlgID;
	static bool fFirstRun;

	C4StartupDlg *pLastDlg, *pCurrDlg; // startup dlg that is currently shown, and dialog that was last shown

protected:
	// break modal loop and...
	void Start(); // ...start game
	void Exit();  // ...quit to system

	bool DoStartup(); // run main dlg
	class C4StartupDlg *SwitchDialog(DialogID eToDlg, bool fFade=true); // do transition to another dialog

	friend class C4StartupMainDlg;
	friend class C4StartupNetDlg;
	friend class C4StartupScenSelDlg;
	friend class C4StartupOptionsDlg;
	friend class C4StartupAboutDlg;
	friend class C4StartupPlrSelDlg;

public:
	static C4Startup *EnsureLoaded(); // create and load startup data if not done yet
	static void Unload(); // make sure startup data is destroyed
	static bool Execute(); // run startup - return false if game is to be closed
	static bool SetStartScreen(const char *szScreen); // set screen that is shown first by case insensitive identifier

	static C4Startup *Get() { assert(pInstance); return pInstance; }
	static bool WasFirstRun() { return fFirstRun; }
};

#endif // INC_C4Startup
