/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2008  Sven Eberhardt
 * Copyright (c) 2008  Matthes Bender
 * Copyright (c) 2008  Armin Burgmeier
 * Copyright (c) 2010  Günther Brammer
 * Copyright (c) 2008-2009, RedWolf Design GmbH, http://www.clonk.de

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

"Clonk" is a registered trademark of Matthes Bender. */
// game over dialog showing winners and losers

#include <C4Include.h>
#include <C4GameOverDlg.h>

#include <C4Def.h>
#include <C4Game.h>
#include <C4FullScreen.h>
#include <C4Player.h>
#include <C4PlayerInfo.h>
#include <C4PlayerInfoListBox.h>
#include <C4PlayerList.h>
#include <C4GameObjects.h>
#include <C4GameControl.h>
#include "C4GraphicsResource.h"


// ---------------------------------------------------
// C4GoalDisplay

C4GoalDisplay::GoalPicture::GoalPicture(const C4Rect &rcBounds, C4ID idGoal, bool fFulfilled)
		: C4GUI::Window(), idGoal(idGoal), fFulfilled(fFulfilled)
{
	// bounds
	SetBounds(rcBounds);
	// can't get specialized desc from object at the moment because of potential script callbacks!
	StdStrBuf strGoalName, strGoalDesc;
	/*C4Object *pGoalObj = ::Objects.FindInternal(idGoal);
	if (pGoalObj)
	  {
	  pGoalObj->GetInfoString().getData();
	  }
	else*/
	{
		// just get desc from def
		C4Def *pGoalDef = ::Definitions.ID2Def(idGoal);
		if (pGoalDef)
		{
			strGoalName.Copy(pGoalDef->GetName());
			// strGoalDesc.Copy(pGoalDef->GetDesc());
		}
	}
	// get tooltip
	StdStrBuf sToolTip;
	if (fFulfilled)
		sToolTip.Format(LoadResStr("IDS_DESC_GOALFULFILLED"), strGoalName.getData(), strGoalDesc.getData());
	else
		sToolTip.Format(LoadResStr("IDS_DESC_GOALNOTFULFILLED"), strGoalName.getData(), strGoalDesc.getData());
	SetToolTip(sToolTip.getData());
	// create buffered picture of goal definition
	C4Def *pDrawDef = ::Definitions.ID2Def(idGoal);
	if (pDrawDef)
	{
		Picture.Create(C4PictureSize, C4PictureSize);
		// get an object instance to draw (optional; may be zero)
		C4Object *pGoalObj = ::Objects.Find(idGoal);
		// draw goal def!
		pDrawDef->Draw(Picture, false, 0, pGoalObj);
	}
	// unfulfilled goal: grey out picture
	if (!fFulfilled)
		Picture.Grayscale(30);
}

void C4GoalDisplay::GoalPicture::DrawElement(C4TargetFacet &cgo)
{
	// draw area
	C4Facet cgoDraw;
	cgoDraw.Set(cgo.Surface, cgo.X+rcBounds.x+cgo.TargetX, cgo.Y+rcBounds.y+cgo.TargetY, rcBounds.Wdt, rcBounds.Hgt);
	// draw buffered picture
	Picture.Draw(cgoDraw);
	// draw star symbol if fulfilled
	if (fFulfilled)
	{
		cgoDraw.Set(cgoDraw.Surface, cgoDraw.X+cgoDraw.Wdt*1/2, cgoDraw.Y+cgoDraw.Hgt*1/2, cgoDraw.Wdt/2, cgoDraw.Hgt/2);
		C4GUI::Icon::GetIconFacet(C4GUI::Ico_Star).Draw(cgoDraw);
	}
}

void C4GoalDisplay::SetGoals(const C4IDList &rAllGoals, const C4IDList &rFulfilledGoals, int32_t iGoalSymbolHeight)
{
	// clear previous
	ClearChildren();
	// determine goal display area by goal count
	int32_t iGoalSymbolMargin = C4GUI_DefDlgSmallIndent;
	int32_t iGoalSymbolAreaHeight = 2*iGoalSymbolMargin + iGoalSymbolHeight;
	int32_t iGoalAreaWdt = GetClientRect().Wdt;
	int32_t iGoalsPerRow = Max<int32_t>(1, iGoalAreaWdt / iGoalSymbolAreaHeight);
	int32_t iGoalCount = rAllGoals.GetNumberOfIDs();
	int32_t iRowCount = (iGoalCount-1) / iGoalsPerRow + 1;
	C4Rect rcNewBounds = GetBounds();
	rcNewBounds.Hgt = GetMarginTop()+GetMarginBottom()+iRowCount*iGoalSymbolAreaHeight;
	SetBounds(rcNewBounds);
	C4GUI::ComponentAligner caAll(GetClientRect(), 0,0, true);
	// place goal symbols in this area
	int32_t iGoal = 0;
	for (int32_t iRow=0; iRow<iRowCount; ++iRow)
	{
		int32_t iColCount = Min<int32_t>(iGoalCount - iGoal, iGoalsPerRow);
		C4GUI::ComponentAligner caGoalArea(caAll.GetFromTop(iGoalSymbolAreaHeight, iColCount*iGoalSymbolAreaHeight), iGoalSymbolMargin,iGoalSymbolMargin, false);
		for (int32_t iCol=0; iCol<iColCount; ++iCol,++iGoal)
		{
			C4ID idGoal = rAllGoals.GetID(iGoal);
			bool fFulfilled = !!rFulfilledGoals.GetIDCount(idGoal, 1);
			AddElement(new GoalPicture(caGoalArea.GetGridCell(iCol, iColCount, iRow, iRowCount), idGoal, fFulfilled));
		}
	}
}

// ---------------------------------------------------
// C4GameOverDlg

bool C4GameOverDlg::is_shown = false;

C4GameOverDlg::C4GameOverDlg() : C4GUI::Dialog( (C4GUI::GetScreenWdt() < 800) ? (C4GUI::GetScreenWdt()-10) : Min<int32_t>(C4GUI::GetScreenWdt()-150, 800),
		    (C4GUI::GetScreenHgt() < 600) ? (C4GUI::GetScreenHgt()-10) : Min<int32_t>(C4GUI::GetScreenHgt()-150, 600),
		    LoadResStr("IDS_TEXT_EVALUATION"),
		    false), pNetResultLabel(NULL), fIsNetDone(false), fHasNextMissionButton(false)
{
	is_shown = true; // assume dlg will be shown, soon
	UpdateOwnPos();
	// indents / sizes
	int32_t iDefBtnHeight = 32;
	int32_t iIndentX1=10;
	int32_t iIndentY1=6, iIndentY2=0;
	// main screen components
	C4GUI::ComponentAligner caMain(GetClientRect(), 0,iIndentY1,true);
	int32_t iMainTextWidth = caMain.GetWidth() - 6*iIndentX1;
	caMain.GetFromBottom(iIndentY2);
	// lower button-area
	C4GUI::ComponentAligner caBottom(caMain.GetFromBottom(iDefBtnHeight+iIndentY1*2), iIndentX1,0);
	int32_t iBottomButtonSize = caBottom.GetInnerWidth();
	iBottomButtonSize = Min<int32_t>(iBottomButtonSize/2-2*iIndentX1, ::GraphicsResource.CaptionFont.GetTextWidth("Quit it, baby! And some.")*2);
	// goal display
	const C4IDList &rGoals = Game.RoundResults.GetGoals();
	const C4IDList &rFulfilledGoals = Game.RoundResults.GetFulfilledGoals();
	if (rGoals.GetNumberOfIDs())
	{
		C4GoalDisplay *pGoalDisplay = new C4GoalDisplay(caMain.GetFromTop(C4GUI_IconExHgt));
		pGoalDisplay->SetGoals(rGoals, rFulfilledGoals, C4GUI_IconExHgt);
		AddElement(pGoalDisplay);
		// goal display may have resized itself; adjust component aligner
		caMain.ExpandTop(C4GUI_IconExHgt - pGoalDisplay->GetBounds().Hgt);
	}
	// league/network result, present or pending
	fIsNetDone = false;
	bool fHasNetResult = Game.RoundResults.HasNetResult();
	const char *szNetResult = NULL;
	if (Game.Parameters.isLeague() || fHasNetResult)
	{
		if (fHasNetResult)
			szNetResult = Game.RoundResults.GetNetResultString();
		else
			szNetResult = LoadResStr("IDS_TEXT_LEAGUEWAITINGFOREVALUATIO");
		pNetResultLabel = new C4GUI::Label("", caMain.GetFromTop(::GraphicsResource.TextFont.GetLineHeight()*2, iMainTextWidth), ACenter, C4GUI_Caption2FontClr, NULL, false, false, true);
		AddElement(pNetResultLabel);
		// only add label - contents and fIsNetDone will be set in next update
	}
	else
	{
		// otherwise, network is always done
		fIsNetDone = true;
	}
	// extra evaluation string area
	const char *szCustomEvaluationStrings = Game.RoundResults.GetCustomEvaluationStrings();
	if (szCustomEvaluationStrings && *szCustomEvaluationStrings)
	{
		int32_t iMaxHgt = caMain.GetInnerHeight() / 3; // max 1/3rd of height for extra data
		C4GUI::MultilineLabel *pCustomStrings = new C4GUI::MultilineLabel(caMain.GetFromTop(0 /* resized later*/, iMainTextWidth), 0,0, "    ", true, true);
		pCustomStrings->AddLine(szCustomEvaluationStrings, &::GraphicsResource.TextFont, C4GUI_MessageFontClr, true, false, NULL);
		C4Rect rcCustomStringBounds = pCustomStrings->GetBounds();
		if (rcCustomStringBounds.Hgt > iMaxHgt)
		{
			// Buffer too large: Use a scrollbox instead
			delete pCustomStrings;
			rcCustomStringBounds.Hgt = iMaxHgt;
			C4GUI::TextWindow *pCustomStringsWin = new C4GUI::TextWindow(rcCustomStringBounds, 0,0,0, 0,0,"    ",true, NULL,0, true);
			pCustomStringsWin->SetDecoration(false, false, NULL, false);
			pCustomStringsWin->AddTextLine(szCustomEvaluationStrings, &::GraphicsResource.TextFont, C4GUI_MessageFontClr, true, false, NULL);
			caMain.ExpandTop(-iMaxHgt);
			AddElement(pCustomStringsWin);
		}
		else
		{
			// buffer size OK: Reserve required space
			caMain.ExpandTop(-rcCustomStringBounds.Hgt);
			AddElement(pCustomStrings);
		}
	}
	// player list area
	C4GUI::ComponentAligner caPlayerArea(caMain.GetAll(), iIndentX1,0);
	iPlrListCount = 1; bool fSepTeamLists = false;
	if (Game.Teams.GetTeamCount() == 2 && !Game.Teams.IsAutoGenerateTeams())
	{
		// exactly two predefined teams: Use two player list boxes; one for each team
		iPlrListCount = 2;
		fSepTeamLists = true;
	}
	ppPlayerLists = new C4PlayerInfoListBox *[iPlrListCount];
	for (int32_t i=0; i<iPlrListCount; ++i)
	{
		ppPlayerLists[i] = new C4PlayerInfoListBox(caPlayerArea.GetGridCell(i,iPlrListCount,0,1), C4PlayerInfoListBox::PILBM_Evaluation, fSepTeamLists ? Game.Teams.GetTeamByIndex(i)->GetID() : 0);
		/*if (fSepTeamLists) not necessary and popping up on too much area
		  ppPlayerLists[i]->SetToolTip(FormatString(LoadResStr("IDS_DESC_TEAM"), Game.Teams.GetTeamByIndex(i)->GetName()).getData());
		else
		  ppPlayerLists[i]->SetToolTip(LoadResStr("IDS_DESC_LISTOFPLAYERSWHOPARTICIPA"));*/
		//ppPlayerLists[i]->SetCustomFont(&::GraphicsResource.FontTooltip, 0xff000000); - display black on white?
		ppPlayerLists[i]->SetSelectionDiabled(true);
		ppPlayerLists[i]->SetDecoration(false, NULL, true, false);
		AddElement(ppPlayerLists[i]);
	}
	// add buttons
	C4GUI::CallbackButton<C4GameOverDlg> *btnExit;
	pBtnExit = btnExit = new C4GUI::CallbackButton<C4GameOverDlg>(LoadResStr("IDS_BTN_ENDROUND"), caBottom.GetGridCell(0,2, 0,1, iBottomButtonSize, -1, true), &C4GameOverDlg::OnExitBtn);
	btnExit->SetToolTip(LoadResStr("IDS_DESC_ENDTHEROUND"));
	AddElement(btnExit);
	C4GUI::CallbackButton<C4GameOverDlg> *btnContinue;
	pBtnContinue = btnContinue = new C4GUI::CallbackButton<C4GameOverDlg>(LoadResStr("IDS_BTN_CONTINUEGAME"), caBottom.GetGridCell(1,2, 0,1, iBottomButtonSize, -1, true), &C4GameOverDlg::OnContinueBtn);
	btnContinue->SetToolTip(LoadResStr("IDS_DESC_CONTINUETHEROUNDWITHNOFUR"));
	AddElement(btnContinue);
	// convert continue button to "next mission" button if available
	if (Game.NextMission)
	{
		// not available for regular replay and network clients, obviously
		// it is available for films though, so you can create cinematics for adventures
		if (::Control.isCtrlHost() || (Game.C4S.Head.Film == 2))
		{
			fHasNextMissionButton = true;
			btnContinue->SetText(Game.NextMissionText.getData());
			btnContinue->SetToolTip(Game.NextMissionDesc.getData());
		}
	}
	// updates
	Application.Add(this);
	Update();
	// initial focus on quit button if visible, so space/enter/low gamepad buttons quit
	fIsQuitBtnVisible = fIsNetDone || !::Network.isHost();
	if (fIsQuitBtnVisible) SetFocus(btnExit, false);
}

C4GameOverDlg::~C4GameOverDlg()
{
	Application.Remove(this);
	delete ppPlayerLists;
	is_shown = false;
}

void C4GameOverDlg::Update()
{
	for (int32_t i=0; i<iPlrListCount; ++i) ppPlayerLists[i]->Update();
	if (pNetResultLabel)
	{
		SetNetResult(Game.RoundResults.GetNetResultString(), Game.RoundResults.GetNetResult(), ::Network.getPendingStreamData(), ::Network.isStreaming());
	}
	// exit/continue button only visible for host if league streaming finished
	bool fBtnsVisible = fIsNetDone || !::Network.isHost();
	if (fBtnsVisible != fIsQuitBtnVisible)
	{
		fIsQuitBtnVisible = fBtnsVisible;
		pBtnExit->SetVisibility(fBtnsVisible);
		pBtnContinue->SetVisibility(fBtnsVisible);
		if (fIsQuitBtnVisible) SetFocus(pBtnExit, false);
	}
}

void C4GameOverDlg::SetNetResult(const char *szResultString, C4RoundResults::NetResult eResultType, size_t iPendingStreamingData, bool fIsStreaming)
{
	// add info about pending streaming data
	StdStrBuf sResult(szResultString);
	if (fIsStreaming)
	{
		sResult.AppendChar('|');
		sResult.AppendFormat("[!]Transmitting record to league server... (%d kb remaining)", int(iPendingStreamingData/1024));
	}
	// message linebreak into box
	StdStrBuf sBrokenResult;
	::GraphicsResource.TextFont.BreakMessage(sResult.getData(), pNetResultLabel->GetBounds().Wdt, &sBrokenResult, true);
	pNetResultLabel->SetText(sBrokenResult.getData(), false);
	// all done?
	if (eResultType != C4RoundResults::NR_None && !fIsStreaming)
	{
		// a final result is determined and all streaming data has been transmitted
		fIsNetDone = true;
	}
	// network error?
	if (eResultType == C4RoundResults::NR_NetError)
	{
		// disconnected. Do not show winners/losers
		for (int32_t i=0; i<iPlrListCount; ++i) ppPlayerLists[i]->SetMode(C4PlayerInfoListBox::PILBM_EvaluationNoWinners);
	}
}

void C4GameOverDlg::OnExitBtn(C4GUI::Control *btn)
{
	// callback: exit button pressed.
	Close(false);
}

void C4GameOverDlg::OnContinueBtn(C4GUI::Control *btn)
{
	// callback: continue button pressed
	Close(true);
}

void C4GameOverDlg::OnShown()
{
	// close some other dialogs
	Game.Scoreboard.HideDlg();
	FullScreen.CloseMenu();
	for (C4Player *plr = ::Players.First; plr; plr = plr->Next)
		plr->CloseMenu();
	// pause game when round results dlg is shown
	Game.Pause();
}

void C4GameOverDlg::OnClosed(bool fOK)
{
	typedef C4GUI::Dialog BaseClass;
	bool fNextMissBtn = fHasNextMissionButton;
	BaseClass::OnClosed(fOK); // deletes this!
	// continue round
	if (fOK)
	{
		if (fNextMissBtn)
		{
			// switch to next mission if next mission button is pressed
			Application.SetNextMission(Game.NextMission.getData());
			Application.QuitGame();
		}
		else
		{
			// unpause game when continue is pressed
			Game.Unpause();
		}
	}
	// end round
	else
	{
		Application.QuitGame();
	}
}
