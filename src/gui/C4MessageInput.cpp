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
// handles input dialogs, last-message-buffer, MessageBoard-commands

#include "C4Include.h"
#include "gui/C4MessageInput.h"

#include "control/C4GameControl.h"
#include "editor/C4Console.h"
#include "game/C4Application.h"
#include "game/C4GraphicsSystem.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4Gui.h"
#include "gui/C4GameLobby.h"
#include "object/C4Object.h"
#include "player/C4Player.h"
#include "player/C4PlayerList.h"

// --------------------------------------------------
// C4ChatInputDialog

// singleton
C4ChatInputDialog *C4ChatInputDialog::pInstance = nullptr;

// helper func: Determine whether input text is good for a chat-style-layout dialog
bool IsSmallInputQuery(const char *szInputQuery)
{
	if (!szInputQuery) return true;
	int32_t w,h;
	if (SCharCount('|', szInputQuery)) return false;
	if (!::GraphicsResource.TextFont.GetTextExtent(szInputQuery, w,h, true))
		return false; // ???
	return w<C4GUI::GetScreenWdt()/5;
}

C4ChatInputDialog::C4ChatInputDialog(bool fObjInput, C4Object *pScriptTarget, bool fUppercase, bool fTeam, int32_t iPlr, const StdStrBuf &rsInputQuery)
		: C4GUI::InputDialog(fObjInput ? rsInputQuery.getData() : LoadResStrNoAmp("IDS_CTL_CHAT"), nullptr, C4GUI::Ico_None, nullptr, !fObjInput || IsSmallInputQuery(rsInputQuery.getData())),
		fObjInput(fObjInput), fUppercase(fUppercase), pTarget(pScriptTarget), iPlr(iPlr), BackIndex(-1), fProcessed(false)
{
	// singleton-var
	pInstance = this;
	// set custom edit control
	SetCustomEdit(new C4GUI::CallbackEdit<C4ChatInputDialog>(C4Rect(0,0,10,10), this, &C4ChatInputDialog::OnChatInput, &C4ChatInputDialog::OnChatCancel));
	// key bindings
	pKeyHistoryUp  = new C4KeyBinding(C4KeyCodeEx(K_UP  ), "ChatHistoryUp"  , KEYSCOPE_Gui, new C4GUI::DlgKeyCBEx<C4ChatInputDialog, bool>(*this, true , &C4ChatInputDialog::KeyHistoryUpDown), C4CustomKey::PRIO_CtrlOverride);
	pKeyHistoryDown= new C4KeyBinding(C4KeyCodeEx(K_DOWN), "ChatHistoryDown", KEYSCOPE_Gui, new C4GUI::DlgKeyCBEx<C4ChatInputDialog, bool>(*this, false, &C4ChatInputDialog::KeyHistoryUpDown), C4CustomKey::PRIO_CtrlOverride);
	pKeyAbort = new C4KeyBinding(C4KeyCodeEx(K_F2), "ChatAbort", KEYSCOPE_Gui, new C4GUI::DlgKeyCB<C4GUI::Dialog>(*this, &C4GUI::Dialog::KeyEscape), C4CustomKey::PRIO_CtrlOverride);
	pKeyNickComplete = new C4KeyBinding(C4KeyCodeEx(K_TAB), "ChatNickComplete", KEYSCOPE_Gui, new C4GUI::DlgKeyCB<C4ChatInputDialog>(*this, &C4ChatInputDialog::KeyCompleteNick), C4CustomKey::PRIO_CtrlOverride);
	pKeyPlrControl = new C4KeyBinding(C4KeyCodeEx(KEY_Any, KEYS_Control), "ChatForwardPlrCtrl", KEYSCOPE_Gui, new C4GUI::DlgKeyCBPassKey<C4ChatInputDialog>(*this, &C4ChatInputDialog::KeyPlrControl), C4CustomKey::PRIO_Dlg);
	pKeyGamepadControl = new C4KeyBinding(C4KeyCodeEx(KEY_Any), "ChatForwardGamepadCtrl", KEYSCOPE_Gui, new C4GUI::DlgKeyCBPassKey<C4ChatInputDialog>(*this, &C4ChatInputDialog::KeyGamepadControlDown, &C4ChatInputDialog::KeyGamepadControlUp, &C4ChatInputDialog::KeyGamepadControlPressed), C4CustomKey::PRIO_PlrControl);
	pKeyBackClose = new C4KeyBinding(C4KeyCodeEx(K_BACK), "ChatBackspaceClose", KEYSCOPE_Gui, new C4GUI::DlgKeyCB<C4ChatInputDialog>(*this, &C4ChatInputDialog::KeyBackspaceClose), C4CustomKey::PRIO_CtrlOverride);
	// free when closed...
	SetDelOnClose();
	// initial team text
	if (fTeam) pEdit->InsertText("/team ", true);
}

C4ChatInputDialog::~C4ChatInputDialog()
{
	delete pKeyHistoryUp;
	delete pKeyHistoryDown;
	delete pKeyAbort;
	delete pKeyNickComplete;
	delete pKeyPlrControl;
	delete pKeyGamepadControl;
	delete pKeyBackClose;
	if (this==pInstance) pInstance=nullptr;
}

void C4ChatInputDialog::OnChatCancel()
{
	// abort chat: Make sure msg board query is aborted
	fProcessed = true;
	if (fObjInput)
	{
		// check if the target input is still valid
		C4Player *pPlr = ::Players.Get(iPlr);
		if (!pPlr) return;
		if (pPlr->MarkMessageBoardQueryAnswered(pTarget))
		{
			// there was an associated query - it must be removed on all clients synchronized via queue
			::Control.DoInput(CID_MsgBoardReply, new C4ControlMsgBoardReply(nullptr, pTarget ? pTarget->Number : 0, iPlr), CDT_Decide);
		}
	}
}

void C4ChatInputDialog::OnClosed(bool fOK)
{
	// make sure chat input is processed, even if closed by other means than Enter on edit
	if (!fProcessed)
		if (fOK)
				OnChatInput(pEdit, false, false); else OnChatCancel();
	else
		OnChatCancel();
	typedef C4GUI::InputDialog BaseDlg;
	BaseDlg::OnClosed(fOK);
}

C4GUI::Edit::InputResult C4ChatInputDialog::OnChatInput(C4GUI::Edit *edt, bool fPasting, bool fPastingMore)
{
	// no double processing
	if (fProcessed) return C4GUI::Edit::IR_CloseDlg;
	// get edit text
	auto *pEdt = reinterpret_cast<C4GUI::Edit *>(edt);
	auto *szInputText = const_cast<char *>(pEdt->GetText());
	// Store to back buffer
	::MessageInput.StoreBackBuffer(szInputText);
	// script queried input?
	if (fObjInput)
	{
		fProcessed = true;
		// check if the target input is still valid
		C4Player *pPlr = ::Players.Get(iPlr);
		if (!pPlr) return C4GUI::Edit::IR_CloseDlg;
		if (!pPlr->MarkMessageBoardQueryAnswered(pTarget))
		{
			// there was no associated query!
			return C4GUI::Edit::IR_CloseDlg;
		}
		// then do a script callback, incorporating the input into the answer
		if (fUppercase) SCapitalize(szInputText);
		::Control.DoInput(CID_MsgBoardReply, new C4ControlMsgBoardReply(szInputText, pTarget ? pTarget->Number : 0, iPlr), CDT_Decide);
		return C4GUI::Edit::IR_CloseDlg;
	}
	else
		// reroute to message input class
		::MessageInput.ProcessInput(szInputText);
	// safety: message board commands may do strange things
	if (this!=pInstance) return C4GUI::Edit::IR_Abort;
	// select all text to be removed with next keypress
	// just for pasting mode; usually the dlg will be closed now anyway
	pEdt->SelectAll();
	// avoid dlg close, if more content is to be pasted
	if (fPastingMore) return C4GUI::Edit::IR_None;
	fProcessed = true;
	return C4GUI::Edit::IR_CloseDlg;
}

bool C4ChatInputDialog::KeyHistoryUpDown(bool fUp)
{
	// browse chat history
	pEdit->SelectAll(); pEdit->DeleteSelection();
	const char *szPrevInput = ::MessageInput.GetBackBuffer(fUp ? (++BackIndex) : (--BackIndex));
	if (!szPrevInput || !*szPrevInput)
		BackIndex = -1;
	else
	{
		pEdit->InsertText(szPrevInput, true);
		pEdit->SelectAll();
	}
	return true;
}

bool C4ChatInputDialog::KeyPlrControl(const C4KeyCodeEx &key)
{
	// Control pressed while doing this key: Reroute this key as a player-control
	Game.DoKeyboardInput(WORD(key.Key), KEYEV_Down, !!(key.dwShift & KEYS_Alt), false, !!(key.dwShift & KEYS_Shift), key.IsRepeated(), nullptr, true);
	// mark as processed, so it won't get any double processing
	return true;
}

bool C4ChatInputDialog::KeyGamepadControlDown(const C4KeyCodeEx &key)
{
	// filter gamepad control
	if (!Key_IsGamepad(key.Key)) return false;
	// forward it
	Game.DoKeyboardInput(key.Key, KEYEV_Down, false, false, false, key.IsRepeated(), nullptr, true);
	return true;
}

bool C4ChatInputDialog::KeyGamepadControlUp(const C4KeyCodeEx &key)
{
	// filter gamepad control
	if (!Key_IsGamepad(key.Key)) return false;
	// forward it
	Game.DoKeyboardInput(key.Key, KEYEV_Up, false, false, false, key.IsRepeated(), nullptr, true);
	return true;
}

bool C4ChatInputDialog::KeyGamepadControlPressed(const C4KeyCodeEx &key)
{
	// filter gamepad control
	if (!Key_IsGamepad(key.Key)) return false;
	// forward it
	Game.DoKeyboardInput(key.Key, KEYEV_Pressed, false, false, false, key.IsRepeated(), nullptr, true);
	return true;
}

bool C4ChatInputDialog::KeyBackspaceClose()
{
	// close if chat text box is empty (on backspace)
	if (pEdit->GetText() && *pEdit->GetText()) return false;
	Close(false);
	return true;
}

bool C4ChatInputDialog::KeyCompleteNick()
{
	if (!pEdit) return false;
	char IncompleteNick[256+1];
	// get current word in edit
	if (!pEdit->GetCurrentWord(IncompleteNick, 256)) return false;
	if (!*IncompleteNick) return false;
	C4Player *plr = ::Players.First;
	while (plr)
	{
		// Compare name and input
		if (SEqualNoCase(plr->GetName(), IncompleteNick, SLen(IncompleteNick)))
		{
			pEdit->InsertText(plr->GetName() + SLen(IncompleteNick), true);
			return true;
		}
		else
			plr = plr->Next;
	}
	// no match found
	return false;
}


// --------------------------------------------------
// C4MessageInput

bool C4MessageInput::Init()
{
	// add default commands
	if (!pCommands)
	{
		AddCommand("speed", "SetGameSpeed(%d)");
	}
	return true;
}

void C4MessageInput::Default()
{
	// clear backlog
	for (auto & cnt : BackBuffer) cnt[0]=0;
}

void C4MessageInput::Clear()
{
	// close any dialog
	CloseTypeIn();
	// free messageboard-commands
	C4MessageBoardCommand *pCmd;
	while ((pCmd = pCommands))
	{
		pCommands = pCmd->Next;
		delete pCmd;
	}
}

bool C4MessageInput::CloseTypeIn()
{
	// close dialog if present and valid
	C4ChatInputDialog *pDlg = GetTypeIn();
	if (!pDlg) return false;
	pDlg->Close(false);
	return true;
}

bool C4MessageInput::StartTypeIn(bool fObjInput, C4Object *pObj, bool fUpperCase, bool fTeam, int32_t iPlr, const StdStrBuf &rsInputQuery)
{
	// close any previous
	if (IsTypeIn()) CloseTypeIn();
	// start new
	return ::pGUI->ShowRemoveDlg(new C4ChatInputDialog(fObjInput, pObj, fUpperCase, fTeam, iPlr, rsInputQuery));
}

bool C4MessageInput::KeyStartTypeIn(bool fTeam)
{
	// fullscreen only
	if (Application.isEditor) return false;
	// OK, start typing
	return StartTypeIn(false, nullptr, false, fTeam);
}

bool C4MessageInput::ToggleTypeIn()
{
	// toggle off?
	if (IsTypeIn())
	{
		// no accidental close of script queried dlgs by chat request
		if (GetTypeIn()->IsScriptQueried()) return false;
		return CloseTypeIn();
	}
	else
		// toggle on!
		return StartTypeIn();
}

bool C4MessageInput::IsTypeIn()
{
	// check GUI and dialog
	return C4ChatInputDialog::IsShown();
}

bool C4MessageInput::ProcessInput(const char *szText)
{
	// helper variables
	char OSTR[402]; // cba
	C4ControlMessageType eMsgType;
	const char *szMsg = nullptr;
	int32_t iToPlayer = -1;

	// Starts with '^', "team:" or "/team ": Team message
	if (szText[0] == '^' || SEqual2NoCase(szText, "team:") || SEqual2NoCase(szText, "/team "))
	{
		if (!Game.Teams.IsTeamVisible())
		{
			// team not known; can't send!
			Log(LoadResStr("IDS_MSG_CANTSENDTEAMMESSAGETEAMSN"));
			return false;
		}
		else
		{
			eMsgType = C4CMT_Team;
			szMsg = szText[0] == '^' ? szText+1 :
			        szText[0] == '/' ? szText+6 : szText+5;
		}
	}
	// Starts with "/private ": Private message (running game only)
	else if (Game.IsRunning && SEqual2NoCase(szText, "/private "))
	{
		// get target name
		char szTargetPlr[C4MaxName + 1];
		SCopyUntil(szText + 9, szTargetPlr, ' ', C4MaxName);
		// search player
		C4Player *pToPlr = ::Players.GetByName(szTargetPlr);
		if (!pToPlr) return false;
		// set
		eMsgType = C4CMT_Private;
		iToPlayer = pToPlr->Number;
		szMsg = szText + 10 + SLen(szText);
		if (szMsg > szText + SLen(szText)) return false;
	}
	// Starts with "/me ": Me-Message
	else if (SEqual2NoCase(szText, "/me "))
	{
		eMsgType = C4CMT_Me;
		szMsg = szText+4;
	}
	// Starts with "/sound ": Sound-Message
	else if (SEqual2NoCase(szText, "/sound "))
	{
		eMsgType = C4CMT_Sound;
		szMsg = szText+7;
	}
	// Disabled due to spamming
	// Starts with "/alert": Taskbar flash (message optional)
	else if (SEqual2NoCase(szText, "/alert ") || SEqualNoCase(szText, "/alert"))
	{
		eMsgType = C4CMT_Alert;
		szMsg = szText+6;
		if (*szMsg) ++szMsg;
	}
	// Starts with '"': Let the clonk say it
	else if (Game.IsRunning && szText[0] == '"')
	{
		eMsgType = C4CMT_Say;
		// Append '"', if neccessary
		StdStrBuf text(szText);
		SCopy(szText, OSTR, 400);
		char *pEnd = OSTR + SLen(OSTR) - 1;
		if (*pEnd != '"') { *++pEnd = '"'; *++pEnd = 0; }
		szMsg = OSTR;
	}
	// Starts with '/': Command
	else if (szText[0] == '/')
		return ProcessCommand(szText);
	// Regular message
	else
	{
		eMsgType = C4CMT_Normal;
		szMsg = szText;
	}

	// message?
	if (szMsg)
	{
		char szMessage[C4MaxMessage + 1];
		// go over whitespaces, check empty message
		while (IsWhiteSpace(*szMsg)) szMsg++;
		if (!*szMsg)
		{
			if (eMsgType != C4CMT_Alert) return true;
			*szMessage = '\0';
		}
		else
		{
			// trim right
			const char *szEnd = szMsg + SLen(szMsg) - 1;
			while (IsWhiteSpace(*szEnd) && szEnd >= szMsg) szEnd--;
			// Say: Strip quotation marks in cinematic film mode
			if (Game.C4S.Head.Film == C4SFilm_Cinematic)
			{
				if (eMsgType == C4CMT_Say) { ++szMsg; szEnd--; }
			}
			// get message
			SCopy(szMsg, szMessage, std::min<ptrdiff_t>(C4MaxMessage, szEnd - szMsg + 1));
		}
		// get sending player (if any)
		C4Player *pPlr = Game.IsRunning ? ::Players.GetLocalByIndex(0) : nullptr;
		// send
		::Control.DoInput(CID_Message,
		                  new C4ControlMessage(eMsgType, szMessage, pPlr ? pPlr->Number : -1, iToPlayer),
		                  CDT_Private);
	}

	return true;
}

bool C4MessageInput::ProcessCommand(const char *szCommand)
{
	C4GameLobby::MainDlg *pLobby = ::Network.GetLobby();
	// command
	// must be 1 char longer than the longest command only. If given commands are longer, they will be truncated, and such a command won't exist anyway
	const int32_t MaxCommandLen = 20;
	char szCmdName[MaxCommandLen + 1];
	SCopyUntil(szCommand + 1, szCmdName, ' ', MaxCommandLen);
	// parameter
	const char *pCmdPar = SSearch(szCommand, " ");
	if (!pCmdPar) pCmdPar = "";

	// CAUTION when implementing special commands (like /quit) here:
	// those must not be executed when text is pasted, because that could crash the GUI system
	// when there are additional lines to paste, but the edit field is destructed by the command

	// lobby-only commands
	if (!Game.IsRunning && SEqualNoCase(szCmdName, "joinplr"))
	{
		// compose path from given filename
		StdStrBuf plrPath;
		plrPath.Format("%s%s", Config.General.UserDataPath, pCmdPar);
		// player join - check filename
		if (!ItemExists(plrPath.getData()))
		{
			C4GameLobby::LobbyError(FormatString(LoadResStr("IDS_MSG_CMD_JOINPLR_NOFILE"), plrPath.getData()).getData());
		}
		else
			::Network.Players.JoinLocalPlayer(plrPath.getData());
		return true;
	}
	if (!Game.IsRunning && SEqualNoCase(szCmdName, "plrclr"))
	{
		// get player name from input text
		int iSepPos = SCharPos(' ', pCmdPar, 0);
		C4PlayerInfo *pNfo=nullptr;
		int32_t idLocalClient = -1;
		if (::Network.Clients.GetLocal()) idLocalClient = ::Network.Clients.GetLocal()->getID();
		if (iSepPos>0)
		{
			// a player name is given: Parse it
			StdStrBuf sPlrName;
			sPlrName.Copy(pCmdPar, iSepPos);
			pCmdPar += iSepPos+1; int32_t id=0;
			while ((pNfo = Game.PlayerInfos.GetNextPlayerInfoByID(id)))
			{
				id = pNfo->GetID();
				if (WildcardMatch(sPlrName.getData(), pNfo->GetName())) break;
			}
		}
		else
			// no player name: Set local player
			pNfo = Game.PlayerInfos.GetPrimaryInfoByClientID(idLocalClient);
		C4ClientPlayerInfos *pCltNfo=nullptr;
		if (pNfo) pCltNfo = Game.PlayerInfos.GetClientInfoByPlayerID(pNfo->GetID());
		if (!pCltNfo)
		{
			C4GameLobby::LobbyError(LoadResStr("IDS_MSG_CMD_PLRCLR_NOPLAYER"));
		}
		else
		{
			// may color of this client be set?
			if (pCltNfo->GetClientID() != idLocalClient && !::Network.isHost())
			{
				C4GameLobby::LobbyError(LoadResStr("IDS_MSG_CMD_PLRCLR_NOACCESS"));
			}
			else
			{
				// get color to set
				uint32_t dwNewClr;
				if (sscanf(pCmdPar, "%x", &dwNewClr) != 1)
				{
					C4GameLobby::LobbyError(LoadResStr("IDS_MSG_CMD_PLRCLR_USAGE"));
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
		return true;
	}
	if (!Game.IsRunning && SEqualNoCase(szCmdName, "start"))
	{
		// timeout given?
		int32_t iTimeout = Config.Lobby.CountdownTime;
		if (!::Network.isHost())
			C4GameLobby::LobbyError(LoadResStr("IDS_MSG_CMD_HOSTONLY"));
		else if (pCmdPar && *pCmdPar && (!sscanf(pCmdPar, "%d", &iTimeout) || iTimeout<0))
			C4GameLobby::LobbyError(LoadResStr("IDS_MSG_CMD_START_USAGE"));
		else if (pLobby)
		{
			// abort previous countdown
			if (::Network.isLobbyCountDown()) ::Network.AbortLobbyCountdown();
			// start new countdown (aborts previous if necessary)
			pLobby->Start(iTimeout);
		}
		else
		{
			if (iTimeout)
				::Network.StartLobbyCountdown(iTimeout);
			else
				::Network.Start();
		}
		return true;
	}
	if (!Game.IsRunning && SEqualNoCase(szCmdName, "abort"))
	{
		if (!::Network.isHost())
			C4GameLobby::LobbyError(LoadResStr("IDS_MSG_CMD_HOSTONLY"));
		else if (::Network.isLobbyCountDown())
			::Network.AbortLobbyCountdown();
		else
			C4GameLobby::LobbyError(LoadResStr("IDS_MSG_CMD_ABORT_NOCOUNTDOWN"));
		return true;
	}

	if (SEqual(szCmdName, "help"))
	{
		Log(LoadResStr(pLobby ? "IDS_TEXT_COMMANDSAVAILABLEDURINGLO" : "IDS_TEXT_COMMANDSAVAILABLEDURINGGA"));
		if (!Game.IsRunning)
		{
			LogF("/start [time] - %s", LoadResStr("IDS_TEXT_STARTTHEROUNDWITHSPECIFIE"));
			LogF("/abort - %s", LoadResStr("IDS_TEXT_ABORTSTARTCOUNTDOWN"));
			LogF("/alert - %s", LoadResStr("IDS_TEXT_ALERTTHEHOSTIFTHEHOSTISAW"));
			LogF("/joinplr [filename] - %s", LoadResStr("IDS_TEXT_JOINALOCALPLAYERFROMTHESP"));
			LogF("/plrclr [player] [RGB] - %s", LoadResStr("IDS_TEXT_CHANGETHECOLOROFTHESPECIF"));
			LogF("/plrclr [RGB] - %s", LoadResStr("IDS_TEXT_CHANGEYOUROWNPLAYERCOLOR"));
		}
		else
		{
			LogF("/fast [x] - %s", LoadResStr("IDS_TEXT_SETTOFASTMODESKIPPINGXFRA"));
			LogF("/slow - %s", LoadResStr("IDS_TEXT_SETTONORMALSPEEDMODE"));
			LogF("/chart - %s", LoadResStr("IDS_TEXT_DISPLAYNETWORKSTATISTICS"));
			LogF("/nodebug - %s", LoadResStr("IDS_TEXT_PREVENTDEBUGMODEINTHISROU"));
			LogF("/script [script] - %s", LoadResStr("IDS_TEXT_EXECUTEASCRIPTCOMMAND"));
			LogF("/screenshot [zoom] - %s", LoadResStr("IDS_TEXT_SAFEZOOMEDFULLSCREENSHOT"));
		}
		LogF("/kick [client] - %s", LoadResStr("IDS_TEXT_KICKTHESPECIFIEDCLIENT"));
		LogF("/observer [client] - %s", LoadResStr("IDS_TEXT_SETTHESPECIFIEDCLIENTTOOB"));
		LogF("/me [action] - %s", LoadResStr("IDS_TEXT_PERFORMANACTIONINYOURNAME"));
		LogF("/sound [sound] - %s", LoadResStr("IDS_TEXT_PLAYASOUNDFROMTHEGLOBALSO"));
		if (Game.IsRunning) LogF("/private [player] [message] - %s", LoadResStr("IDS_MSG_SENDAPRIVATEMESSAGETOTHES"));
		LogF("/team [message] - %s", LoadResStr("IDS_MSG_SENDAPRIVATEMESSAGETOYOUR"));
		LogF("/set comment [comment] - %s", LoadResStr("IDS_TEXT_SETANEWNETWORKCOMMENT"));
		LogF("/set password [password] - %s", LoadResStr("IDS_TEXT_SETANEWNETWORKPASSWORD"));
		LogF("/set maxplayer [number] - %s", LoadResStr("IDS_TEXT_SETANEWMAXIMUMNUMBEROFPLA"));
		LogF("/todo [text] - %s", LoadResStr("IDS_TEXT_ADDTODO"));
		LogF("/clear - %s", LoadResStr("IDS_MSG_CLEARTHEMESSAGEBOARD"));
		return true;
	}
	// dev-scripts
	if (SEqual(szCmdName, "script"))
	{
		if (!Game.IsRunning) return false;
		if (!Game.DebugMode) return false;

		::Control.DoInput(CID_Script, new C4ControlScript(pCmdPar, C4ControlScript::SCOPE_Console), CDT_Decide);
		return true;
	}
	// set runtime properties
	if (SEqual(szCmdName, "set"))
	{
		if (SEqual2(pCmdPar, "maxplayer "))
		{
			if (::Control.isCtrlHost())
			{
				if (atoi(pCmdPar+10) == 0 && !SEqual(pCmdPar+10, "0"))
				{
					Log("Syntax: /set maxplayer count");
					return false;
				}
				::Control.DoInput(CID_Set,
				                  new C4ControlSet(C4CVT_MaxPlayer, atoi(pCmdPar+10)),
				                  CDT_Decide);
				return true;
			}
		}
		if (SEqual2(pCmdPar, "comment ") || SEqual(pCmdPar, "comment"))
		{
			if (!::Network.isEnabled() || !::Network.isHost()) return false;
			// Set in configuration, update reference
			Config.Network.Comment.CopyValidated(pCmdPar[7] ? (pCmdPar+8) : "");
			::Network.InvalidateReference();
			Log(LoadResStr("IDS_NET_COMMENTCHANGED"));
			return true;
		}
		if (SEqual2(pCmdPar, "password ") || SEqual(pCmdPar, "password"))
		{
			if (!::Network.isEnabled() || !::Network.isHost()) return false;
			::Network.SetPassword(pCmdPar[8] ? (pCmdPar+9) : nullptr);
			if (pLobby) pLobby->UpdatePassword();
			return true;
		}
		// unknown property
		return false;
	}
	// get szen from network folder - not in lobby; use res tab there
	if (SEqual(szCmdName, "netgetscen"))
	{
		if (::Network.isEnabled() && !::Network.isHost() && !pLobby)
		{
			const C4Network2ResCore *pResCoreScen = Game.Parameters.Scenario.getResCore();
			if (pResCoreScen)
			{
				C4Network2Res::Ref pScenario = ::Network.ResList.getRefRes(pResCoreScen->getID());
				if (pScenario)
					if (C4Group_CopyItem(pScenario->getFile(), Config.AtUserDataPath(GetFilename(Game.ScenarioFilename))))
					{
						LogF(LoadResStr("IDS_MSG_CMD_NETGETSCEN_SAVED"), Config.AtUserDataPath(GetFilename(Game.ScenarioFilename)));
						return true;
					}
			}
		}
		return false;
	}
	// clear message board
	if (SEqual(szCmdName, "clear"))
	{
		// lobby
		if (pLobby)
		{
			pLobby->ClearLog();
		}
		// fullscreen
		else if (::GraphicsSystem.MessageBoard)
			::GraphicsSystem.MessageBoard->ClearLog();
		else
		{
			// EM mode
			Console.ClearLog();
		}
		return true;
	}
	// kick client
	if (SEqual(szCmdName, "kick"))
	{
		if (::Network.isEnabled() && ::Network.isHost())
		{
			// find client
			C4Client *pClient = Game.Clients.getClientByName(pCmdPar);
			if (!pClient)
			{
				LogF(LoadResStr("IDS_MSG_CMD_NOCLIENT"), pCmdPar);
				return false;
			}
			// league: Kick needs voting
			if (Game.Parameters.isLeague() && ::Players.GetAtClient(pClient->getID()))
				::Network.Vote(VT_Kick, true, pClient->getID());
			else
				// add control
				Game.Clients.CtrlRemove(pClient, LoadResStr("IDS_MSG_KICKFROMMSGBOARD"));
		}
		return true;
	}
	// set fast mode
	if (SEqual(szCmdName, "fast"))
	{
		if (!Game.IsRunning) return false;
		int32_t iFS;
		if ((iFS=atoi(pCmdPar)) == 0) return false;
		// set frameskip and fullspeed flag
		Game.FrameSkip=Clamp<int32_t>(iFS,1,500);
		Game.FullSpeed=true;
		// start calculation immediatly
		Application.NextTick();
		return true;
	}
	// reset fast mode
	if (SEqual(szCmdName, "slow"))
	{
		if (!Game.IsRunning) return false;
		Game.FullSpeed=false;
		Game.FrameSkip=1;
		return true;
	}

	if (SEqual(szCmdName, "nodebug"))
	{
		if (!Game.IsRunning) return false;
		::Control.DoInput(CID_Set, new C4ControlSet(C4CVT_DisableDebug, 0), CDT_Decide);
		return true;
	}

	// kick/activate/deactivate/observer
	if (SEqual(szCmdName, "activate") || SEqual(szCmdName, "deactivate") || SEqual(szCmdName, "observer"))
	{
		if (!::Network.isEnabled() || !::Network.isHost())
			{ Log(LoadResStr("IDS_MSG_CMD_HOSTONLY")); return false; }
		// search for client
		C4Client *pClient = Game.Clients.getClientByName(pCmdPar);
		if (!pClient)
		{
			LogF(LoadResStr("IDS_MSG_CMD_NOCLIENT"), pCmdPar);
			return false;
		}
		// what to do?
		C4ControlClientUpdate *pCtrl = nullptr;
		if (szCmdName[0] == 'a') // activate
			pCtrl = new C4ControlClientUpdate(pClient->getID(), CUT_Activate, true);
		else if (szCmdName[0] == 'd' && !Game.Parameters.isLeague()) // deactivate
			pCtrl = new C4ControlClientUpdate(pClient->getID(), CUT_Activate, false);
		else if (szCmdName[0] == 'o' && !Game.Parameters.isLeague()) // observer
			pCtrl = new C4ControlClientUpdate(pClient->getID(), CUT_SetObserver);
		// perform it
		if (pCtrl)
			::Control.DoInput(CID_ClientUpdate, pCtrl, CDT_Sync);
		else
			Log(LoadResStr("IDS_LOG_COMMANDNOTALLOWEDINLEAGUE"));
		return true;
	}

	// control mode
	if (SEqual(szCmdName, "centralctrl") || SEqual(szCmdName, "decentralctrl")  || SEqual(szCmdName, "asyncctrl"))
	{
		if (!::Network.isEnabled() || !::Network.isHost())
			{ Log(LoadResStr("IDS_MSG_CMD_HOSTONLY")); return false; }
		::Network.SetCtrlMode(*szCmdName == 'c' ? CNM_Central : *szCmdName == 'd' ? CNM_Decentral : CNM_Async);
		return true;
	}

	// show chart
	if (Game.IsRunning)
		if (SEqual(szCmdName, "chart"))
			return Game.ToggleChart();

	// whole map screenshot
	if (SEqual(szCmdName, "screenshot"))
	{
		double zoom = atof(pCmdPar);
		if (zoom<=0) zoom = 2;
		::GraphicsSystem.SaveScreenshot(true, zoom);
		return true;
	}

	// add to TODO list
	if (SEqual(szCmdName, "todo"))
	{
		// must add something
		if (!pCmdPar || !*pCmdPar) return false;
		// try writing main file (usually {SCENARIO}/TODO.txt); if access is not possible, e.g. because scenario is packed, write to alternate file
		const char *todo_filenames[] = { ::Config.Developer.TodoFilename, ::Config.Developer.AltTodoFilename };
		bool success = false;
		for (auto & i : todo_filenames)
		{
			StdCopyStrBuf todo_filename(i);
			todo_filename.Replace("{USERPATH}", Config.General.UserDataPath);
			int replacements = todo_filename.Replace("{SCENARIO}", Game.ScenarioFile.GetFullName().getData());
			// sanity checks for writing scenario TODO file
			if (replacements)
			{
				// entered in editor with no file open?
				if (!::Game.ScenarioFile.IsOpen()) continue;
				// not into packed
				if (::Game.ScenarioFile.IsPacked()) continue;
				// not into temp network file
				if (::Control.isNetwork() && !::Control.isCtrlHost()) continue;
			}
			// try to append. May fail e.g. on packed scenario file, name getting too long, etc. Then fallback to alternate location.
			CStdFile todo_file;
			if (!todo_file.Append(todo_filename.getData())) continue;
			if (!todo_file.WriteString(pCmdPar)) continue;
			// check on file close because CStdFile may do a delayed write
			if (!todo_file.Close()) continue;
			success = true;
			break;
		}
		// no message on success to avoid cluttering the chat during debug sessions
		if (!success) Log(LoadResStr("IDS_ERR_TODO"));
		return true;
	}

	// custom command
	C4MessageBoardCommand *pCmd;
	if (Game.IsRunning)
		if ((pCmd = GetCommand(szCmdName)))
		{
			// get player number of first local player; if multiple players
			// share one computer, we can't distinguish between them anyway
			int32_t player_num = NO_OWNER;
			C4Player *player = ::Players.GetLocalByIndex(0);
			if (player) player_num = player->Number;

			// send command to network
			::Control.DoInput(CID_MsgBoardCmd, new C4ControlMsgBoardCmd(szCmdName, pCmdPar, player_num), CDT_Decide);

			// ok
			return true;
		}

	// unknown command
	StdStrBuf sErr; sErr.Format(LoadResStr("IDS_ERR_UNKNOWNCMD"), szCmdName);
	if (pLobby) pLobby->OnError(sErr.getData()); else Log(sErr.getData());
	return false;
}

void C4MessageInput::AddCommand(const char *strCommand, const char *strScript)
{
	if (GetCommand(strCommand)) return;
	// create entry
	C4MessageBoardCommand *pCmd = new C4MessageBoardCommand();
	SCopy(strCommand, pCmd->Name, C4MaxName);
	SCopy(strScript, pCmd->Script, _MAX_FNAME+30);
	// add to list
	pCmd->Next = pCommands; pCommands = pCmd;
}

C4MessageBoardCommand *C4MessageInput::GetCommand(const char *strName)
{
	for (C4MessageBoardCommand *pCmd = pCommands; pCmd; pCmd = pCmd->Next)
		if (SEqual(pCmd->Name, strName))
			return pCmd;
	return nullptr;
}

void C4MessageInput::ClearPointers(C4Object *pObj)
{
	// target object loose? stop input
	C4ChatInputDialog *pDlg = GetTypeIn();
	if (pDlg && pDlg->GetScriptTargetObject() == pObj) CloseTypeIn();
}

void C4MessageInput::AbortMsgBoardQuery(C4Object *pObj, int32_t iPlr)
{
	// close typein if it is used for the given parameters
	C4ChatInputDialog *pDlg = GetTypeIn();
	if (pDlg && pDlg->IsScriptQueried() && pDlg->GetScriptTargetObject() == pObj && pDlg->GetScriptTargetPlayer() == iPlr) CloseTypeIn();
}

void C4MessageInput::StoreBackBuffer(const char *szMessage)
{
	if (!szMessage || !szMessage[0]) return;
	int32_t i,cnt;
	// Check: Remove doubled buffer
	for (i=0; i<C4MSGB_BackBufferMax-1; ++i)
		if (SEqual(BackBuffer[i], szMessage))
			break;
	// Move up buffers
	for (cnt=i; cnt>0; cnt--) SCopy(BackBuffer[cnt-1],BackBuffer[cnt]);
	// Add message
	SCopy(szMessage,BackBuffer[0], C4MaxMessage);
}

const char *C4MessageInput::GetBackBuffer(int32_t iIndex)
{
	if (!Inside<int32_t>(iIndex, 0, C4MSGB_BackBufferMax-1)) return nullptr;
	return BackBuffer[iIndex];
}

C4MessageBoardCommand::C4MessageBoardCommand()
{
	Name[0] = '\0'; Script[0] = '\0'; Next = nullptr;
}

void C4MessageBoardQuery::CompileFunc(StdCompiler *pComp)
{
	// note that this CompileFunc does not save the fAnswered-flag, so pending message board queries will be re-asked when resuming SaveGames
	pComp->Separator(StdCompiler::SEP_START); // '('
	// callback object number
	pComp->Value(CallbackObj); pComp->Separator();
	// input query string
	pComp->Value(sInputQuery); pComp->Separator();
	// options
	pComp->Value(fIsUppercase);
	// list end
	pComp->Separator(StdCompiler::SEP_END); // ')'
}

C4MessageInput MessageInput;
