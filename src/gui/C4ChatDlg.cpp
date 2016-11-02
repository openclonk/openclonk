/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// IRC client dialog

#include "C4Include.h"
#include "gui/C4ChatDlg.h"

#include "game/C4Application.h"
#include "lib/C4InputValidation.h"
#include "network/C4Network2IRC.h"
#include "gui/C4MessageInput.h"
#include "graphics/C4GraphicsResource.h"

/* C4ChatControl::ChatSheet::NickItem */

C4ChatControl::ChatSheet::NickItem::NickItem(class C4Network2IRCUser *pByUser) : pStatusIcon(nullptr), pNameLabel(nullptr), fFlaggedExisting(false), iStatus(0)
{
	// create elements - will be positioned when resized
	C4Rect rcDefault(0,0,10,10);
	AddElement(pStatusIcon = new C4GUI::Icon(rcDefault, C4GUI::Ico_None));
	AddElement(pNameLabel = new C4GUI::Label("", rcDefault, ALeft, C4GUI_CaptionFontClr, nullptr, false, false, false));
	// set height (pos and width set when added to the list)
	CStdFont *pUseFont = &::GraphicsResource.TextFont;
	rcBounds.Set(0,0, 100,pUseFont->GetLineHeight());
	// initial update
	Update(pByUser);
}

void C4ChatControl::ChatSheet::NickItem::UpdateOwnPos()
{
	typedef C4GUI::Window ParentClass;
	ParentClass::UpdateOwnPos();
	// reposition elements
	if (pStatusIcon && pNameLabel)
	{
		C4GUI::ComponentAligner caMain(GetContainedClientRect(), 1,0);
		pStatusIcon->SetBounds(caMain.GetFromLeft(caMain.GetInnerHeight()));
		pNameLabel->SetBounds(caMain.GetAll());
	}
}

void C4ChatControl::ChatSheet::NickItem::Update(class C4Network2IRCUser *pByUser)
{
	// set status icon
	const char *szPrefix = pByUser->getPrefix();
	if (!szPrefix) szPrefix = "";
	C4GUI::Icons eStatusIcon;
	switch (*szPrefix)
	{
	case '!': eStatusIcon = C4GUI::Ico_Rank1; iStatus = 4; break;
	case '@': eStatusIcon = C4GUI::Ico_Rank2; iStatus = 3; break;
	case '%': eStatusIcon = C4GUI::Ico_Rank3; iStatus = 2; break;
	case '+': eStatusIcon = C4GUI::Ico_AddPlr; iStatus = 1; break;
	case '\0': eStatusIcon = C4GUI::Ico_Player; iStatus = 0; break;
	default:  eStatusIcon = C4GUI::Ico_Player; iStatus = 0;  break;
	}
	pStatusIcon->SetIcon(eStatusIcon);
	// set name
	pNameLabel->SetText(pByUser->getName());
	// tooltip is status+name
	SetToolTip(FormatString("%s%s", szPrefix, pByUser->getName()).getData());
}

int32_t C4ChatControl::ChatSheet::NickItem::SortFunc(const C4GUI::Element *pEl1, const C4GUI::Element *pEl2, void *)
{
	const NickItem *pNickItem1 = static_cast<const NickItem *>(pEl1);
	const NickItem *pNickItem2 = static_cast<const NickItem *>(pEl2);
	int32_t s1 = pNickItem1->GetStatus(), s2 = pNickItem2->GetStatus();
	if (s1 != s2) return s1 - s2;
	return stricmp(pNickItem2->GetNick(), pNickItem1->GetNick());
}


/* C4ChatControl::ChatSheet */

C4ChatControl::ChatSheet::ChatSheet(C4ChatControl *pChatControl, const char *szTitle, const char *szIdent, SheetType eType)
		: C4GUI::Tabular::Sheet(szTitle, C4Rect(0,0,10,10), C4GUI::Ico_None, true, false), pChatControl(pChatControl), pNickList(nullptr), pInputLbl(nullptr), iBackBufferIndex(-1), eType(eType), fHasUnread(false)
{
	if (szIdent) sIdent.Copy(szIdent);
	// create elements - positioned later
	C4Rect rcDefault(0,0,10,10);
	pChatBox = new C4GUI::TextWindow(rcDefault,0,0,0,100,4096,"  ",false,0,0,true);
	pChatBox->SetDecoration(false, false, nullptr, false);
	AddElement(pChatBox);
	if (eType == CS_Channel)
	{
		pNickList = new C4GUI::ListBox(rcDefault);
		pNickList->SetDecoration(false, nullptr, true, false);
		pNickList->SetSelectionDblClickFn(new C4GUI::CallbackHandler<C4ChatControl::ChatSheet>(this, &C4ChatControl::ChatSheet::OnNickDblClick));
		AddElement(pNickList);
	}
	if (eType != CS_Server)
		pInputLbl = new C4GUI::WoodenLabel(LoadResStr("IDS_DLG_CHAT"), rcDefault, C4GUI_CaptionFontClr, &::GraphicsResource.TextFont);
	pInputEdit = new C4GUI::CallbackEdit<C4ChatControl::ChatSheet>(rcDefault, this, &C4ChatControl::ChatSheet::OnChatInput);
	pInputEdit->SetToolTip(LoadResStr("IDS_DLGTIP_CHAT"));
	if (pInputLbl)
	{
		pInputLbl->SetToolTip(LoadResStr("IDS_DLGTIP_CHAT"));
		pInputLbl->SetClickFocusControl(pInputEdit);
		AddElement(pInputLbl);
	}
	AddElement(pInputEdit);
	// key bindings
	pKeyHistoryUp  = new C4KeyBinding(C4KeyCodeEx(K_UP  ), "ChatHistoryUp"  , KEYSCOPE_Gui, new C4GUI::DlgKeyCBEx<C4ChatControl::ChatSheet, bool>(*this, true , &C4ChatControl::ChatSheet::KeyHistoryUpDown), C4CustomKey::PRIO_CtrlOverride);
	pKeyHistoryDown= new C4KeyBinding(C4KeyCodeEx(K_DOWN), "ChatHistoryDown", KEYSCOPE_Gui, new C4GUI::DlgKeyCBEx<C4ChatControl::ChatSheet, bool>(*this, false, &C4ChatControl::ChatSheet::KeyHistoryUpDown), C4CustomKey::PRIO_CtrlOverride);
}

C4ChatControl::ChatSheet::~ChatSheet()
{
	delete pKeyHistoryUp;
	delete pKeyHistoryDown;
}

void C4ChatControl::ChatSheet::UpdateSize()
{
	// parent update
	typedef C4GUI::Window ParentClass;
	ParentClass::UpdateSize();
	// position child elements
	C4GUI::ComponentAligner caMain(GetContainedClientRect(), 0,0);
	C4GUI::ComponentAligner caChat(caMain.GetFromBottom(C4GUI::Edit::GetDefaultEditHeight()), 0,0);
	if (pNickList) pNickList->SetBounds(caMain.GetFromRight(std::max<int32_t>(caMain.GetInnerWidth()/5, 100)));
	pChatBox->SetBounds(caMain.GetAll());
	if (pInputLbl) pInputLbl->SetBounds(caChat.GetFromLeft(40));
	pInputEdit->SetBounds(caChat.GetAll());
}

void C4ChatControl::ChatSheet::OnShown(bool fByUser)
{
	ResetUnread();
	if (fByUser)
	{
		Update(true);
		pChatControl->UpdateTitle();
	}
}

C4GUI::Edit::InputResult C4ChatControl::ChatSheet::OnChatInput(C4GUI::Edit *edt, bool fPasting, bool fPastingMore)
{
	C4GUI::Edit::InputResult eResult = C4GUI::Edit::IR_None;
	// get edit text
	const char *szInputText = pInputEdit->GetText();
	// no input?
	if (!szInputText || !*szInputText)
	{
		// do some error sound then
		DoError(nullptr);
	}
	else
	{
		// remember in history
		::MessageInput.StoreBackBuffer(szInputText);
		// forward to chat control for processing
		if (!pChatControl->ProcessInput(szInputText, this)) eResult = C4GUI::Edit::IR_Abort;
	}
	// clear edit field after text has been processed
	pInputEdit->SelectAll(); pInputEdit->DeleteSelection();
	// reset backbuffer-index of chat history
	iBackBufferIndex = -1;
	// OK, on we go
	return eResult;
}

bool C4ChatControl::ChatSheet::KeyHistoryUpDown(bool fUp)
{
	// chat input only
	if (!IsFocused(pInputEdit)) return false;
	pInputEdit->SelectAll(); pInputEdit->DeleteSelection();
	const char *szPrevInput = ::MessageInput.GetBackBuffer(fUp ? (++iBackBufferIndex) : (--iBackBufferIndex));
	if (!szPrevInput || !*szPrevInput)
		iBackBufferIndex = -1;
	else
	{
		pInputEdit->InsertText(szPrevInput, true);
		pInputEdit->SelectAll();
	}
	return true;
}

void C4ChatControl::ChatSheet::OnNickDblClick(class C4GUI::Element *pEl)
{
	if (!pEl) return;
	NickItem *pNickItem = static_cast<NickItem *>(pEl);
	pChatControl->OpenQuery(pNickItem->GetNick(), true, nullptr);
}

void C4ChatControl::ChatSheet::AddTextLine(const char *szText, uint32_t dwClr)
{
	// strip stuff that would confuse Clonk
	StdStrBuf sText; sText.Copy(szText);
	for (char c='\x01'; c<' '; ++c)
		sText.ReplaceChar(c, ' ');
	// convert incoming Windows-1252
	sText.EnsureUnicode();
	// add text line to chat box
	CStdFont *pUseFont = &::GraphicsResource.TextFont;
	pChatBox->AddTextLine(sText.getData(), pUseFont, dwClr, true, false);
	pChatBox->ScrollToBottom();
	// sheet now has unread messages if not selected
	if (!fHasUnread && !IsActiveSheet())
	{
		fHasUnread = true;
		SetCaptionColor(C4GUI_Caption2FontClr);
	}
}

void C4ChatControl::ChatSheet::ResetUnread()
{
	// mark messages as read
	if (fHasUnread)
	{
		fHasUnread = false;
		SetCaptionColor();
	}
}

void C4ChatControl::ChatSheet::DoError(const char *szError)
{
	if (szError)
	{
		AddTextLine(szError, C4GUI_ErrorFontClr);
	}
	C4GUI::GUISound("UI::Error");
}

void C4ChatControl::ChatSheet::Update(bool fLock)
{
	// lock IRC client data if desired
	if (fLock)
	{
		CStdLock Lock(pChatControl->getIRCClient()->getCSec());
		Update(false);
		return;
	}
	// only channels need updates
	if (eType == CS_Channel)
	{
		C4Network2IRCChannel *pIRCChan = pChatControl->getIRCClient()->getChannel(GetIdent());
		if (pIRCChan)
		{
			// update user list (if not locked, becuase it's being received)
			if (!pIRCChan->isUsersLocked()) UpdateUsers(pIRCChan->getUsers());
			// update topic
			const char *szTopic = pIRCChan->getTopic();
			sChatTitle.Format("%s%s%s", sIdent.getData(), szTopic ? ": " : "", szTopic ? szTopic : "");
			sChatTitle.EnsureUnicode();
		}
	}
}

void C4ChatControl::ChatSheet::UpdateUsers(C4Network2IRCUser *pUsers)
{
	NickItem *pNickItem, *pNextNickItem;
	// update existing users
	for (; pUsers; pUsers = pUsers->getNext())
	{
		if ((pNickItem = GetNickItem(pUsers->getName())))
		{
			pNickItem->Update(pUsers);
		}
		else
		{
			// new user!
			pNickItem = new NickItem(pUsers);
			pNickList->AddElement(pNickItem);
		}
		pNickItem->SetFlaggedExisting(true);
	}
	// remove left users
	pNextNickItem = GetFirstNickItem();
	while ((pNickItem = pNextNickItem))
	{
		pNextNickItem = GetNextNickItem(pNickItem);
		if (!pNickItem->IsFlaggedExisting())
		{
			// this user left
			delete pNickItem;
		}
		else
		{
			// user didn't leave; reset flag for next check
			pNickItem->SetFlaggedExisting(false);
		}
	}
	// sort the rest
	pNickList->SortElements(&NickItem::SortFunc, nullptr);
}

void C4ChatControl::ChatSheet::UserClose()
{
	typedef C4GUI::Tabular::Sheet ParentClass;
	switch (eType)
	{
	case CS_Server:
		// closing server window? Always forward to control
		pChatControl->UserQueryQuit();
		break;

	case CS_Channel:
		// channel: Send part. Close done by server.
		pChatControl->getIRCClient()->Part(sIdent.getData());
		break;

	case CS_Query:
		// query: Always allow simple close
		ParentClass::UserClose();
		break;
	}
}

C4ChatControl::ChatSheet::NickItem *C4ChatControl::ChatSheet::GetNickItem(const char *szByNick)
{
	// find by name
	for (NickItem *pNickItem = GetFirstNickItem(); pNickItem; pNickItem = GetNextNickItem(pNickItem))
		if (SEqualNoCase(pNickItem->GetNick(), szByNick))
			return pNickItem;
	// not found
	return nullptr;
}


/* C4ChatControl */

C4ChatControl::C4ChatControl(C4Network2IRCClient *pnIRCClient) : C4GUI::Window(), pTitleChangeBC(nullptr), pIRCClient(pnIRCClient), fInitialMessagesReceived(false)
{
	// create elements - positioned later
	C4Rect rcDefault(0,0,10,10);
	// main tabular tabs between chat components (login and channels)
	pTabMain = new C4GUI::Tabular(rcDefault, C4GUI::Tabular::tbNone);
	pTabMain->SetDrawDecoration(false);
	pTabMain->SetSheetMargin(0);
	AddElement(pTabMain);
	C4GUI::Tabular::Sheet *pSheetLogin = pTabMain->AddSheet(nullptr);
	C4GUI::Tabular::Sheet *pSheetChats = pTabMain->AddSheet(nullptr);
	// login sheet
	CStdFont *pUseFont = &::GraphicsResource.TextFont;
	pSheetLogin->AddElement(pLblLoginNick = new C4GUI::Label(LoadResStr("IDS_CTL_NICK"), rcDefault, ALeft, C4GUI_CaptionFontClr, pUseFont, false, true));
	pSheetLogin->AddElement(pEdtLoginNick = new C4GUI::CallbackEdit<C4ChatControl>(rcDefault, this, &C4ChatControl::OnLoginDataEnter));
	pSheetLogin->AddElement(pLblLoginPass = new C4GUI::Label(LoadResStr("IDS_CTL_PASSWORDOPTIONAL"), rcDefault, ALeft, C4GUI_CaptionFontClr, pUseFont, false, true));
	pSheetLogin->AddElement(pEdtLoginPass = new C4GUI::CallbackEdit<C4ChatControl>(rcDefault, this, &C4ChatControl::OnLoginDataEnter));
	pEdtLoginPass->SetPasswordMask('*');
	pSheetLogin->AddElement(pLblLoginRealName = new C4GUI::Label(LoadResStr("IDS_CTL_REALNAME"), rcDefault, ALeft, C4GUI_CaptionFontClr, pUseFont, false, true));
	pSheetLogin->AddElement(pEdtLoginRealName = new C4GUI::CallbackEdit<C4ChatControl>(rcDefault, this, &C4ChatControl::OnLoginDataEnter));
	pSheetLogin->AddElement(pLblLoginChannel = new C4GUI::Label(LoadResStr("IDS_CTL_CHANNEL"), rcDefault, ALeft, C4GUI_CaptionFontClr, pUseFont, false, true));
	pSheetLogin->AddElement(pEdtLoginChannel = new C4GUI::CallbackEdit<C4ChatControl>(rcDefault, this, &C4ChatControl::OnLoginDataEnter));
	pSheetLogin->AddElement(pBtnLogin = new C4GUI::CallbackButtonEx<C4ChatControl>(LoadResStr("IDS_BTN_CONNECT"), rcDefault, this, &C4ChatControl::OnConnectBtn));
	// channel/query tabular
	pTabChats = new C4GUI::Tabular(rcDefault, C4GUI::Tabular::tbTop);
	pTabChats->SetSheetMargin(0);
	pSheetChats->AddElement(pTabChats);
	// initial connection values
	const char *szNick = Config.IRC.Nick, *szRealName = Config.IRC.RealName;
	StdStrBuf sNick, sRealName;
	if (!*szNick) szNick = Config.Network.Nick.getData();
	pEdtLoginNick->SetText(szNick, false);
	pEdtLoginRealName->SetText(szRealName, false);
	pEdtLoginChannel->SetText(Config.IRC.Channel, false);
	// initial sheets
	ClearChatSheets();
	// set IRC event callback
	Application.InteractiveThread.SetCallback(Ev_IRC_Message, this);
}

C4ChatControl::~C4ChatControl()
{
	Application.InteractiveThread.ClearCallback(Ev_IRC_Message, this);
	delete pTitleChangeBC;
}

void C4ChatControl::SetTitleChangeCB(C4GUI::BaseInputCallback *pNewCB)
{
	delete pTitleChangeBC;
	pTitleChangeBC = pNewCB;
	// initial title
	if (pTitleChangeBC) pTitleChangeBC->OnOK(sTitle);
}

void C4ChatControl::UpdateSize()
{
	// parent update
	typedef C4GUI::Window ParentClass;
	ParentClass::UpdateSize();
	// position child elements
	pTabMain->SetBounds(GetContainedClientRect());
	pTabChats->SetBounds(pTabChats->GetParent()->GetContainedClientRect());
	C4GUI::Tabular::Sheet *pSheetLogin = pTabMain->GetSheet(0);
	C4GUI::ComponentAligner caLoginSheet(pSheetLogin->GetContainedClientRect(), 0,0, false);
	CStdFont *pUseFont = &::GraphicsResource.TextFont;
	int32_t iIndent1 = C4GUI_DefDlgSmallIndent/2, iIndent2 = C4GUI_DefDlgIndent/2;
	int32_t iLoginHgt = pUseFont->GetLineHeight() * 8 + iIndent1*10 + iIndent2*10 + C4GUI_ButtonHgt + 20;
	int32_t iLoginWdt = iLoginHgt*2/3;
	C4GUI::ComponentAligner caLogin(caLoginSheet.GetCentered(std::min<int32_t>(iLoginWdt, caLoginSheet.GetInnerWidth()), std::min<int32_t>(iLoginHgt, caLoginSheet.GetInnerHeight())), iIndent1, iIndent1);
	pLblLoginNick->SetBounds(caLogin.GetFromTop(pUseFont->GetLineHeight()));
	pEdtLoginNick->SetBounds(caLogin.GetFromTop(C4GUI::Edit::GetDefaultEditHeight()));
	caLogin.ExpandTop(2*(iIndent1-iIndent2));
	pLblLoginPass->SetBounds(caLogin.GetFromTop(pUseFont->GetLineHeight()));
	pEdtLoginPass->SetBounds(caLogin.GetFromTop(C4GUI::Edit::GetDefaultEditHeight()));
	caLogin.ExpandTop(2*(iIndent1-iIndent2));
	pLblLoginRealName->SetBounds(caLogin.GetFromTop(pUseFont->GetLineHeight()));
	pEdtLoginRealName->SetBounds(caLogin.GetFromTop(C4GUI::Edit::GetDefaultEditHeight()));
	caLogin.ExpandTop(2*(iIndent1-iIndent2));
	pLblLoginChannel->SetBounds(caLogin.GetFromTop(pUseFont->GetLineHeight()));
	pEdtLoginChannel->SetBounds(caLogin.GetFromTop(C4GUI::Edit::GetDefaultEditHeight()));
	caLogin.ExpandTop(2*(iIndent1-iIndent2));
	pBtnLogin->SetBounds(caLogin.GetFromTop(C4GUI_ButtonHgt, C4GUI_DefButtonWdt));
}

void C4ChatControl::OnShown()
{
	UpdateShownPage();
}

C4GUI::Control *C4ChatControl::GetDefaultControl()
{
	// only return a default control if no control is selected to prevent deselection of other controls
	if (GetDlg()->GetFocus()) return nullptr;
	ChatSheet *pActiveSheet = GetActiveChatSheet();
	if (pActiveSheet) return pActiveSheet->GetInputEdit();
	if (pBtnLogin->IsVisible()) return pBtnLogin;
	return nullptr;
}

C4ChatControl::ChatSheet *C4ChatControl::GetActiveChatSheet()
{
	if (pTabChats->IsVisible())
	{
		C4GUI::Tabular::Sheet *pSheet = pTabChats->GetActiveSheet();
		if (pSheet) return static_cast<ChatSheet *>(pSheet);
	}
	return nullptr;
}

C4ChatControl::ChatSheet *C4ChatControl::GetSheetByIdent(const char *szIdent, C4ChatControl::ChatSheet::SheetType eType)
{
	int32_t i=0; C4GUI::Tabular::Sheet *pSheet; const char *szCheckIdent;
	while ((pSheet = pTabChats->GetSheet(i++)))
	{
		ChatSheet *pChatSheet = static_cast<ChatSheet *>(pSheet);
		if ((szCheckIdent = pChatSheet->GetIdent()))
			if (SEqualNoCase(szCheckIdent, szIdent))
				if (eType == pChatSheet->GetSheetType())
					return pChatSheet;
	}
	return nullptr;
}

C4ChatControl::ChatSheet *C4ChatControl::GetSheetByTitle(const char *szTitle, C4ChatControl::ChatSheet::SheetType eType)
{
	int32_t i=0; C4GUI::Tabular::Sheet *pSheet; const char *szCheckTitle;
	while ((pSheet = pTabChats->GetSheet(i++)))
		if ((szCheckTitle = pSheet->GetTitle()))
			if (SEqualNoCase(szCheckTitle, szTitle))
			{
				ChatSheet *pChatSheet = static_cast<ChatSheet *>(pSheet);
				if (eType == pChatSheet->GetSheetType())
					return pChatSheet;
			}
	return nullptr;
}

C4ChatControl::ChatSheet *C4ChatControl::GetServerSheet()
{
	// server sheet is always the first
	return static_cast<ChatSheet *>(pTabChats->GetSheet(0));
}

C4GUI::Edit::InputResult C4ChatControl::OnLoginDataEnter(C4GUI::Edit *edt, bool fPasting, bool fPastingMore)
{
	// advance focus when user presses enter in one of the login edits
	GetDlg()->AdvanceFocus(false);
	// no more pasting
	return C4GUI::Edit::IR_Abort;
}

void C4ChatControl::OnConnectBtn(C4GUI::Control *btn)
{
	// check parameters
	StdCopyStrBuf sNick(pEdtLoginNick->GetText());
	StdCopyStrBuf sPass(pEdtLoginPass->GetText());
	StdCopyStrBuf sRealName(pEdtLoginRealName->GetText());
	StdCopyStrBuf sChannel (pEdtLoginChannel->GetText());
	StdCopyStrBuf sServer(Config.IRC.Server);
	if (C4InVal::ValidateString(sNick, C4InVal::VAL_IRCName))
	{
		GetScreen()->ShowErrorMessage(LoadResStr("IDS_ERR_INVALIDNICKNAME"));
		GetDlg()->SetFocus(pEdtLoginNick, false);
		return;
	}
	if (sPass.getLength() && C4InVal::ValidateString(sPass, C4InVal::VAL_IRCPass))
	{
		GetScreen()->ShowErrorMessage(LoadResStr("IDS_ERR_INVALIDPASSWORDMAX31CHARA"));
		GetDlg()->SetFocus(pEdtLoginPass, false);
		return;
	}
	if (sChannel.getLength() && C4InVal::ValidateString(sChannel, C4InVal::VAL_IRCChannel))
	{
		GetScreen()->ShowErrorMessage(LoadResStr("IDS_ERR_INVALIDCHANNELNAME"));
		pEdtLoginChannel->SetText(sChannel.getData(), false);
		GetDlg()->SetFocus(pEdtLoginChannel, false);
		return;
	}
	if (!sPass.getLength()) sPass.Clear();
	if (!sChannel.getLength()) sChannel.Clear();
	// store back config values
	SCopy(sNick.getData(), Config.IRC.Nick, CFG_MaxString);
	SCopy(sRealName.getData(), Config.IRC.RealName, CFG_MaxString);
	SCopy(sChannel.getData(), Config.IRC.Channel, CFG_MaxString);
	// show chat warning
	StdStrBuf sWarnMsg;
	sWarnMsg.Format(LoadResStr("IDS_MSG_YOUAREABOUTTOCONNECTTOAPU"), sServer.getData());
	if (!GetScreen()->ShowMessageModal(sWarnMsg.getData(), LoadResStr("IDS_MSG_CHATDISCLAIMER"), C4GUI::MessageDialog::btnOKAbort, C4GUI::Ico_Notify, &Config.Startup.HideMsgIRCDangerous))
		return;
	// set up IRC callback
	pIRCClient->SetNotify(&Application.InteractiveThread);
	// initiate connection
	if (!pIRCClient->Connect(sServer.getData(), sNick.getData(), sRealName.getData(), sPass.getData(), sChannel.getData()))
	{
		GetScreen()->ShowErrorMessage(FormatString(LoadResStr("IDS_ERR_IRCCONNECTIONFAILED"), pIRCClient->GetError()).getData());
		return;
	}
	// enable client execution
	Application.InteractiveThread.AddProc(pIRCClient);
	// reset chat sheets (close queries, etc.)
	ClearChatSheets();
	// connection message
	ChatSheet *pServerSheet = GetServerSheet();
	if (pServerSheet)
	{
		pServerSheet->SetChatTitle(sServer.getData());
		pServerSheet->AddTextLine(FormatString(LoadResStr("IDS_NET_CONNECTING"), sServer.getData(), "").getData(), C4GUI_MessageFontClr);
	}
	// switch to server window
	UpdateShownPage();
}

void C4ChatControl::UpdateShownPage()
{
	if (pIRCClient->IsActive())
	{
		// connected to a server: Show chat window
		pTabMain->SelectSheet(1, false);
		Update();
	}
	else
	{
		// not connected: Login stuff
		pTabMain->SelectSheet((int)0, false);
		UpdateTitle();
	}
}

bool C4ChatControl::IsServiceName(const char *szName)
{
	// return true for some hardcoded list of service names
	if (!szName) return false;
	const char *szServiceNames [] = { "NickServ", "ChanServ", "MemoServ", "HelpServ", "Global", nullptr }, *szServiceName;
	int32_t i = 0;
	while ((szServiceName = szServiceNames[i++]))
		if (SEqualNoCase(szName, szServiceName))
			return true;
	return false;
}

void C4ChatControl::Update()
{
	CStdLock Lock(pIRCClient->getCSec());
	// update channels
	for (C4Network2IRCChannel *pChan = pIRCClient->getFirstChannel(); pChan; pChan = pIRCClient->getNextChannel(pChan))
	{
		ChatSheet *pChanSheet = GetSheetByIdent(pChan->getName(), ChatSheet::CS_Channel);
		if (!pChanSheet)
		{
			// new channel! Create sheet for it
			pTabChats->AddCustomSheet(pChanSheet = new ChatSheet(this, pChan->getName(), pChan->getName(), ChatSheet::CS_Channel));
			// and show immediately
			pTabChats->SelectSheet(pChanSheet, false);
		}
	}
	// remove parted channels
	int32_t i=0; C4GUI::Tabular::Sheet *pSheet;
	while ((pSheet = pTabChats->GetSheet(i++)))
	{
		C4Network2IRCChannel *pIRCChan;
		ChatSheet *pChatSheet = static_cast<ChatSheet *>(pSheet);
		if (pChatSheet->GetSheetType() == ChatSheet::CS_Channel)
			if (!(pIRCChan = pIRCClient->getChannel(pChatSheet->GetTitle())))
			{
				delete pChatSheet;
				--i;
			}
	}
	// retrieve messages: All messages in initial update; only unread in subsequent calls
	C4Network2IRCMessage *pMsg;
	if (fInitialMessagesReceived)
	{
		pMsg = pIRCClient->getUnreadMessageLog();
	}
	else
	{
		pMsg = pIRCClient->getMessageLog();
		fInitialMessagesReceived = true;
	}
	// update messages
	for (; pMsg; pMsg = pMsg->getNext())
	{
		// get target sheet to put message into
		ChatSheet *pChatSheet; StdStrBuf sUser, sIdent;
		bool fMsgToService = false;
		if (pMsg->getType() == MSG_Server)
		{
			// server messages in server sheet
			pChatSheet = GetServerSheet();
		}
		else
		{
			if (pMsg->getType() != MSG_Status) sUser.Copy(pMsg->getSource());
			if (!sUser.SplitAtChar('!', &sIdent)) sIdent.Ref(sUser);
			// message: Either channel or user message (or channel notify). Get correct sheet.
			if (pMsg->isChannel())
			{
				// if no sheet is found, don't create - assume it's an outdated message with the cahnnel window already closed
				pChatSheet = GetSheetByIdent(pMsg->getTarget(), ChatSheet::CS_Channel);
			}
			else if (IsServiceName(sUser.getData()))
			{
				// notices and messages by services always in server sheet
				pChatSheet = GetServerSheet();
			}
			else if (pMsg->getType() == MSG_Notice)
			{
				// notifies in current sheet; default to server sheet
				pChatSheet = GetActiveChatSheet();
				if (!pChatSheet) pChatSheet = GetServerSheet();
			}
			else if (pMsg->getType() == MSG_Status || !sUser.getLength())
			{
				// server message
				pChatSheet = GetServerSheet();
			}
			else if (sUser == pIRCClient->getUserName())
			{
				// private message by myself
				// message to a service into service window; otherwise (new) query
				if (IsServiceName(pMsg->getTarget()))
				{
					pChatSheet = GetServerSheet();
					fMsgToService = true;
				}
				else
				{
					pChatSheet = OpenQuery(pMsg->getTarget(), true, nullptr);
					if (pChatSheet) pChatSheet->SetChatTitle(pMsg->getTarget());
				}
			}
			else
			{
				// private message
				pChatSheet = OpenQuery(sUser.getData(), false, sIdent.getData());
				if (pChatSheet) pChatSheet->SetChatTitle(pMsg->getSource());
			}
		}
		if (pChatSheet)
		{
			// get message formatting and color
			StdStrBuf sMsg; uint32_t dwClr = C4GUI_MessageFontClr;
			switch (pMsg->getType())
			{
			case MSG_Server:
				sMsg.Format("- %s", pMsg->getData());
				break;

			case MSG_Status:
				sMsg.Format("- %s", pMsg->getData());
				dwClr = C4GUI_InactMessageFontClr;
				break;

			case MSG_Notice:
				if (sUser.getLength())
					if (sUser != pIRCClient->getUserName())
						sMsg.Format("-%s- %s", sUser.getData(), pMsg->getData());
					else
						sMsg.Format("-> -%s- %s", pMsg->getTarget(), pMsg->getData());
				else
					sMsg.Format("* %s", pMsg->getData());
				dwClr = C4GUI_NotifyFontClr;
				break;

			case MSG_Message:
				if (fMsgToService)
					sMsg.Format("-> *%s* %s", pMsg->getTarget(), pMsg->getData());
				else if (sUser.getLength())
					sMsg.Format("<%s> %s", sUser.getData(), pMsg->getData());
				else
					sMsg.Format("* %s", pMsg->getData());
				break;

			case MSG_Action:
				if (sUser.getLength())
					sMsg.Format("* %s %s", sUser.getData(), pMsg->getData());
				else
					sMsg.Format("* %s", pMsg->getData());
				break;

			default:
				sMsg.Format("??? %s", pMsg->getData());
				dwClr = C4GUI_ErrorFontClr;
				break;
			}
			pChatSheet->AddTextLine(sMsg.getData(), dwClr);
		}
	}
	// OK; all messages processed. Delete overflow messages.
	pIRCClient->MarkMessageLogRead();
	// update selected channel (users, topic)
	ChatSheet *pActiveSheet = GetActiveChatSheet();
	if (pActiveSheet) pActiveSheet->Update(false);
	// update title
	UpdateTitle();
}

C4ChatControl::ChatSheet *C4ChatControl::OpenQuery(const char *szForNick, bool fSelect, const char *szIdentFallback)
{
	// search existing query first
	ChatSheet *pChatSheet = GetSheetByTitle(szForNick, ChatSheet::CS_Query);
	// not found but ident given? Then search for ident as well
	if (!pChatSheet && szIdentFallback) pChatSheet = GetSheetByIdent(szIdentFallback, ChatSheet::CS_Query);
	// auto-open query if not found
	if (!pChatSheet)
	{
		pTabChats->AddCustomSheet(pChatSheet = new ChatSheet(this, szForNick, szIdentFallback, ChatSheet::CS_Query));
		// initial chat title just user name; changed to user name+ident if a message from the nick arrives
		pChatSheet->SetChatTitle(szForNick);
	}
	else
	{
		// query already open: Update user name if necessary
		pChatSheet->SetTitle(szForNick);
		if (szIdentFallback) pChatSheet->SetIdent(szIdentFallback);
	}
	if (fSelect) pTabChats->SelectSheet(pChatSheet, true);
	return pChatSheet;
}

void C4ChatControl::UpdateTitle()
{
	StdCopyStrBuf sNewTitle;
	if (pTabMain->GetActiveSheetIndex() == 0)
	{
		// login title
		sNewTitle = LoadResStr("IDS_CHAT_NOTCONNECTED");
	}
	else
	{
		// login by active sheet
		ChatSheet *pActiveSheet = GetActiveChatSheet();
		if (pActiveSheet)
		{
			sNewTitle = pActiveSheet->GetChatTitle();
		}
		else
			sNewTitle = "";
	}
	// call update proc only if title changed
	if (sTitle != sNewTitle)
	{
		sTitle.Take(std::move(sNewTitle));
		if (pTitleChangeBC) pTitleChangeBC->OnOK(sTitle);
	}
	// reload the channel join string from config to fetch C4Network2IRCClient's changes
	pEdtLoginChannel->SetText(Config.IRC.Channel, false);
}

bool C4ChatControl::DlgEnter()
{
	// enter on connect button connects
	if (GetDlg()->GetFocus() == pBtnLogin) { OnConnectBtn(pBtnLogin); return true; }
	return false;
}

void C4ChatControl::ClearChatSheets()
{
	pTabChats->ClearSheets();
	// add server sheet
	pTabChats->AddCustomSheet(new ChatSheet(this, LoadResStr("IDS_CHAT_SERVER"), nullptr, ChatSheet::CS_Server));
}

bool C4ChatControl::ProcessInput(const char *szInput, ChatSheet *pChatSheet)
{
	CStdLock Lock(pIRCClient->getCSec());
	// process chat input - return false if no more pasting is to be done (i.e., on /quit or /part in channel)
	bool fResult = true;
	bool fIRCSuccess = true;
	// not connected?
	if (!pIRCClient->IsConnected())
	{
		pChatSheet->DoError(LoadResStr("IDS_ERR_NOTCONNECTEDTOSERVER"));
		return fResult;
	}
	// safety
	if (!szInput || !*szInput || !pChatSheet) return fResult;
	// command?
	if (*szInput == '/' && !SEqual2NoCase(szInput + 1, "me "))
	{
		StdStrBuf sCommand, sParam(""); sCommand.Copy(szInput+1);
		sCommand.SplitAtChar(' ', &sParam);
		if (SEqualNoCase(sCommand.getData(), "help"))
		{
			pChatSheet->DoError(LoadResStr("IDS_ERR_HELPCMD"));
			fResult = false;
		}
		else if (SEqualNoCase(sCommand.getData(), "quit"))
		{
			// query disconnect from IRC server
			fIRCSuccess = pIRCClient->Quit(sParam.getData());
			fResult = false;
		}
		else if (SEqualNoCase(sCommand.getData(), "part"))
		{
			// part channel. Default to current channel if typed within a channel
			if (!sParam.getLength() && pChatSheet->GetSheetType() == ChatSheet::CS_Channel)
			{
				sParam.Copy(pChatSheet->GetIdent());
				fResult = false;
			}
			fIRCSuccess = pIRCClient->Part(sParam.getData());
		}
		else if (SEqualNoCase(sCommand.getData(), "join") || SEqualNoCase(sCommand.getData(), "j"))
		{
			if (!sParam.getLength()) sParam.Copy(Config.IRC.Channel);
			fIRCSuccess = pIRCClient->Join(sParam.getData());
		}
		else if (SEqualNoCase(sCommand.getData(), "notice") || SEqualNoCase(sCommand.getData(), "msg"))
		{
			bool fMsg = SEqualNoCase(sCommand.getData(), "msg");
			StdStrBuf sMsg;
			if (!sParam.SplitAtChar(' ', &sMsg) || !sMsg.getLength())
			{
				pChatSheet->DoError(FormatString(LoadResStr("IDS_ERR_INSUFFICIENTPARAMETERS"), sCommand.getData()).getData());
			}
			else
			{
				if (fMsg)
					fIRCSuccess = pIRCClient->Message(sParam.getData(), sMsg.getData());
				else
					fIRCSuccess = pIRCClient->Notice(sParam.getData(), sMsg.getData());
			}
		}
		else if (SEqualNoCase(sCommand.getData(), "raw"))
		{
			if (!sParam.getLength())
				pChatSheet->DoError(FormatString(LoadResStr("IDS_ERR_INSUFFICIENTPARAMETERS"), sCommand.getData()).getData());
			else
				fIRCSuccess = pIRCClient->Send(sParam.getData());
		}
		else if (SEqualNoCase(sCommand.getData(), "ns") || SEqualNoCase(sCommand.getData(), "cs") || SEqualNoCase(sCommand.getData(), "ms"))
		{
			if (!sParam.getLength())
				pChatSheet->DoError(FormatString(LoadResStr("IDS_ERR_INSUFFICIENTPARAMETERS"), sCommand.getData()).getData());
			else
			{
				const char *szMsgTarget;
				if (SEqualNoCase(sCommand.getData(), "ns")) szMsgTarget = "NickServ";
				else if (SEqualNoCase(sCommand.getData(), "cs")) szMsgTarget = "ChanServ";
				else                                             szMsgTarget = "MemoServ";
				fIRCSuccess = pIRCClient->Message(szMsgTarget, sParam.getData());
			}
		}
		else if (SEqualNoCase(sCommand.getData(), "query") || SEqualNoCase(sCommand.getData(), "q"))
		{
			if (!sParam.getLength())
				pChatSheet->DoError(FormatString(LoadResStr("IDS_ERR_INSUFFICIENTPARAMETERS"), sCommand.getData()).getData());
			else
			{
				StdStrBuf sMsg;
				sParam.SplitAtChar(' ', &sMsg);
				OpenQuery(sParam.getData(), true, nullptr);
			}
		}
		else if (SEqualNoCase(sCommand.getData(), "nick"))
		{
			if (C4InVal::ValidateString(sParam, C4InVal::VAL_IRCName))
				pChatSheet->DoError(FormatString(LoadResStr("IDS_ERR_INVALIDNICKNAME2"), sCommand.getData()).getData());
			else
				fIRCSuccess = pIRCClient->ChangeNick(sParam.getData());
		}
		else
		{
			// unknown command
			pChatSheet->DoError(FormatString(LoadResStr("IDS_ERR_UNKNOWNCMD"), sCommand.getData()).getData());
		}
	}
	else
	{
		// regular chat input: Send as message to current channel/user
		const char *szMsgTarget;
		ChatSheet::SheetType eSheetType = pChatSheet->GetSheetType();
		if (eSheetType == ChatSheet::CS_Server)
		{
			pChatSheet->DoError(LoadResStr("IDS_ERR_NOTONACHANNEL"));
		}
		else
		{
			szMsgTarget = pChatSheet->GetTitle();
			if (*szInput == '/') // action
				fIRCSuccess = pIRCClient->Action(szMsgTarget, szInput+4);
			else
				fIRCSuccess = pIRCClient->Message(szMsgTarget, szInput);
		}
	}
	// IRC sending error? log it
	if (!fIRCSuccess)
	{
		pChatSheet->DoError(pIRCClient->GetError());
	}
	return fResult;
}

void C4ChatControl::UserQueryQuit()
{
	// still connected? Then confirm first
	if (pIRCClient->IsActive())
	{
		if (!GetScreen()->ShowMessageModal(LoadResStr("IDS_MSG_DISCONNECTFROMSERVER"), LoadResStr("IDS_DLG_CHAT"), C4GUI::MessageDialog::btnOKAbort, C4GUI::Ico_Confirm, nullptr))
			return;
	}
	// disconnect from server
	pIRCClient->Close();
	// change back to login page
	UpdateShownPage();
}


/* C4ChatDlg */

C4ChatDlg *C4ChatDlg::pInstance = nullptr;

C4ChatDlg::C4ChatDlg() : C4GUI::Dialog(100, 100, "IRC", false)
{
	// child elements - positioned later
	pChatCtrl = new C4ChatControl(&Application.IRCClient);
	pChatCtrl->SetTitleChangeCB(new C4GUI::InputCallback<C4ChatDlg>(this, &C4ChatDlg::OnChatTitleChange));
	AddElement(pChatCtrl);
	// del dlg when closed
	SetDelOnClose();
	// set initial element positions
	UpdateSize();
	// intial focus
	SetFocus(GetDefaultControl(), false);
}

C4ChatDlg::~C4ChatDlg()
{
}

C4ChatDlg *C4ChatDlg::ShowChat()
{
	if (!pInstance)
	{
		pInstance = new C4ChatDlg();
		pInstance->Show(::pGUI, true);
	}
	else
	{
		::pGUI->ActivateDialog(pInstance);
	}
	return pInstance;
}

void C4ChatDlg::StopChat()
{
	if (!pInstance) return;
	pInstance->Close(false);
	Application.IRCClient.Close();
}

bool C4ChatDlg::ToggleChat()
{
	if (pInstance && pInstance->IsShown())
		pInstance->Close(false);
	else
		ShowChat();
	return true;
}

bool C4ChatDlg::IsChatActive()
{
	// not if chat is disabled
	if (!IsChatEnabled()) return false;
	// check whether IRC is connected
	return Application.IRCClient.IsActive();
}

bool C4ChatDlg::IsChatEnabled()
{
	return true;
}

C4GUI::Control *C4ChatDlg::GetDefaultControl()
{
	return pChatCtrl->GetDefaultControl();
}

bool C4ChatDlg::DoPlacement(C4GUI::Screen *pOnScreen, const C4Rect &rPreferredDlgRect)
{
	// ignore preferred rect; place over complete screen
	C4Rect rcPos = pOnScreen->GetContainedClientRect();
	rcPos.x += rcPos.Wdt/10; rcPos.Wdt -= rcPos.Wdt/5;
	rcPos.y += rcPos.Hgt/10; rcPos.Hgt -= rcPos.Hgt/5;
	SetBounds(rcPos);
	return true;
}

void C4ChatDlg::OnClosed(bool fOK)
{
	// callback when dlg got closed
	pInstance = nullptr;
	typedef C4GUI::Dialog ParentClass;
	ParentClass::OnClosed(fOK);
}

void C4ChatDlg::OnShown()
{
	// callback when shown - should not delete the dialog
	typedef C4GUI::Dialog ParentClass;
	ParentClass::OnShown();
	pChatCtrl->OnShown();
}

void C4ChatDlg::UpdateSize()
{
	// parent update
	typedef C4GUI::Dialog ParentClass;
	ParentClass::UpdateSize();
	// position child elements
	C4GUI::ComponentAligner caMain(GetContainedClientRect(), 0,0);
	pChatCtrl->SetBounds(caMain.GetAll());
}

void C4ChatDlg::OnExitBtn(C4GUI::Control *btn)
{
	UserClose(false);
}

void C4ChatDlg::OnChatTitleChange(const StdStrBuf &sNewTitle)
{
	SetTitle(FormatString("%s - %s", LoadResStr("IDS_DLG_CHAT"), sNewTitle.getData()).getData());
}
