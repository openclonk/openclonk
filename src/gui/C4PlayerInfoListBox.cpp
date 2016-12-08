/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2008-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// player listbox used in lobby and game over dlg

#include "C4Include.h"
#include "gui/C4PlayerInfoListBox.h"

#include "control/C4PlayerInfo.h"
#include "network/C4Network2Dialogs.h"
#include "control/C4Teams.h"
#include "game/C4Game.h"
#include "gui/C4FileSelDlg.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"
#include "gui/C4MouseControl.h"
#include "network/C4Network2.h"
#include "control/C4GameControl.h"
#include "control/C4RoundResults.h"
#include "gui/C4GameLobby.h"

DWORD GenerateRandomPlayerColor(int32_t iTry); // in C4PlayerInfoConflicts.cpp

// ----------- ListItem --------------------------------------------------------------------------------

// helper
C4GameLobby::MainDlg *C4PlayerInfoListBox::ListItem::GetLobby() const
{
	return ::Network.GetLobby();
}

bool C4PlayerInfoListBox::ListItem::CanLocalChooseTeams(int32_t idPlayer) const
{
	// whether the local client can change any teams
	// only if teams are available
	if (!Game.Teams.IsMultiTeams()) return false;
	// only if global change allowed
	C4GameLobby::MainDlg *pLobby = GetLobby();
	if (!pLobby) return false;
	if (pLobby->IsCountdown()) return false;
	// only for unjoined players
	if (idPlayer)
	{
		C4PlayerInfo *pInfo = Game.PlayerInfos.GetPlayerInfoByID(idPlayer);
		if (pInfo && pInfo->HasJoined()) return false;
	}
	// finally, only if team settings permit
	return Game.Teams.CanLocalChooseTeam(idPlayer);
}

void C4PlayerInfoListBox::ListItem::DrawElement(C4TargetFacet &cgo)
{
	if (dwBackground) pDraw->DrawBoxDw(cgo.Surface, cgo.TargetX+rcBounds.x, cgo.TargetY+rcBounds.y, cgo.TargetX+rcBounds.x+rcBounds.Wdt-1, cgo.TargetY+rcBounds.y+rcBounds.Hgt-1, dwBackground);
	typedef C4GUI::Window BaseClass;
	BaseClass::DrawElement(cgo);
}

// ----------- PlayerListItem -----------------------------------------------------------------------

C4PlayerInfoListBox::PlayerListItem::PlayerListItem(C4PlayerInfoListBox *pForListBox, int32_t idClient,
    int32_t idPlayer, bool fSavegamePlayer, C4GUI::Element *pInsertBeforeElement)
		: ListItem(pForListBox), pScoreLabel(nullptr), pTimeLabel(nullptr), pExtraLabel(nullptr),
		pRankIcon(nullptr), pTeamCombo(nullptr), pTeamPic(nullptr), fIconSet(false), fJoinedInfoSet(false),
		dwJoinClr(0), dwPlrClr(0), idClient(idClient), idPlayer(idPlayer), fFreeSavegamePlayer(fSavegamePlayer)
{
	bool fIsEvaluation = pForListBox->IsEvaluation(), fIsLobby = pForListBox->IsLobby();
	C4PlayerInfo *pInfo = GetPlayerInfo(); assert(pInfo);
	uint32_t dwTextColor = pForListBox->GetTextColor();
	CStdFont *pCustomFont = pForListBox->GetCustomFont();
	uint32_t dwPlayerColor;
	if (fIsEvaluation)
		dwPlayerColor = dwTextColor;
	else
		dwPlayerColor = pInfo->GetLobbyColor() | C4GUI_MessageFontAlpha;
	// league account name? Overwrite the shown name
	StdStrBuf sPlayerName(pInfo->GetLobbyName());
	// calc height
	int32_t iHeight = ::GraphicsResource.TextFont.GetLineHeight() + C4GUI::ComboBox::GetDefaultHeight() + 3 * IconLabelSpacing;
	// create subcomponents
	pIcon = new C4GUI::Icon(C4Rect(0, 0, iHeight, iHeight), C4GUI::Ico_UnknownPlayer);
	if (Game.Parameters.isLeague())
		pRankIcon = new C4GUI::Icon(C4Rect(0, 0, C4GUI::ComboBox::GetDefaultHeight(), C4GUI::ComboBox::GetDefaultHeight()), C4GUI::Ico_None);
	if (Game.Teams.IsMultiTeams() && !(fIsEvaluation && pList->IsTeamFilter()))
	{
		// will be moved when the item is added to the list, and the position moved
		pTeamCombo = new C4GUI::ComboBox(C4Rect(0,0,10,10));
		pTeamCombo->SetComboCB(new C4GUI::ComboBox_FillCallback<PlayerListItem>(this, &PlayerListItem::OnTeamComboFill, &PlayerListItem::OnTeamComboSelChange));
		pTeamCombo->SetSimple(true);
		if (pCustomFont)
		{
			pTeamCombo->SetFont(pCustomFont);
			pTeamCombo->SetColors(dwTextColor, C4GUI_StandardBGColor, 0);
		}
		UpdateTeam();
	}
	// Evaluation
	if (fIsEvaluation)
	{
		// Team image if known and if not placed on top of box anyway
		if (!pList->IsTeamFilter())
		{
			C4Team *pTeam = Game.Teams.GetTeamByID(pInfo->GetTeam());
			if (pTeam && pTeam->GetIconSpec() && *pTeam->GetIconSpec())
			{
				pTeamPic = new C4GUI::Picture(C4Rect(iHeight + IconLabelSpacing, 0, iHeight, iHeight), true);
				Game.DrawTextSpecImage(pTeamPic->GetMFacet(), pTeam->GetIconSpec(), nullptr, pTeam->GetColor());
				pTeamPic->SetDrawColor(pTeam->GetColor());
			}
		}
		// Total playing time (not in team filter because then the second line is taken by the score label)
		if (!pList->IsTeamFilter())
		{
			StdStrBuf sTimeLabelText;
			C4RoundResultsPlayer *pRoundResultsPlr = Game.RoundResults.GetPlayers().GetByID(idPlayer);
			uint32_t iTimeTotal = pRoundResultsPlr ? pRoundResultsPlr->GetTotalPlayingTime() : 0 /* unknown - should not happen */;
			sTimeLabelText.Format(LoadResStr("IDS_CTL_TOTALPLAYINGTIME"), iTimeTotal/3600, (iTimeTotal/60)%60, iTimeTotal%60);
			pTimeLabel = new C4GUI::Label(sTimeLabelText.getData(), 0, 0, ARight, dwTextColor, pForListBox->GetCustomFont(), false, true);
		}
		// Extra info set by script
		C4RoundResultsPlayer *pEvaluationPlayer = Game.RoundResults.GetPlayers().GetByID(idPlayer);;
		if (pEvaluationPlayer)
		{
			const char *szCustomEval = pEvaluationPlayer->GetCustomEvaluationStrings();
			if (szCustomEval && *szCustomEval)
			{
				pExtraLabel = new C4GUI::Label(szCustomEval, 0,0, ARight); // positioned later
				iHeight += ::GraphicsResource.TextFont.GetLineHeight();
			}
		}
	}
	pNameLabel = new C4GUI::Label(sPlayerName.getData(), (iHeight + IconLabelSpacing) * (1+!!pTeamPic), IconLabelSpacing, ALeft, dwPlayerColor, pCustomFont, !fIsEvaluation, true);
	// calc own bounds - list box needs height only; width and pos will be moved by list
	SetBounds(C4Rect(0,0,10,iHeight));
	// add components
	AddElement(pIcon); AddElement(pNameLabel);
	if (pTeamPic) AddElement(pTeamPic);
	if (pTimeLabel) AddElement(pTimeLabel);
	if (pTeamCombo) AddElement(pTeamCombo);
	if (pRankIcon) AddElement(pRankIcon);
	if (pExtraLabel) AddElement(pExtraLabel);
	// add to listbox (will get resized horizontally and moved)
	pForListBox->InsertElement(this, pInsertBeforeElement, PlayerListBoxIndent);
	// league score update
	UpdateScoreLabel(pInfo);
	// set ID
	if (fFreeSavegamePlayer)
		idListItemID.idType = ListItem::ID::PLI_SAVEGAMEPLR;
	else
		idListItemID.idType = ListItem::ID::PLI_PLAYER;
	idListItemID.id = idPlayer;
	UpdateIcon(pInfo, GetJoinedInfo());
	// context menu for list item
	if (fIsLobby) SetContextHandler(new C4GUI::CBContextHandler<PlayerListItem>(this, &PlayerListItem::OnContext));
	// update collapsed/not collapsed
	fShownCollapsed = false;
	UpdateCollapsed();
}

void C4PlayerInfoListBox::PlayerListItem::UpdateOwnPos()
{
	// parent for client rect
	typedef C4GUI::Window ParentClass;
	ParentClass::UpdateOwnPos();
	C4GUI::ComponentAligner caBounds(GetContainedClientRect(), IconLabelSpacing, IconLabelSpacing);
	// subtract icon(s)
	caBounds.GetFromLeft(pIcon->GetBounds().Wdt);
	if (pTeamPic) caBounds.GetFromLeft(pTeamPic->GetBounds().Wdt - IconLabelSpacing);
	C4Rect rcExtraDataRect;
	// extra data label area
	if (pExtraLabel) rcExtraDataRect = caBounds.GetFromBottom(::GraphicsResource.TextFont.GetLineHeight());
	// second line (team+rank)
	C4GUI::ComponentAligner caTeamArea(caBounds.GetFromBottom(C4GUI::ComboBox::GetDefaultHeight()), 0,0);
	C4Rect rcRankIcon;
	if (pList->IsEvaluation())
	{
		if (pRankIcon)
		{
			rcRankIcon = caBounds.GetFromRight(caBounds.GetInnerHeight());
			if (pExtraLabel) rcExtraDataRect.Wdt = caBounds.GetInnerWidth(); // In evaluation view, rank icon has its own coloumn
		}
	}
	else
	{
		if (pRankIcon) rcRankIcon = caTeamArea.GetFromRight(caTeamArea.GetInnerHeight());
	}
	C4Rect rcTeam = caTeamArea.GetAll();
	// item to positions: team combo box
	if (pTeamCombo)
	{
		pTeamCombo->SetBounds(rcTeam);
	}
	// rank icon
	if (pRankIcon)
	{
		pRankIcon->SetBounds(rcRankIcon);
	}
	// time label
	if (pTimeLabel)
	{
		C4Rect rcUpperBounds = caBounds.GetAll();
		pTimeLabel->SetBounds(rcTeam);
		pTimeLabel->SetX0(rcUpperBounds.x + rcUpperBounds.Wdt);
	}
	// extra label
	if (pExtraLabel) pExtraLabel->SetBounds(rcExtraDataRect);
}

int32_t C4PlayerInfoListBox::PlayerListItem::GetListItemTopSpacing()
{
	int32_t iSpacing = C4GUI_DefaultListSpacing;
	// evaluation: Add some extra spacing between players of different teams
	if (pList->IsEvaluation())
	{
		C4GUI::Element *pPrevItem = GetPrev();
		if (pPrevItem)
		{
			C4PlayerInfoListBox::ListItem *pPrevListItem = static_cast<C4PlayerInfoListBox::ListItem *>(pPrevItem);
			if (pPrevListItem->idListItemID.idType == ListItem::ID::PLI_PLAYER)
			{
				PlayerListItem *pPrevPlayerListItem = static_cast<C4PlayerInfoListBox::PlayerListItem *>(pPrevListItem);
				C4PlayerInfo *pThisInfo = GetPlayerInfo();
				C4PlayerInfo *pPrevInfo = pPrevPlayerListItem->GetPlayerInfo();
				if (pThisInfo && pPrevInfo)
				{
					if (pPrevInfo->GetTeam() != pThisInfo->GetTeam())
					{
						iSpacing += 10;
					}
				}
			}
		}
	}
	return iSpacing;
}

void C4PlayerInfoListBox::PlayerListItem::UpdateIcon(C4PlayerInfo *pInfo, C4PlayerInfo *pJoinedInfo)
{
	// check whether icon is known
	bool fResPresent = false;
	C4Network2Res *pRes = nullptr;
	if (pInfo)
		if ((pRes = pInfo->GetRes()))
			fResPresent = pRes->isComplete();
	C4RoundResultsPlayer *pEvaluationPlayer = nullptr;
	if (pList->IsEvaluation()) pEvaluationPlayer = Game.RoundResults.GetPlayers().GetByID(idPlayer);
	bool fHasIcon = fResPresent || pEvaluationPlayer || (!::Network.isEnabled() && pInfo);
	// check whether joined info is present
	bool fHasJoinedInfo = !!pJoinedInfo;
	DWORD dwJoinedInfoClr = pJoinedInfo ? pJoinedInfo->GetLobbyColor() : 0;
	DWORD dwPlayerClr = pInfo ? pInfo->GetLobbyColor() : 0;
	// already up-to-date?
	if (fHasIcon == fIconSet && fJoinedInfoSet == fHasJoinedInfo && dwJoinedInfoClr == dwJoinClr && dwPlayerClr == dwPlrClr) return;
	// update then
	// redraw player icon
	if (fHasIcon)
	{
		// custom icon?
		if (pEvaluationPlayer && pEvaluationPlayer->GetBigIcon().Surface)
		{
			pIcon->SetFacet(pEvaluationPlayer->GetBigIcon());
			fIconSet = true;
		}
		else
			fIconSet = pInfo->LoadBigIcon(pIcon->GetMFacet());
		if (!fIconSet)
		{
			// no custom icon: create default by player color
			pIcon->GetMFacet().Create(64,64); // the bigicon is bigger than the normal 40x40 icon
			::GraphicsResource.fctPlayerClr.DrawClr(pIcon->GetMFacet(), true, dwPlayerClr);
		}
		fIconSet = true;
	}
	else
		// no player info known - either res not retrieved yet or script player
		pIcon->SetIcon((pInfo && pInfo->GetType() == C4PT_Script) ? C4GUI::Ico_Host : C4GUI::Ico_UnknownPlayer);
	// join
	if (fHasJoinedInfo)
	{
		// make sure we're not drawing on GraphicsResource
		if (!pIcon->EnsureOwnSurface()) return;
		// draw join info
		C4Facet fctDraw = pIcon->GetFacet();
		int32_t iSizeMax = std::max<int32_t>(fctDraw.Wdt, fctDraw.Hgt);
		int32_t iCrewClrHgt = iSizeMax/2;
		fctDraw.Hgt -= iCrewClrHgt; fctDraw.Y += iCrewClrHgt;
		fctDraw.Wdt = iSizeMax/2;
		fctDraw.X = 2;
		// shadow
		DWORD dwPrevMod; bool fPrevMod = pDraw->GetBlitModulation(dwPrevMod);
		pDraw->ActivateBlitModulation(1);
		::GraphicsResource.fctCrewClr.DrawClr(fctDraw, true, dwJoinedInfoClr);
		if (fPrevMod) pDraw->ActivateBlitModulation(dwPrevMod); else pDraw->DeactivateBlitModulation();
		fctDraw.X = 0;
		// gfx
		::GraphicsResource.fctCrewClr.DrawClr(fctDraw, true, dwJoinedInfoClr);
	}
	fJoinedInfoSet = fHasJoinedInfo;
	dwJoinClr = dwJoinedInfoClr;
	dwPlrClr = dwPlayerClr;
}

void C4PlayerInfoListBox::PlayerListItem::UpdateTeam()
{
	if (!pTeamCombo) return; // unassigned for no teams
	const char *szTeamName = ""; bool fReadOnly = true;
	fReadOnly = !CanLocalChooseTeam();
	int32_t idTeam; C4Team *pTeam;
	C4PlayerInfo *pInfo = GetPlayerInfo();
	if (!Game.Teams.CanLocalSeeTeam())
		szTeamName = LoadResStr("IDS_MSG_RNDTEAM");
	else if (pInfo)
		if ((idTeam = pInfo->GetTeam()))
			if ((pTeam = Game.Teams.GetTeamByID(idTeam)))
				szTeamName = pTeam->GetName();
	pTeamCombo->SetText(szTeamName);
	pTeamCombo->SetReadOnly(fReadOnly);
}

void C4PlayerInfoListBox::PlayerListItem::UpdateScoreLabel(C4PlayerInfo *pInfo)
{
	assert(pInfo);
	C4RoundResultsPlayer *pRoundResultsPlr = nullptr;
	if (pList->IsEvaluation()) pRoundResultsPlr = Game.RoundResults.GetPlayers().GetByID(idPlayer);

	if (pInfo->getLeagueScore() || pInfo->IsLeagueProjectedGainValid() || pRoundResultsPlr)
	{
		int32_t iScoreRightPos = ((pRankIcon && pList->IsEvaluation()) ? pRankIcon->GetBounds().x : GetBounds().Wdt) - IconLabelSpacing;
		int32_t iScoreYPos = IconLabelSpacing;
		// if evaluation and team lists, move score label into second line - TODO: some hack only, still needs to be done right
		C4RoundResultsPlayer *pEvaluationPlayer = Game.RoundResults.GetPlayers().GetByID(pInfo->GetID());;
		bool fPlayerHasEvaluationData=false;
		if (pEvaluationPlayer)
		{
			const char *szCustomEval = pEvaluationPlayer->GetCustomEvaluationStrings();
			if (szCustomEval && *szCustomEval)
				fPlayerHasEvaluationData=true;
		}
		if (pList->IsEvaluation() && pList->IsTeamFilter())
			iScoreYPos = GetBounds().Hgt - (C4GUI::ComboBox::GetDefaultHeight()*(1+(int32_t)fPlayerHasEvaluationData)) - IconLabelSpacing;

		// score label visible
		if (!pScoreLabel)
		{
			AddElement(pScoreLabel = new C4GUI::Label("", iScoreRightPos, iScoreYPos, ARight, pList->GetTextColor(), pList->GetCustomFont(), false));
			if (pList->IsEvaluation())
				pScoreLabel->SetToolTip(LoadResStr("IDS_DESC_OLDANDNEWSCORE"));
			else
				pScoreLabel->SetToolTip(LoadResStr("IDS_DESC_LEAGUESCOREANDPROJECTEDGA"));
		}
		StdStrBuf sText;
		// Evaluation (GameOver)
		if (pList->IsEvaluation())
		{
			if (pInfo->getLeagueScore() || pInfo->IsLeagueProjectedGainValid() || (pRoundResultsPlr && pRoundResultsPlr->IsLeagueScoreNewValid()))
			{
				if (pRoundResultsPlr && pRoundResultsPlr->IsLeagueScoreNewValid())
				{
					// Show old league score, gain, and new league score
					// Normally, the league server should make sure that [Old score] + [Gain] == [New score]
					int32_t iOldScore = pInfo->getLeagueScore(), iScoreGain = pRoundResultsPlr->GetLeagueScoreGain(), iNewScore = pRoundResultsPlr->GetLeagueScoreNew();
					int32_t iDiscrepancy = iNewScore - (iOldScore + iScoreGain);
					if (!iDiscrepancy)
					{
						sText.Format("{{Ico:League}}<c afafaf>%d (%+d)</c> %d %s", (int)iOldScore, (int)iScoreGain, (int)iNewScore, LoadResStr("IDS_TEXT_SCORE"));
					}
					else
					{
						// If there's a discrepancy, there must have been some kind of admin intervention during the game - display it in red!
						sText.Format("{{Ico:League}}<c afafaf>%d (%+d)</c><c ff0000>(%+d)</c> %d %s", (int)iOldScore, (int)iScoreGain, (int)iDiscrepancy, (int)iNewScore, LoadResStr("IDS_TEXT_SCORE"));
					}
				}
				// Show old league score only
				else
				{
					sText.Format("{{Ico:League}}<c afafaf>(%d)</c> %s", (int)pInfo->getLeagueScore(), LoadResStr("IDS_TEXT_SCORE"));
				}
			}
			else if (pRoundResultsPlr && pRoundResultsPlr->IsScoreNewValid() && !Game.RoundResults.SettlementScoreIsHidden())
			{
				// new score known
				sText.Format("{{Ico:Settlement}}<c afafaf>%d (%+d)</c> %d %s", (int)pRoundResultsPlr->GetScoreOld(), (int)(pRoundResultsPlr->GetScoreNew()-pRoundResultsPlr->GetScoreOld()), (int)pRoundResultsPlr->GetScoreNew(), LoadResStr("IDS_TEXT_SCORE"));
			}
			else if (pRoundResultsPlr && !pRoundResultsPlr->IsScoreNewValid() && !Game.RoundResults.SettlementScoreIsHidden())
			{
				// only old score known (e.g., player disconnected)
				sText.Format("{{Ico:Settlement}}<c afafaf>(%d)</c> %s", (int)pRoundResultsPlr->GetScoreOld(), LoadResStr("IDS_TEXT_SCORE"));
			}
			else
			{
				// nothing known. Shouldn't really happen.
				sText.Ref("");
			}
		}
		// Pre-evaluation (Lobby)
		else
		{
			// Show current league score and projected gain
			// Don't show if team invisible, so random surprise teams don't get spoiled
			if (pInfo->IsLeagueProjectedGainValid() && Game.Teams.IsTeamVisible())
				sText.Format("%d (%+d)", (int)pInfo->getLeagueScore(), (int)pInfo->GetLeagueProjectedGain());
			// Show current league score only
			else
				sText.Format("%d", (int)pInfo->getLeagueScore());
		}
		pScoreLabel->SetX0(iScoreRightPos);
		pScoreLabel->SetText(sText.getData(), false);
	}
	else if (pScoreLabel)
	{
		// score label invisible
		delete pScoreLabel;
		pScoreLabel = nullptr;
	}
	if (pRankIcon)
	{
		int32_t iSym = 0;
		if (pRoundResultsPlr && pRoundResultsPlr->IsLeagueScoreNewValid())
			iSym = pRoundResultsPlr->GetLeagueRankSymbolNew();
		if (!iSym)
			iSym = pInfo->getLeagueRankSymbol();
		if (iSym && !fShownCollapsed)
		{
			C4GUI::Icons eRankIcon = (C4GUI::Icons) (C4GUI::Ico_Rank1 + Clamp<int32_t>(iSym-1, 0, C4GUI::Ico_Rank9-C4GUI::Ico_Rank1));
			pRankIcon->SetVisibility(true);
			pRankIcon->SetIcon(eRankIcon);
		}
		else
		{
			pRankIcon->SetVisibility(false);
		}
	}
}

void C4PlayerInfoListBox::PlayerListItem::UpdateCollapsed()
{
	bool fShouldBeCollapsed = pList->IsPlayerItemCollapsed(this);
	if (fShouldBeCollapsed == fShownCollapsed) return;
	// so update collapsed state
	int32_t iHeight; int32_t iNameLblX0;
	if ((fShownCollapsed = fShouldBeCollapsed))
	{
		// calc height
		iHeight = ::GraphicsResource.TextFont.GetLineHeight() + 2 * IconLabelSpacing;
		// teamcombo not visible if collapsed
		if (pTeamCombo) pTeamCombo->SetVisibility(false);
	}
	else
	{
		// calc height
		iHeight = ::GraphicsResource.TextFont.GetLineHeight() + C4GUI::ComboBox::GetDefaultHeight() + 3 * IconLabelSpacing;
		// teamcombo visible if not collapsed
		if (pTeamCombo) pTeamCombo->SetVisibility(true);
	}
	// update subcomponents
	iNameLblX0 = iHeight + IconLabelSpacing;
	pIcon->GetBounds() = C4Rect(0, 0, iHeight, iHeight);
	pIcon->UpdateOwnPos();
	pNameLabel->SetX0(iNameLblX0);
	// calc own bounds - use icon bounds only, because only the height is used when the item is added
	SetBounds(pIcon->GetBounds());
	// update positions
	pList->UpdateElementPosition(this, PlayerListBoxIndent);
}


C4GUI::ContextMenu *C4PlayerInfoListBox::PlayerListItem::OnContext(C4GUI::Element *pListItem, int32_t iX, int32_t iY)
{
	C4PlayerInfo *pInfo = GetPlayerInfo();
	assert(pInfo);
	// no context menu for evaluation
	if (!GetLobby()) return nullptr;
	// create context menu
	C4GUI::ContextMenu *pMenu = new C4GUI::ContextMenu();
	// if this is a free player, add an option to take it over
	if (fFreeSavegamePlayer)
	{
		if (pInfo->GetType() != C4PT_Script)
		{
			StdCopyStrBuf strTakeOver(LoadResStr("IDS_MSG_TAKEOVERPLR"));
			pMenu->AddItem(strTakeOver.getData(), LoadResStr("IDS_MSG_TAKEOVERPLR_DESC"), C4GUI::Ico_Player, nullptr,
			               new C4GUI::CBContextHandler<PlayerListItem>(this, &PlayerListItem::OnContextTakeOver));
		}
	}
	else
	{
		// owned players or host can manipulate players
		if (::Network.isHost() || IsLocalClientPlayer())
		{
			// player removal (except for joined script players)
			if (pInfo->GetType() != C4PT_Script || !pInfo->GetAssociatedSavegamePlayerID())
			{
				StdCopyStrBuf strRemove(LoadResStr("IDS_MSG_REMOVEPLR"));
				pMenu->AddItem(strRemove.getData(), LoadResStr("IDS_MSG_REMOVEPLR_DESC"), C4GUI::Ico_Close,
				               new C4GUI::CBMenuHandler<PlayerListItem>(this, &PlayerListItem::OnCtxRemove), nullptr);
			}
			// color was changed: Add option to assign a new color
			C4PlayerInfo *pInfo = GetPlayerInfo();
			assert (pInfo);
			if (pInfo && pInfo->HasAutoGeneratedColor() && (!Game.Teams.IsTeamColors() || !pInfo->GetTeam()))
			{
				StdCopyStrBuf strNewColor(LoadResStr("IDS_MSG_NEWPLRCOLOR"));
				pMenu->AddItem(strNewColor.getData(), LoadResStr("IDS_MSG_NEWPLRCOLOR_DESC"), C4GUI::Ico_Player,
				               new C4GUI::CBMenuHandler<PlayerListItem>(this, &PlayerListItem::OnCtxNewColor), nullptr);
			}
		}
	}
	// open it
	return pMenu;
}

C4GUI::ContextMenu *C4PlayerInfoListBox::PlayerListItem::OnContextTakeOver(C4GUI::Element *pListItem, int32_t iX, int32_t iY)
{
	// create context menu
	C4GUI::ContextMenu *pMenu = new C4GUI::ContextMenu();
	// add options for all own, unassigned players
	C4ClientPlayerInfos *pkInfo = ::Network.Players.GetLocalPlayerInfoPacket();
	if (pkInfo)
	{
		int32_t i=0; C4PlayerInfo *pInfo;
		while ((pInfo = pkInfo->GetPlayerInfo(i++)))
			if (!pInfo->HasJoinIssued())
				if (!pInfo->GetAssociatedSavegamePlayerID())
				{
					pMenu->AddItem(FormatString(LoadResStr("IDS_MSG_USINGPLR"), pInfo->GetName()).getData(),
					               LoadResStr("IDS_MSG_USINGPLR_DESC"), C4GUI::Ico_Player,
					               new C4GUI::CBMenuHandlerEx<PlayerListItem, int32_t>(this, &PlayerListItem::OnCtxTakeOver, pInfo->GetID()));
				}
	}
	// add option to use a new one... TODO
	// add option to take over from savegame player TODO
	// open it
	return pMenu;
}

void C4PlayerInfoListBox::PlayerListItem::OnCtxTakeOver(C4GUI::Element *pListItem, const int32_t &idPlayer)
{
	// use player idPlayer to take over this one
	// this must be processed as a request by the host
	// some safety first...
	C4ClientPlayerInfos *pLocalInfo = ::Network.Players.GetLocalPlayerInfoPacket();
	if (!fFreeSavegamePlayer || !idPlayer || !pLocalInfo) return;
	C4ClientPlayerInfos LocalInfoRequest(*pLocalInfo);
	C4PlayerInfo *pGrabbingInfo = LocalInfoRequest.GetPlayerInfoByID(idPlayer);
	if (!pGrabbingInfo) return;
	// now adjust info packet
	pGrabbingInfo->SetAssociatedSavegamePlayer(this->idPlayer);
	// and request this update (host processes it directly)
	::Network.Players.RequestPlayerInfoUpdate(LocalInfoRequest);
}

void C4PlayerInfoListBox::PlayerListItem::OnCtxRemove(C4GUI::Element *pListItem)
{
	// only host or own player
	if (!::Network.isEnabled() || (!::Network.isHost() && !IsLocalClientPlayer())) return;
	// remove the player
	// this must be processed as a request by the host
	// now change it in its own request packet
	C4ClientPlayerInfos *pChangeInfo = Game.PlayerInfos.GetInfoByClientID(idClient);
	if (!pChangeInfo || !idPlayer) return;
	C4ClientPlayerInfos LocalInfoRequest(*pChangeInfo);
	if (!LocalInfoRequest.GetPlayerInfoByID(idPlayer)) return;
	LocalInfoRequest.RemoveInfo(idPlayer);
	// and request this update (host processes it directly)
	::Network.Players.RequestPlayerInfoUpdate(LocalInfoRequest);
}

void C4PlayerInfoListBox::PlayerListItem::OnCtxNewColor(C4GUI::Element *pListItem)
{
	// only host or own player
	if (!::Network.isEnabled() || (!::Network.isHost() && !IsLocalClientPlayer())) return;
	// just send a request to reclaim the original color to the host
	// the host will deny this and decide on a new color
	C4ClientPlayerInfos *pChangeInfo = Game.PlayerInfos.GetInfoByClientID(idClient);
	if (!pChangeInfo || !idPlayer) return;
	C4ClientPlayerInfos LocalInfoRequest(*pChangeInfo);
	C4PlayerInfo *pPlrInfo = LocalInfoRequest.GetPlayerInfoByID(idPlayer);
	if (!pPlrInfo) return;
	pPlrInfo->SetColor(pPlrInfo->GetOriginalColor());
	// and request this update (host processes it directly)
	::Network.Players.RequestPlayerInfoUpdate(LocalInfoRequest);
}

void C4PlayerInfoListBox::PlayerListItem::OnTeamComboFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// add all possible teams
	C4Team *pTeam; int32_t i=0;
	while ((pTeam = Game.Teams.GetTeamByIndex(i++)))
		if (!pTeam->IsFull() || GetPlayerInfo()->GetTeam() == pTeam->GetID())
			pFiller->AddEntry(pTeam->GetName(), pTeam->GetID());
}

bool C4PlayerInfoListBox::PlayerListItem::OnTeamComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
{
	// always return true to mark combo sel as processed, so the GUI won't change the team text
	// get new team id by name
	C4Team *pNewTeam = Game.Teams.GetTeamByID(idNewSelection);
	// some safety first...
	if (!CanLocalChooseTeam() || !pNewTeam) return true;
	C4ClientPlayerInfos *pChangeInfo = Game.PlayerInfos.GetInfoByClientID(idClient);
	if (!pChangeInfo || !idPlayer) return true;
	// this must be processed as a request by the host
	// now change it in its own request packet
	C4ClientPlayerInfos LocalInfoRequest(*pChangeInfo);
	C4PlayerInfo *pChangedInfo = LocalInfoRequest.GetPlayerInfoByID(idPlayer);
	if (!pChangedInfo) return true;
	pChangedInfo->SetTeam(pNewTeam->GetID());
	// and request this update (host processes it directly)
	::Network.Players.RequestPlayerInfoUpdate(LocalInfoRequest);
	// next update will change the combo box text
	return true;
}

void C4PlayerInfoListBox::PlayerListItem::Update()
{
	UpdateCollapsed();
	UpdateIcon(GetPlayerInfo(), GetJoinedInfo());
	UpdateTeam();
	C4PlayerInfo *pNfo = GetPlayerInfo();
	if (pNfo)
	{
		UpdateScoreLabel(pNfo);
		// update name + color
		StdStrBuf sShowName(pNfo->GetLobbyName());
		if (pList->IsEvaluation())
		{
			bool fShowWinners = (pList->GetMode() != PILBM_EvaluationNoWinners);
			bool fHasWon = fShowWinners && pNfo->HasTeamWon();
			// Append "winner" or "loser" to player name
			if (fShowWinners)
			{
				sShowName.Take(FormatString("%s (%s)", sShowName.getData(), LoadResStr(fHasWon ? "IDS_CTL_WON" : "IDS_CTL_LOST")));
			}
			// evaluation: Golden color+background for winners; gray for losers or no winner show
			if (fHasWon)
			{
				pNameLabel->SetColor(C4GUI_WinningTextColor, false);
				dwBackground = C4GUI_WinningBackgroundColor;
			}
			else
			{
				pNameLabel->SetColor(C4GUI_LosingTextColor, false);
				dwBackground = C4GUI_LosingBackgroundColor;
			}
		}
		else
		{
			// lobby: Label color by player color
			pNameLabel->SetColor(pNfo->GetLobbyColor());
		}
		pNameLabel->SetText(sShowName.getData(), false);
	}
}

C4PlayerInfo *C4PlayerInfoListBox::PlayerListItem::GetPlayerInfo() const
{
	return fFreeSavegamePlayer ? Game.RestorePlayerInfos.GetPlayerInfoByID(idPlayer) : Game.PlayerInfos.GetPlayerInfoByID(idPlayer);
}

C4PlayerInfo *C4PlayerInfoListBox::PlayerListItem::GetJoinedInfo() const
{
	// safety
	C4PlayerInfo *pInfo = GetPlayerInfo();
	if (!pInfo) return nullptr;
	// is it a joined savegame player?
	if (fFreeSavegamePlayer)
		// then this is the joined player
		return pInfo;
	// otherwise, does it have a savegame association?
	int32_t idSavegameInfo;
	if ((idSavegameInfo = pInfo->GetAssociatedSavegamePlayerID()))
		// then return the respective info from savegame recreation list
		return Game.RestorePlayerInfos.GetPlayerInfoByID(idSavegameInfo);
	// not joined
	return nullptr;
}

bool C4PlayerInfoListBox::PlayerListItem::CanLocalChooseTeam() const
{
	// never on savegame players
	if (fFreeSavegamePlayer || GetJoinedInfo()) return false;
	// only host or own player
	if (!::Network.isHost() && !IsLocalClientPlayer()) return false;
	// finally, only if team settings permit
	return CanLocalChooseTeams(idPlayer);
}

bool C4PlayerInfoListBox::PlayerListItem::IsLocalClientPlayer() const
{
	// check whether client is local
	// if no client can be found, assume network disconnect and everythign local then
	C4Network2Client *pClient = GetNetClient();
	return !pClient || pClient->isLocal();
}

C4Network2Client *C4PlayerInfoListBox::PlayerListItem::GetNetClient() const
{
	return ::Network.Clients.GetClientByID(idClient);
}


// ----------- ClientListItem -----------------------------------------------------------------

C4PlayerInfoListBox::ClientListItem::ClientListItem(C4PlayerInfoListBox *pForListBox, const C4ClientCore &rClientInfo, ListItem *pInsertBefore) // ctor
		: ListItem(pForListBox), idClient(rClientInfo.getID()), dwClientClr(0xffffff), tLastSoundTime(0)
{
	// set current active-flag (not really needed until player info is retrieved)
	fIsShownActive = rClientInfo.isActivated();
	// set ID
	idListItemID.idType = ListItem::ID::PLI_CLIENT;
	idListItemID.id = idClient;
	// get height
	int32_t iIconSize = ::GraphicsResource.TextFont.GetLineHeight();
	// create subcomponents
	pStatusIcon = new C4GUI::Icon(C4Rect(0, 0, iIconSize, iIconSize), GetCurrentStatusIcon());
	pNameLabel = new C4GUI::Label(rClientInfo.getName(), iIconSize + IconLabelSpacing,0, ALeft, dwClientClr | C4GUI_MessageFontAlpha, nullptr, true, false);
	pPingLabel = nullptr;
	C4GUI::CallbackButton<ClientListItem, C4GUI::IconButton> *btnAddPlayer = nullptr;
	if (IsLocalClientPlayer())
	{
		// this computer: add player button
		btnAddPlayer = new C4GUI::CallbackButton<ClientListItem, C4GUI::IconButton>(C4GUI::Ico_AddPlr, C4Rect(0, 0, iIconSize, iIconSize), 'P' /* 2do TODO */, &ClientListItem::OnBtnAddPlr, this);
	}
	// calc own bounds
	C4Rect rcOwnBounds = pNameLabel->GetBounds();
	rcOwnBounds.Wdt += rcOwnBounds.x; rcOwnBounds.x = 0;
	rcOwnBounds.Hgt += rcOwnBounds.y; rcOwnBounds.y = 0;
	SetBounds(rcOwnBounds);
	// add components
	AddElement(pStatusIcon); AddElement(pNameLabel);
	if (btnAddPlayer) AddElement(btnAddPlayer);
	// tooltip (same for all components for now. separate tooltip for status icon later?)
	SetToolTip(FormatString("Client %s (%s)", rClientInfo.getName(), rClientInfo.getNick()).getData());
	// insert into listbox at correct order
	// (will eventually get resized horizontally and moved)
	pForListBox->InsertElement(this, pInsertBefore);
	// after move: update add player button pos
	if (btnAddPlayer)
	{
		int32_t iHgt = GetClientRect().Hgt;
		btnAddPlayer->GetBounds() = GetToprightCornerRect(iHgt,iHgt,2,0);
	}
	// context menu for list item
	SetContextHandler(new C4GUI::CBContextHandler<ClientListItem>(this, &ClientListItem::OnContext));
	// update (also sets color)
	Update();
}

void C4PlayerInfoListBox::ClientListItem::SetPing(int32_t iToPing)
{
	// no ping?
	if (iToPing == -1)
	{
		// remove any ping label
		if (pPingLabel) { delete pPingLabel; pPingLabel = nullptr; }
		return;
	}
	// get ping as text
	StdStrBuf ping;
	ping.Format("%d ms", iToPing);
	// create ping label if necessary
	if (!pPingLabel)
	{
		pPingLabel = new C4GUI::Label(ping.getData(), GetBounds().Wdt, 0, ARight, C4GUI_MessageFontClr);
		pPingLabel->SetToolTip(LoadResStr("IDS_DLGTIP_PING"));
		AddElement(pPingLabel);
	}
	else
		// or just set updated text
		pPingLabel->SetText(ping.getData());
}

void C4PlayerInfoListBox::ClientListItem::UpdateInfo()
{
	// update color (always, because it can change silently)
	SetColor(::Network.Players.GetClientChatColor(idClient, true));
	// update activation status
	fIsShownActive = GetClient() && GetClient()->isActivated();
	// update status icon
	SetStatus(GetCurrentStatusIcon());
}

C4Client *C4PlayerInfoListBox::ClientListItem::GetClient() const
{
	// search (let's hope it exists)
	return Game.Clients.getClientByID(idClient);
}

bool C4PlayerInfoListBox::ClientListItem::IsLocalClientPlayer() const
{
	// check whether client is local
	// if no client can be found, something is wrong - assume network disconnect and everything local then
	C4Network2Client *pClient = GetNetClient();
	assert(pClient);
	return !pClient || pClient->isLocal();
}

C4Network2Client *C4PlayerInfoListBox::ClientListItem::GetNetClient() const
{
	return ::Network.Clients.GetClientByID(idClient);
}

bool C4PlayerInfoListBox::ClientListItem::IsLocal() const
{
	// it's local if client ID matches local ID
	return idClient == Game.Clients.getLocalID();
}

C4GUI::Icons C4PlayerInfoListBox::ClientListItem::GetCurrentStatusIcon()
{
	if (GetClient()->IsIgnored()) return C4GUI::Ico_Ignored;

	// sound icon?
	if (tLastSoundTime)
	{
		time_t dt = time(nullptr) - tLastSoundTime;
		if (dt >= SoundIconShowTime)
		{
			// stop showing sound icon
			tLastSoundTime = 0;
		}
		else
		{
			// time not up yet: show sound icon
			return C4GUI::Ico_Sound;
		}
	}
	// info present?
	C4ClientPlayerInfos *pInfoPacket = Game.PlayerInfos.GetInfoByClientID(idClient);
	if (!pInfoPacket || !GetClient())
		// unknown status
		return C4GUI::Ico_UnknownClient;
	// host?
	if (GetClient()->isHost()) return C4GUI::Ico_Host;
	// active client?
	if (GetClient()->isActivated())
	{
		if (GetClient()->isLobbyReady())
		{
			return C4GUI::Ico_Ready;
		}
		else
		{
			return C4GUI::Ico_Client;
		}
	}
	// observer
	return C4GUI::Ico_ObserverClient;
}

void C4PlayerInfoListBox::ClientListItem::UpdatePing()
{
	// safety for removed clients
	if (!GetClient()) return;
	// default value indicating no ping
	int32_t iPing = -1;
	C4Network2Client *pClient = GetNetClient();
	C4Network2IOConnection *pConn;
	// must be a remote client
	if (pClient && !pClient->isLocal())
	{
		// must have a connection
		if ((pConn = pClient->getMsgConn()))
			// get ping of that connection
			iPing = pConn->getLag();
		// check data connection if msg conn gave no value
		// what's the meaning of those two connections anyway? o_O
		if (iPing<=0)
			if ((pConn = pClient->getDataConn()))
				iPing = pConn->getLag();
	}
	// set that ping in label
	SetPing(iPing);
}

void C4PlayerInfoListBox::ClientListItem::SetSoundIcon()
{
	// remember time for reset
	tLastSoundTime = time(nullptr);
	// force icon
	SetStatus(GetCurrentStatusIcon());
}

C4GUI::ContextMenu *C4PlayerInfoListBox::ClientListItem::OnContext(C4GUI::Element *pListItem, int32_t iX, int32_t iY)
{
	// safety
	if (!::Network.isEnabled()) return nullptr;
	// get associated client
	C4Client *pClient = GetClient();
	// create context menu
	C4GUI::ContextMenu *pMenu = new C4GUI::ContextMenu();
	// host options
	if (::Network.isHost() && GetNetClient())
	{
		StdCopyStrBuf strKickDesc(LoadResStr("IDS_NET_KICKCLIENT_DESC"));
		pMenu->AddItem(LoadResStr("IDS_NET_KICKCLIENT"), strKickDesc.getData(), C4GUI::Ico_None,
		               new C4GUI::CBMenuHandler<ClientListItem>(this, &ClientListItem::OnCtxKick));
		StdCopyStrBuf strActivateDesc(LoadResStr("IDS_NET_ACTIVATECLIENT_DESC"));
		pMenu->AddItem(LoadResStr(pClient->isActivated() ? "IDS_NET_DEACTIVATECLIENT" : "IDS_NET_ACTIVATECLIENT"),
		               strActivateDesc.getData(), C4GUI::Ico_None,
		               new C4GUI::CBMenuHandler<ClientListItem>(this, &ClientListItem::OnCtxActivate));
	}
	// info
	StdCopyStrBuf strClientInfoDesc(LoadResStr("IDS_NET_CLIENTINFO_DESC"));
	pMenu->AddItem(LoadResStr("IDS_NET_CLIENTINFO"), strClientInfoDesc.getData(), C4GUI::Ico_None,
	               new C4GUI::CBMenuHandler<ClientListItem>(this, &ClientListItem::OnCtxInfo));
	//Ignore button
	if(!pClient->isLocal())
	{
		StdCopyStrBuf strNewColor(LoadResStr(pClient->IsIgnored() ? "IDS_NET_CLIENT_UNIGNORE" : "IDS_NET_CLIENT_IGNORE"));
		pMenu->AddItem(strNewColor.getData(), FormatString(LoadResStr("IDS_NET_CLIENT_IGNORE_DESC"), pClient->getName()).getData(),
			C4GUI::Ico_None, new C4GUI::CBMenuHandler<ClientListItem>(this, &ClientListItem::OnCtxIgnore), nullptr);
	}
	
	// open it
	return pMenu;
}

void C4PlayerInfoListBox::ClientListItem::OnCtxIgnore(C4GUI::Element *pListItem)
{
	GetClient()->ToggleIgnore();
}

void C4PlayerInfoListBox::ClientListItem::OnCtxKick(C4GUI::Element *pListItem)
{
	// host only
	if (!::Network.isEnabled() || !::Network.isHost()) return;
	// add control
	Game.Clients.CtrlRemove(GetClient(), LoadResStr("IDS_MSG_KICKFROMLOBBY"));
}

void C4PlayerInfoListBox::ClientListItem::OnCtxActivate(C4GUI::Element *pListItem)
{
	// host only
	C4Client *pClient = GetClient();
	if (!::Network.isEnabled() || !::Network.isHost() || !pClient) return;
	// add control
	::Control.DoInput(CID_ClientUpdate, new C4ControlClientUpdate(idClient, CUT_Activate, !pClient->isActivated()), CDT_Sync);
}

void C4PlayerInfoListBox::ClientListItem::OnCtxInfo(C4GUI::Element *pListItem)
{
	// show client info dialog
	::pGUI->ShowRemoveDlg(new C4Network2ClientDlg(idClient));
}

void C4PlayerInfoListBox::ClientListItem::OnBtnAddPlr(C4GUI::Control *btn)
{
	// show player add dialog
	GetScreen()->ShowRemoveDlg(new C4PlayerSelDlg(new C4FileSel_CBEx<C4GameLobby::MainDlg>(GetLobby(), &C4GameLobby::MainDlg::OnClientAddPlayer, idClient)));
}


// ----------- TeamListItem ---------------------------------------------

C4PlayerInfoListBox::TeamListItem::TeamListItem(C4PlayerInfoListBox *pForListBox, int32_t idTeam, ListItem *pInsertBefore)
		: ListItem(pForListBox), idTeam(idTeam)
{
	bool fEvaluation = pList->IsEvaluation();
	// get team data
	const char *szTeamName;
	C4Team *pTeam = nullptr;
	if (idTeam == TEAMID_Unknown)
		szTeamName = LoadResStr("IDS_MSG_RNDTEAM");
	else
	{
		pTeam = Game.Teams.GetTeamByID(idTeam); assert(pTeam);
		if (pTeam) szTeamName = pTeam->GetName(); else szTeamName = "INTERNAL TEAM ERROR";
	}
	// set ID
	idListItemID.idType = ListItem::ID::PLI_TEAM;
	idListItemID.id = idTeam;
	// get height
	int32_t iIconSize; CStdFont *pFont;
	if (!fEvaluation)
	{
		pFont = &::GraphicsResource.TextFont;
		iIconSize = pFont->GetLineHeight();
	}
	else
	{
		pFont = &::GraphicsResource.TitleFont;
		iIconSize = C4SymbolSize; // C4PictureSize doesn't fit...
	}
	// create subcomponents
	pIcon = new C4GUI::Icon(C4Rect(0, 0, iIconSize, iIconSize), C4GUI::Ico_Team);
	pNameLabel = new C4GUI::Label(szTeamName, iIconSize + IconLabelSpacing, (iIconSize - pFont->GetLineHeight())/2, ALeft, pList->GetTextColor(), pFont, false);
	if (fEvaluation && pTeam && pTeam->GetIconSpec() && *pTeam->GetIconSpec())
	{
		C4FacetSurface fctSymbol;
		fctSymbol.Create(C4SymbolSize,C4SymbolSize);
		Game.DrawTextSpecImage(fctSymbol, pTeam->GetIconSpec(), nullptr, pTeam->GetColor());
		pIcon->GetMFacet().GrabFrom(fctSymbol);
	}
	// calc own bounds
	C4Rect rcOwnBounds = pNameLabel->GetBounds();
	rcOwnBounds.Wdt += rcOwnBounds.x; rcOwnBounds.x = 0;
	rcOwnBounds.Hgt += rcOwnBounds.y; rcOwnBounds.y = 0;
	SetBounds(rcOwnBounds);
	// add components
	AddElement(pIcon); AddElement(pNameLabel);
	// tooltip
	SetToolTip(FormatString(LoadResStr("IDS_DESC_TEAM"), szTeamName).getData());
	// insert into listbox at correct order
	// (will eventually get resized horizontally and moved)
	pForListBox->InsertElement(this, pInsertBefore);
}

void C4PlayerInfoListBox::TeamListItem::MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
{
	// double click on team to enter it with all local players
	if (iButton == C4MC_Button_LeftDouble)
	{
		MoveLocalPlayersIntoTeam();
	}
	else
		ListItem::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
}

void C4PlayerInfoListBox::TeamListItem::UpdateOwnPos()
{
	// parent for client rect
	typedef C4GUI::Window ParentClass;
	ParentClass::UpdateOwnPos();
	// evaluation: Center team label
	if (pList->IsEvaluation())
	{
		int32_t iTotalWdt = pIcon->GetBounds().Wdt + IconLabelSpacing + ::GraphicsResource.TitleFont.GetTextWidth(pNameLabel->GetText());
		C4GUI::ComponentAligner caAll(GetContainedClientRect(), 0,0);
		C4GUI::ComponentAligner caBounds(caAll.GetCentered(iTotalWdt, caAll.GetInnerHeight()), 0,0);
		pIcon->SetBounds(caBounds.GetFromLeft(pIcon->GetBounds().Wdt, pIcon->GetBounds().Hgt));
		pNameLabel->SetBounds(caBounds.GetCentered(caBounds.GetInnerWidth(), ::GraphicsResource.TitleFont.GetLineHeight()));
	}
}

void C4PlayerInfoListBox::TeamListItem::MoveLocalPlayersIntoTeam()
{
	// check if changing teams is allowed
	if (!CanLocalChooseTeams()) return;
	// safety: Clicked team must exist
	if (!Game.Teams.GetTeamByID(idTeam)) return;
	// get local client to change teams of
	bool fAnyChange = false;
	C4ClientPlayerInfos *pChangeInfo = Game.PlayerInfos.GetInfoByClientID(Game.Clients.getLocalID());
	if (!pChangeInfo) return;
	// this must be processed as a request by the host
	// now change it in its own request packet
	C4ClientPlayerInfos LocalInfoRequest(*pChangeInfo);
	C4PlayerInfo *pInfo; int32_t i=0;
	while ((pInfo = LocalInfoRequest.GetPlayerInfo(i++)))
		if (pInfo->GetTeam() != idTeam)
			if (pInfo->GetType() == C4PT_User)
			{
				pInfo->SetTeam(idTeam);
				fAnyChange = true;
			}
	if (!fAnyChange) return;
	// and request this update (host processes it directly)
	::Network.Players.RequestPlayerInfoUpdate(LocalInfoRequest);
	// next update will move the player labels
}

void C4PlayerInfoListBox::TeamListItem::Update()
{
	// evaluation: update color by team winning status
	if (pList->IsEvaluation())
	{
		C4Team *pTeam = Game.Teams.GetTeamByID(idTeam);
		if (pTeam && pTeam->HasWon())
		{
			pNameLabel->SetColor(C4GUI_WinningTextColor, false);
		}
		else
		{
			pNameLabel->SetColor(C4GUI_LosingTextColor, false);
		}
	}
}



// ----------- FreeSavegamePlayersListItem ---------------------------------------------

C4PlayerInfoListBox::FreeSavegamePlayersListItem::FreeSavegamePlayersListItem(C4PlayerInfoListBox *pForListBox, ListItem *pInsertBefore)
		: ListItem(pForListBox)
{
	// set ID
	idListItemID.idType = ListItem::ID::PLI_SAVEGAMEPLR;
	idListItemID.id = 0;
	// get height
	int32_t iIconSize = ::GraphicsResource.TextFont.GetLineHeight();
	// create subcomponents
	pIcon = new C4GUI::Icon(C4Rect(0, 0, iIconSize, iIconSize), C4GUI::Ico_SavegamePlayer);
	pNameLabel = new C4GUI::Label(LoadResStr("IDS_MSG_FREESAVEGAMEPLRS"), iIconSize + IconLabelSpacing,0, ALeft);
	// calc own bounds
	C4Rect rcOwnBounds = pNameLabel->GetBounds();
	rcOwnBounds.Wdt += rcOwnBounds.x; rcOwnBounds.x = 0;
	rcOwnBounds.Hgt += rcOwnBounds.y; rcOwnBounds.y = 0;
	SetBounds(rcOwnBounds);
	// add components
	AddElement(pIcon); AddElement(pNameLabel);
	// tooltip
	SetToolTip(LoadResStr("IDS_DESC_UNASSOCIATEDSAVEGAMEPLAYE"));
	// insert into listbox at correct order
	// (will eventually get resized horizontally and moved)
	pForListBox->InsertElement(this, pInsertBefore);
	// initial update
	Update();
}

void C4PlayerInfoListBox::FreeSavegamePlayersListItem::Update()
{
	// 2do: none-label
}


// ----------- ScriptPlayersListItem ---------------------------------------------

C4PlayerInfoListBox::ScriptPlayersListItem::ScriptPlayersListItem(C4PlayerInfoListBox *pForListBox, ListItem *pInsertBefore)
		: ListItem(pForListBox)
{
	// set ID
	idListItemID.idType = ListItem::ID::PLI_SCRIPTPLR;
	idListItemID.id = 0;
	// get height
	int32_t iIconSize = ::GraphicsResource.TextFont.GetLineHeight();
	// create subcomponents
	pIcon = new C4GUI::Icon(C4Rect(0, 0, iIconSize, iIconSize), C4GUI::Ico_Record);
	pNameLabel = new C4GUI::Label(LoadResStr("IDS_CTL_SCRIPTPLAYERS"), iIconSize + IconLabelSpacing,0, ALeft);
	btnAddPlayer = nullptr;
	if (::Control.isCtrlHost())
	{
		btnAddPlayer = new C4GUI::CallbackButton<ScriptPlayersListItem, C4GUI::IconButton>(C4GUI::Ico_AddPlr, C4Rect(0, 0, iIconSize, iIconSize), 'A' /* 2do TODO */, &ScriptPlayersListItem::OnBtnAddPlr, this);
	}
	// calc own bounds
	C4Rect rcOwnBounds = pNameLabel->GetBounds();
	rcOwnBounds.Wdt += rcOwnBounds.x; rcOwnBounds.x = 0;
	rcOwnBounds.Hgt += rcOwnBounds.y; rcOwnBounds.y = 0;
	SetBounds(rcOwnBounds);
	// add components
	AddElement(pIcon); AddElement(pNameLabel);
	if (btnAddPlayer) AddElement(btnAddPlayer);
	// tooltip
	SetToolTip(LoadResStr("IDS_DESC_PLAYERSCONTROLLEDBYCOMPUT"));
	// insert into listbox at correct order
	// (will eventually get resized horizontally and moved)
	pForListBox->InsertElement(this, pInsertBefore);
	// after move: update add player button pos
	if (btnAddPlayer)
	{
		int32_t iHgt = GetClientRect().Hgt;
		btnAddPlayer->GetBounds() = GetToprightCornerRect(iHgt,iHgt,2,0);
	}
	// initial update
	Update();
}

void C4PlayerInfoListBox::ScriptPlayersListItem::Update()
{
	// player join button: Visible if there's still some room for script players
	if (btnAddPlayer)
	{
		bool fCanJoinScriptPlayers = (Game.Teams.GetMaxScriptPlayers() - Game.PlayerInfos.GetActiveScriptPlayerCount(true, true) > 0);
		btnAddPlayer->SetVisibility(fCanJoinScriptPlayers);
	}
}

void C4PlayerInfoListBox::ScriptPlayersListItem::OnBtnAddPlr(C4GUI::Control *btn)
{
	// safety
	int32_t iCurrScriptPlrCount = Game.PlayerInfos.GetActiveScriptPlayerCount(true, true);
	bool fCanJoinScriptPlayers = (Game.Teams.GetMaxScriptPlayers() - iCurrScriptPlrCount > 0);
	if (!fCanJoinScriptPlayers) return;
	if (!::Control.isCtrlHost()) return;
	// request a script player join
	C4PlayerInfo *pScriptPlrInfo = new C4PlayerInfo();
	pScriptPlrInfo->SetAsScriptPlayer(Game.Teams.GetScriptPlayerName().getData(), GenerateRandomPlayerColor(iCurrScriptPlrCount), 0, C4ID::None);
	C4ClientPlayerInfos JoinPkt(nullptr, true, pScriptPlrInfo);
	// add to queue!
	Game.PlayerInfos.DoPlayerInfoUpdate(&JoinPkt);
}


// ----------- ReplayPlayersListItem ---------------------------------------------

C4PlayerInfoListBox::ReplayPlayersListItem::ReplayPlayersListItem(C4PlayerInfoListBox *pForListBox, ListItem *pInsertBefore)
		: ListItem(pForListBox)
{
	// set ID
	idListItemID.idType = ListItem::ID::PLI_REPLAY;
	idListItemID.id = 0;
	// get height
	int32_t iIconSize = ::GraphicsResource.TextFont.GetLineHeight();
	// create subcomponents
	pIcon = new C4GUI::Icon(C4Rect(0, 0, iIconSize, iIconSize), C4GUI::Ico_Record);
	pNameLabel = new C4GUI::Label(LoadResStr("IDS_MSG_REPLAYPLRS"), iIconSize + IconLabelSpacing,0, ALeft);
	// calc own bounds
	C4Rect rcOwnBounds = pNameLabel->GetBounds();
	rcOwnBounds.Wdt += rcOwnBounds.x; rcOwnBounds.x = 0;
	rcOwnBounds.Hgt += rcOwnBounds.y; rcOwnBounds.y = 0;
	SetBounds(rcOwnBounds);
	// add components
	AddElement(pIcon); AddElement(pNameLabel);
	// tooltip
	SetToolTip(LoadResStr("IDS_MSG_REPLAYPLRS_DESC"));
	// insert into listbox at correct order
	// (will eventually get resized horizontally and moved)
	pForListBox->InsertElement(this, pInsertBefore);
}



// ------------------- C4PlayerInfoListBox ------------------------

C4PlayerInfoListBox::C4PlayerInfoListBox(const C4Rect &rcBounds, Mode eMode, int32_t iTeamFilter)
		: C4GUI::ListBox(rcBounds), eMode(eMode), iMaxUncollapsedPlayers(10), fIsCollapsed(false), iTeamFilter(iTeamFilter), dwTextColor(C4GUI_MessageFontClr), pCustomFont(nullptr)
{
	// update if client listbox selection changes
	SetSelectionChangeCallbackFn(new C4GUI::CallbackHandler<C4PlayerInfoListBox>(this, &C4PlayerInfoListBox::OnPlrListSelChange));
	// initial update
	Update();
}

void C4PlayerInfoListBox::SetClientSoundIcon(int32_t iForClientID)
{
	// get client element
	ListItem *pItem = GetPlayerListItem(ListItem::ID::PLI_CLIENT, iForClientID);
	if (pItem)
	{
		ClientListItem *pClientItem = static_cast<ClientListItem *>(pItem);
		pClientItem->SetSoundIcon();
	}
}

C4PlayerInfoListBox::ListItem *C4PlayerInfoListBox::GetPlayerListItem(ListItem::ID::IDType eType, int32_t id)
{
	ListItem::ID idSearch(eType, id);
	// search through listbox
	for (C4GUI::Element *pEItem = GetFirst(); pEItem; pEItem = pEItem->GetNext())
	{
		// only playerlistitems in this box
		ListItem *pItem = static_cast<ListItem *>(pEItem);
		if (pItem->idListItemID == idSearch) return pItem;
	}
	// nothing found
	return nullptr;
}

bool C4PlayerInfoListBox::PlrListItemUpdate(ListItem::ID::IDType eType, int32_t id, class ListItem **pEnsurePos)
{
	assert(pEnsurePos);
	// search item
	ListItem *pItem = GetPlayerListItem(eType, id);
	if (!pItem) return false;
	// ensure its position is correct
	if (pItem != *pEnsurePos)
	{
		RemoveElement(pItem);
		InsertElement(pItem, *pEnsurePos);
	}
	else
	{
		// pos correct; advance past it
		*pEnsurePos = static_cast<ListItem *>(pItem->GetNext());
	}
	// update item
	pItem->Update();
	// done, success
	return true;
}

// static safety var to prevent recusive updates
static bool fPlayerListUpdating=false;

void C4PlayerInfoListBox::Update()
{
	if (fPlayerListUpdating) return;
	fPlayerListUpdating = true;

	// synchronize current list with what it should be
	// call update on all other list items
	ListItem *pCurrInList = static_cast<ListItem *>(GetFirst()); // list item being compared with the searched item

	// free savegame players first
	UpdateSavegamePlayers(&pCurrInList);

	// next comes the regular players, sorted either by clients or teams, or special sort for evaluation mode
	switch (eMode)
	{
	case PILBM_LobbyTeamSort:
		// sort by team
		if (Game.Teams.CanLocalSeeTeam())
			UpdatePlayersByTeam(&pCurrInList);
		else
			UpdatePlayersByRandomTeam(&pCurrInList);
		break;

	case PILBM_LobbyClientSort:
		// sort by client
		// replay players first?
		if (Game.C4S.Head.Replay) UpdateReplayPlayers(&pCurrInList);
		// script controlled players from the main list
		UpdateScriptPlayers(&pCurrInList);
		// regular players
		UpdatePlayersByClient(&pCurrInList);
		break;

	case PILBM_Evaluation:
	case PILBM_EvaluationNoWinners:
		UpdatePlayersByEvaluation(&pCurrInList, eMode == PILBM_Evaluation);
		break;
	}

	// finally: remove any remaining list items at the end
	while (pCurrInList)
	{
		ListItem *pDel = pCurrInList;
		pCurrInList = static_cast<ListItem *>(pCurrInList->GetNext());
		delete pDel;
	}

	// update done
	fPlayerListUpdating = false;

	// check whether view needs to be collapsed
	if (!fIsCollapsed && IsScrollingActive() && !IsEvaluation())
	{
		// then collapse it, and update window
		iMaxUncollapsedPlayers = Game.PlayerInfos.GetPlayerCount()-1;
		fIsCollapsed = true;
		Update(); // recursive call!
	}
	else if (fIsCollapsed && Game.PlayerInfos.GetPlayerCount() <= iMaxUncollapsedPlayers)
	{
		// player count dropped below collapse-limit: uncollapse
		// note that this may again cause a collapse after that update, if scrolling was still necessary
		// however, it will then not recurse any further, because iMaxUncollapsedPlayers will have been updated
		fIsCollapsed = false;
		Update();
	}
}

void C4PlayerInfoListBox::UpdateSavegamePlayers(ListItem **ppCurrInList)
{
	// add unassociated savegame players (script players excluded)
	if (Game.RestorePlayerInfos.GetActivePlayerCount(true) - Game.RestorePlayerInfos.GetActiveScriptPlayerCount(true, true))
	{
		// caption
		if (!PlrListItemUpdate(ListItem::ID::PLI_SAVEGAMEPLR, 0, ppCurrInList))
			new FreeSavegamePlayersListItem(this, *ppCurrInList);
		// the players
		bool fAnyPlayers = false;
		C4PlayerInfo *pInfo; int32_t iInfoID=0;
		while ((pInfo = Game.RestorePlayerInfos.GetNextPlayerInfoByID(iInfoID)))
		{
			iInfoID = pInfo->GetID();
			// skip assigned
			if (Game.PlayerInfos.GetPlayerInfoBySavegameID(iInfoID)) continue;
			// skip script controlled - those are put into the script controlled player list
			if (pInfo->GetType() == C4PT_Script) continue;
			// players are in the list
			fAnyPlayers = true;
			// show them
			if (!PlrListItemUpdate(ListItem::ID::PLI_SAVEGAMEPLR, iInfoID, ppCurrInList))
				new PlayerListItem(this, -1, iInfoID, true, *ppCurrInList);
		}
		// 2do: none-label
		(void) fAnyPlayers;
	}

}

void C4PlayerInfoListBox::UpdateReplayPlayers(ListItem **ppCurrInList)
{
	// header
	if (!PlrListItemUpdate(ListItem::ID::PLI_REPLAY, 0, ppCurrInList))
		new ReplayPlayersListItem(this, *ppCurrInList);
	// players
	C4PlayerInfo *pInfo; int32_t iInfoID=0;
	while ((pInfo = Game.PlayerInfos.GetNextPlayerInfoByID(iInfoID)))
	{
		if (pInfo->IsInvisible()) continue;
		iInfoID = pInfo->GetID();
		// show them
		if (!PlrListItemUpdate(ListItem::ID::PLI_PLAYER, iInfoID, ppCurrInList))
			new PlayerListItem(this, -1, iInfoID, false, *ppCurrInList);
	}
	// 2do: none-label
}

void C4PlayerInfoListBox::UpdateScriptPlayers(ListItem **ppCurrInList)
{
	// script controlled players from the main list
	// processing the restore list would be redundant, because all script players should have been taken over by a new script player join automatically
	// also show the label if script players can be joined
	if (Game.Teams.GetMaxScriptPlayers() || Game.PlayerInfos.GetActiveScriptPlayerCount(true, false))
	{
		// header
		if (!PlrListItemUpdate(ListItem::ID::PLI_SCRIPTPLR, 0, ppCurrInList))
			new ScriptPlayersListItem(this, *ppCurrInList);
		// players
		C4PlayerInfo *pInfo; int32_t iClientIdx=0; C4ClientPlayerInfos *pInfos;
		while ((pInfos = Game.PlayerInfos.GetIndexedInfo(iClientIdx++)))
		{
			int32_t iInfoIdx=0;
			while ((pInfo = pInfos->GetPlayerInfo(iInfoIdx++)))
			{
				if (pInfo->GetType() != C4PT_Script) continue;
				if (pInfo->IsRemoved()) continue;
				if (pInfo->IsInvisible()) continue;
				// show them
				int32_t iInfoID = pInfo->GetID();
				if (!PlrListItemUpdate(ListItem::ID::PLI_PLAYER, iInfoID, ppCurrInList))
					new PlayerListItem(this, pInfos->GetClientID(), iInfoID, false, *ppCurrInList);
			}
		}
	}
}

void C4PlayerInfoListBox::UpdatePlayersByTeam(ListItem **ppCurrInList)
{
	// sort by team
	C4Team *pTeam; int32_t i=0;
	while ((pTeam = Game.Teams.GetTeamByIndex(i++)))
	{
		// no empty teams that are not used
		if (Game.Teams.IsAutoGenerateTeams() && !pTeam->GetPlayerCount()) continue;
		// the team label
		if (!PlrListItemUpdate(ListItem::ID::PLI_TEAM, pTeam->GetID(), ppCurrInList))
			new TeamListItem(this, pTeam->GetID(), *ppCurrInList);
		// players for this team
		int32_t idPlr, j=0; int32_t idClient; C4Client *pClient; C4PlayerInfo *pPlrInfo;
		while ((idPlr = pTeam->GetIndexedPlayer(j++)))
			if ((pPlrInfo = Game.PlayerInfos.GetPlayerInfoByID(idPlr, &idClient)))
				if (!pPlrInfo->IsInvisible())
					if ((pClient=Game.Clients.getClientByID(idClient)) && pClient->isActivated())
						if (!PlrListItemUpdate(ListItem::ID::PLI_PLAYER, idPlr, ppCurrInList))
							new PlayerListItem(this, idClient, idPlr, false, *ppCurrInList);
	}
}

void C4PlayerInfoListBox::UpdatePlayersByRandomTeam(ListItem **ppCurrInList)
{
	// team sort but teams set to random and invisible: Show all players within one "Random Team"-label
	bool fTeamLabelPut = false;
	C4Client *pClient = nullptr;
	while ((pClient = Game.Clients.getClient(pClient)))
	{
		// player infos for this client - not for deactivated, and never in replays
		if (Game.C4S.Head.Replay || !pClient->isActivated()) continue;
		C4ClientPlayerInfos *pInfoPacket = Game.PlayerInfos.GetInfoByClientID(pClient->getID());
		if (pInfoPacket)
		{
			C4PlayerInfo *pPlrInfo; int32_t i=0;
			while ((pPlrInfo = pInfoPacket->GetPlayerInfo(i++)))
			{
				if (pPlrInfo->IsInvisible()) continue;
				if (!fTeamLabelPut)
				{
					if (!PlrListItemUpdate(ListItem::ID::PLI_TEAM, TEAMID_Unknown, ppCurrInList))
						new TeamListItem(this, TEAMID_Unknown, *ppCurrInList);
					fTeamLabelPut = true;
				}
				if (!PlrListItemUpdate(ListItem::ID::PLI_PLAYER, pPlrInfo->GetID(), ppCurrInList))
					new PlayerListItem(this, pClient->getID(), pPlrInfo->GetID(), false, *ppCurrInList);
			}
		}
	}
}

void C4PlayerInfoListBox::UpdatePlayersByClient(ListItem **ppCurrInList)
{
	// regular players
	C4Client *pClient = nullptr;
	while ((pClient = Game.Clients.getClient(pClient)))
	{
		// the client label
		if (!PlrListItemUpdate(ListItem::ID::PLI_CLIENT, pClient->getID(), ppCurrInList))
			new ClientListItem(this, pClient->getCore(), *ppCurrInList);
		// player infos for this client - not for observers, and never in replays
		// could also check for activated here. However, non-observers will usually be activated later and thus be using their players
		if (Game.C4S.Head.Replay || pClient->isObserver()) continue;
		C4ClientPlayerInfos *pInfoPacket = Game.PlayerInfos.GetInfoByClientID(pClient->getID());
		if (pInfoPacket)
		{
			C4PlayerInfo *pPlrInfo; int32_t i=0;
			while ((pPlrInfo = pInfoPacket->GetPlayerInfo(i++)))
			{
				if (pPlrInfo->GetType() == C4PT_Script) continue;
				if (pPlrInfo->IsRemoved()) continue;
				if (pPlrInfo->IsInvisible()) continue;
				if (!PlrListItemUpdate(ListItem::ID::PLI_PLAYER, pPlrInfo->GetID(), ppCurrInList))
					new PlayerListItem(this, pClient->getID(), pPlrInfo->GetID(), false, *ppCurrInList);
			}
		}
	}
}

void C4PlayerInfoListBox::UpdatePlayersByEvaluation(ListItem **ppCurrInList, bool fShowWinners)
{
	// if a team filter is provided, add team label first
	if (iTeamFilter)
		if (!PlrListItemUpdate(ListItem::ID::PLI_TEAM, iTeamFilter, ppCurrInList))
			new TeamListItem(this, iTeamFilter, *ppCurrInList);
	// Add by teams: In show-winner-mode winning teams first
	// Otherwise, just add all
	AddMode pShowWinnersAddModes[] = { AM_Winners, AM_Losers };
	AddMode pHideWinnersAddModes[] = { AM_All };
	AddMode *pAddModes; int32_t iAddModeCount;
	if (fShowWinners)
	{
		pAddModes = pShowWinnersAddModes; iAddModeCount = 2;
	}
	else
	{
		pAddModes = pHideWinnersAddModes; iAddModeCount = 1;
	}
	for (int32_t iAddMode = 0; iAddMode < iAddModeCount; ++iAddMode)
	{
		AddMode eAddMode = pAddModes[iAddMode];
		if (iTeamFilter)
		{
			// Team filter mode: Add only players of specified team
			UpdatePlayersByEvaluation(ppCurrInList, Game.Teams.GetTeamByID(iTeamFilter), eAddMode);
		}
		else
		{
			// Normal mode: Add all teams of winning status
			C4Team *pTeam; int32_t i=0;
			while ((pTeam = Game.Teams.GetTeamByIndex(i++)))
			{
				UpdatePlayersByEvaluation(ppCurrInList, pTeam, eAddMode);
			}
			// Add teamless players of winning status
			UpdatePlayersByEvaluation(ppCurrInList, nullptr, eAddMode);
		}
	}
}

void C4PlayerInfoListBox::UpdatePlayersByEvaluation(ListItem **ppCurrInList, C4Team *pTeam, C4PlayerInfoListBox::AddMode eWinMode)
{
	// check winning status of team first
	if (pTeam && eWinMode != AM_All) if (pTeam->HasWon() != (eWinMode == AM_Winners)) return;
	// now add all matching players
	int32_t iTeamID = pTeam ? pTeam->GetID() : 0;
	C4ClientPlayerInfos *pInfoPacket; int32_t iClient=0;
	while ((pInfoPacket = Game.PlayerInfos.GetIndexedInfo(iClient++)))
	{
		C4PlayerInfo *pPlrInfo; int32_t i=0;
		while ((pPlrInfo = pInfoPacket->GetPlayerInfo(i++)))
		{
			if (!pPlrInfo->HasJoined()) continue;
			if (pPlrInfo->GetTeam() != iTeamID) continue;
			if (pPlrInfo->IsInvisible()) continue;
			if (!pTeam && eWinMode != AM_All && pPlrInfo->HasWon() != (eWinMode == AM_Winners)) continue;
			if (!PlrListItemUpdate(ListItem::ID::PLI_PLAYER, pPlrInfo->GetID(), ppCurrInList))
				new PlayerListItem(this, pInfoPacket->GetClientID(), pPlrInfo->GetID(), false, *ppCurrInList);
		}
	}
}

bool C4PlayerInfoListBox::IsPlayerItemCollapsed(PlayerListItem *pItem)
{
	// never if view is not collapsed
	if (!fIsCollapsed) return false;
	// collapsed if not selected
	return GetSelectedItem() != pItem;
}

void C4PlayerInfoListBox::SetMode(Mode eNewMode)
{
	if (eMode != eNewMode)
	{
		eMode = eNewMode;
		Update();
	}
}

void C4PlayerInfoListBox::SetCustomFont(CStdFont *pNewFont, uint32_t dwTextColor)
{
	pCustomFont = pNewFont;
	this->dwTextColor = dwTextColor;
	// update done later by caller anyway
}
