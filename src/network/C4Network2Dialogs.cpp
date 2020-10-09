/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// dialogs for network information

#include "C4Include.h"
#include "network/C4Network2Dialogs.h"

#include "control/C4GameControl.h"
#include "game/C4Viewport.h"
#include "graphics/C4Draw.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4GameOptions.h"
#include "gui/C4Startup.h"
#include "lib/StdColors.h"
#include "network/C4Network2.h"
#include "network/C4Network2Stats.h"
#include "player/C4PlayerList.h"

#ifndef HAVE_WINSOCK
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

// --------------------------------------------------
// C4Network2ClientDlg

C4Network2ClientDlg::C4Network2ClientDlg(int iForClientID)
		: C4GUI::InfoDialog(LoadResStr("IDS_NET_CLIENT_INFO"), 10), iClientID(iForClientID)
{
	// initial text update
	UpdateText();
}

void C4Network2ClientDlg::UpdateText()
{
	// begin updating (clears previous text)
	BeginUpdateText();
	// get core
	const C4Client *pClient = Game.Clients.getClientByID(iClientID);
	if (!pClient)
	{
		// client ID unknown
		AddLineFmt(LoadResStr("IDS_NET_CLIENT_INFO_UNKNOWNID"), iClientID);
	}
	else
	{
		// get client (may be nullptr for local info)
		C4Network2Client *pNetClient = pClient->getNetClient();
		// show some info
		StdCopyStrBuf strInfo;
		if (!pClient->isActivated()) { strInfo.Append(LoadResStr("IDS_MSG_INACTIVE")); strInfo.Append(" "); }
		if (pClient->isLocal()) { strInfo.Append(LoadResStr("IDS_MSG_LOCAL")); strInfo.Append(" "); }
		strInfo.AppendFormat("%s %s (ID #%d)%s",
			LoadResStr(pClient->isHost() ? "IDS_MSG_HOST" : "IDS_MSG_CLIENT"),
			pClient->getName(),
			iClientID,
			::Network.isHost() && pNetClient && !pNetClient->isReady() ? " (!ack)" : "");
		AddLine(strInfo.getData());
		// show addresses
		int iCnt;
		if ((iCnt=pNetClient->getAddrCnt()))
		{
			AddLine(LoadResStr("IDS_NET_CLIENT_INFO_ADDRESSES"));
			for (int i=0; i<iCnt; ++i)
			{
				C4Network2Address addr = pNetClient->getAddr(i);
				AddLineFmt("  %d: %s",
				           i,                        // adress index
				           addr.toString().getData());
			}
		}
		else
			AddLine(LoadResStr("IDS_NET_CLIENT_INFO_NOADDRESSES"));
		// show connection
		if (pNetClient)
		{
			// connections
			if (pNetClient->isConnected())
			{
				AddLineFmt(LoadResStr("IDS_NET_CLIENT_INFO_CONNECTIONS"),
				           pNetClient->getMsgConn() == pNetClient->getDataConn() ? "Msg/Data" : "Msg",
				           ::Network.NetIO.getNetIOName(pNetClient->getMsgConn()->getNetClass()),
						   pNetClient->getMsgConn()->getPeerAddr().ToString().getData(),
				           pNetClient->getMsgConn()->getPingTime());
				if (pNetClient->getMsgConn() != pNetClient->getDataConn())
					AddLineFmt(LoadResStr("IDS_NET_CLIENT_INFO_CONNDATA"),
					           ::Network.NetIO.getNetIOName(pNetClient->getDataConn()->getNetClass()),
					           pNetClient->getDataConn()->getPeerAddr().ToString().getData(),
					           pNetClient->getDataConn()->getPingTime());
			}
			else
				AddLine(LoadResStr("IDS_NET_CLIENT_INFO_NOCONNECTIONS"));
		}
	}
	// update done
	EndUpdateText();
}


// --------------------------------------------------
// C4Network2ClientListBox::ClientListItem

C4Network2ClientListBox::ClientListItem::ClientListItem(class C4Network2ClientListBox *pForDlg, int iClientID) // ctor
		: ListItem(pForDlg, iClientID), pStatusIcon(nullptr), pName(nullptr), pPing(nullptr), pActivateBtn(nullptr), pKickBtn(nullptr), last_sound_time(0)
{
	// get associated client
	const C4Client *pClient = GetClient();
	// get size
	int iIconSize = ::GraphicsResource.TextFont.GetLineHeight();
	if (pForDlg->IsStartup()) iIconSize *= 2;
	int iWidth = pForDlg->GetItemWidth();
	int iVerticalIndent = 2;
	SetBounds(C4Rect(0, 0, iWidth, iIconSize+2*iVerticalIndent));
	C4GUI::ComponentAligner ca(GetContainedClientRect(), 0,iVerticalIndent);
	// create subcomponents
	bool fIsHost = pClient && pClient->isHost();
	pStatusIcon = new C4GUI::Icon(ca.GetFromLeft(iIconSize), fIsHost ? C4GUI::Ico_Host : C4GUI::Ico_Client);
	StdStrBuf sNameLabel;
	if (pClient)
	{
		if (pForDlg->IsStartup())
			sNameLabel.Ref(pClient->getName());
		else
			sNameLabel.Format("%s:%s", pClient->getName(), pClient->getNick());
	}
	else
	{
		sNameLabel.Ref("???");
	}
	pName = new C4GUI::Label(sNameLabel.getData(), iIconSize + IconLabelSpacing,iVerticalIndent, ALeft);
	int iPingRightPos = GetBounds().Wdt - IconLabelSpacing;
	if (::Network.isHost()) iPingRightPos -= 48;
	if (::Network.isHost() && pClient && !pClient->isHost())
	{
		// activate/deactivate and kick btns for clients at host
		if (!pForDlg->IsStartup())
		{
			pActivateBtn = new C4GUI::CallbackButtonEx<C4Network2ClientListBox::ClientListItem, C4GUI::IconButton>(C4GUI::Ico_Active, GetToprightCornerRect(std::max(iIconSize, 16),std::max(iIconSize, 16),2,1,1), 0, this, &ClientListItem::OnButtonActivate);
			fShownActive = true;
		}
		pKickBtn = new  C4GUI::CallbackButtonEx<C4Network2ClientListBox::ClientListItem, C4GUI::IconButton>(C4GUI::Ico_Kick, GetToprightCornerRect(std::max(iIconSize, 16),std::max(iIconSize, 16),2,1,0), 0, this, &ClientListItem::OnButtonKick);
		pKickBtn->SetToolTip(LoadResStrNoAmp("IDS_NET_KICKCLIENT"));
	}
	if (!pForDlg->IsStartup()) if (pClient && !pClient->isLocal())
		{
			// wait time
			pPing = new C4GUI::Label("???", iPingRightPos, iVerticalIndent, ARight);
			pPing->SetToolTip(LoadResStr("IDS_DESC_CONTROLWAITTIME"));
		}
	// add components
	AddElement(pStatusIcon); AddElement(pName);
	if (pPing) AddElement(pPing);
	if (pActivateBtn) AddElement(pActivateBtn);
	if (pKickBtn) AddElement(pKickBtn);
	// add to listbox (will eventually get moved)
	pForDlg->AddElement(this);
	// first-time update
	Update();
}

void C4Network2ClientListBox::ClientListItem::Update()
{
	// update wait label
	if (pPing)
	{
		int iWait = ::Control.Network.ClientPerfStat(iClientID);
		pPing->SetText(FormatString("%d ms", iWait).getData());
		pPing->SetColor(C4RGB(
		                  Clamp(255-Abs(iWait)*5, 0, 255),
		                  Clamp(255-iWait*5, 0, 255),
		                  Clamp(255+iWait*5, 0, 255)));
	}
	// update activation status
	const C4Client *pClient = GetClient(); if (!pClient) return;
	bool fIsActive = pClient->isActivated();
	if (fIsActive != fShownActive)
	{
		fShownActive = fIsActive;
		if (!pClient->isHost()) pStatusIcon->SetIcon(fIsActive ? C4GUI::Ico_Client : C4GUI::Ico_ObserverClient);
		if (pActivateBtn)
		{
			pActivateBtn->SetIcon(fIsActive ? C4GUI::Ico_Active : C4GUI::Ico_Inactive);
			pActivateBtn->SetToolTip(LoadResStrNoAmp(fIsActive ? "IDS_NET_DEACTIVATECLIENT" : "IDS_NET_ACTIVATECLIENT"));
		}
	}
	// update players in tooltip
	StdStrBuf sCltPlrs(Game.PlayerInfos.GetActivePlayerNames(false, iClientID));
	pName->SetToolTip(sCltPlrs.getData());
	// update icon: Network status
	C4GUI::Icons icoStatus = C4GUI::Ico_UnknownClient;
	C4Network2Client *pClt = pClient->getNetClient();
	if (pClt)
	{
		switch (pClt->getStatus())
		{
		case NCS_Joining: // waiting for join data
		case NCS_Chasing: // client is behind (status not acknowledged, isn't waited for)
		case NCS_NotReady: // client is behind (status not acknowledged)
			icoStatus = C4GUI::Ico_Loading;
			break;

		case NCS_Ready: // client acknowledged network status
			icoStatus = C4GUI::Ico_Ready;
			break;

		case NCS_Remove: // client is to be removed
			icoStatus = C4GUI::Ico_Kick;
			break;

		default: // whatever
			assert(false);
			icoStatus = C4GUI::Ico_Loading;
			break;
		}
	}
	// sound icon?
	if (last_sound_time)
	{
		time_t dt = time(nullptr) - last_sound_time;
		if (dt >= SoundIconShowTime)
		{
			// stop showing sound icon
			last_sound_time = 0;
		}
		else
		{
			// time not up yet: show sound icon
			icoStatus = C4GUI::Ico_Sound;
		}
	}
	// network OK - control ready?
	if (!pForDlg->IsStartup() && (icoStatus == C4GUI::Ico_Ready))
	{
		if (!::Control.Network.ClientReady(iClientID, ::Control.ControlTick))
		{
			// control not ready
			icoStatus = C4GUI::Ico_NetWait;
		}
	}
	// set new icon
	pStatusIcon->SetIcon(icoStatus);
}

const C4Client *C4Network2ClientListBox::ClientListItem::GetClient() const
{
	return Game.Clients.getClientByID(iClientID);
}

C4Network2ClientListBox::ClientListItem *C4Network2ClientListBox::GetClientListItem(int32_t client_id)
{
	// find list item that is not the connection item
	// search through listbox
	for (C4GUI::Element *list_item = GetFirst(); list_item; list_item = list_item->GetNext())
	{
		// only playerlistitems in this box
		ListItem *list_item2 = static_cast<ListItem *>(list_item);
		if (list_item2->GetClientID() == client_id)
			if (list_item2->GetConnectionID() == -1)
				return static_cast<ClientListItem *>(list_item2);
	}
	// nothing found
	return nullptr;
}

void C4Network2ClientListBox::ClientListItem::OnButtonActivate(C4GUI::Control *pButton)
{
	// league: Do not deactivate clients with players
	if (Game.Parameters.isLeague() && ::Players.GetAtClient(iClientID))
	{
		Log(LoadResStr("IDS_LOG_COMMANDNOTALLOWEDINLEAGUE"));
		return;
	}
	// change to status that is not currently shown
	::Control.DoInput(CID_ClientUpdate, new C4ControlClientUpdate(iClientID, CUT_Activate, !fShownActive), CDT_Sync);
}

void C4Network2ClientListBox::ClientListItem::OnButtonKick(C4GUI::Control *pButton)
{
	// try kick
	// league: Kick needs voting
	if (Game.Parameters.isLeague() && ::Players.GetAtClient(iClientID))
		::Network.Vote(VT_Kick, true, iClientID);
	else
		Game.Clients.CtrlRemove(GetClient(), LoadResStr(pForDlg->IsStartup() ? "IDS_MSG_KICKFROMSTARTUPDLG" : "IDS_MSG_KICKFROMCLIENTLIST"));
}

void C4Network2ClientListBox::ClientListItem::SetSoundIcon()
{
	// remember time for reset
	last_sound_time = time(nullptr);
	// force icon
	Update();
}


// --------------------------------------------------
// C4Network2ClientListBox::ConnectionListItem

C4Network2ClientListBox::ConnectionListItem::ConnectionListItem(class C4Network2ClientListBox *pForDlg, int32_t iClientID, int32_t iConnectionID) // ctor
		: ListItem(pForDlg, iClientID), iConnID(iConnectionID), pDesc(nullptr), pPing(nullptr), pDisconnectBtn(nullptr)
{
	// get size
	CStdFont &rUseFont = ::GraphicsResource.TextFont;
	int iIconSize = rUseFont.GetLineHeight();
	int iWidth = pForDlg->GetItemWidth();
	int iVerticalIndent = 2;
	SetBounds(C4Rect(0, 0, iWidth, iIconSize+2*iVerticalIndent));
	C4GUI::ComponentAligner ca(GetContainedClientRect(), 0,iVerticalIndent);
	// left indent
	ca.ExpandLeft(-iIconSize*2);
	// create subcomponents
	// disconnect button
	if (!Game.Parameters.isLeague())
	{
		pDisconnectBtn = new  C4GUI::CallbackButtonEx<C4Network2ClientListBox::ConnectionListItem, C4GUI::IconButton>(C4GUI::Ico_Disconnect, ca.GetFromRight(iIconSize, iIconSize), 0, this, &ConnectionListItem::OnButtonDisconnect);
		pDisconnectBtn->SetToolTip(LoadResStr("IDS_MENU_DISCONNECT"));
	}
	// ping time
	int32_t sx=40, sy=iIconSize;
	rUseFont.GetTextExtent("???? ms", sx,sy, true);
	pPing = new C4GUI::Label("???", ca.GetFromRight(sx, sy), ARight);
	pPing->SetToolTip(LoadResStr("IDS_NET_CONTROL_PING"));
	// main description item
	pDesc = new C4GUI::Label("???", ca.GetAll(), ALeft);
	// add components
	AddElement(pDesc);
	AddElement(pPing);
	if (pDisconnectBtn) AddElement(pDisconnectBtn);
	// add to listbox (will eventually get moved)
	pForDlg->AddElement(this);
	// first-time update
	Update();
}

C4Network2IOConnection *C4Network2ClientListBox::ConnectionListItem::GetConnection() const
{
	// get connection by connection ID
	C4Network2Client *pNetClient = ::Network.Clients.GetClientByID(iClientID);
	if (!pNetClient) return nullptr;
	if (iConnID == 0) return pNetClient->getDataConn();
	if (iConnID == 1) return pNetClient->getMsgConn();
	return nullptr;
}

void C4Network2ClientListBox::ConnectionListItem::Update()
{
	C4Network2IOConnection *pConn = GetConnection();
	if (!pConn)
	{
		// No connection: Shouldn't happen
		pDesc->SetText("???");
		pPing->SetText("???");
		return;
	}
	// update connection ping
	int iPing = pConn->getLag();
	pPing->SetText(FormatString("%d ms", iPing).getData());
	// update description
	// get connection usage
	const char *szConnType;
	C4Network2Client *pNetClient = ::Network.Clients.GetClientByID(iClientID);
	if (pNetClient->getDataConn() == pNetClient->getMsgConn())
		szConnType = "Data/Msg";
	else if (iConnID)
		szConnType = "Msg";
	else
		szConnType = "Data";
	// display info
	pDesc->SetText(FormatString("%s: %s (%s l%d)",
	                            szConnType,
	                            ::Network.NetIO.getNetIOName(pConn->getNetClass()),
	                            pConn->getPeerAddr().ToString().getData(),
	                            pConn->getPacketLoss()).getData());
}

void C4Network2ClientListBox::ConnectionListItem::OnButtonDisconnect(C4GUI::Control *pButton)
{
	// close connection
	C4Network2IOConnection *pConn = GetConnection();
	if (pConn)
	{
		pConn->Close();
	}
}

void C4Network2ClientListBox::ConnectionListItem::OnButtonReconnect(C4GUI::Control *pButton)
{
	// 2do
}


// --------------------------------------------------
// C4Network2ClientListBox

C4Network2ClientListBox::C4Network2ClientListBox(C4Rect &rcBounds, bool fStartup) : ListBox(rcBounds), fStartup(fStartup)
{
	// hook into timer callback
	Application.Add(this);
	// initial update
	Update();
}

C4Network2ClientListBox::~C4Network2ClientListBox()
{
	Application.Remove(this);
}

void C4Network2ClientListBox::Update()
{
	// sync with client list
	ListItem *pItem = static_cast<ListItem *>(pClientWindow->GetFirst()), *pNext;
	const C4Client *pClient = nullptr;
	while ((pClient = Game.Clients.getClient(pClient)))
	{
		// skip host in startup board
		if (IsStartup() && pClient->isHost()) continue;
		// deleted client(s) present? this will also delete unneeded client connections of previous client
		while (pItem && (pItem->GetClientID() < pClient->getID()))
		{
			pNext = static_cast<ListItem *>(pItem->GetNext());
			delete pItem; pItem = pNext;
		}
		// same present for update?
		// need not check for connection ID, because a client item will always be placed before the corresponding connection items
		if (pItem && pItem->GetClientID() == pClient->getID())
		{
			pItem->Update();
			pItem = static_cast<ListItem *>(pItem->GetNext());
		}
		else
			// not present: insert (or add if pItem=nullptr)
			InsertElement(new ClientListItem(this, pClient->getID()), pItem);
		// update connections for client
		// but no connections in startup board
		if (IsStartup()) continue;
		// enumerate client connections
		C4Network2Client *pNetClient = pClient->getNetClient();
		if (!pNetClient) continue; // local client does not have connections
		C4Network2IOConnection *pLastConn = nullptr;
		for (int i = 0; i<2; ++i)
		{
			C4Network2IOConnection *pConn = i ? pNetClient->getMsgConn() : pNetClient->getDataConn();
			if (!pConn) continue;
			if (pConn == pLastConn) continue; // combined connection: Display only one
			pLastConn = pConn;
			// del leading items
			while (pItem && ((pItem->GetClientID() < pClient->getID()) || ((pItem->GetClientID() == pClient->getID()) && (pItem->GetConnectionID() < i))))
			{
				pNext = static_cast<ListItem *>(pItem->GetNext());
				delete pItem; pItem = pNext;
			}
			// update connection item
			if (pItem && pItem->GetClientID() == pClient->getID() && pItem->GetConnectionID() == i)
			{
				pItem->Update();
				pItem = static_cast<ListItem *>(pItem->GetNext());
			}
			else
			{
				// new connection: create it
				InsertElement(new ConnectionListItem(this, pClient->getID(), i), pItem);
			}
		}
	}
	// del trailing items
	while (pItem)
	{
		pNext = static_cast<ListItem *>(pItem->GetNext());
		delete pItem; pItem = pNext;
	}
}

void C4Network2ClientListBox::SetClientSoundIcon(int32_t client_id)
{
	// sound icon on client element
	ClientListItem *item = GetClientListItem(client_id);
	if (item) item->SetSoundIcon();
}


// --------------------------------------------------
// C4Network2ClientListDlg

// singleton
C4Network2ClientListDlg *C4Network2ClientListDlg::pInstance = nullptr;

C4Network2ClientListDlg::C4Network2ClientListDlg()
		: Dialog(::pGUI->GetPreferredDlgRect().Wdt*3/4, ::pGUI->GetPreferredDlgRect().Hgt*3/4, LoadResStr("IDS_NET_CAPTION"), false)
{
	// component layout
	CStdFont *pUseFont = &::GraphicsResource.TextFont;
	C4GUI::ComponentAligner caAll(GetContainedClientRect(), 0,0);
	C4Rect rcStatus = caAll.GetFromBottom(pUseFont->GetLineHeight());
	// create game options; max 1/2 of dialog height
	pGameOptions = new C4GameOptionsList(caAll.GetFromTop(caAll.GetInnerHeight()/2), true, C4GameOptionsList::GOLS_Runtime);
	pGameOptions->SetDecoration(false, nullptr, true, false);
	pGameOptions->SetSelectionDiabled();
	// but resize to actually used height
	int32_t iFreedHeight = pGameOptions->ContractToElementHeight();
	caAll.ExpandTop(iFreedHeight);
	AddElement(pGameOptions);
	// create client list box
	AddElement(pListBox = new C4Network2ClientListBox(caAll.GetAll(), false));
	// create status label
	AddElement(pStatusLabel = new C4GUI::Label("", rcStatus));
	// add timer
	Application.Add(this);
	// initial update
	Update();
}

C4Network2ClientListDlg::~C4Network2ClientListDlg()
{
	if (this==pInstance) pInstance=nullptr;
	Application.Remove(this);
}

void C4Network2ClientListDlg::Update()
{
	// Compose status text
	StdStrBuf sStatusText;
	sStatusText.Format("Tick %d, Behind %d, Rate %d, PreSend %d, ACT: %d",
	                   (int)::Control.ControlTick, (int)::Control.Network.GetBehind(::Control.ControlTick),
	                   (int)::Control.ControlRate, (int)::Control.Network.getControlPreSend(),
	                   (int)::Control.Network.getAvgControlSendTime());
	// Update status label
	pStatusLabel->SetText(sStatusText.getData());
}

bool C4Network2ClientListDlg::Toggle()
{
	// toggle off?
	if (pInstance) { pInstance->Close(true); return true; }
	// toggle on!
	return ::pGUI->ShowRemoveDlg(pInstance = new C4Network2ClientListDlg());
}

void C4Network2ClientListDlg::OnSound(class C4Client *singer)
{
	if (singer) pListBox->SetClientSoundIcon(singer->getID());
}


// --------------------------------------------------
// C4Network2StartWaitDlg

C4Network2StartWaitDlg::C4Network2StartWaitDlg()
		: C4GUI::Dialog(DialogWidth, DialogHeight, LoadResStr("IDS_NET_CAPTION"), false)
{
	C4GUI::ComponentAligner caAll(GetContainedClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent);
	C4GUI::ComponentAligner caButtonArea(caAll.GetFromBottom(C4GUI_ButtonAreaHgt), 0,0);
	// create top label
	C4GUI::Label *pLbl;
	AddElement(pLbl = new C4GUI::Label(LoadResStr("IDS_NET_WAITFORSTART"), caAll.GetFromTop(25), ACenter));
	// create client list box
	AddElement(pClientListBox = new C4Network2ClientListBox(caAll.GetAll(), true));
	// place abort button
	C4GUI::Button *pBtnAbort = new C4GUI::CancelButton(caButtonArea.GetCentered(C4GUI_DefButtonWdt, C4GUI_ButtonHgt));
	AddElement(pBtnAbort);
}



// ---------------------------------------------------
// C4GameOptionButtons

C4GameOptionButtons::C4GameOptionButtons(const C4Rect &rcBounds, bool fNetwork, bool fHost, bool fLobby)
		: C4GUI::Window(), fNetwork(fNetwork), fHost(fHost), fLobby(fLobby), fCountdown(false)
{
	SetBounds(rcBounds);
	// calculate button size from area
	int32_t iButtonCount = fNetwork ? fHost ? 6 : 3 : 2;
	int32_t iIconSize = std::min<int32_t>(C4GUI_IconExHgt, rcBounds.Hgt), iIconSpacing = rcBounds.Wdt/(rcBounds.Wdt >= 400 ? 64 : 128);
	if ((iIconSize+iIconSpacing*2)*iButtonCount > rcBounds.Wdt)
	{
		if (iIconSize*iButtonCount <= rcBounds.Wdt)
		{
			iIconSpacing = std::max<int32_t>(0, (rcBounds.Wdt-iIconSize*iButtonCount)/(iButtonCount*2)-1);
		}
		else
		{
			iIconSpacing = 0;
			iIconSize = rcBounds.Wdt / iButtonCount;
		}
	}
	C4GUI::ComponentAligner caButtonArea(rcBounds,0,0,true);
	C4GUI::ComponentAligner caButtons(caButtonArea.GetCentered((iIconSize+2*iIconSpacing)*iButtonCount, iIconSize),iIconSpacing,0);
	// add buttons
	if (fNetwork && fHost)
	{
		bool fIsInternet = !!Config.Network.MasterServerSignUp, fIsDisabled = false;
		// league currently implies master server signup, and can thus not be turned off
		if (fLobby && Game.Parameters.isLeague())
		{
			fIsInternet = true;
			fIsDisabled = true;
		}
		btnInternet = new C4GUI::CallbackButton<C4GameOptionButtons, C4GUI::IconButton>(fIsInternet ? C4GUI::Ico_Ex_InternetOn : C4GUI::Ico_Ex_InternetOff, caButtons.GetFromLeft(iIconSize, iIconSize), LoadResStr("IDS_DLGTIP_STARTINTERNETGAME"), &C4GameOptionButtons::OnBtnInternet, this);
		btnInternet->SetEnabled(!fIsDisabled);
		AddElement(btnInternet);
	}
	else btnInternet = nullptr;
	bool fIsLeague = false;
	// League button
	if (fNetwork)
	{
		C4GUI::Icons eLeagueIcon;
		fIsLeague = fLobby ? Game.Parameters.isLeague() : !!Config.Network.LeagueServerSignUp;
		eLeagueIcon = fIsLeague ? C4GUI::Ico_Ex_LeagueOn : C4GUI::Ico_Ex_LeagueOff;
		btnLeague = new C4GUI::CallbackButton<C4GameOptionButtons, C4GUI::IconButton>(eLeagueIcon, caButtons.GetFromLeft(iIconSize, iIconSize), LoadResStr("IDS_DLGTIP_STARTLEAGUEGAME"), &C4GameOptionButtons::OnBtnLeague, this);
		btnLeague->SetEnabled(fHost && !fLobby);
		AddElement(btnLeague);
	}
	else btnLeague=nullptr;
	if (fNetwork && fHost)
	{
		btnPassword = new C4GUI::CallbackButton<C4GameOptionButtons, C4GUI::IconButton>(::Network.isPassworded() ? C4GUI::Ico_Ex_Locked : C4GUI::Ico_Ex_Unlocked, caButtons.GetFromLeft(iIconSize, iIconSize), LoadResStr("IDS_NET_PASSWORD_DESC"), &C4GameOptionButtons::OnBtnPassword, this);
		AddElement(btnPassword);
		btnComment = new C4GUI::CallbackButton<C4GameOptionButtons, C4GUI::IconButton>(C4GUI::Ico_Ex_Comment, caButtons.GetFromLeft(iIconSize, iIconSize), LoadResStr("IDS_DESC_COMMENTDESCRIPTIONFORTHIS"), &C4GameOptionButtons::OnBtnComment, this);
		AddElement(btnComment);
	}
	else btnPassword=btnComment=nullptr;
	btnRecord = new C4GUI::CallbackButton<C4GameOptionButtons, C4GUI::IconButton>(Game.Record || fIsLeague ? C4GUI::Ico_Ex_RecordOn : C4GUI::Ico_Ex_RecordOff, caButtons.GetFromLeft(iIconSize, iIconSize), LoadResStr("IDS_DLGTIP_RECORD"), &C4GameOptionButtons::OnBtnRecord, this);
	btnRecord->SetEnabled(!fIsLeague);
	AddElement(btnRecord);
}

void C4GameOptionButtons::OnBtnInternet(C4GUI::Control *btn)
{
	if (!fNetwork || !fHost) return;
	bool fCheck = (Config.Network.MasterServerSignUp = !Config.Network.MasterServerSignUp);
	// in lobby mode, do actual termination from masterserver
	if (fLobby)
	{
		if (fCheck)
		{
			fCheck = ::Network.LeagueSignupEnable();
		}
		else
		{
			::Network.LeagueSignupDisable();
		}
	}
	btnInternet->SetIcon(fCheck ? C4GUI::Ico_Ex_InternetOn : C4GUI::Ico_Ex_InternetOff);
	// also update league button, because turning off masterserver also turns off the league
	if (!fCheck)
	{
		Config.Network.LeagueServerSignUp = false;
		if (btnLeague)
			btnLeague->SetIcon(C4GUI::Ico_Ex_LeagueOff);
	}
	// re-set in config for the case of failure
	Config.Network.MasterServerSignUp = fCheck;
}

void C4GameOptionButtons::OnBtnLeague(C4GUI::Control *btn)
{
	if (!fNetwork || !fHost) return;
	bool fCheck = (Config.Network.LeagueServerSignUp = !Config.Network.LeagueServerSignUp);
	btnLeague->SetIcon(fCheck ? C4GUI::Ico_Ex_LeagueOn : C4GUI::Ico_Ex_LeagueOff);
	if (!Game.Record) OnBtnRecord(btnRecord);
	btnRecord->SetEnabled(!fCheck);
	// if the league is turned on, the game must be signed up at the masterserver
	if (fCheck && !Config.Network.MasterServerSignUp) OnBtnInternet(btnInternet);
	// refresh options in scenario selection dialogue
	if (C4Startup::Get()) C4Startup::Get()->OnLeagueOptionChanged();
}

void C4GameOptionButtons::OnBtnRecord(C4GUI::Control *btn)
{
	Game.Record = !Game.Record;
	Config.General.DefRec = Game.Record;
	btnRecord->SetIcon(Game.Record ? C4GUI::Ico_Ex_RecordOn : C4GUI::Ico_Ex_RecordOff);
}

void C4GameOptionButtons::OnBtnPassword(C4GUI::Control *btn)
{
	if (!fNetwork || !fHost) return;
	// password is currently set - a single click only clears the password
	if (::Network.GetPassword() && *::Network.GetPassword())
	{
		StdStrBuf empty;
		OnPasswordSet(empty);
		return;
	}
	// password button pressed: Show dialog to set/change current password
	C4GUI::InputDialog *pDlg;
	GetScreen()->ShowRemoveDlg(pDlg=new C4GUI::InputDialog(LoadResStr("IDS_MSG_ENTERPASSWORD"), LoadResStr("IDS_DLG_PASSWORD"), C4GUI::Ico_Ex_LockedFrontal, new C4GUI::InputCallback<C4GameOptionButtons>(this, &C4GameOptionButtons::OnPasswordSet), false));
	pDlg->SetMaxText(CFG_MaxString);
	const char *szPassPreset = ::Network.GetPassword();
	if (!szPassPreset || !*szPassPreset) szPassPreset = Config.Network.LastPassword;
	if (*szPassPreset) pDlg->SetInputText(szPassPreset);
}

void C4GameOptionButtons::OnPasswordSet(const StdStrBuf &rsNewPassword)
{
	// password input dialog answered with OK: Set/clear network password
	const char *szPass;
	::Network.SetPassword(szPass=rsNewPassword.getData());
	// update icon to reflect if a password is set
	UpdatePasswordBtn();
	// remember password for next round
	bool fHasPassword = (szPass && *szPass);
	if (fHasPassword)
	{
		SCopy(szPass, Config.Network.LastPassword, CFG_MaxString);
	}
	// acoustic feedback
	C4GUI::GUISound("UI::Confirmed");
}

void C4GameOptionButtons::UpdatePasswordBtn()
{
	// update icon to reflect if a password is set
	const char *szPass = ::Network.GetPassword();
	bool fHasPassword = szPass && *szPass;
	btnPassword->SetIcon(fHasPassword ? C4GUI::Ico_Ex_Locked : C4GUI::Ico_Ex_Unlocked);
}

void C4GameOptionButtons::OnBtnComment(C4GUI::Control *btn)
{
	// password button pressed: Show dialog to set/change current password
	C4GUI::InputDialog *pDlg;
	GetScreen()->ShowRemoveDlg(pDlg=new C4GUI::InputDialog(LoadResStr("IDS_CTL_ENTERCOMMENT"), LoadResStr("IDS_CTL_COMMENT"), C4GUI::Ico_Ex_Comment, new C4GUI::InputCallback<C4GameOptionButtons>(this, &C4GameOptionButtons::OnCommentSet), false));
	pDlg->SetMaxText(C4MaxComment);
	pDlg->SetInputText(Config.Network.Comment.getData());
}

void C4GameOptionButtons::OnCommentSet(const StdStrBuf &rsNewComment)
{
	// check for change; no reference invalidation if not changed
	if (rsNewComment == Config.Network.Comment) return;
	// Set in configuration, update reference
	Config.Network.Comment.CopyValidated(rsNewComment.getData());
	::Network.InvalidateReference();
	// message feedback
	Log(LoadResStr("IDS_NET_COMMENTCHANGED"));
	// acoustic feedback
	C4GUI::GUISound("UI::Confirmed");
}

void C4GameOptionButtons::SetCountdown(bool fToVal)
{
	fCountdown = fToVal;
}

// ---------------------------------------------------
// C4Chart

int GetValueDecade(int iVal)
{
	// get enclosing decade
	int iDec = 1;
	while (iVal) { iVal/=10; iDec*=10; }
	return iDec;
}

int GetAxisStepRange(int iRange, int iMaxSteps)
{
	// try in steps of 5s and 10s
	int iDec = GetValueDecade(iRange);
	if (iDec == 1) return 1;
	int iNextStepDivider = 2;
	while (iDec>=iNextStepDivider && iNextStepDivider*iRange/iDec <= iMaxSteps)
	{
		iDec/=iNextStepDivider;
		iNextStepDivider = 7 - iNextStepDivider;
	}
	return iDec;
}

void C4Chart::DrawElement(C4TargetFacet &cgo)
{
	typedef C4Graph::ValueType ValueType;
	typedef C4Graph::TimeType TimeType;
	// transparent w/o graph
	if (!pDisplayGraph) return;
	int iSeriesCount = pDisplayGraph->GetSeriesCount();
	if (!iSeriesCount) return;
	assert(iSeriesCount>0);
	StdStrBuf sbuf;
	pDisplayGraph->Update(); // update averages, etc.
	// calc metrics
	CStdFont &rFont = ::GraphicsResource.MiniFont;
	int       YAxisWdt       = 5,
	          XAxisHgt       = 15;
			  
	const int AxisArrowLen   = 6,
	          AxisMarkerLen  = 5,
	          AxisArrowThickness = 3,
	          AxisArrowIndent=  2; // margin between axis arrow and last value
	int32_t       YAxisMinStepHgt, XAxisMinStepWdt;
	// get value range
	int iMinTime = pDisplayGraph->GetStartTime();
	int iMaxTime = pDisplayGraph->GetEndTime() - 1;
	if (iMinTime >= iMaxTime) return;
	ValueType iMinVal = pDisplayGraph->GetMinValue();
	ValueType iMaxVal = pDisplayGraph->GetMaxValue();
	if (iMinVal == iMaxVal) ++iMaxVal;
	if (iMinVal > 0 && iMaxVal/iMinVal >= 2) iMinVal = 0; // go zero-based if this creates less than 50% unused space
	else if (iMaxVal < 0 && iMinVal/iMaxVal >= 2) iMaxVal = 0;
	int ddv;
	if (iMaxVal>0 && (ddv=GetValueDecade(int(iMaxVal))/50))
		iMaxVal = ((iMaxVal-(iMaxVal>0))/ddv+(iMaxVal>0))*ddv;
	if (iMinVal && (ddv=GetValueDecade(int(iMinVal))/50))
		iMinVal = ((iMinVal-(iMinVal<0))/ddv+(iMinVal<0))*ddv;
	ValueType dv=iMaxVal-iMinVal; TimeType dt=iMaxTime-iMinTime;
	// axis calculations
	sbuf.Format("-%d", (int) std::max(Abs(iMaxVal), Abs(iMinVal)));
	rFont.GetTextExtent(sbuf.getData(), XAxisMinStepWdt, YAxisMinStepHgt, false);
	YAxisWdt += XAxisMinStepWdt; XAxisHgt += YAxisMinStepHgt;
	XAxisMinStepWdt += 2; YAxisMinStepHgt += 2;
	int tw = rcBounds.Wdt - YAxisWdt;
	int th = rcBounds.Hgt - XAxisHgt;
	int tx = rcBounds.x + int(cgo.TargetX) + YAxisWdt;
	int ty = rcBounds.y + int(cgo.TargetY);
	// show a legend, if more than one graph is shown
	if (iSeriesCount > 1)
	{
		int iSeries = 0; const C4Graph *pSeries;
		int32_t iLegendWdt = 0, Q,W;
		while ((pSeries = pDisplayGraph->GetSeries(iSeries++)))
		{
			rFont.GetTextExtent(pSeries->GetTitle(), W, Q, true);
			iLegendWdt = std::max(iLegendWdt, W);
		}
		tw -= iLegendWdt+1;
		iSeries = 0;
		int iYLegendDraw = (th - iSeriesCount*Q)/2 + ty;
		while ((pSeries = pDisplayGraph->GetSeries(iSeries++)))
		{
			pDraw->TextOut(pSeries->GetTitle(), rFont, 1.0f, cgo.Surface, tx+tw, iYLegendDraw, pSeries->GetColorDw() | 0xff000000, ALeft, true);
			iYLegendDraw += Q;
		}
	}
	// safety: too small?
	if (tw < 10 || th < 10) return;
	// draw axis
	pDraw->DrawLineDw(cgo.Surface, tx, ty+th, tx+tw-1, ty+th, C4RGB(0x91, 0x91, 0x91));
	pDraw->DrawLineDw(cgo.Surface, tx+tw-1, ty+th, tx+tw-1-AxisArrowLen, ty+th-AxisArrowThickness, C4RGB(0x91, 0x91, 0x91));
	pDraw->DrawLineDw(cgo.Surface, tx+tw-1, ty+th, tx+tw-1-AxisArrowLen, ty+th+AxisArrowThickness, C4RGB(0x91, 0x91, 0x91));
	pDraw->DrawLineDw(cgo.Surface, tx, ty, tx, ty+th, C4RGB(0x91, 0x91, 0x91));
	pDraw->DrawLineDw(cgo.Surface, tx, ty, tx-AxisArrowThickness, ty+AxisArrowLen, C4RGB(0x91, 0x91, 0x91));
	pDraw->DrawLineDw(cgo.Surface, tx, ty, tx+AxisArrowThickness, ty+AxisArrowLen, C4RGB(0x91, 0x91, 0x91));
	tw -= AxisArrowLen + AxisArrowIndent;
	th -= AxisArrowLen + AxisArrowIndent; ty += AxisArrowLen + AxisArrowIndent;
	// do axis numbering
	int iXAxisSteps = GetAxisStepRange(dt, tw / XAxisMinStepWdt),
	                  iYAxisSteps = GetAxisStepRange(int(dv), th / YAxisMinStepHgt);
	int iX, iY, iTime, iVal;
	iY = 0;
	iTime = ((iMinTime-(iMinTime>0))/iXAxisSteps+(iMinTime>0))*iXAxisSteps;
	for (; iTime <= iMaxTime; iTime += iXAxisSteps)
	{
		iX = tx + tw * (iTime-iMinTime) / dt;
		pDraw->DrawLineDw(cgo.Surface, iX, ty+th+1, iX, ty+th+AxisMarkerLen, C4RGB(0x91, 0x91, 0x91));
		sbuf.Format("%d", (int) iTime);
		pDraw->TextOut(sbuf.getData(), rFont, 1.0f, cgo.Surface, iX, ty+th+AxisMarkerLen, 0xff7f7f7f, ACenter, false);
	}
	iVal = int( ((iMinVal-(iMinVal>0))/iYAxisSteps+(iMinVal>0))*iYAxisSteps );
	for (; iVal <= iMaxVal; iVal += iYAxisSteps)
	{
		iY = ty+th - int((iVal-iMinVal) / dv * th);
		pDraw->DrawLineDw(cgo.Surface, tx-AxisMarkerLen, iY, tx-1, iY, C4RGB(0x91, 0x91, 0x91));
		sbuf.Format("%d", (int) iVal);
		pDraw->TextOut(sbuf.getData(), rFont, 1.0f, cgo.Surface, tx-AxisMarkerLen, iY-rFont.GetLineHeight()/2, 0xff7f7f7f, ARight, false);
	}
	// draw graph series(es)
	int iSeries = 0;
	while (const C4Graph *pSeries = pDisplayGraph->GetSeries(iSeries++))
	{
		int iThisMinTime = std::max(iMinTime, pSeries->GetStartTime());
		int iThisMaxTime = std::min(iMaxTime, pSeries->GetEndTime());
		bool fAnyVal = false;
		for (iX = 0; iX<tw; ++iX)
		{
			iTime = iMinTime + dt*iX/tw;
			if (!Inside(iTime, iThisMinTime, iThisMaxTime)) continue;
			int iY2 = int((-pSeries->GetValue(iTime) + iMinVal) * th / dv) + ty+th;
			if (fAnyVal) pDraw->DrawLineDw(cgo.Surface, (float) (tx+iX-1), (float) iY, (float) (tx+iX), (float) iY2, pSeries->GetColorDw() | 0xff000000);
			iY = iY2;
			fAnyVal = true;
		}
	}
}

C4Chart::C4Chart(C4Rect &rcBounds) : Element(), pDisplayGraph(nullptr), fOwnGraph(false)
{
	this->rcBounds = rcBounds;
}

C4Chart::~C4Chart()
{
	if (fOwnGraph && pDisplayGraph) delete pDisplayGraph;
}


// singleton
C4ChartDialog *C4ChartDialog::pChartDlg=nullptr;

C4ChartDialog::C4ChartDialog() : Dialog(DialogWidth, DialogHeight, LoadResStr("IDS_NET_STATISTICS"), false)
{
	// register singleton
	pChartDlg = this;
	// add main chart switch component
	C4GUI::ComponentAligner caAll(GetContainedClientRect(), 5,5);
	pChartTabular = new C4GUI::Tabular(caAll.GetAll(), C4GUI::Tabular::tbTop);
	AddElement(pChartTabular);
	// add some graphs as subcomponents
	AddChart(StdStrBuf("oc"));
	AddChart(StdStrBuf("FPS"));
	AddChart(StdStrBuf("NetIO"));
	if (::Network.isEnabled())
		AddChart(StdStrBuf("Pings"));
	AddChart(StdStrBuf("Control"));
	AddChart(StdStrBuf("APM"));
}

void C4ChartDialog::AddChart(const StdStrBuf &rszName)
{
	// get graph by name
	if (!Game.pNetworkStatistics || !pChartTabular) return;
	bool fOwnGraph = false;
	C4Graph *pGraph = Game.pNetworkStatistics->GetGraphByName(rszName, fOwnGraph);
	if (!pGraph) return;
	// add sheet for name
	C4GUI::Tabular::Sheet *pSheet = pChartTabular->AddSheet(rszName.getData());
	if (!pSheet) { if (fOwnGraph) delete pGraph; return; }
	// add chart to sheet
	C4Chart *pNewChart = new C4Chart(pSheet->GetClientRect());
	pNewChart->SetGraph(pGraph, fOwnGraph);
	pSheet->AddElement(pNewChart);
}

void C4ChartDialog::Toggle()
{
	// close if open
	if (pChartDlg) { pChartDlg->Close(false); return; }
	// otherwise, open
	C4ChartDialog *pDlg = new C4ChartDialog();
	if (!::pGUI->ShowRemoveDlg(pDlg)) if (pChartDlg) delete pChartDlg;
}
