/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2003-2008  Sven Eberhardt
 * Copyright (c) 2005-2006, 2009-2010  Günther Brammer
 * Copyright (c) 2005, 2009  Peter Wortmann
 * Copyright (c) 2006  Florian Groß
 * Copyright (c) 2007-2008  Matthes Bender
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2010  Carl-Philip Hänsch
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
// the ingame-lobby

#include <C4Include.h>
#include <C4GameLobby.h>

#include "C4FullScreen.h"
#include "C4Network2Dialogs.h"
#include "C4GameOptions.h"
#include "C4GameDialogs.h"
#include "C4RTF.h"
#include "C4ChatDlg.h"
#include "C4PlayerInfoListBox.h"
#include <C4MessageInput.h>
#include <C4Game.h>
#include <C4Network2.h>
#include "C4GraphicsResource.h"

namespace C4GameLobby
{

	bool UserAbort = false;

// ----------- C4PacketCountdown ---------------------------------------------

	void C4PacketCountdown::CompileFunc(StdCompiler *pComp)
	{
		pComp->Value(mkNamingAdapt(iCountdown, "Countdown", 0));
	}

	StdStrBuf C4PacketCountdown::GetCountdownMsg(bool fInitialMsg) const
	{
		const char *szCountdownMsg;
		if (iCountdown < AlmostStartCountdownTime && !fInitialMsg) szCountdownMsg = "%d..."; else szCountdownMsg = LoadResStr("IDS_PRC_COUNTDOWN");
		return FormatString(szCountdownMsg, (int)iCountdown);
	}


// ----------- ScenDescs ---------------------------------------------

	ScenDesc::ScenDesc(const C4Rect &rcBounds, bool fActive) : C4GUI::Window(), fDescFinished(false)
	{
		// build components
		//CStdFont &rTitleFont = ::GraphicsResource.CaptionFont;
		SetBounds(rcBounds);
		C4GUI::ComponentAligner caMain(GetClientRect(), 0,0, true);
		//AddElement(pTitle = new C4GUI::Label("", caMain.GetFromTop(rTitleFont.GetLineHeight()), ALeft, C4GUI_CaptionFontClr, &rTitleFont, true));
		AddElement(pDescBox = new C4GUI::TextWindow(caMain.GetAll(), 0, 0, 0, 100, 4096, "", true));
		pDescBox->SetDecoration(false, false, NULL, true);
		//pDescBox->SetToolTip(LoadResStr("IDS_MSG_SCENARIODESC")); annoying when scrolling through desc...
		// initial update to set current data
		if (fActive) Activate();
	}

	void ScenDesc::Update()
	{
		// scenario present?
		C4Network2Res *pRes = Game.Parameters.Scenario.getNetRes();
		if (!pRes) return; // something's wrong
		CStdFont &rTitleFont = ::GraphicsResource.CaptionFont;
		CStdFont &rTextFont = ::GraphicsResource.TextFont;
		pDescBox->ClearText(false);
		if (pRes->isComplete())
		{
			C4Group ScenarioFile;
			if (!ScenarioFile.Open(pRes->getFile()))
			{
				pDescBox->AddTextLine("scenario file load error", &rTextFont, C4GUI_MessageFontClr, false, true);
			}
			else
			{
				StdStrBuf sDesc;
				// load desc
				C4ComponentHost DefDesc;
				if (DefDesc.LoadEx("Desc", ScenarioFile, C4CFN_ScenarioDesc, Config.General.LanguageEx))
				{
					C4RTFFile rtf;
					rtf.Load(StdBuf(DefDesc.GetData(), SLen(DefDesc.GetData())));
					sDesc.Take(rtf.GetPlainText());
				}
				DefDesc.Close();
				if (!!sDesc)
					pDescBox->AddTextLine(sDesc.getData(), &rTextFont, C4GUI_MessageFontClr, false, true, &rTitleFont);
				else
					pDescBox->AddTextLine(Game.ScenarioTitle.getData(), &rTitleFont, C4GUI_CaptionFontClr, false, true);
			}
			// okay, done loading. No more updates.
			fDescFinished = true;
			Deactivate();
		}
		else
		{
			pDescBox->AddTextLine(FormatString(LoadResStr("IDS_MSG_SCENARIODESC_LOADING"), (int) pRes->getPresentPercent()).getData(),
			                      &rTextFont, C4GUI_MessageFontClr, false, true);
		}
		pDescBox->UpdateHeight();
	}

	void ScenDesc::Activate()
	{
		// final desc set? no update then
		if (fDescFinished) return;
		// register timer if necessary
		Application.Add(this);
		// force an update
		Update();
	}

	void ScenDesc::Deactivate()
	{
		// release timer if set
		Application.Remove(this);
	}

// ----------- MainDlg -----------------------------------------------------------------------------

	MainDlg::MainDlg(bool fHost)
			: C4GUI::FullscreenDialog(!Game.ScenarioTitle ?
			                          (const char *) LoadResStr("IDS_DLG_LOBBY"):
			                          FormatString("%s - %s", Game.ScenarioTitle.getData(), LoadResStr("IDS_DLG_LOBBY")).getData(),
			                          Game.ScenarioTitle.getData()),
			pPlayerList(NULL), pResList(NULL), pChatBox(NULL), pRightTabLbl(NULL), pRightTab(NULL),
			pEdt(NULL), btnRun(NULL), btnPlayers(NULL), btnResources(NULL), btnTeams(NULL), btnChat(NULL)
	{
		// key bindings
		pKeyHistoryUp  = new C4KeyBinding(C4KeyCodeEx(K_UP  ), "LobbyChatHistoryUp"  , KEYSCOPE_Gui, new C4GUI::DlgKeyCBEx<MainDlg, bool>(*this, true , &MainDlg::KeyHistoryUpDown), C4CustomKey::PRIO_CtrlOverride);
		pKeyHistoryDown= new C4KeyBinding(C4KeyCodeEx(K_DOWN), "LobbyChatHistoryDown", KEYSCOPE_Gui, new C4GUI::DlgKeyCBEx<MainDlg, bool>(*this, false, &MainDlg::KeyHistoryUpDown), C4CustomKey::PRIO_CtrlOverride);
		// timer
		Application.Add(this);
		// indents / sizes
		int32_t iDefBtnHeight = 32;
		int32_t iIndentX1, iIndentX2, iIndentX3/*, iIndentX4*/;
		int32_t iIndentY1, iIndentY2, iIndentY3, iIndentY4, iButtonAreaHgt;
		int32_t iClientListWdt;
		if (GetClientRect().Wdt > 500)
		{
			// normal dlg
			iIndentX1 = 10;   // lower button area
			iIndentX2 = 20;   // center area (chat)
			iIndentX3 = 5;    // client/player list
			iClientListWdt = GetClientRect().Wdt / 3;
		}
		else
		{
			// small dlg
			iIndentX1 = 2;   // lower button area
			iIndentX2 = 2;   // center area (chat)
			iIndentX3 = 1;   // client/player list
			iClientListWdt = GetClientRect().Wdt / 2;
		}
		if (GetClientRect().Hgt > 320)
		{
			// normal dlg
			iIndentY1 = 16;    // lower button area
			iIndentY2 = 20;    // status bar offset
			iIndentY3 = 8;     // center area (chat)
			iIndentY4 = 8;     // client/player list
			iButtonAreaHgt = C4GUI_IconExHgt;
		}
		else
		{
			// small dlg
			iIndentY1 = 2;     // lower button area
			iIndentY2 = 2;     // status bar offset
			iIndentY3 = 1;     // center area (chat)
			iIndentY4 = 1;     // client/player list
			iButtonAreaHgt = iDefBtnHeight;
		}
		// set subtitle ToolTip
		if (pSubTitle)
			pSubTitle->SetToolTip(LoadResStr("IDS_DLG_SCENARIOTITLE"));
		C4GUI::Label *pLbl;
		// main screen components
		C4GUI::ComponentAligner caMain(GetClientRect(), 0,0,true);
		caMain.GetFromBottom(iIndentY2);
		// lower button-area
		C4GUI::ComponentAligner caBottom(caMain.GetFromBottom(iDefBtnHeight+iIndentY1*2), iIndentX1,iIndentY1);
		// add buttons
		C4GUI::CallbackButton<MainDlg> *btnExit;
		btnExit = new C4GUI::CallbackButton<MainDlg>(LoadResStr("IDS_DLG_EXIT"), caBottom.GetFromLeft(100), &MainDlg::OnExitBtn);
		if (fHost)
			btnRun = new C4GUI::CallbackButton<MainDlg>(LoadResStr("IDS_DLG_GAMEGO"), caBottom.GetFromRight(100), &MainDlg::OnRunBtn);
		else
			// 2do: Ready-checkbox
			caBottom.GetFromRight(90);
		//C4GUI::CallbackButton<MainDlg> *btnTest = new C4GUI::CallbackButton<MainDlg>("test", caBottom.GetFromRight(90), &MainDlg::OnTestBtn);
		pGameOptionButtons = new C4GameOptionButtons(caBottom.GetCentered(caBottom.GetInnerWidth(), Min<int32_t>(C4GUI_IconExHgt, caBottom.GetHeight())), true, fHost, true);

		// players / ressources sidebar
		C4GUI::ComponentAligner caRight(caMain.GetFromRight(iClientListWdt), iIndentX3,iIndentY4);
		pRightTabLbl = new C4GUI::WoodenLabel("", caRight.GetFromTop(C4GUI::WoodenLabel::GetDefaultHeight(&(::GraphicsResource.TextFont))), C4GUI_CaptionFontClr, &::GraphicsResource.TextFont, ALeft);
		caRight.ExpandTop(iIndentY4*2 + 1); // undo margin, so client list is located directly under label
		pRightTab = new C4GUI::Tabular(caRight.GetAll(), C4GUI::Tabular::tbNone);
		C4GUI::Tabular::Sheet *pPlayerSheet = pRightTab->AddSheet(LoadResStr("IDS_DLG_PLAYERS"));
		C4GUI::Tabular::Sheet *pResSheet = pRightTab->AddSheet(LoadResStr("IDS_DLG_RESOURCES"));
		C4GUI::Tabular::Sheet *pOptionsSheet = pRightTab->AddSheet(LoadResStr("IDS_DLG_OPTIONS"));
		C4GUI::Tabular::Sheet *pScenarioSheet = pRightTab->AddSheet(LoadResStr("IDS_DLG_SCENARIO"));
		pPlayerList = new C4PlayerInfoListBox(pPlayerSheet->GetContainedClientRect(), C4PlayerInfoListBox::PILBM_LobbyClientSort);
		pPlayerSheet->AddElement(pPlayerList);
		pResList = new C4Network2ResDlg(pResSheet->GetContainedClientRect(), false);
		pResSheet->AddElement(pResList);
		pOptionsList = new C4GameOptionsList(pResSheet->GetContainedClientRect(), false, false);
		pOptionsSheet->AddElement(pOptionsList);
		pScenarioInfo = new ScenDesc(pResSheet->GetContainedClientRect(), false);
		pScenarioSheet->AddElement(pScenarioInfo);
		pRightTabLbl->SetContextHandler(new C4GUI::CBContextHandler<C4GameLobby::MainDlg>(this, &MainDlg::OnRightTabContext));
		pRightTabLbl->SetClickFocusControl(pPlayerList);

		bool fHasTeams = Game.Teams.IsMultiTeams();
		bool fHasChat = C4ChatDlg::IsChatActive();
		int32_t iBtnNum = 4+fHasTeams+fHasChat;
		//btnAddPlayer = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_AddPlr, pRightTabLbl->GetToprightCornerRect(16,16,4,4,2+fHasTeams), pPlayerSheet->GetHotkey(), &MainDlg::OnTabTeams);
		if (fHasTeams)
			btnTeams = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Team, pRightTabLbl->GetToprightCornerRect(16,16,4,4,--iBtnNum), pPlayerSheet->GetHotkey(), &MainDlg::OnTabTeams);
		btnPlayers = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Player, pRightTabLbl->GetToprightCornerRect(16,16,4,4,--iBtnNum), pPlayerSheet->GetHotkey(), &MainDlg::OnTabPlayers);
		btnResources = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Resource, pRightTabLbl->GetToprightCornerRect(16,16,4,4,--iBtnNum), pResSheet->GetHotkey(), &MainDlg::OnTabRes);
		btnOptions = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Options, pRightTabLbl->GetToprightCornerRect(16,16,4,4,--iBtnNum), pOptionsSheet->GetHotkey(), &MainDlg::OnTabOptions);
		btnScenario = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Gfx, pRightTabLbl->GetToprightCornerRect(16,16,4,4,--iBtnNum), pOptionsSheet->GetHotkey(), &MainDlg::OnTabScenario);
		if (fHasChat)
			btnChat = new C4GUI::CallbackButton<MainDlg, C4GUI::IconButton>(C4GUI::Ico_Ex_Chat, pRightTabLbl->GetToprightCornerRect(16,16,4,4,--iBtnNum), 0 /* 2do*/, &MainDlg::OnBtnChat);

		// update labels and tooltips for player list
		UpdateRightTab();

		// chat area
		C4GUI::ComponentAligner caCenter(caMain.GetAll(), iIndentX2, iIndentY3);
		// chat input box
		C4GUI::ComponentAligner caChat(caCenter.GetFromBottom(C4GUI::Edit::GetDefaultEditHeight()), 0,0);
		pLbl = new C4GUI::WoodenLabel(LoadResStr("IDS_CTL_CHAT"), caChat.GetFromLeft(40), C4GUI_CaptionFontClr, &::GraphicsResource.TextFont);
		pEdt = new C4GUI::CallbackEdit<MainDlg>(caChat.GetAll(), this, &MainDlg::OnChatInput);
		pEdt->SetToolTip(LoadResStr("IDS_DLGTIP_CHAT")); pLbl->SetToolTip(LoadResStr("IDS_DLGTIP_CHAT"));
		pLbl->SetClickFocusControl(pEdt);
		// log box
		pChatBox = new C4GUI::TextWindow(caCenter.GetAll());
		//pPaintBox = new PaintBox();
		// add components in tab-order
		//AddElement(pPaintBox); pPaintBox->SetToolTip("Paintbox :D");
		AddElement(pChatBox);
		AddElement(pLbl); AddElement(pEdt); // chat

		AddElement(pRightTabLbl);
		//(new C4GUI::ContextButton(pRightTabLbl, true))->SetToolTip("[.!] Switches between player and ressource view"); // right tab label+ctx menu
		if (btnTeams) AddElement(btnTeams);
		AddElement(btnPlayers);
		AddElement(btnResources);
		AddElement(btnOptions);
		AddElement(btnScenario);
		if (btnChat) AddElement(btnChat);

		AddElement(pRightTab);
		AddElement(btnExit); btnExit->SetToolTip(LoadResStr("IDS_DLGTIP_EXIT"));
		AddElement(pGameOptionButtons);
		if (fHost)
		{
			AddElement(btnRun);
			btnRun->SetToolTip(LoadResStr("IDS_DLGTIP_GAMEGO"));
		}
		else
		{
			// 2do: Ready-checkbox
		}
		// set initial focus
		SetFocus(pEdt, false);

		// stuff
		eCountdownState = CDS_None;
		iBackBufferIndex = -1;

		// initial player list update
		UpdatePlayerList();
	}

	MainDlg::~MainDlg()
	{
		Application.Remove(this);
		delete pKeyHistoryUp;
		delete pKeyHistoryDown;
	}

	void MainDlg::OnExitBtn(C4GUI::Control *btn)
	{
		// abort dlg
		Close(false);
	}

	void MainDlg::OnTestBtn(C4GUI::Control *btn)
	{
		pPlayerList->SetMode(C4PlayerInfoListBox::PILBM_Evaluation);
		pPlayerList->SetTeamFilter(1);
		//pPlayerList->SetCustomFont(&::GraphicsResource.FontTooltip, 0xff000000);
	}

	void MainDlg::SetCountdownState(CountdownState eToState, int32_t iTimer)
	{
		// no change?
		if (eToState == eCountdownState) return;
		// changing away from countdown?
		if (eCountdownState == CDS_Countdown)
		{
			StopSoundEffect("Elevator", NULL);
			if (eToState != CDS_Start) StartSoundEffect("Pshshsh");
		}
		// change to game start?
		if (eToState == CDS_Start)
		{
			// announce it!
			StartSoundEffect("Blast3");
		}
		else if (eToState == CDS_Countdown)
		{
			StartSoundEffect("Fuse");
			StartSoundEffect("Elevator", true);
		}
		if (eToState == CDS_Countdown || eToState == CDS_LongCountdown)
		{
			// game start notify
			Application.NotifyUserIfInactive();
			if (!eCountdownState)
			{
				// host update start button to be abort button
				if (btnRun) btnRun->SetText(LoadResStr("IDS_DLG_CANCEL"));
			}
		}
		// countdown abort?
		if (!eToState)
		{
			// host update start button to be start button again
			if (btnRun) btnRun->SetText(LoadResStr("IDS_DLG_GAMEGO"));
			// countdown abort message
			OnLog(LoadResStr("IDS_PRC_STARTABORTED"), C4GUI_LogFontClr2);
		}
		// set new state
		eCountdownState = eToState;
		// update stuff (makes team sel and fair crew btn available)
		pGameOptionButtons->SetCountdown(IsCountdown());
		UpdatePlayerList();
	}

	void MainDlg::OnCountdownPacket(const C4PacketCountdown &Pkt)
	{
		// determine new countdown state
		int32_t iTimer = 0;
		CountdownState eNewState;
		if (Pkt.IsAbort())
			eNewState = CDS_None;
		else
		{
			iTimer = Pkt.GetCountdown();
			if (!iTimer)
				eNewState = CDS_Start; // game is about to be started (boom)
			else if (iTimer <= AlmostStartCountdownTime)
				eNewState = CDS_Countdown; // eToState
			else
				eNewState = CDS_LongCountdown;
		}
		bool fWasCountdown = !!eCountdownState;
		SetCountdownState(eNewState, iTimer);
		// display countdown (except last, which ends the lobby anyway)
		if (iTimer)
		{
			// first countdown message
			OnLog(Pkt.GetCountdownMsg(!fWasCountdown).getData(), C4GUI_LogFontClr2);
			StartSoundEffect("Command");
		}
	}

	bool MainDlg::IsCountdown()
	{
		// flag as countdown if countdown running or game is about to start
		//   so team choice, etc. will not become available in the last split-second
		return eCountdownState >= CDS_Countdown;
	}

	void MainDlg::OnClosed(bool fOK)
	{
		// lobby aborted by user: remember not to display error log
		if (!fOK)
			C4GameLobby::UserAbort = true;
		// finish countdown if running
		// (may not be finished if status change packet from host is faster than the countdown-initiate)
		if (eCountdownState) SetCountdownState(fOK ? CDS_Start : CDS_None, 0);
	}

	void MainDlg::OnRunBtn(C4GUI::Control *btn)
	{
		// only for host
		if (!::Network.isHost()) return;
		// already started? then abort
		if (eCountdownState) { ::Network.AbortLobbyCountdown(); return; }
		// otherwise start, utilizing correct countdown time
		Start(Config.Lobby.CountdownTime);
	}

	void MainDlg::Start(int32_t iCountdownTime)
	{
		// network savegame resumes: Warn if not all players have been associated
		if (Game.C4S.Head.SaveGame)
			if (Game.PlayerInfos.FindUnassociatedRestoreInfo(Game.RestorePlayerInfos))
			{
				StdStrBuf sMsg; sMsg.Ref(LoadResStr("IDS_MSG_NOTALLSAVEGAMEPLAYERSHAVE"));
				if (!GetScreen()->ShowMessageModal(sMsg.getData(), LoadResStr("IDS_MSG_FREESAVEGAMEPLRS"), C4GUI::MessageDialog::btnYesNo, C4GUI::Ico_SavegamePlayer, &Config.Startup.HideMsgPlrNoTakeOver))
					return;
			}
		// validate countdown time
		iCountdownTime = ValidatedCountdownTime(iCountdownTime);
		// either direct start...
		if (!iCountdownTime)
			::Network.Start();
		else
			// ...or countdown
			::Network.StartLobbyCountdown(iCountdownTime);
	}

	C4GUI::Edit::InputResult MainDlg::OnChatInput(C4GUI::Edit *edt, bool fPasting, bool fPastingMore)
	{
		// get edit text
		C4GUI::Edit *pEdt = reinterpret_cast<C4GUI::Edit *>(edt);
		const char *szInputText = pEdt->GetText();
		// no input?
		if (!szInputText || !*szInputText)
		{
			// do some error sound then
			C4GUI::GUISound("Error");
		}
		else
		{
			// store input in backbuffer before processing commands
			// because those might kill the edit field
			::MessageInput.StoreBackBuffer(szInputText);
			bool fProcessed = false;
			// check confidential data
			if (Config.IsConfidentialData(szInputText))
			{
				::pGUI->ShowErrorMessage(LoadResStr("IDS_ERR_WARNINGYOUWERETRYINGTOSEN"));
				fProcessed = true;
			}
			// CAUTION when implementing special commands (like /quit) here:
			// those must not be executed when text is pasted, because that could crash the GUI system
			// when there are additional lines to paste, but the edit field is destructed by the command
			if (!fProcessed && *szInputText == '/')
			{
				// must be 1 char longer than the longest command only. If given commands are longer, they will be truncated, and such a command won't exist anyway
				const int32_t MaxCommandLen = 20;
				char Command[MaxCommandLen+1];
				const char *szPar = szInputText;
				// parse command until first space
				int32_t iParPos = SCharPos(' ', szInputText);
				if (iParPos<0)
				{
					// command w/o par
					SCopy(szInputText, Command, MaxCommandLen);
					szPar += SLen(szPar);
				}
				else
				{
					// command with following par
					SCopy(szInputText, Command, Min(MaxCommandLen, iParPos));
					szPar += iParPos+1;
				}
				fProcessed = true;
				// evaluate lobby-only commands
				// ------------------------------------------------------
				if (SEqualNoCase(Command, "/joinplr"))
				{
					// compose path from given filename
					StdStrBuf plrPath;
					plrPath.Format("%s%s", Config.General.UserDataPath, szPar);
					// player join - check filename
					if (!ItemExists(plrPath.getData()))
					{
						LobbyError(FormatString(LoadResStr("IDS_MSG_CMD_JOINPLR_NOFILE"), plrPath.getData()).getData());
					}
					else
						::Network.Players.JoinLocalPlayer(plrPath.getData(), true);
				}
				// ------------------------------------------------------
				else if (SEqualNoCase(Command, "/plrclr"))
				{
					// get player name from input text
					int iSepPos = SCharPos(' ', szPar, 0);
					C4PlayerInfo *pNfo=NULL;
					int32_t idLocalClient = -1;
					if (::Network.Clients.GetLocal()) idLocalClient = ::Network.Clients.GetLocal()->getID();
					if (iSepPos>0)
					{
						// a player name is given: Parse it
						StdStrBuf sPlrName;
						sPlrName.Copy(szPar, iSepPos);
						szPar += iSepPos+1; int32_t id=0;
						while ((pNfo = Game.PlayerInfos.GetNextPlayerInfoByID(id)))
						{
							id = pNfo->GetID();
							if (WildcardMatch(sPlrName.getData(), pNfo->GetName())) break;
						}
					}
					else
						// no player name: Set local player
						pNfo = Game.PlayerInfos.GetPrimaryInfoByClientID(idLocalClient);
					C4ClientPlayerInfos *pCltNfo=NULL;
					if (pNfo) pCltNfo = Game.PlayerInfos.GetClientInfoByPlayerID(pNfo->GetID());
					if (!pCltNfo)
					{
						LobbyError(LoadResStr("IDS_MSG_CMD_PLRCLR_NOPLAYER"));
					}
					else
					{
						// may color of this client be set?
						if (pCltNfo->GetClientID() != idLocalClient && !::Network.isHost())
						{
							LobbyError(LoadResStr("IDS_MSG_CMD_PLRCLR_NOACCESS"));
						}
						else
						{
							// get color to set
							uint32_t dwNewClr;
							if (sscanf(szPar, "%x", &dwNewClr) != 1)
							{
								LobbyError(LoadResStr("IDS_MSG_CMD_PLRCLR_USAGE"));
							}
							else
							{
								// color validation
								dwNewClr |= 0xff000000;
								if (!dwNewClr) ++dwNewClr;
								// request a color change to this color
								C4ClientPlayerInfos LocalInfoRequest = *pCltNfo;
								C4PlayerInfo *pPlrInfo = LocalInfoRequest.GetPlayerInfoByID(pNfo->GetID());
								assert(pPlrInfo);
								if (pPlrInfo)
								{
									pPlrInfo->SetOriginalColor(dwNewClr); // set this as a new color wish
									::Network.Players.RequestPlayerInfoUpdate(LocalInfoRequest);
								}
							}
						}
					}
				}
				// ------------------------------------------------------
				else if (SEqualNoCase(Command, "/start"))
				{
					// timeout given?
					int32_t iTimeout = Config.Lobby.CountdownTime;
					if (!::Network.isHost())
						LobbyError(LoadResStr("IDS_MSG_CMD_HOSTONLY"));
					else if (szPar && *szPar && (!sscanf(szPar, "%d", &iTimeout) || iTimeout<0))
						LobbyError(LoadResStr("IDS_MSG_CMD_START_USAGE"));
					else
					{
						// abort previous countdown
						if (eCountdownState) ::Network.AbortLobbyCountdown();
						// start new countdown (aborts previous if necessary)
						Start(iTimeout);
					}
				}
				// ------------------------------------------------------
				else if (SEqualNoCase(Command, "/abort"))
				{
					if (!::Network.isHost())
						LobbyError(LoadResStr("IDS_MSG_CMD_HOSTONLY"));
					else if (eCountdownState)
						::Network.AbortLobbyCountdown();
					else
						LobbyError(LoadResStr("IDS_MSG_CMD_ABORT_NOCOUNTDOWN"));
				}
				// ------------------------------------------------------
				else if (SEqualNoCase(Command, "/help"))
				{
					Log(LoadResStr("IDS_TEXT_COMMANDSAVAILABLEDURINGLO"));
					LogF("/start [time] - %s", LoadResStr("IDS_TEXT_STARTTHEROUNDWITHSPECIFIE"));
					LogF("/abort - %s", LoadResStr("IDS_TEXT_ABORTSTARTCOUNTDOWN"));
					LogF("/alert - %s", LoadResStr("IDS_TEXT_ALERTTHEHOSTIFTHEHOSTISAW"));
					LogF("/joinplr [filename] - %s", LoadResStr("IDS_TEXT_JOINALOCALPLAYERFROMTHESP"));
					LogF("/kick [client] - %s", LoadResStr("IDS_TEXT_KICKTHESPECIFIEDCLIENT"));
					LogF("/observer [client] - %s", LoadResStr("IDS_TEXT_SETTHESPECIFIEDCLIENTTOOB"));
					LogF("/me [action] - %s", LoadResStr("IDS_TEXT_PERFORMANACTIONINYOURNAME"));
					LogF("/sound [sound] - %s", LoadResStr("IDS_TEXT_PLAYASOUNDFROMTHEGLOBALSO"));
					LogF("/team [message] - %s", LoadResStr("IDS_MSG_SENDAPRIVATEMESSAGETOYOUR"));
					LogF("/plrclr [player] [RGB] - %s", LoadResStr("IDS_TEXT_CHANGETHECOLOROFTHESPECIF"));
					LogF("/plrclr [RGB] - %s", LoadResStr("IDS_TEXT_CHANGEYOUROWNPLAYERCOLOR"));
					LogF("/set comment [comment] - %s", LoadResStr("IDS_TEXT_SETANEWNETWORKCOMMENT"));
					LogF("/set password [password] - %s", LoadResStr("IDS_TEXT_SETANEWNETWORKPASSWORD"));
					LogF("/set maxplayer [number] - %s", LoadResStr("IDS_TEXT_SETANEWMAXIMUMNUMBEROFPLA"));
					LogF("/clear - %s", LoadResStr("IDS_MSG_CLEARTHEMESSAGEBOARD"));
				}
				// ------------------------------------------------------
				else
				{
					// command not known or not a specific lobby command - forward to messageinput
					fProcessed = false;
				}
			}
			// not processed? Then forward to messageinput, which parses additional commands and sends regular messages
			if (!fProcessed) ::MessageInput.ProcessInput(szInputText);
		}
		// clear edit field after text has been processed
		pEdt->SelectAll(); pEdt->DeleteSelection();
		// reset backbuffer-index of chat history
		iBackBufferIndex = -1;
		// OK, on we go. Leave edit intact
		return C4GUI::Edit::IR_None;
	}

	void MainDlg::OnClientJoin(C4Client *pNewClient)
	{
		// update list
		UpdatePlayerList();
	}

	void MainDlg::OnClientConnect(C4Client *pClient, C4Network2IOConnection *pConn)
	{
	}

	void MainDlg::OnClientPart(C4Client *pPartClient)
	{
		// update list
		UpdatePlayerList();
	}

	void MainDlg::HandlePacket(char cStatus, const C4PacketBase *pPacket, C4Network2Client *pClient)
	{
		// note that player info update packets are not handled by this function,
		// but by player info list and then forwarded to MainDlg::OnPlayerUpdate
		// this is necessary because there might be changes (e.g. duplicate colors)
		// done by player info list
		// besides, this releases the lobby from doing any host/client-specializations
#define GETPKT(type, name) \
    assert(pPacket); const type &name = \
      /*dynamic_cast*/ static_cast<const type &>(*pPacket);
		switch (cStatus)
		{
		case PID_LobbyCountdown: // initiate or abort countdown
		{
			GETPKT(C4PacketCountdown, Pkt);
			// do countdown
			OnCountdownPacket(Pkt);
		}
		break;
		};
#undef GETPKT
	}

	bool MainDlg::OnMessage(C4Client *pOfClient, const char *szMessage)
	{
		// output message should be prefixed with client already
		const char *szMsgBuf = szMessage;
		// 2do: log with player colors?
		if (pChatBox && !pOfClient->IsIgnored())
		{
			pChatBox->AddTextLine(szMsgBuf, &::GraphicsResource.TextFont, ::Network.Players.GetClientChatColor(pOfClient ? pOfClient->getID() : Game.Clients.getLocalID(), true) | C4GUI_MessageFontAlpha, true, true);
			pChatBox->ScrollToBottom();
		}
		// log it
		LogSilent(szMsgBuf);
		// done, success
		return true;
	}

	void MainDlg::OnClientSound(C4Client *pOfClient)
	{
		// show that someone played a sound
		if (pOfClient && pPlayerList)
		{
			pPlayerList->SetClientSoundIcon(pOfClient->getID());
		}
	}

	void MainDlg::OnLog(const char *szLogMsg, DWORD dwClr)
	{
		if (pChatBox)
		{
			pChatBox->AddTextLine(szLogMsg, &::GraphicsResource.TextFont, dwClr, true, true);
			pChatBox->ScrollToBottom();
		}
	}

	void MainDlg::OnError(const char *szErrMsg)
	{
		if (pChatBox)
		{
			StartSoundEffect("Error");
			pChatBox->AddTextLine(szErrMsg, &::GraphicsResource.TextFont, C4GUI_ErrorFontClr, true, true);
			pChatBox->ScrollToBottom();
		}
	}

	void MainDlg::OnSec1Timer()
	{
		UpdatePlayerList();
	}

	void MainDlg::UpdatePlayerList()
	{
		// this updates ping label texts and teams
		if (pPlayerList) pPlayerList->Update();
	}

	C4GUI::ContextMenu *MainDlg::OnRightTabContext(C4GUI::Element *pLabel, int32_t iX, int32_t iY)
	{
		// create context menu
		C4GUI::ContextMenu *pMenu = new C4GUI::ContextMenu();
		// players/ressources
		C4GUI::Tabular::Sheet *pPlayerSheet = pRightTab->GetSheet(0);
		C4GUI::Tabular::Sheet *pResSheet = pRightTab->GetSheet(1);
		C4GUI::Tabular::Sheet *pOptionsSheet = pRightTab->GetSheet(2);
		pMenu->AddItem(pPlayerSheet->GetTitle(), pPlayerSheet->GetToolTip(), C4GUI::Ico_Player,
		               new C4GUI::CBMenuHandler<MainDlg>(this, &MainDlg::OnCtxTabPlayers));
		if (Game.Teams.IsMultiTeams())
		{
			StdCopyStrBuf strShowTeamsDesc(LoadResStr("IDS_MSG_SHOWTEAMS_DESC"));
			pMenu->AddItem(LoadResStr("IDS_MSG_SHOWTEAMS"), strShowTeamsDesc.getData(), C4GUI::Ico_Team,
			               new C4GUI::CBMenuHandler<MainDlg>(this, &MainDlg::OnCtxTabTeams));
		}
		pMenu->AddItem(pResSheet->GetTitle(), pResSheet->GetToolTip(), C4GUI::Ico_Resource,
		               new C4GUI::CBMenuHandler<MainDlg>(this, &MainDlg::OnCtxTabRes));
		pMenu->AddItem(pOptionsSheet->GetTitle(), pOptionsSheet->GetToolTip(), C4GUI::Ico_Options,
		               new C4GUI::CBMenuHandler<MainDlg>(this, &MainDlg::OnCtxTabOptions));
		// open it
		return pMenu;
	}

	void MainDlg::OnClientAddPlayer(const char *szFilename, int32_t idClient)
	{
		// check client number
		if (idClient != Game.Clients.getLocalID())
		{
			LobbyError(FormatString(LoadResStr("IDS_ERR_JOINPLR_NOLOCALCLIENT"), szFilename, idClient).getData());
			return;
		}
		// player join - check filename
		if (!ItemExists(szFilename))
		{
			LobbyError(FormatString(LoadResStr("IDS_ERR_JOINPLR_NOFILE"), szFilename).getData());
			return;
		}
		// join!
		::Network.Players.JoinLocalPlayer(Config.AtRelativePath(szFilename), true);
	}

	void MainDlg::OnTabPlayers(C4GUI::Control *btn)
	{
		if (pPlayerList) pPlayerList->SetMode(C4PlayerInfoListBox::PILBM_LobbyClientSort);
		if (pRightTab)
		{
			pRightTab->SelectSheet(SheetIdx_PlayerList, true);
			UpdateRightTab();
		}
	}

	void MainDlg::OnTabTeams(C4GUI::Control *btn)
	{
		if (pPlayerList) pPlayerList->SetMode(C4PlayerInfoListBox::PILBM_LobbyTeamSort);
		if (pRightTab)
		{
			pRightTab->SelectSheet(SheetIdx_PlayerList, true);
			UpdateRightTab();
		}
	}

	void MainDlg::OnTabRes(C4GUI::Control *btn)
	{
		if (pRightTab)
		{
			pRightTab->SelectSheet(SheetIdx_Res, true);
			UpdateRightTab();
		}
	}

	void MainDlg::OnTabOptions(C4GUI::Control *btn)
	{
		if (pRightTab)
		{
			pRightTab->SelectSheet(SheetIdx_Options, true);
			UpdateRightTab();
		}
	}

	void MainDlg::OnTabScenario(C4GUI::Control *btn)
	{
		if (pRightTab)
		{
			pRightTab->SelectSheet(SheetIdx_Scenario, true);
			UpdateRightTab();
		}
	}

	void MainDlg::UpdateRightTab()
	{
		if (!pRightTabLbl || !pRightTab || !pRightTab->GetActiveSheet() || !pPlayerList) return;
		// copy active sheet data to label
		pRightTabLbl->SetText(pRightTab->GetActiveSheet()->GetTitle());
		pRightTabLbl->SetToolTip(pRightTab->GetActiveSheet()->GetToolTip());
		// update
		if (pRightTab->GetActiveSheetIndex() == SheetIdx_PlayerList) UpdatePlayerList();
		if (pRightTab->GetActiveSheetIndex() == SheetIdx_Res) pResList->Activate(); else pResList->Deactivate();
		if (pRightTab->GetActiveSheetIndex() == SheetIdx_Options) pOptionsList->Activate(); else pOptionsList->Deactivate();
		if (pRightTab->GetActiveSheetIndex() == SheetIdx_Scenario) pScenarioInfo->Activate(); else pScenarioInfo->Deactivate();
		// update selection buttons
		if (btnPlayers) btnPlayers->SetHighlight(pRightTab->GetActiveSheetIndex() == SheetIdx_PlayerList && pPlayerList->GetMode() == C4PlayerInfoListBox::PILBM_LobbyClientSort);
		if (btnResources) btnResources->SetHighlight(pRightTab->GetActiveSheetIndex() == SheetIdx_Res);
		if (btnTeams) btnTeams->SetHighlight(pRightTab->GetActiveSheetIndex() == SheetIdx_PlayerList && pPlayerList->GetMode() == C4PlayerInfoListBox::PILBM_LobbyTeamSort);
		if (btnOptions) btnOptions->SetHighlight(pRightTab->GetActiveSheetIndex() == SheetIdx_Options);
		if (btnScenario) btnScenario->SetHighlight(pRightTab->GetActiveSheetIndex() == SheetIdx_Scenario);
	}

	void MainDlg::OnBtnChat(C4GUI::Control *btn)
	{
		// open chat dialog
		C4ChatDlg::ShowChat();
	}

	bool MainDlg::KeyHistoryUpDown(bool fUp)
	{
		// chat input only
		if (!IsFocused(pEdt)) return false;
		pEdt->SelectAll(); pEdt->DeleteSelection();
		const char *szPrevInput = ::MessageInput.GetBackBuffer(fUp ? (++iBackBufferIndex) : (--iBackBufferIndex));
		if (!szPrevInput || !*szPrevInput)
			iBackBufferIndex = -1;
		else
		{
			pEdt->InsertText(szPrevInput, true);
			pEdt->SelectAll();
		}
		return true;
	}

	void MainDlg::UpdatePassword()
	{
		// if the password setting has changed, make sure the buttons reflect this change
		pGameOptionButtons->UpdatePasswordBtn();
	}

	int32_t MainDlg::ValidatedCountdownTime(int32_t iTimeout)
	{
		// no negative timeouts
		if (iTimeout < 0) iTimeout = 5;
		// in leage mode, there must be at least five seconds timeout
		if (Game.Parameters.isLeague() && iTimeout < 5) iTimeout = 5;
		return iTimeout;
	}

	void MainDlg::ClearLog()
	{
		pChatBox->ClearText(true);
	}

	void LobbyError(const char *szErrorMsg)
	{
		// get lobby
		MainDlg *pLobby = ::Network.GetLobby();
		if (pLobby) pLobby->OnError(szErrorMsg);
	}


	/* Countdown */

	Countdown::Countdown(int32_t iStartTimer) : iStartTimer(iStartTimer)
	{
		// only on network hosts
		assert(::Network.isHost());
		// ctor: Init; sends initial countdown packet
		C4PacketCountdown pck(iStartTimer);
		::Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_LobbyCountdown, pck));
		// also process on host
		MainDlg *pLobby = ::Network.GetLobby();
		if (pLobby)
		{
			pLobby->OnCountdownPacket(pck);
		}
		else
		{
			// no lobby: Message to log for dedicated/console hosts
			Log(pck.GetCountdownMsg().getData());
		}

		// init timer callback
		Application.Add(this);
	}

	Countdown::~Countdown()
	{
		// release timer
		Application.Remove(this);
	}

	void Countdown::OnSec1Timer()
	{
		// count down
		iStartTimer = Max<int32_t>(iStartTimer - 1, 0);
		// only send "important" start timer numbers to all clients
		if (iStartTimer <= AlmostStartCountdownTime || // last seconds
		    (iStartTimer <= 600 && !(iStartTimer % 10)) || // last minute: 10s interval
		    !(iStartTimer % 60)) // otherwise, minute interval
		{
			C4PacketCountdown pck(iStartTimer);
			::Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_LobbyCountdown, pck));
			// also process on host
			MainDlg *pLobby = ::Network.GetLobby();
			if (pLobby)
				pLobby->OnCountdownPacket(pck);
			else if (iStartTimer)
			{
				// no lobby: Message to log for dedicated/console hosts
				Log(pck.GetCountdownMsg().getData());
			}
		}
		// countdown done
		if (!iStartTimer)
		{
			// Dedicated server: if there are not enough players for this game, abort and quit the application
			if (!::Network.GetLobby() && (Game.PlayerInfos.GetPlayerCount() < Game.C4S.GetMinPlayer()))
			{
				Log(LoadResStr("IDS_MSG_NOTENOUGHPLAYERSFORTHISRO")); // it would also be nice to send this message to all clients...
				Application.Quit();
			}
			// Start the game
			else
				::Network.Start();
		}
	}

	void Countdown::Abort()
	{
		// host sends packets
		if (!::Network.isHost()) return;
		C4PacketCountdown pck(C4PacketCountdown::Abort);
		::Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_LobbyCountdown, pck));
		// also process on host
		MainDlg *pLobby = ::Network.GetLobby();
		if (pLobby)
		{
			pLobby->OnCountdownPacket(pck);
		}
		else
		{
			// no lobby: Message to log for dedicated/console hosts
			Log(LoadResStr("IDS_PRC_STARTABORTED"));
		}
	}

} // end of namespace


