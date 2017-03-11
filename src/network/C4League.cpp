/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006-2009, RedWolf Design GmbH, http://www.clonk.de/
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

/* engine handler of league system */

#include "C4Include.h"
#include "network/C4League.h"

#include "game/C4Game.h"
#include "control/C4RoundResults.h"
#include "graphics/C4GraphicsResource.h"

// *** C4LeagueRequestHead

void C4LeagueRequestHead::CompileFunc(StdCompiler *pComp)
{

	StdEnumEntry<C4LeagueAction> Actions[] =
	{
		{ "Start",            C4LA_Start        },
		{ "Update",           C4LA_Update       },
		{ "End",              C4LA_End          },
		{ "Join",             C4LA_PlrAuthCheck },

		{ "",                 C4LA_RefQuery     },
		{ "Auth",             C4LA_PlrAuth      },

		{ "ReportDisconnect", C4LA_ReportDisconnect },
	};

	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(eAction, Actions), "Action", C4LA_RefQuery));
	pComp->Value(mkNamingAdapt(mkParAdapt(CSID, StdCompiler::RCT_IdtfAllowEmpty), "CSID", ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(AUID, StdCompiler::RCT_IdtfAllowEmpty), "AUID", ""));

	// Auth
	pComp->Value(mkNamingAdapt(mkParAdapt(Account, StdCompiler::RCT_All), "Account", ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(Password, StdCompiler::RCT_All), "Password", ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(NewAccount, StdCompiler::RCT_All), "NewAccount", ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(NewPassword, StdCompiler::RCT_All), "NewPassword", ""));
	pComp->Value(mkNamingAdapt(fRememberLogin, "RememberLogin", false));

}

void C4LeagueRequestHead::SetAuth(const char *szAccount, const char *szPassword, bool fRememberLogin)
{
	Account = szAccount;
	Password = szPassword;
	this->fRememberLogin = fRememberLogin;
}

void C4LeagueRequestHead::SetNewAccount(const char *szNewAccount)
{
	NewAccount = szNewAccount;
}

void C4LeagueRequestHead::SetNewPassword(const char *szNewPassword)
{
	NewPassword = szNewPassword;
}

// *** C4LeagueReportDisconnectHead

void C4LeagueReportDisconnectHead::CompileFunc(StdCompiler *pComp)
{
	// inherited fields
	C4LeagueRequestHead::CompileFunc(pComp);
	// reason
	StdEnumEntry<C4LeagueDisconnectReason> Reasons[] =
	{
		{ "",                 C4LDR_Unknown     },
		{ "ConnectionFailed", C4LDR_ConnectionFailed},
		{ "Desync",           C4LDR_Desync      },
	};
	pComp->Value(mkNamingAdapt(mkEnumAdaptT<uint8_t>(eReason, Reasons), "Reason", C4LDR_Unknown));
}

// *** C4LeagueRequestHeadEnd

void C4LeagueRequestHeadEnd::CompileFunc(StdCompiler *pComp)
{
	C4LeagueRequestHead::CompileFunc(pComp);
	pComp->Value(mkNamingAdapt(mkParAdapt(RecordName, StdCompiler::RCT_All), "RecordName", ""));
	if (RecordName.getLength())
		pComp->Value(mkNamingAdapt(mkHexAdapt(RecordSHA, sizeof(RecordSHA)), "RecordSHA"));
}

// *** C4LeagueResponseHead

void C4LeagueResponseHead::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(mkParAdapt(Status, StdCompiler::RCT_IdtfAllowEmpty), "Status", ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(CSID, StdCompiler::RCT_IdtfAllowEmpty), "CSID", ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(Message, StdCompiler::RCT_All), "Message", ""));

	// Auth
	pComp->Value(mkNamingAdapt(mkParAdapt(Account, StdCompiler::RCT_All), "Account", ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(AUID, StdCompiler::RCT_All), "AUID", ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(FBID, StdCompiler::RCT_All), "FBID", ""));
	pComp->Value(mkNamingAdapt(mkParAdapt(LoginToken, StdCompiler::RCT_All), "LoginToken", ""));
}

// *** C4LeagueResponseHeadStart

void C4LeagueResponseHeadStart::CompileFunc(StdCompiler *pComp)
{
	// Base members
	C4LeagueResponseHead::CompileFunc(pComp);

	// League name
	pComp->Value(mkNamingAdapt(League, "League", ""));
	pComp->Value(mkNamingAdapt(StreamingAddr, "StreamTo", ""));
	pComp->Value(mkNamingCountAdapt(fHaveSeed, "Seed"));
	if (fHaveSeed)
		pComp->Value(mkNamingAdapt(iSeed, "Seed", (int32_t)0));
	pComp->Value(mkNamingAdapt(iMaxPlayers, "MaxPlayers", (int32_t)0));
}

// *** C4LeagueResponseHeadUpdate

void C4LeagueResponseHeadUpdate::CompileFunc(StdCompiler *pComp)
{
	// Base members
	C4LeagueResponseHead::CompileFunc(pComp);

	// League name
	pComp->Value(mkNamingAdapt(League, "League", ""));

	// player infos
	pComp->Value(mkNamingAdapt(PlrInfos, "PlayerInfos"));
}

// *** C4LeagueRequestHeadAuthCheck

int32_t C4LeagueResponseHeadAuthCheck::getScore(const char *szLeague) const
{
	for (int32_t i = 0; i < C4NetMaxLeagues; i++)
		if (Leagues[i] == szLeague)
			return Scores[i];
	return 0;
}

int32_t C4LeagueResponseHeadAuthCheck::getRank(const char *szLeague) const
{
	for (int32_t i = 0; i < C4NetMaxLeagues; i++)
		if (Leagues[i] == szLeague)
			return Ranks[i];
	return 0;
}

int32_t C4LeagueResponseHeadAuthCheck::getRankSymbol(const char *szLeague) const
{
	for (int32_t i = 0; i < C4NetMaxLeagues; i++)
		if (Leagues[i] == szLeague)
			return RankSymbols[i];
	return 0;
}

const char *C4LeagueResponseHeadAuthCheck::getProgressData(const char *szLeague) const
{
	// progress data is the same for all leagues
	return ProgressData.getData();
}

void C4LeagueResponseHeadAuthCheck::CompileFunc(StdCompiler *pComp)
{
	// Base members
	C4LeagueResponseHead::CompileFunc(pComp);

	// Leagues, Scores, Ranks
	pComp->Value(mkNamingAdapt(mkArrayAdapt(Leagues, C4NetMaxLeagues, ""), "League"));
	pComp->Value(mkNamingAdapt(mkArrayAdapt(Scores, C4NetMaxLeagues, 0), "Score"));
	pComp->Value(mkNamingAdapt(mkArrayAdapt(Ranks, C4NetMaxLeagues, 0), "Rank"));
	pComp->Value(mkNamingAdapt(mkArrayAdapt(RankSymbols, C4NetMaxLeagues, 0), "RankSymbol"));

	// Progress data (per scenario; not per league)
	pComp->Value(mkNamingAdapt(mkParAdapt(ProgressData, StdCompiler::RCT_All), "ProgressData", ""));

	// Clan tag
	pComp->Value(mkNamingAdapt(mkParAdapt(ClanTag, StdCompiler::RCT_All), "ClanTag", ""));

}

// *** C4LeagueFBIDList

void C4LeagueFBIDList::Clear()
{
	while (pFirst)
	{
		FBIDItem *pDel = pFirst;
		pFirst = pDel->pNext;
		delete pDel;
	}
}

bool C4LeagueFBIDList::FindFBIDByAccount(const char *szAccount, StdStrBuf *pFBIDOut)
{
	assert(szAccount);
	if (!szAccount) return false;
	for (FBIDItem *pItem = pFirst; pItem; pItem = pItem->pNext)
		if (pItem->Account == szAccount)
		{
			if (pFBIDOut) pFBIDOut->Copy(pItem->FBID);
			return true;
		}
	return false;
}

void C4LeagueFBIDList::RemoveFBIDByAccount(const char *szAccount)
{
	FBIDItem *pPrev = nullptr, *pItem = pFirst;
	while (pItem)
	{
		// Delete?
		if (pItem->Account == szAccount)
		{
			(pPrev ? pPrev->pNext : pFirst) = pItem->pNext;
			delete pItem;
			return;
		}
		// Next
		pPrev = pItem; pItem = pItem->pNext;
	}
}

void C4LeagueFBIDList::AddFBID(const char *szFBID, const char *szAccount)
{
	// add new FBID item to head of list
	assert(szFBID); assert(szAccount);
	// remove any existing FBIDs
	RemoveFBIDByAccount(szAccount);
	// add new entry
	FBIDItem *pNewItem = new FBIDItem();
	pNewItem->FBID.Copy(szFBID);
	pNewItem->Account.Copy(szAccount);
	pNewItem->pNext = pFirst;
	pFirst = pNewItem;
}

class DisconnectData
{
private:
	C4LeagueFBIDList &rFBIDList;
	const C4ClientPlayerInfos &rPlayerInfos;

public:
	DisconnectData(C4LeagueFBIDList &rFBIDList, const C4ClientPlayerInfos &rPlayerInfos)
			: rFBIDList(rFBIDList), rPlayerInfos(rPlayerInfos) {}

	void CompileFunc(StdCompiler *pComp)
	{
		if (pComp->isDeserializer())
		{
			// compiling not yet needed
			assert(!"DisconnectData::CompileFunc not defined for compiler!");
		}
		else
		{
			// decompiling: Compile all joined and not removed player infos.
			//   Compile them even if they're not in the FBID-List, but omit
			//   the FBID (used for host message)
			int32_t i=0; C4PlayerInfo *pInfo;
			while ((pInfo = rPlayerInfos.GetPlayerInfo(i++)))
				if (pInfo->IsJoined() && !pInfo->IsRemoved())
				{
					pComp->Name("Player");
					try
					{
						pComp->Value(mkNamingAdapt(mkDecompileAdapt(pInfo->GetID()), "ID"));
						StdCopyStrBuf sFBID;
						if (rFBIDList.FindFBIDByAccount(pInfo->getLeagueAccount(), &sFBID)) pComp->Value(mkNamingAdapt(mkParAdapt(sFBID, StdCompiler::RCT_IdtfAllowEmpty), "FBID"));
					}
					catch (StdCompiler::Exception *)
					{
						pComp->NameEnd();
						throw;
					}
					pComp->NameEnd();
				}
		}
	}
};


// *** C4LeagueClient

bool C4LeagueClient::Start(const C4Network2Reference &Ref)
{
	// Create query
	eCurrAction = C4LA_Start;
	C4LeagueRequestHead Head(eCurrAction);
	StdStrBuf QueryText = DecompileToBuf<StdCompilerINIWrite>(
	                        mkInsertAdapt(
	                          mkNamingAdapt(Head, "Request"),
	                          mkNamingAdapt(mkDecompileAdapt(Ref), "Reference"),
	                          false));
	// Perform query
	return Query(QueryText.getData(), false);
}

bool C4LeagueClient::GetStartReply(StdStrBuf *pMessage, StdStrBuf *pLeague, StdStrBuf *pStreamingAddr, int32_t *pSeed, int32_t *pMaxPlayers)
{
	if (!isSuccess() || eCurrAction != C4LA_Start) return false;
	// Parse response head
	C4LeagueResponseHeadStart Head;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(Head, "Response"), ResultString, "Start Reply"))
		return false;
	// Get message, league and seed
	if (pMessage)
		pMessage->Copy(Head.getMessage());
	if (pLeague)
		pLeague->Copy(Head.getLeague());
	if (pStreamingAddr)
		pStreamingAddr->Copy(Head.getStreamingAddr());
	if (pSeed && Head.haveSeed())
		*pSeed = Head.getSeed();
	if (pMaxPlayers)
		*pMaxPlayers = Head.getMaxPlayers();
	// No success?
	if (!Head.isSuccess())
		return false;
	// Got no CSID or empty CSID?
	if (!Head.getCSID() || !*Head.getCSID() || *Head.getCSID() == '\0')
	{
		if (pMessage)
			pMessage->Copy(LoadResStr("IDS_LGA_INVALIDRESPONSE3"));
		return false;
	}
	// So save back CSID
	CSID = Head.getCSID();
	return true;
}

bool C4LeagueClient::Update(const C4Network2Reference &Ref)
{
	assert(CSID.getLength());
	// Create query
	eCurrAction = C4LA_Update;
	C4LeagueRequestHead Head(eCurrAction, CSID.getData());
	StdStrBuf QueryText = DecompileToBuf<StdCompilerINIWrite>(
	                        mkInsertAdapt(
	                          mkNamingAdapt(Head, "Request"),
	                          mkNamingAdapt(mkDecompileAdapt(Ref), "Reference"),
	                          false));
	// Perform query
	return Query(QueryText.getData(), false);
}

bool C4LeagueClient::GetUpdateReply(StdStrBuf *pMessage, C4ClientPlayerInfos *pPlayerLeagueInfos)
{
	C4LeagueResponseHeadUpdate Reply;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(Reply, "Response"), ResultString, "Update Reply"))
		return false;
	// Get message
	if (pMessage)
		pMessage->Copy(Reply.getMessage());
	// get plr infos
	if (pPlayerLeagueInfos)
		*pPlayerLeagueInfos = Reply.GetPlrInfos();
	// Success
	return true;
}

bool C4LeagueClient::End(const C4Network2Reference &Ref, const char *szRecordName, const BYTE *pRecordSHA)
{
	assert(CSID.getLength());
	// Create query
	eCurrAction = C4LA_End;
	C4LeagueRequestHeadEnd Head(eCurrAction, CSID.getData(), szRecordName, pRecordSHA);
	StdStrBuf QueryText = DecompileToBuf<StdCompilerINIWrite>(
	                        mkInsertAdapt(
	                          mkNamingAdapt(Head, "Request"),
	                          mkNamingAdapt(mkDecompileAdapt(Ref), "Reference"),
	                          false));
	// Perform query
	return Query(QueryText.getData(), false);
}

bool C4LeagueClient::GetEndReply(StdStrBuf *pMessage, C4RoundResultsPlayers *pRoundResults)
{
	// Parse response head
	C4LeagueResponseHead Head;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(Head, "Response"), ResultString, "End Reply"))
		return false;
	// Get message
	if (pMessage)
		pMessage->Copy(Head.getMessage());
	if (pRoundResults)
		CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(mkNamingAdapt(*pRoundResults, "PlayerInfos"), "Response"), ResultString, "Round Results");
	// Done
	return Head.isSuccess();
}

bool C4LeagueClient::Auth(const C4PlayerInfo &PlrInfo, const char *szAccount, const char *szPassword, const char *szNewAccount, const char *szNewPassword, bool fRememberLogin)
{
	// Build header
	eCurrAction = C4LA_PlrAuth;
	C4LeagueRequestHead Head(eCurrAction);
	Head.SetAuth(szAccount, szPassword, fRememberLogin);
	if (szNewAccount)
		Head.SetNewAccount(szNewAccount);
	if (szNewPassword)
		Head.SetNewPassword(szNewPassword);
	// Create query
	StdStrBuf QueryText = DecompileToBuf<StdCompilerINIWrite>(
	                        mkInsertAdapt(
	                          mkNamingAdapt(Head, "Request"),
	                          mkNamingAdapt(mkDecompileAdapt(PlrInfo), "PlrInfo"),
	                          false));
	// Perform query
	return Query(QueryText.getData(), false);
}

bool C4LeagueClient::GetAuthReply(StdStrBuf *pMessage, StdStrBuf *pAUID, StdStrBuf *pAccount, bool *pRegister, StdStrBuf *pLoginToken)
{
	C4LeagueResponseHead Head;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(Head, "Response"), ResultString, "Auth Reply"))
		return false;
	// Get message & account
	if (pMessage)
		pMessage->Copy(Head.getMessage());
	if (pAccount)
		pAccount->Copy(Head.getAccount());
	if (pRegister)
		*pRegister = Head.isStatusRegister();
	if (pLoginToken)
		pLoginToken->Copy(Head.getLoginToken());
	// No success?
	if (!Head.isSuccess())
		return false;
	// Check AUID
	if (!Head.getAUID() || !*Head.getAUID())
	{
		pMessage->Ref(LoadResStr("IDS_MSG_LEAGUESERVERREPLYWITHOUTA"));
		return false;
	}
	// Success
	if (pAUID)
		pAUID->Copy(Head.getAUID());
	FBIDList.AddFBID(Head.getFBID(), Head.getAccount());
	return true;
}

bool C4LeagueClient::AuthCheck(const C4PlayerInfo &PlrInfo)
{
	assert(CSID.getLength());
	// Build header
	eCurrAction = C4LA_PlrAuthCheck;
	C4LeagueRequestHead Head(eCurrAction, CSID.getData(), PlrInfo.getAuthID());
	// Create query
	StdStrBuf QueryText = DecompileToBuf<StdCompilerINIWrite>(
	                        mkInsertAdapt(
	                          mkNamingAdapt(Head, "Request"),
	                          mkNamingAdapt(mkDecompileAdapt(PlrInfo), "PlrInfo"),
	                          false));
	// Perform query
	return Query(QueryText.getData(), false);
}

bool C4LeagueClient::GetAuthCheckReply(StdStrBuf *pMessage, const char *szLeague, C4PlayerInfo *pPlrInfo)
{
	// Parse response head
	C4LeagueResponseHeadAuthCheck Head;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(Head, "Response"), ResultString, "Auth Check Reply"))
		return false;
	// Get message and additional data
	if (pMessage)
		pMessage->Copy(Head.getMessage());
	if (szLeague && pPlrInfo)
		pPlrInfo->SetLeagueData(Head.getAccount(), Head.getClanTag(), Head.getScore(szLeague), Head.getRank(szLeague), Head.getRankSymbol(szLeague), Head.getProgressData(szLeague));
	return Head.isSuccess();
}

bool C4LeagueClient::ReportDisconnect(const C4ClientPlayerInfos &rFeedbackClient, C4LeagueDisconnectReason eReason)
{
	// Build header
	eCurrAction = C4LA_ReportDisconnect;
	C4LeagueReportDisconnectHead Head(CSID.getData(), eReason);
	// Create query
	StdStrBuf QueryText = DecompileToBuf<StdCompilerINIWrite>(
	                        mkInsertAdapt(
	                          mkNamingAdapt(Head, "Request"),
	                          mkNamingAdapt(DisconnectData(FBIDList, rFeedbackClient), "PlayerInfos"),
	                          false));
	// Perform query
	return Query(QueryText.getData(), false);
}

bool C4LeagueClient::GetReportDisconnectReply(StdStrBuf *pMessage)
{
	// Parse response head
	C4LeagueResponseHead Head;
	if (!CompileFromBuf_LogWarn<StdCompilerINIRead>(mkNamingAdapt(Head, "Response"), ResultString, "Report Disconnect"))
		return false;
	// Get message
	if (pMessage)
		pMessage->Copy(Head.getMessage());
	// Done
	return Head.isSuccess();
}

// *** C4LeagueSignupDialog

C4LeagueSignupDialog::C4LeagueSignupDialog(const char *szPlayerName, const char *szLeagueName, const char *szLeagueServerName, const char *szAccountPref, const char *szPassPref, bool fWarnThirdParty, bool fRegister, bool fRememberLogin)
		: C4GUI::Dialog(C4GUI_MessageDlgWdt, 100 /* will be resized as needed */, FormatString(LoadResStr("IDS_DLG_LEAGUESIGNUPON"), szLeagueServerName).getData(), false), strPlayerName(szPlayerName), pChkRememberLogin(nullptr)
{
	// get positions
	C4GUI::ComponentAligner caMain(GetClientRect(), C4GUI_DefDlgIndent, C4GUI_DefDlgIndent, true);
	// place icon
	C4Rect rcIcon = caMain.GetFromLeft(C4GUI_IconWdt); rcIcon.Hgt = C4GUI_IconHgt;
	C4GUI::Icon *pIcon = new C4GUI::Icon(rcIcon, C4GUI::Ico_Ex_League); AddElement(pIcon);
	caMain.GetFromRight(C4GUI_IconWdt/2);
	// place message label
	// use text with line breaks
	StdStrBuf sMsg, sMsgBroken;
	if (fRegister)
		sMsg.Format(LoadResStr("IDS_MSG_LEAGUE_REGISTRATION"), szPlayerName);
	else
		sMsg.Format(LoadResStr("IDS_MSG_PASSWORDFORPLAYER"), szPlayerName);
	int32_t iLabelHgt = ::GraphicsResource.TextFont.BreakMessage(sMsg.getData(), caMain.GetInnerWidth(), &sMsgBroken, true);
	C4GUI::Label *pLblMessage = new C4GUI::Label(sMsgBroken.getData(), caMain.GetFromTop(iLabelHgt), ALeft, C4GUI_MessageFontClr, &::GraphicsResource.TextFont);
	AddElement(pLblMessage);
	// registering and no account pref available
	if (fRegister && (!szAccountPref || !szAccountPref[0]))
		// use player name as default for league name
		szAccountPref = szPlayerName;
	// place username input box
	bool fSideEdits=true; int iCtrlHeight;
	StdStrBuf sAccountTxt; sAccountTxt.Copy(LoadResStr("IDS_CTL_LEAGUE_ACCOUNT"));
	C4GUI::LabeledEdit::GetControlSize(nullptr, &iCtrlHeight, sAccountTxt.getData(), nullptr, fSideEdits);
	AddElement(pEdtAccount = new C4GUI::LabeledEdit(caMain.GetFromTop(iCtrlHeight), sAccountTxt.getData(), fSideEdits, szAccountPref));
	// registering? Make password field optional
	if (fRegister)
	{
		// place the checkbox
		const char *szChkPasswordCaption = LoadResStr("IDS_CTL_LEAGUE_CHK_PLRPW");
		C4GUI::CheckBox::GetStandardCheckBoxSize(nullptr, &iCtrlHeight, szChkPasswordCaption, nullptr);
		AddElement(pChkPassword = new C4GUI::CheckBox(caMain.GetFromTop(iCtrlHeight), szChkPasswordCaption, false));
		pChkPassword->SetOnChecked(new C4GUI::CallbackHandlerNoPar<C4LeagueSignupDialog>(this, &C4LeagueSignupDialog::OnChkPassword));
		pChkPassword->SetToolTip(LoadResStr("IDS_DESC_LEAGUECHECKPASSWORD"));
		// place password edit boxes
		C4GUI::ComponentAligner caTemp = caMain;
		const char *szEdtPassCaption = LoadResStr("IDS_CTL_LEAGUE_PLRPW");
		const char *szEdtPass2Caption = LoadResStr("IDS_CTL_LEAGUE_PLRPW2");
		C4GUI::LabeledEdit::GetControlSize(nullptr, &iCtrlHeight, szEdtPassCaption, nullptr, fSideEdits);
		AddElement(pEdtPass = new C4GUI::LabeledEdit(caTemp.GetFromTop(iCtrlHeight), szEdtPassCaption, fSideEdits, szPassPref));
		AddElement(pEdtPass2 = new C4GUI::LabeledEdit(caTemp.GetFromTop(iCtrlHeight), szEdtPass2Caption, fSideEdits, szPassPref));
		// hide them
		pEdtPass->SetVisibility(false);
		pEdtPass2->SetVisibility(false);
		// save how much to move the controls later
		iEdtPassSpace = caTemp.GetHeight() - caMain.GetHeight();
	}
	else
	{
		// No password checkbox
		pChkPassword = nullptr;
		// But a password edit box
		const char *szEdtPassCaption = LoadResStr("IDS_CTL_LEAGUE_PLRPW");
		C4GUI::LabeledEdit::GetControlSize(nullptr, &iCtrlHeight, szEdtPassCaption, nullptr, fSideEdits);
		AddElement(pEdtPass = new C4GUI::LabeledEdit(caMain.GetFromTop(iCtrlHeight), szEdtPassCaption, fSideEdits, szPassPref));
		// No second password edit box
		pEdtPass2 = nullptr;
		// remember login-checkbox
		const char *szRememberPasswordCaption = LoadResStr("IDS_CTL_LEAGUE_CHK_REMEMBERLOGIN");
		C4GUI::CheckBox::GetStandardCheckBoxSize(nullptr, &iCtrlHeight, szRememberPasswordCaption, nullptr);
		AddElement(pChkRememberLogin = new C4GUI::CheckBox(caMain.GetFromTop(iCtrlHeight), szRememberPasswordCaption, fRememberLogin));
		pChkRememberLogin->SetToolTip(LoadResStr("IDS_DESC_REMEMBERLOGIN"));
	}
	// Set password box options
	pEdtPass->GetEdit()->SetPasswordMask('*');
	if (pEdtPass2)
	{
		pEdtPass2->GetEdit()->SetPasswordMask('*');
	}
	// place confirmation buttons
	C4GUI::ComponentAligner caButtonArea(caMain.GetFromTop(C4GUI_ButtonAreaHgt), 0,0);
	C4Rect rcBtn = caButtonArea.GetCentered(2*C4GUI_DefButton2Wdt+C4GUI_DefButton2HSpace, C4GUI_ButtonHgt);
	rcBtn.Wdt = C4GUI_DefButton2Wdt;
	pBtnOK = new C4GUI::OKButton(rcBtn);
	AddElement(pBtnOK);
	rcBtn.x += C4GUI_DefButton2Wdt+C4GUI_DefButton2HSpace;
	pBtnAbort = new C4GUI::CancelButton(rcBtn);
	AddElement(pBtnAbort);
	// resize to actually needed size
	SetClientSize(GetClientRect().Wdt, GetClientRect().Hgt - caMain.GetHeight());
	// initial focus
	SetFocus(fRegister ? pEdtAccount->GetEdit() : pEdtPass->GetEdit(), false);
}

void C4LeagueSignupDialog::UserClose(bool fOK)
{
	// Abort? That's always okay
	if (!fOK)
	{
		Dialog::UserClose(fOK);
		::pGUI->ShowMessageModal(FormatString(LoadResStr("IDS_MSG_LEAGUESIGNUPCANCELLED"), strPlayerName.getData()).getData(), LoadResStr("IDS_DLG_LEAGUESIGNUP"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Notify);
		return;
	}
	// Check for empty account name
	const char *szAccount = pEdtAccount->GetText();
	if (!szAccount || !*szAccount)
	{
		SetFocus(pEdtAccount->GetEdit(), false);
		::pGUI->ShowMessageModal(LoadResStr("IDS_MSG_LEAGUEMISSINGUSERNAME"), LoadResStr("IDS_DLG_INVALIDENTRY"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
		return;
	}
	// Check for valid username if this is registration
	if (pEdtPass2)
	{
		// Username contains invalid characters
		if (SCharCountEx(szAccount, C4League_Name_Valid_Characters) != SLen(szAccount))
		{
			SetFocus(pEdtAccount->GetEdit(), false);
			::pGUI->ShowMessageModal(LoadResStr("IDS_MSG_LEAGUEINVALIDUSERNAME"), LoadResStr("IDS_DLG_INVALIDENTRY"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return;
		}
		// Username is too short
		if (SLen(szAccount) < 3)
		{
			SetFocus(pEdtAccount->GetEdit(), false);
			::pGUI->ShowMessageModal(LoadResStr("IDS_MSG_LEAGUEUSERNAMETOOSHORT"), LoadResStr("IDS_DLG_INVALIDENTRY"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return;
		}
	}
	// Check password
	if (!pChkPassword || pChkPassword->GetChecked())
	{
		// Check for empty password
		const char *szPassword = pEdtPass->GetText();
		if (!szPassword || !*szPassword)
		{
			SetFocus(pEdtPass->GetEdit(), false);
			::pGUI->ShowMessageModal(LoadResStr("IDS_MSG_LEAGUEMISSINGPASSWORD"), LoadResStr("IDS_DLG_INVALIDENTRY"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return;
		}
		// Check second password
		if (pEdtPass2 && !SEqual(szPassword, pEdtPass2->GetText()))
		{
			SetFocus(pEdtPass2->GetEdit(), false);
			pEdtPass2->GetEdit()->SetText("", false);
			::pGUI->ShowMessageModal(LoadResStr("IDS_MSG_LEAGUEMISMATCHPASSWORD"), LoadResStr("IDS_DLG_INVALIDENTRY"), C4GUI::MessageDialog::btnOK, C4GUI::Ico_Error);
			return;
		}
	}
	// Okay then
	Dialog::UserClose(fOK);
}

bool C4LeagueSignupDialog::ShowModal(const char *szPlayerName, const char *szLeagueName, const char *szLeagueServerName, StdStrBuf *psCUID, StdStrBuf *psPass, bool fWarnThirdParty, bool fRegister, bool *pfRememberLogin)
{
	// show league signup dlg modally; return whether user pressed OK and change user and pass buffers in that case
	assert(psCUID); assert(psPass);
	if (!psCUID || !psPass) return false;
	C4LeagueSignupDialog *pDlg = new C4LeagueSignupDialog(szPlayerName, szLeagueName, szLeagueServerName, psCUID->getData(), psPass->getData(), fWarnThirdParty, fRegister, *pfRememberLogin);
	bool fResult = ::pGUI->ShowModalDlg(pDlg, false);
	if (fResult)
	{
		psCUID->Copy(pDlg->GetAccount());
		if (pDlg->HasPass())
			psPass->Copy(pDlg->GetPass());
		else
			psPass->Clear();
		*pfRememberLogin = pDlg->GetRememberLogin();
	}
	delete pDlg;
	return fResult;
}

void C4LeagueSignupDialog::OnChkPassword()
{
	if (!pChkPassword) return;
	// Show password elements?
	if (!pChkPassword->GetChecked())
	{
		// Enlarge dialog
		C4Rect bnds = GetClientRect();
		SetClientSize(bnds.Wdt, bnds.Hgt + iEdtPassSpace);
		// Show edit controls
		pEdtPass->SetVisibility(false);
		pEdtPass2->SetVisibility(false);
		// Move controls down
		bnds = pBtnOK->GetBounds();
		pBtnOK->SetPos(bnds.x, bnds.y + iEdtPassSpace);
		bnds = pBtnAbort->GetBounds();
		pBtnAbort->SetPos(bnds.x, bnds.y + iEdtPassSpace);
	}
	else
	{
		// Shrink dialog
		C4Rect bnds = GetClientRect();
		SetClientSize(bnds.Wdt, bnds.Hgt - iEdtPassSpace);
		// Hide edit controls
		pEdtPass->SetVisibility(true);
		pEdtPass2->SetVisibility(true);
		// Move controls down
		bnds = pBtnOK->GetBounds();
		pBtnOK->SetPos(bnds.x, bnds.y - iEdtPassSpace);
		bnds = pBtnAbort->GetBounds();
		pBtnAbort->SetPos(bnds.x, bnds.y - iEdtPassSpace);
	}
}
