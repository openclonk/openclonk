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
// game over dialog showing winners and losers

#ifndef INC_C4GameOverDlg
#define INC_C4GameOverDlg

#include "gui/C4Gui.h"
#include "control/C4RoundResults.h"

// horizontal display of goal symbols; filfilled goals marked
// maybe to be reused for a game goal dialog?
class C4GoalDisplay : public C4GUI::Window
{
private:
	// element that draws one goal
	class GoalPicture : public C4GUI::Window
	{
	private:
		C4ID idGoal;
		bool fFulfilled;
		C4FacetSurface Picture;

	public:
		GoalPicture(const C4Rect &rcBounds, C4ID idGoal, bool fFulfilled);

	protected:
		virtual void DrawElement(C4TargetFacet &cgo);
	};

public:
	C4GoalDisplay(const C4Rect &rcBounds) : C4GUI::Window() { SetBounds(rcBounds); }
	virtual ~C4GoalDisplay() {}

	void SetGoals(const C4IDList &rAllGoals, const C4IDList &rFulfilledGoals, int32_t iGoalSymbolHeight);
};

class C4GameOverDlg : public C4GUI::Dialog, private C4ApplicationSec1Timer
{
private:
	static bool is_shown;
	int32_t iPlrListCount;
	class C4PlayerInfoListBox **ppPlayerLists;
	C4GoalDisplay *pGoalDisplay;
	C4GUI::Label *pNetResultLabel; // label showing league result, disconnect, etc.
	C4GUI::Button *pBtnExit, *pBtnContinue;
	bool fIsNetDone; // set if league is evaluated and round results arrived
	bool fIsQuitBtnVisible; // quit button available? set if not host or when fIsNetDone
	bool fHasNextMissionButton; // continue button replaced by "next mission"-button?

private:
	void OnExitBtn(C4GUI::Control *btn); // callback: exit button pressed
	void OnContinueBtn(C4GUI::Control *btn); // callback: continue button pressed

	void Update();
	void SetNetResult(const char *szResultString, C4RoundResults::NetResult eResultType, size_t iPendingStreamingData, bool fIsStreaming);

protected:
	virtual void OnShown();
	virtual void OnClosed(bool fOK);

	virtual bool OnEnter() { if (fIsQuitBtnVisible) OnExitBtn(nullptr); return true; } // enter on non-button: Always quit
	virtual bool OnEscape() { if (fIsQuitBtnVisible) UserClose(false); return true; } // escape ignored if still streaming

	// true for dialogs that should span the whole screen
	// not just the mouse-viewport
	virtual bool IsFreePlaceDialog() { return true; }

	// true for dialogs that receive full keyboard and mouse input even in shared mode
	virtual bool IsExclusiveDialog() { return true; }

	// sec1 timer
	virtual void OnSec1Timer() { Update(); }

public:
	C4GameOverDlg();
	~C4GameOverDlg();

	static bool IsShown() { return is_shown; }
};


#endif // INC_C4GameOverDlg
