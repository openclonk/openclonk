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
// script-controlled InGame dialog to show player infos

#include "C4Include.h"
#include "gui/C4Scoreboard.h"

#include "gui/C4Gui.h"
#include "gui/C4GameOverDlg.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"


class C4ScoreboardDlg : public C4GUI::Dialog
{
private:
	int32_t *piColWidths;
	C4Scoreboard *pBrd;

	enum { XIndent = 4, YIndent = 4, XMargin = 3, YMargin = 3 };

public:
	C4ScoreboardDlg(C4Scoreboard *pForScoreboard);
	~C4ScoreboardDlg();

protected:
	void InvalidateRows() { delete [] piColWidths; piColWidths = nullptr; }
	void Update(); // update row widths and own size and caption

	virtual bool DoPlacement(C4GUI::Screen *pOnScreen, const C4Rect &rPreferredDlgRect);
	virtual void Draw(C4TargetFacet &cgo);
	virtual void DrawElement(C4TargetFacet &cgo);

	virtual const char *GetID() { return "Scoreboard"; }

	virtual bool IsMouseControlled() { return false; }

	friend class C4Scoreboard;
};

// ************************************************
// *** C4Scoreboard

void C4Scoreboard::Entry::SwapWith(Entry *pSwap)
{
	Entry swp; swp.Text.Take(std::move(Text)); swp.iVal = iVal;
	Text.Take(std::move(pSwap->Text)); iVal = pSwap->iVal;
	pSwap->Text.Take(std::move(swp.Text)); pSwap->iVal = swp.iVal;
}

void C4Scoreboard::Clear()
{
	// del all cells
	delete [] pEntries;
	pEntries = nullptr; iRows=iCols=0;
	// del dialog
	iDlgShow = 0;
	if (pDlg) { delete pDlg; pDlg = nullptr; }
}

void C4Scoreboard::AddRow(int32_t iInsertBefore)
{
	// counts
	int32_t iNewEntryCount = (iRows+1) * iCols;
	if (!iNewEntryCount) { ++iRows; return; }
	// realloc and copy array
	Entry *pNewEntries = new Entry[iNewEntryCount];
	for (int32_t iRow = 0, iFromRow = 0; iFromRow < iRows; ++iRow,++iFromRow)
	{
		if (iFromRow == iInsertBefore) ++iRow;
		for (int32_t iCol = 0; iCol < iCols; ++iCol)
			pNewEntries[iRow*iCols+iCol].GrabFrom(GetCell(iCol, iFromRow));
	}
	++iRows;
	delete [] pEntries; pEntries = pNewEntries;
}

void C4Scoreboard::AddCol(int32_t iInsertBefore)
{
	// counts
	int32_t iNewEntryCount = iRows * (iCols + 1);
	if (!iNewEntryCount) { ++iCols; return; }
	// realloc and copy array
	Entry *pNewEntries = new Entry[iNewEntryCount];
	for (int32_t iRow = 0; iRow < iRows; ++iRow)
		for (int32_t iCol = 0, iFromCol = 0; iFromCol < iCols; ++iCol, ++iFromCol)
		{
			if (iFromCol == iInsertBefore) ++iCol;
			pNewEntries[iRow*(iCols+1)+iCol].GrabFrom(GetCell(iFromCol, iRow));
		}
	++iCols; // CAUTION: must inc after add, so GetCell won't point into bogus!
	delete [] pEntries; pEntries = pNewEntries;
}

void C4Scoreboard::DelRow(int32_t iDelIndex)
{
	// counts
	int32_t iNewEntryCount = (iRows-1) * iCols;
	if (!iNewEntryCount) { --iRows; delete [] pEntries; pEntries = nullptr; return; }
	// realloc and copy array
	Entry *pNewEntries = new Entry[iNewEntryCount]; Entry *pCpy = pNewEntries;
	for (int32_t iRow = 0, iFromRow = 0; iRow < (iRows-1); ++iRow,++iFromRow)
	{
		if (iFromRow == iDelIndex) ++iFromRow;
		for (int32_t iCol = 0; iCol < iCols; ++iCol)
			pCpy++->GrabFrom(GetCell(iCol, iFromRow));
	}
	--iRows;
	delete [] pEntries; pEntries = pNewEntries;
}

void C4Scoreboard::DelCol(int32_t iDelIndex)
{
	// counts
	int32_t iNewEntryCount = iRows * (iCols-1);
	if (!iNewEntryCount) { --iCols; delete [] pEntries; pEntries = nullptr; return; }
	// realloc and copy array
	Entry *pNewEntries = new Entry[iNewEntryCount]; Entry *pCpy = pNewEntries;
	for (int32_t iRow = 0; iRow < iRows; ++iRow)
		for (int32_t iCol = 0, iFromCol = 0; iCol < (iCols-1); ++iCol,++iFromCol)
		{
			if (iFromCol == iDelIndex) ++iFromCol;
			pCpy++->GrabFrom(GetCell(iFromCol, iRow));
		}
	--iCols; // CAUTION: must dec after add, so GetCell won't point into bogus!
	delete [] pEntries; pEntries = pNewEntries;
}

void C4Scoreboard::SwapRows(int32_t iRow1, int32_t iRow2)
{
	Entry *pXChg1 = pEntries+iRow1*iCols;
	Entry *pXChg2 = pEntries+iRow2*iCols;
	int32_t i = iCols; while (i--) pXChg1++->SwapWith(pXChg2++);
}

int32_t C4Scoreboard::GetColByKey(int32_t iKey) const
{
	// safety
	if (!iRows) return -1;
	// check all col headers
	Entry *pCheck = pEntries;
	for (int32_t i = 0; i < iCols; ++i)
		if (pCheck++->iVal == iKey) return i;
	return -1;
}

int32_t C4Scoreboard::GetRowByKey(int32_t iKey) const
{
	// safety
	if (!iCols) return -1;
	// check all row headers
	Entry *pCheck = pEntries;
	for (int32_t i = 0; i < iRows; ++i,(pCheck+=iCols))
		if (pCheck->iVal == iKey) return i;
	return -1;
}

void C4Scoreboard::SetCell(int32_t iColKey, int32_t iRowKey, const char *szValue, int32_t iValue)
{
	// ensure primary row/col exists
	if (!iCols || !iRows)
	{
		if (!iCols) AddCol(0);
		if (!iRows) AddRow(0);
		GetCell(0, 0)->iVal = TitleKey;
	}
	// get row/col; create new if not yet existing
	int32_t iCol = GetColByKey(iColKey);
	if (iCol<0) { AddCol(iCol=iCols); GetCell(iCol, 0)->iVal = iColKey; }
	int32_t iRow = GetRowByKey(iRowKey);
	if (iRow<0) { AddRow(iRow=iRows); GetCell(0, iRow)->iVal = iRowKey; }
	// now set values
	Entry *pCell = GetCell(iCol, iRow);
	pCell->Text.Copy(szValue);
	if (iCol && iRow) pCell->iVal = iValue; // do NOT overwrite index keys!
	// if an empty value was set, prune empty
	if (!szValue || !*szValue)
	{
		// prune empty row (not label row)
		int32_t i;
		if (iRow)
		{
			for (i=1; i<iCols; ++i) if (GetCell(i, iRow)->Text) break;
			if (i == iCols) DelRow(iRow);
		}
		// prune empty col (not label col)
		if (iCol)
		{
			for (i=1; i<iRows; ++i) if (GetCell(iCol, i)->Text) break;
			if (i == iRows) DelCol(iCol);
		}
	}
	/*  // prune empty board? but this would prevent boards that just sort
	  if (iRows == 1 && iCols == 1)
	    Clear(); // must not do this, because it will del pDlg
	  else*/
	// recalc row widths in display (else done by clear)
	InvalidateRows();
}

const char *C4Scoreboard::GetCellString(int32_t iColKey, int32_t iRowKey)
{
	// get row/col
	int32_t iCol = GetColByKey(iColKey);
	int32_t iRow = GetRowByKey(iRowKey);
	if (iCol<0 || iRow<0) return nullptr;
	// now get value
	Entry *pCell = GetCell(iCol, iRow);
	return pCell->Text.getData();
}

int32_t C4Scoreboard::GetCellData(int32_t iColKey, int32_t iRowKey)
{
	// get row/col
	int32_t iCol = GetColByKey(iColKey);
	int32_t iRow = GetRowByKey(iRowKey);
	if (iCol<0 || iRow<0) return 0;
	// now get value
	Entry *pCell = GetCell(iCol, iRow);
	return pCell->iVal;
}

void C4Scoreboard::RemoveCol(int32_t iColKey)
{
	int32_t iCol = GetColByKey(iColKey);
	if (iCol>=0) DelCol(iCol);
	InvalidateRows(); // recalc row widths in display
}

void C4Scoreboard::RemoveRow(int32_t iRowKey)
{
	int32_t iRow = GetRowByKey(iRowKey);
	if (iRow>=0) DelRow(iRow);
	InvalidateRows(); // recalc row widths in display
}

bool C4Scoreboard::SortBy(int32_t iColKey, bool fReverse)
{
	// get sort col
	int32_t iCol = GetColByKey(iColKey);
	if (iCol<0) return false;
	// sort
	int32_t iSortDir = fReverse ? -1 : +1;
	int32_t iSortBegin=1, iSortEnd=iRows-1;
	while (iSortBegin < iSortEnd)
	{
		int32_t iNewBorder = iSortBegin; int32_t i;
		for (i = iSortBegin; i < iSortEnd; ++i)
			if (GetCell(iCol, i)->iVal * iSortDir > GetCell(iCol, i+1)->iVal * iSortDir)
			{
				SwapRows(i, i+1);
				iNewBorder = i;
			}
		iSortEnd = iNewBorder;
		for (i = iSortEnd; i > iSortBegin; --i)
			if (GetCell(iCol, i-1)->iVal * iSortDir > GetCell(iCol, i)->iVal * iSortDir)
			{
				SwapRows(i-1, i);
				iNewBorder = i;
			}
		iSortBegin = iNewBorder;
	}
	return true;
}

void C4Scoreboard::InvalidateRows()
{
	// recalculate row sizes
	if (pDlg) pDlg->InvalidateRows();
}

void C4Scoreboard::DoDlgShow(int32_t iChange, bool fUserToggle)
{
	if (::pGUI->IsExclusive()) return;
	// update dlg show
	iDlgShow += iChange;
	if (!fUserToggle)
		// script update: Dlg on off if iDlgShow variable passed zero
		fUserToggle = (ShouldBeShown() == !pDlg);
	else
		// user pressed Tab: Always toggle except if the scoreboard cannot be shown at all
		if (!CanBeShown() && !pDlg) fUserToggle = false;
	if (fUserToggle)
	{
		if (!pDlg)
		{
			if (!C4GameOverDlg::IsShown()) // never show during game over dlg
				::pGUI->ShowRemoveDlg(pDlg = new C4ScoreboardDlg(this));
		}
		else
			pDlg->Close(false);
	}
}

void C4Scoreboard::HideDlg()
{
	if (::pGUI->IsExclusive()) return;
	// hide scoreboard if it was active
	if (pDlg) pDlg->Close(false);
}

void C4Scoreboard::CompileFunc(StdCompiler *pComp)
{
	bool deserializing = pComp->isDeserializer();
	if (deserializing) Clear();
	pComp->Value(mkNamingAdapt(iRows,     "Rows",     0));
	pComp->Value(mkNamingAdapt(iCols,     "Cols",     0));
	pComp->Value(mkNamingAdapt(iDlgShow,  "DlgShow",  0));
	if (iRows * iCols)
	{
		if (deserializing) pEntries = new Entry[iRows * iCols];
		for (int32_t iRow = 0; iRow < iRows; ++iRow)
			for (int32_t iCol = 0; iCol < iCols; ++iCol)
			{
				Entry *pEnt = GetCell(iCol, iRow);
				pComp->Value(mkNamingAdapt(pEnt->Text, FormatString("Cell%i_%iString", iCol, iRow).getData()));
				pComp->Value(mkNamingAdapt(pEnt->iVal, FormatString("Cell%i_%iValue", iCol, iRow).getData()));
			}
		// recheck dlg show in read mode
		// will usually not do anything, because reading is done before enetering shared mode
		if (pComp->isDeserializer()) DoDlgShow(0, false);
	}
}


// ************************************************
// *** C4ScoreboardDlg

C4ScoreboardDlg::C4ScoreboardDlg(C4Scoreboard *pForScoreboard)
		: C4GUI::Dialog(100, 100, "nops", false), piColWidths(nullptr), pBrd(pForScoreboard)
{
	Update();
}

C4ScoreboardDlg::~C4ScoreboardDlg()
{
	delete [] piColWidths;
	pBrd->pDlg = nullptr;
}

void C4ScoreboardDlg::Update()
{
	// counts
	int32_t iRowCount = pBrd->iRows; int32_t iColCount = pBrd->iCols;
	delete [] piColWidths; piColWidths = nullptr;
	// invalid board - scipters can create those, but there's no reason why the engine
	// should display something pretty then; just keep dialog defaults
	if (!iRowCount || !iColCount) return;
	// calc sizes as max col widths plus some indent pixels
	piColWidths = new int32_t[iColCount];
	int32_t iWdt=XMargin*2, iHgt=YMargin*2;
	for (int32_t iCol = 0; iCol < iColCount; ++iCol)
	{
		piColWidths[iCol] = XIndent;
		for (int32_t iRow = 0; iRow < iRowCount; ++iRow)
		{
			C4Scoreboard::Entry *pCell = pBrd->GetCell(iCol, iRow);
			if ((iRow || iCol) && !pCell->Text.isNull()) piColWidths[iCol] = std::max<int32_t>(piColWidths[iCol], ::GraphicsResource.FontRegular.GetTextWidth(pCell->Text.getData()) + XIndent);
		}
		iWdt += piColWidths[iCol];
	}
	iHgt += iRowCount * (::GraphicsResource.FontRegular.GetLineHeight() + YIndent);
	const char *szTitle = pBrd->GetCell(0,0)->Text.getData();
	if (szTitle) iWdt = std::max<int32_t>(iWdt, ::GraphicsResource.FontRegular.GetTextWidth(szTitle) + 40);
	if (!pTitle != !szTitle) SetTitle(szTitle); // needed for title margin...
	iWdt += GetMarginLeft() + GetMarginRight();
	iHgt += GetMarginTop() + GetMarginBottom();
	// update dialog
	SetBounds(C4Rect(rcBounds.x, rcBounds.y, iWdt, iHgt));
	SetTitle(szTitle);
	if (szTitle && pTitle) pTitle->SetIcon(C4GUI::Icon::GetIconFacet(C4GUI::Ico_Player));
	// realign
	C4GUI::Screen *pScr = GetScreen();
	if (pScr) DoPlacement(pScr, pScr->GetPreferredDlgRect());
}

bool C4ScoreboardDlg::DoPlacement(C4GUI::Screen *pOnScreen, const C4Rect &rPreferredDlgRect)
{
	// align topright
	SetPos(rPreferredDlgRect.x + rPreferredDlgRect.Wdt - rcBounds.Wdt - 20, rPreferredDlgRect.y + 38);
	return true;
}

void C4ScoreboardDlg::Draw(C4TargetFacet &cgo)
{
	if (!piColWidths) Update();
	typedef C4GUI::Dialog ParentClass;
	ParentClass::Draw(cgo);
}

void C4ScoreboardDlg::DrawElement(C4TargetFacet &cgo)
{
	typedef C4GUI::Dialog ParentClass;
	ParentClass::DrawElement(cgo);
	// draw spreadsheet
	int32_t iRowCount = pBrd->iRows; int32_t iColCount = pBrd->iCols;
	int32_t iY = YMargin + int32_t(cgo.TargetY) + rcClientRect.y;
	for (int32_t iRow = 0; iRow < iRowCount; ++iRow)
	{
		int32_t iX = XMargin + int32_t(cgo.TargetX) + rcClientRect.x;
		for (int32_t iCol = 0; iCol < iColCount; ++iCol)
		{
			const char *szText = pBrd->GetCell(iCol, iRow)->Text.getData();
			if (szText && *szText && (iRow || iCol))
				pDraw->TextOut(szText, ::GraphicsResource.FontRegular, 1.0, cgo.Surface, iCol ? iX + piColWidths[iCol]/2 : iX, iY, 0xffffffff, iCol ? ACenter : ALeft);
			iX += piColWidths[iCol];
		}
		iY += ::GraphicsResource.FontRegular.GetLineHeight() + YIndent;
	}
}
