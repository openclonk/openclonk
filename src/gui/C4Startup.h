/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// Startup screen for non-parameterized engine start

#ifndef INC_C4Startup
#define INC_C4Startup

#include "gui/C4Gui.h"

#define C4CFN_StartupBackgroundMain    "StartupMainMenuBG"

// special colors for startup designs
const int32_t
	C4StartupFontClr         = 0xff000000,
    C4StartupFontClrDisabled = 0xff7f7f7f,
	C4StartupEditBGColor     = 0x00000000,
	C4StartupEditBorderColor = 0xffa4947a,
	C4StartupBtnFontClr      = 0xff202020,
	C4StartupBtnBorderColor1 = 0xffccc3b4,
	C4StartupBtnBorderColor2 = 0xff94846a;

// Titles in StartupAboutTitles
enum
{
    C4StartupAboutEngineAndTools,
    C4StartupAboutScriptingAndContent,
    C4StartupAboutAdministration,
    C4StartupAboutArtAndContent,
    C4StartupAboutMusicAndSound,
    C4StartupAboutContributors,
    C4StartupAboutTitleCount
};

// graphics needed only by startup
class C4StartupGraphics
{
private:
	bool LoadFile(C4FacetID &rToFct, const char *szFilename);

public:
	// backgrounds
	C4FacetID fctPlrPropBG;   // for player property subpage
	C4FacetID fctAboutTitles; // for about screen
	C4FacetID fctDlgPaper;

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
	C4FacetID fctOptionsIcons, fctOptionsTabClip;

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
	C4StartupDlg(const char *szTitle) : C4GUI::FullscreenDialog(szTitle, nullptr) {}

	virtual bool SetSubscreen(const char *szToScreen) { return false; } // go to specified subdialog, e.g. a specific property sheet in the options dlg
	virtual void OnKeyboardLayoutChanged() {}
	virtual void OnLeagueOptionChanged() {}
};

class C4Startup
{
public:
	C4Startup();
	~C4Startup();

public:
	C4StartupGraphics Graphics;

	enum DialogID { SDID_Main=0, SDID_ScenSel, SDID_ScenSelNetwork, SDID_NetJoin, SDID_Options, SDID_About, SDID_Legal, SDID_PlrSel, SDID_Mods, SDID_Back };

private:
	bool fInStartup{false}, fLastDlgWasBack;
	static C4Startup *pInstance; // singleton instance
	static DialogID eLastDlgID;
	static StdCopyStrBuf sSubDialog; // subdialog to go into (e.g.: property sheet in options dialog)

	C4StartupDlg *pLastDlg{nullptr}, *pCurrDlg{nullptr}; // startup dlg that is currently shown, and dialog that was last shown

protected:
	void DoStartup(); // create main dlg
	void DontStartup(); // close main dlg
	class C4StartupDlg *SwitchDialog(DialogID eToDlg, bool fFade=true, const char *szSubDialog=nullptr); // do transition to another dialog

	friend class C4StartupMainDlg;
	friend class C4StartupNetDlg;
	friend class C4StartupScenSelDlg;
	friend class C4StartupOptionsDlg;
	friend class C4StartupModsDlg;
	friend class C4StartupAboutDlg;
	friend class C4StartupLegalDlg;
	friend class C4StartupPlrSelDlg;

public:
	static C4Startup *EnsureLoaded(); // create and load startup data if not done yet
	static void Unload(); // make sure startup data is destroyed
	static void InitStartup();
	static void CloseStartup();
	static bool SetStartScreen(const char *szScreen, const char *szSubDialog=nullptr); // set screen that is shown first by case insensitive identifier
	void OnKeyboardLayoutChanged();
	void OnLeagueOptionChanged(); // callback from network options dialogue: Updates settings in scenario selction

	static C4Startup *Get() { assert(pInstance); return pInstance; }

};

#endif // INC_C4Startup
