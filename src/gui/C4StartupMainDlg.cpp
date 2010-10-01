/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2007  Sven Eberhardt
 * Copyright (c) 2005-2008  Matthes Bender
 * Copyright (c) 2006  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
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
// Startup screen for non-parameterized engine start (stub)

#include <C4Include.h>
#include <C4StartupMainDlg.h>
#include <C4UpdateDlg.h>
#include <C4Version.h>

#include <C4StartupNetDlg.h>
#include <C4StartupScenSelDlg.h>
#include <C4StartupOptionsDlg.h>
#include <C4StartupAboutDlg.h>
#include <C4StartupPlrSelDlg.h>
#include <C4Startup.h>
#include <C4Game.h>
#include <C4Log.h>
#include <C4Language.h>
#include <C4GraphicsResource.h>


C4StartupMainDlg::C4StartupMainDlg() : C4StartupDlg(NULL) // create w/o title; it is drawn in custom draw proc
{
	// ctor
	fFirstShown = true;
	// screen calculations
	int iButtonPadding = 2;
	int iButtonHeight = C4GUI_BigButtonHgt;
	C4GUI::ComponentAligner caMain(rcBounds, 0,0,true);
	C4GUI::ComponentAligner caRightPanel(caMain.GetFromLeft(rcBounds.Wdt*2/5), rcBounds.Wdt/26, 40+rcBounds.Hgt/5);
	//C4GUI::ComponentAligner caButtons(caRightPanel.GetCentered(caRightPanel.GetWidth(), (iButtonHeight+iButtonPadding) * iButtonCount - iButtonPadding), 0, iButtonPadding);
	C4GUI::ComponentAligner caButtons(caRightPanel.GetAll(), 0, iButtonPadding);
	// main menu buttons
	C4GUI::CallbackButton<C4StartupMainDlg> *btn;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_BTN_LOCALGAME"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnStartBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_STARTGAME"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	pStartButton = btn;
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_BTN_NETWORKGAME"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnNetJoinBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_NETWORKGAME"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_DLG_PLAYERSELECTION"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnPlayerSelectionBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_PLAYERSELECTION"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_DLG_OPTIONS"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnOptionsBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_OPTIONS"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_DLG_ABOUT"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnAboutBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_ABOUT"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	AddElement(btn = new C4GUI::CallbackButton<C4StartupMainDlg>(LoadResStr("IDS_DLG_EXIT"), caButtons.GetFromTop(iButtonHeight), &C4StartupMainDlg::OnExitBtn));
	btn->SetToolTip(LoadResStr("IDS_DLGTIP_EXIT"));
	btn->SetCustomGraphics(&C4Startup::Get()->Graphics.barMainButtons, &C4Startup::Get()->Graphics.barMainButtonsDown);
	// list of selected players
	AddElement(pParticipantsLbl = new C4GUI::Label("test", GetClientRect().Wdt*39/40, GetClientRect().Hgt*9/10, ARight, 0xffffffff, &::GraphicsResource.TitleFont, false));
	pParticipantsLbl->SetToolTip(LoadResStr("IDS_DLGTIP_SELECTEDPLAYERS"));
	// player selection shortcut - to be made optional
	UpdateParticipants();
	pParticipantsLbl->SetContextHandler(new C4GUI::CBContextHandler<C4StartupMainDlg>(this, &C4StartupMainDlg::OnPlayerSelContext));
	/*new C4GUI::ContextButton(pParticipantsLbl, true, 0,0);*/
	// key bindings
	C4CustomKey::CodeList keys;
	keys.push_back(C4KeyCodeEx(K_DOWN)); keys.push_back(C4KeyCodeEx(K_RIGHT));
	if (Config.Controls.GamepadGuiControl)
	{
		keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Down))); // right will be done by Dialog already
	}
	pKeyDown = new C4KeyBinding(keys, "StartupMainCtrlNext", KEYSCOPE_Gui,
	                            new C4GUI::DlgKeyCBEx<C4StartupMainDlg, bool>(*this, false, &C4StartupMainDlg::KeyAdvanceFocus), C4CustomKey::PRIO_CtrlOverride);
	keys.clear(); keys.push_back(C4KeyCodeEx(K_UP)); keys.push_back(C4KeyCodeEx(K_LEFT));
	if (Config.Controls.GamepadGuiControl)
	{
		keys.push_back(C4KeyCodeEx(KEY_Gamepad(0, KEY_JOY_Up))); // left will be done by Dialog already
	}
	pKeyUp = new C4KeyBinding(keys, "StartupMainCtrlPrev", KEYSCOPE_Gui,
	                          new C4GUI::DlgKeyCBEx<C4StartupMainDlg, bool>(*this, true, &C4StartupMainDlg::KeyAdvanceFocus), C4CustomKey::PRIO_CtrlOverride);
	keys.clear(); keys.push_back(C4KeyCodeEx(K_RETURN));
	pKeyEnter = new C4KeyBinding(keys, "StartupMainOK", KEYSCOPE_Gui,
	                             new C4GUI::DlgKeyCB<C4StartupMainDlg>(*this, &C4StartupMainDlg::KeyEnterDown, &C4StartupMainDlg::KeyEnterUp), C4CustomKey::PRIO_CtrlOverride);
}

C4StartupMainDlg::~C4StartupMainDlg()
{
	// dtor
	delete pKeyEnter;
	delete pKeyUp;
	delete pKeyDown;
}

void C4StartupMainDlg::DrawElement(C4TargetFacet &cgo)
{
	// inherited
	typedef C4GUI::FullscreenDialog Base;
	Base::DrawElement(cgo);
	// draw logo
	C4FacetID * fctStartupLogo = &C4Startup::Get()->Graphics.fctStartupLogo;
	float fLogoZoom = 1.0f;
	fctStartupLogo->DrawX(cgo.Surface, rcBounds.Wdt *1/21, rcBounds.Hgt/28, int32_t(fLogoZoom*fctStartupLogo->Wdt), int32_t(fLogoZoom*fctStartupLogo->Hgt));
	// draw version info
	StdStrBuf sVer;
	sVer.Format(LoadResStr("IDS_DLG_VERSION"), C4VERSION);
	lpDDraw->TextOut(sVer.getData(), ::GraphicsResource.TextFont, 1.0f, cgo.Surface, rcBounds.Wdt*1/40, rcBounds.Hgt*38/40, 0xffffffff, ALeft, true);
}

C4GUI::ContextMenu *C4StartupMainDlg::OnPlayerSelContext(C4GUI::Element *pBtn, int32_t iX, int32_t iY)
{
	// preliminary player selection via simple context menu
	C4GUI::ContextMenu *pCtx = new C4GUI::ContextMenu();
	pCtx->AddItem("Add", "Add participant", C4GUI::Ico_None, NULL, new C4GUI::CBContextHandler<C4StartupMainDlg>(this, &C4StartupMainDlg::OnPlayerSelContextAdd));
	pCtx->AddItem("Remove", "Remove participant", C4GUI::Ico_None, NULL, new C4GUI::CBContextHandler<C4StartupMainDlg>(this, &C4StartupMainDlg::OnPlayerSelContextRemove));
	return pCtx;
}

C4GUI::ContextMenu *C4StartupMainDlg::OnPlayerSelContextAdd(C4GUI::Element *pBtn, int32_t iX, int32_t iY)
{
	C4GUI::ContextMenu *pCtx = new C4GUI::ContextMenu();
	const char *szFn;
	StdStrBuf sSearchPath(Config.General.UserDataPath);
//  sSearchPath.Format("%s%s", (const char *) Config.General.ExePath, (const char *) Config.General.PlayerPath);
	for (DirectoryIterator i(sSearchPath.getData()); (szFn=*i); i++)
	{
		szFn = Config.AtRelativePath(szFn);
		if (*GetFilename(szFn) == '.') continue;
		if (!WildcardMatch(C4CFN_PlayerFiles, GetFilename(szFn))) continue;
		if (!SIsModule(Config.General.Participants, szFn, NULL, false))
			pCtx->AddItem(C4Language::IconvClonk(GetFilenameOnly(szFn)).getData(), "Let this player join in next game", C4GUI::Ico_Player,
			              new C4GUI::CBMenuHandlerEx<C4StartupMainDlg, StdCopyStrBuf>(this, &C4StartupMainDlg::OnPlayerSelContextAddPlr, StdCopyStrBuf(szFn)), NULL);
	}
	return pCtx;
}

C4GUI::ContextMenu *C4StartupMainDlg::OnPlayerSelContextRemove(C4GUI::Element *pBtn, int32_t iX, int32_t iY)
{
	C4GUI::ContextMenu *pCtx = new C4GUI::ContextMenu();
	char szPlayer[1024+1];
	for (int i = 0; SCopySegment(Config.General.Participants, i, szPlayer, ';', 1024, true); i++)
		if (*szPlayer)
			pCtx->AddItem(GetFilenameOnly(szPlayer), "Remove this player from participation list", C4GUI::Ico_Player, new C4GUI::CBMenuHandlerEx<C4StartupMainDlg, int>(this, &C4StartupMainDlg::OnPlayerSelContextRemovePlr, i), NULL);
	return pCtx;
}

void C4StartupMainDlg::OnPlayerSelContextAddPlr(C4GUI::Element *pTarget, const StdCopyStrBuf &rsFilename)
{
	SAddModule(Config.General.Participants, rsFilename.getData());
	UpdateParticipants();
}

void C4StartupMainDlg::OnPlayerSelContextRemovePlr(C4GUI::Element *pTarget, const int &iIndex)
{
	char szPlayer[1024+1];
	if (SCopySegment(Config.General.Participants, iIndex, szPlayer, ';', 1024, true))
		SRemoveModule(Config.General.Participants, szPlayer, false);
	UpdateParticipants();
}

void C4StartupMainDlg::UpdateParticipants()
{
	// First validate all participants (files must exist)
	std::string strPlayers(Config.General.Participants);
	std::vector<char> strPlayer(1025);
	*Config.General.Participants=0;
	for (int i = 0; SCopySegment(strPlayers.c_str(), i, &strPlayer[0], ';', strPlayer.size() - 1, true); i++)
	{
		const char *szPlayer = &strPlayer[0];
		std::string strPlayerFile(Config.General.UserDataPath);
		strPlayerFile.append(szPlayer);
		if (!szPlayer || !*szPlayer) continue;
		if (!FileExists(strPlayerFile.c_str())) continue;
		if (!SEqualNoCase(GetExtension(szPlayer), "c4p")) continue; // additional sanity check to clear strange exe-path-only entries in player list?
		SAddModule(Config.General.Participants, szPlayer);
	}
	// Draw selected players - we are currently displaying the players stored in Config.General.Participants.
	// Existence of the player files is not validated and player filenames are displayed directly
	// (names are not loaded from the player core).
	strPlayers = LoadResStr("IDS_DESC_PLRS");
	if (!Config.General.Participants[0])
		strPlayers.append(LoadResStr("IDS_DLG_NOPLAYERSSELECTED"));
	else
		for (int i = 0; SCopySegment(Config.General.Participants, i, &strPlayer[0], ';', 1024, true); i++)
		{
			if (i > 0) strPlayers.append(", ");
			strPlayers.append(C4Language::IconvClonk(GetFilenameOnly(&strPlayer[0])).getData());
		}
	pParticipantsLbl->SetText(strPlayers.c_str());
}

void C4StartupMainDlg::OnClosed(bool fOK)
{
	// if dlg got aborted (by user), quit startup
	// if it got closed with OK, the user pressed one of the buttons and dialog switching has been done already
	if (!fOK) C4Startup::Get()->Exit();
}

void C4StartupMainDlg::OnStartBtn(C4GUI::Control *btn)
{
	// no regular game start if no players were selected
	/*if (!*Config.General.Participants)
	  {
	  StdStrBuf buf(LoadResStrNoAmp("IDS_DLG_STARTGAME"), true);
	  GetScreen()->ShowMessageModal(LoadResStr("IDS_MSG_NOPLAYERSELECTED"), buf.getData(), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Notify);
	  // let's go to the player selection dlg then instead
	  OnPlayerSelectionBtn(NULL);
	  return;
	  }*/
	// advance to scenario selection screen
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_ScenSel);
}

void C4StartupMainDlg::OnPlayerSelectionBtn(C4GUI::Control *btn)
{
	// advance to player selection screen
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_PlrSel);
}

void C4StartupMainDlg::OnNetJoinBtn(C4GUI::Control *btn)
{
	// simple net join dlg
	//GetScreen()->ShowDialog(new C4GUI::InputDialog("Enter host IP", "Direct join", C4GUI::Ico_Host, new C4GUI::InputCallback<C4StartupMainDlg>(this, &C4StartupMainDlg::OnNetJoin)), false);
	// advanced net join and host dlg!
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_NetJoin);
}

void C4StartupMainDlg::OnNetJoin(const StdStrBuf &rsHostAddress)
{
	// no IP given: No join
	if (!rsHostAddress || !*rsHostAddress.getData()) return;
	// set default startup parameters
	*Game.ScenarioFilename=0;
	SCopy("Objects.c4d", Game.DefinitionFilenames);
	Game.NetworkActive = true;
	Game.fLobby = true;
	Game.fObserve = false;
	SCopy(rsHostAddress.getData(), Game.DirectJoinAddress, sizeof(Game.DirectJoinAddress)-1);
	// start with this set!
	C4Startup::Get()->Start();
}

void C4StartupMainDlg::OnOptionsBtn(C4GUI::Control *btn)
{
	// advance to options screen
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_Options);
}

void C4StartupMainDlg::OnAboutBtn(C4GUI::Control *btn)
{
	// advance to about screen
	C4Startup::Get()->SwitchDialog(C4Startup::SDID_About);
}

void C4StartupMainDlg::OnExitBtn(C4GUI::Control *btn)
{
	// callback: exit button pressed
	C4Startup::Get()->Exit();
}

void C4StartupMainDlg::OnTODO(C4GUI::Control *btn)
{
	GetScreen()->ShowMessage("not yet implemented", "2do", C4GUI::Ico_Error);
}

bool C4StartupMainDlg::KeyEnterDown()
{
	// just execute selected button: Re-Send as space
	return Game.DoKeyboardInput(K_SPACE, KEYEV_Down, false, false, false, false, this);
}

bool C4StartupMainDlg::KeyEnterUp()
{
	// just execute selected button: Re-Send as space
	return Game.DoKeyboardInput(K_SPACE, KEYEV_Up, false, false, false, false, this);
}

void C4StartupMainDlg::OnShown()
{

	// Incoming update
	if (Application.IncomingUpdate)
	{
		C4UpdateDlg::ApplyUpdate(Application.IncomingUpdate.getData(), false, GetScreen());
		Application.IncomingUpdate.Clear();
	}
	// Manual update by command line or url
	if (Application.CheckForUpdates)
	{
		C4UpdateDlg::CheckForUpdates(GetScreen(), false);
		Application.CheckForUpdates = false;
	}
	// Automatic update
	else
	{
		if (Config.Network.AutomaticUpdate)
			C4UpdateDlg::CheckForUpdates(GetScreen(), true);
	}

	// first start evaluation
	if (Config.General.FirstStart)
	{
		Config.General.FirstStart = false;
	}
	// first thing that's needed is a new player, if there's none - independent of first start
	bool fHasPlayer = false;
	StdStrBuf sSearchPath(Config.General.UserDataPath);
	const char *szFn;
//  sSearchPath.Format("%s%s", (const char *) Config.General.ExePath, (const char *) Config.General.PlayerPath);
	for (DirectoryIterator i(sSearchPath.getData()); (szFn=*i); i++)
	{
		szFn = Config.AtRelativePath(szFn);
		if (*GetFilename(szFn) == '.') continue; // ignore ".", ".." and private files (".*")
		if (!WildcardMatch(C4CFN_PlayerFiles, GetFilename(szFn))) continue;
		fHasPlayer = true;
		break;
	}
	if (!fHasPlayer)
	{
		// no player created yet: Create one
		C4GUI::Dialog *pDlg;
		GetScreen()->ShowModalDlg(pDlg=new C4StartupPlrPropertiesDlg(NULL, NULL), true);
	}
	// make sure participants are updated after switching back from player selection
	UpdateParticipants();

	// First show
	if (fFirstShown)
	{
		// Activate the application (trying to prevent flickering half-focus in win32...)
		Application.Activate();
		// Set the focus to the start button (we might still not have the focus after the update-check sometimes... :/)
		SetFocus(pStartButton, false);
	}
	fFirstShown = false;
}
