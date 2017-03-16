/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// generic user interface
// room for textual deconvolution

#include "C4Include.h"
#include "gui/C4Gui.h"

#include "game/C4Application.h"
#include "gui/C4MouseControl.h"
#include "graphics/C4GraphicsResource.h"
#include "graphics/C4Draw.h"

namespace C4GUI
{

	const char *Edit::CursorRepresentation = "\xC2\xA6"; // U+00A6 BROKEN BAR

	namespace
	{
		inline bool IsUtf8ContinuationByte(char c)
		{
			return (c & 0xC0) == 0x80;
		}
		inline bool IsUtf8StartByte(char c)
		{
			return (c & 0xC0) == 0xC0;
		}
	}

// ----------------------------------------------------
// Edit

	Edit::Edit(const C4Rect &rtBounds, bool fFocusEdit) : Control(rtBounds), iCursorPos(0), iSelectionStart(0), iSelectionEnd(0), fLeftBtnDown(false)
	{
		// create an initial buffer
		Text = new char[256];
		iBufferSize = 256;
		*Text = 0;
		// def vals
		iMaxTextLength = 255;
		iCursorPos = iSelectionStart = iSelectionEnd = 0;
		iXScroll = 0;
		pFont = &::GraphicsResource.TextFont;
		dwBGClr = C4GUI_EditBGColor;
		dwFontClr = C4GUI_EditFontColor;
		dwBorderColor = 0; // default border
		cPasswordMask = 0;
		// apply client margin
		UpdateOwnPos();
		// add context handler
		SetContextHandler(new CBContextHandler<Edit>(this, &Edit::OnContext));
		// add key handlers
		C4CustomKey::Priority eKeyPrio = fFocusEdit ? C4CustomKey::PRIO_FocusCtrl : C4CustomKey::PRIO_Ctrl;
		pKeyCursorBack  = RegisterCursorOp(COP_BACK  , K_BACK  , "GUIEditCursorBack", eKeyPrio);
		pKeyCursorDel   = RegisterCursorOp(COP_DELETE, K_DELETE, "GUIEditCursorDel",eKeyPrio);
		pKeyCursorLeft  = RegisterCursorOp(COP_LEFT  , K_LEFT  , "GUIEditCursorLeft",eKeyPrio);
		pKeyCursorRight = RegisterCursorOp(COP_RIGHT , K_RIGHT , "GUIEditCursorRight", eKeyPrio);
		pKeyCursorHome  = RegisterCursorOp(COP_HOME  , K_HOME  , "GUIEditCursorHome", eKeyPrio);
		pKeyCursorEnd   = RegisterCursorOp(COP_END   , K_END   , "GUIEditCursorEnd", eKeyPrio);
		pKeyEnter = new C4KeyBinding(C4KeyCodeEx(K_RETURN), "GUIEditConfirm", KEYSCOPE_Gui,
		                             new ControlKeyCB<Edit>(*this, &Edit::KeyEnter), eKeyPrio);
		pKeyCopy = new C4KeyBinding(C4KeyCodeEx(K_C, KEYS_Control), "GUIEditCopy", KEYSCOPE_Gui,
		                            new ControlKeyCB<Edit>(*this, &Edit::KeyCopy), eKeyPrio);
		pKeyPaste = new C4KeyBinding(C4KeyCodeEx(K_V, KEYS_Control), "GUIEditPaste", KEYSCOPE_Gui,
		                             new ControlKeyCB<Edit>(*this, &Edit::KeyPaste), eKeyPrio);
		pKeyCut = new C4KeyBinding(C4KeyCodeEx(K_X, KEYS_Control), "GUIEditCut", KEYSCOPE_Gui,
		                           new ControlKeyCB<Edit>(*this, &Edit::KeyCut), eKeyPrio);
		pKeySelAll = new C4KeyBinding(C4KeyCodeEx(K_A, KEYS_Control), "GUIEditSelAll", KEYSCOPE_Gui,
		                              new ControlKeyCB<Edit>(*this, &Edit::KeySelectAll), eKeyPrio);
	}

	Edit::~Edit()
	{
		delete[] Text;
		delete pKeyCursorBack;
		delete pKeyCursorDel;
		delete pKeyCursorLeft;
		delete pKeyCursorRight;
		delete pKeyCursorHome;
		delete pKeyCursorEnd;
		delete pKeyEnter;
		delete pKeyCopy;
		delete pKeyPaste;
		delete pKeyCut;
		delete pKeySelAll;
	}

	class C4KeyBinding *Edit::RegisterCursorOp(CursorOperation op, C4KeyCode key, const char *szName, C4CustomKey::Priority eKeyPrio)
	{
		// register same op for all shift states; distinction will be done in handling proc
		C4CustomKey::CodeList KeyList;
		KeyList.push_back(C4KeyCodeEx(key));
		KeyList.push_back(C4KeyCodeEx(key, KEYS_Shift));
		KeyList.push_back(C4KeyCodeEx(key, KEYS_Control));
		KeyList.push_back(C4KeyCodeEx(key, C4KeyShiftState(KEYS_Shift | KEYS_Control)));
		return new C4KeyBinding(KeyList, szName, KEYSCOPE_Gui, new ControlKeyCBExPassKey<Edit, CursorOperation>(*this, op, &Edit::KeyCursorOp), eKeyPrio);
	}

	int32_t Edit::GetDefaultEditHeight()
	{
		// edit height for default font
		return GetCustomEditHeight(&::GraphicsResource.TextFont);
	}

	int32_t Edit::GetCustomEditHeight(CStdFont *pUseFont)
	{
		// edit height for custom font: Make it so edits and wooden labels have same height
		return std::max<int32_t>(pUseFont->GetLineHeight()+3, C4GUI_MinWoodBarHgt);
	}

	void Edit::ClearText()
	{
		// free oversized buffers
		if (iBufferSize > 256)
		{
			delete[] Text;
			Text = new char[256];
			iBufferSize = 256;
		}
		// clear text
		*Text=0;
		// reset cursor and selection
		iCursorPos = iSelectionStart = iSelectionEnd = 0;
		iXScroll = 0;
	}

	void Edit::Deselect()
	{
		// reset selection
		iSelectionStart = iSelectionEnd = 0;
		// cursor might have moved: ensure it is shown
		tLastInputTime = C4TimeMilliseconds::Now();
	}

	void Edit::DeleteSelection()
	{
		// move end text to front
		int32_t iSelBegin = std::min(iSelectionStart, iSelectionEnd), iSelEnd = std::max(iSelectionStart, iSelectionEnd);
		if (iSelectionStart == iSelectionEnd) return;
		memmove(Text + iSelBegin, Text + iSelEnd, strlen(Text + iSelEnd)+1);
		// adjust cursor pos
		if (iCursorPos > iSelBegin) iCursorPos = std::max(iSelBegin, iCursorPos - iSelEnd + iSelBegin);
		// cursor might have moved: ensure it is shown
		tLastInputTime = C4TimeMilliseconds::Now();
		// nothing selected
		iSelectionStart = iSelectionEnd = iSelBegin;
	}

	bool Edit::InsertText(const char *szText, bool fUser)
	{
		// empty previous selection
		if (iSelectionStart != iSelectionEnd) DeleteSelection();
		// check buffer length
		int32_t iTextLen = SLen(szText);
		int32_t iTextEnd = SLen(Text);
		bool fBufferOK = (iTextLen + iTextEnd <= (iMaxTextLength-1));
		if (!fBufferOK) iTextLen -= iTextEnd+iTextLen - (iMaxTextLength-1);
		if (iTextLen <= 0) return false;
		// ensure buffer is large enough
		EnsureBufferSize(iTextEnd + iTextLen + 1);
		// move down text buffer after cursor pos (including trailing zero-char)
		int32_t i;
		for (i=iTextEnd; i>=iCursorPos; --i) Text[i + iTextLen] = Text[i];
		// insert buffer into text
		for (i=iTextLen; i; --i) Text[iCursorPos + i - 1] = szText[i - 1];
		if (fUser)
		{
			// advance cursor
			iCursorPos += iTextLen;
			// cursor moved: ensure it is shown
			tLastInputTime = C4TimeMilliseconds::Now();
			ScrollCursorInView();
		}
		// done; return whether everything was inserted
		return fBufferOK;
	}

	int32_t Edit::GetCharPos(int32_t iControlXPos)
	{
		// client offset
		iControlXPos -= rcClientRect.x - rcBounds.x - iXScroll;
		// well, not exactly the best idea...maybe add a new fn to the gfx system?
		// summing up char widths is no good, because there might be spacings between characters
		// 2do: optimize this
		if (cPasswordMask)
		{
			int32_t w, h; char strMask[2] = { cPasswordMask, 0 };
			pFont->GetTextExtent(strMask, w, h, false);
			return Clamp<int32_t>((iControlXPos + w/2) / std::max<int32_t>(1, w), 0, SLen(Text));
		}
		int32_t i = 0;
		for (int32_t iLastW = 0, w,h; Text[i]; ++i)
		{
			int oldi = i;
			if (IsUtf8StartByte(Text[oldi]))
				while (IsUtf8ContinuationByte(Text[++i + 1])) /* EMPTY */;
			char c=Text[i+1]; Text[i+1]=0; pFont->GetTextExtent(Text, w, h, false); Text[i+1]=c;
			if (w - (w-iLastW)/2 >= iControlXPos) return oldi;
			iLastW = w;
		}
		return i;
	}

	void Edit::EnsureBufferSize(int32_t iMinBufferSize)
	{
		// realloc buffer if necessary
		if (iBufferSize < iMinBufferSize)
		{
			// get new buffer size (rounded up to multiples of 256)
			iMinBufferSize = ((iMinBufferSize - 1) & ~0xff) + 0x100;
			// fill new buffer
			char *pNewBuffer = new char[iMinBufferSize];
			SCopy(Text, pNewBuffer);
			// apply new buffer
			delete[] Text; Text = pNewBuffer;
			iBufferSize = iMinBufferSize;
		}
	}

	void Edit::ScrollCursorInView()
	{
		if (rcClientRect.Wdt<5) return;
		// get position of cursor
		int32_t iScrollOff = std::min<int32_t>(20, rcClientRect.Wdt/3);
		int32_t w,h;
		if (!cPasswordMask)
		{
			char c=Text[iCursorPos]; Text[iCursorPos]=0; pFont->GetTextExtent(Text, w, h, false); Text[iCursorPos]=c;
		}
		else
		{
			StdStrBuf Buf; Buf.AppendChars(cPasswordMask, iCursorPos);
			pFont->GetTextExtent(Buf.getData(), w, h, false);
		}
		// need to scroll?
		while (w-iXScroll < rcClientRect.Wdt/5 && w<iScrollOff+iXScroll && iXScroll > 0)
		{
			// left
			iXScroll = std::max(iXScroll - std::min(100, rcClientRect.Wdt/4), 0);
		}
		while (w-iXScroll >= rcClientRect.Wdt/5 && w>=rcClientRect.Wdt-iScrollOff+iXScroll)
		{
			// right
			iXScroll += std::min(100, rcClientRect.Wdt/4);
		}
	}

	bool Edit::DoFinishInput(bool fPasting, bool fPastingMore)
	{
		// do OnFinishInput callback and process result - returns whether pasting operation should be continued
		InputResult eResult = OnFinishInput(fPasting, fPastingMore);
		switch (eResult)
		{
		case IR_None: // do nothing and continue pasting
			return true;

		case IR_CloseDlg: // stop any pastes and close parent dialog successfully
		{
			Dialog *pDlg = GetDlg();
			if (pDlg) pDlg->UserClose(true);
			break;
		}

		case IR_CloseEdit: // stop any pastes and remove this control
			delete this;
			break;

		case IR_Abort: // do nothing and stop any pastes
			break;
		}
		// input has been handled; no more pasting please
		return false;
	}

	bool Edit::Copy()
	{
		// get selected range
		int32_t iSelBegin = std::min(iSelectionStart, iSelectionEnd), iSelEnd = std::max(iSelectionStart, iSelectionEnd);
		if (iSelBegin == iSelEnd) return false;
		// allocate a global memory object for the text.
		std::string buf(Text+iSelBegin, iSelEnd-iSelBegin);
		if (cPasswordMask)
			buf.assign(buf.size(), cPasswordMask);

		return Application.Copy(buf);
	}

	bool Edit::Cut()
	{
		// copy text
		if (!Copy()) return false;
		// delete copied text
		DeleteSelection();
		// done, success
		return true;
	}

	bool Edit::Paste()
	{
		bool fSuccess = false;
		// check clipboard contents
		if(!Application.IsClipboardFull()) return false;
		StdCopyStrBuf text(Application.Paste().c_str());
		char * szText = text.getMData();
		if (text)
		{
			fSuccess = !!*szText;
			// replace any '|'
			int32_t iLBPos=0, iLBPos2;
			// caution when inserting line breaks: Those must be stripped, and sent as Enter-commands
			iLBPos=0;
			for (;;)
			{
				iLBPos = SCharPos(0x0d, szText);
				iLBPos2 = SCharPos(0x0a, szText);
				if (iLBPos<0 && iLBPos2<0) break; // no more linebreaks
				if (iLBPos2>=0 && (iLBPos2<iLBPos || iLBPos<0)) iLBPos = iLBPos2;
				if (!iLBPos) { ++szText; continue; } // empty line
				szText[iLBPos]=0x00;
				if (!InsertText(szText, true)) fSuccess=false; // if the buffer was too long, still try to insert following stuff (don't abort just b/c one line was too long)
				szText += iLBPos+1;
				iLBPos=0;
				if (!DoFinishInput(true, !!*szText))
				{
					// k, pasted
					return true;
				}
			}
			// insert new text (may fail due to overfull buffer, in which case parts of the text will be inserted)
			if (*szText) fSuccess = fSuccess && InsertText(szText, true);
		}
		// return whether insertion was successful
		return fSuccess;
	}

	bool IsWholeWordSpacer(unsigned char c)
	{
		// characters that make up a space between words
		// the extended characters are all seen a letters, because they vary in different
		// charsets (danish, french, etc.) and are likely to represent localized letters
		return !Inside<char>(c, 'A', 'Z')
		       && !Inside<char>(c, 'a', 'z')
		       && !Inside<char>(c, '0', '9')
		       && c!='_' && c<127;
	}

	bool Edit::KeyEnter()
	{
		DoFinishInput(false, false);
		// whatever happens: Enter key has been processed
		return true;
	}

	bool Edit::KeyCursorOp(const C4KeyCodeEx &key, const CursorOperation &op)
	{
		bool fShift = !!(key.dwShift & KEYS_Shift);
		bool fCtrl = !!(key.dwShift & KEYS_Control);
		// any selection?
		if (iSelectionStart != iSelectionEnd)
		{
			// special handling: backspace/del with selection (delete selection)
			if (op == COP_BACK || op == COP_DELETE) { DeleteSelection(); return true; }
			// no shift pressed: clear selection (even if no cursor movement is done)
			if (!fShift) Deselect();
		}
		// movement or regular/word deletion
		int32_t iMoveDir = 0, iMoveLength = 0;
		if (op == COP_LEFT && iCursorPos) iMoveDir = -1;
		else if (op == COP_RIGHT && (uint32_t)iCursorPos < SLen(Text)) iMoveDir = +1;
		else if (op == COP_BACK && iCursorPos && !fShift) iMoveDir = -1;
		else if (op == COP_DELETE && (uint32_t)iCursorPos < SLen(Text) && !fShift) iMoveDir = +1;
		else if (op == COP_HOME) iMoveLength = -iCursorPos;
		else if (op == COP_END) iMoveLength = SLen(Text)-iCursorPos;
		if (iMoveDir || iMoveLength)
		{
			// evaluate move length? (not home+end)
			if (iMoveDir)
			{
				if (fCtrl)
				{
					// move one word
					iMoveLength = 0;
					bool fNoneSpaceFound = false, fSpaceFound = false;;
					while (iCursorPos + iMoveLength + iMoveDir >= 0 && (uint32_t)(iCursorPos + iMoveLength + iMoveDir) <= SLen(Text))
						if (IsWholeWordSpacer(Text[iCursorPos + iMoveLength + (iMoveDir-1)/2]))
						{
							// stop left of a complete word
							if (fNoneSpaceFound && iMoveDir<0) break;
							// continue
							fSpaceFound = true;
							iMoveLength += iMoveDir;
						}
						else
						{
							// stop right of spacings complete word
							if (fSpaceFound && iMoveDir > 0) break;
							// continue
							fNoneSpaceFound = true;
							iMoveLength += iMoveDir;
						}
				}
				else
				{
					// Handle UTF-8
					iMoveLength = iMoveDir;
					while (IsUtf8ContinuationByte(Text[iCursorPos + iMoveLength])) iMoveLength += Sign(iMoveLength);
				}
			}
			// delete stuff
			if (op == COP_BACK || op == COP_DELETE)
			{
				// delete: make backspace command of it
				if (op == COP_DELETE) { iCursorPos += iMoveLength; iMoveLength = -iMoveLength; }
				// move end of string up
				char *c; for (c = Text+iCursorPos; *c; ++c) *(c+iMoveLength) = *c;
				// terminate string
				*(c+iMoveLength) = 0;
				assert(IsValidUtf8(Text));
			}
			else if (fShift)
			{
				// shift+arrow key: make/adjust selection
				if (iSelectionStart == iSelectionEnd) iSelectionStart = iCursorPos;
				iSelectionEnd = iCursorPos + iMoveLength;
			}
			else
				// simple cursor movement: clear any selection
				if (iSelectionStart != iSelectionEnd) Deselect();
			// adjust cursor pos
			iCursorPos += iMoveLength;
		}
		// show cursor
		tLastInputTime = C4TimeMilliseconds::Now();
		ScrollCursorInView();
		// operation recognized
		return true;
	}

	bool Edit::CharIn(const char * c)
	{
		// no control codes
		if (((unsigned char)(c[0]))<' ' || c[0]==0x7f) return false;
		// no '|'
		if (c[0]=='|') return false;
		// all extended characters are OK
		// insert character at cursor position
		return InsertText(c, true);
	}

	void Edit::MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// inherited first - this may give focus to this element
		Control::MouseInput(rMouse, iButton, iX, iY, dwKeyParam);
		// update dragging area
		int32_t iPrevCursorPos = iCursorPos;
		// dragging area is updated by drag proc
		// process left down and up
		switch (iButton)
		{
		case C4MC_Button_LeftDown:
			// mark button as being down
			fLeftBtnDown = true;
			// set selection start
			iSelectionStart = iSelectionEnd = GetCharPos(iX);
			// set cursor pos here, too
			iCursorPos = iSelectionStart;
			// remember drag target
			// no dragging movement will be done w/o drag component assigned
			// but text selection should work even if the user goes outside the component
			if (!rMouse.pDragElement) rMouse.pDragElement = this;
			break;

		case C4MC_Button_LeftUp:
			// only if button was down... (might have dragged here)
			if (fLeftBtnDown)
			{
				// it's now up :)
				fLeftBtnDown = false;
				// set cursor to this pos
				iCursorPos = iSelectionEnd;
			}
			break;

		case C4MC_Button_LeftDouble:
		{
			// word selection
			// get character pos (half-char-offset, use this to allow selection at half-space-offset around a word)
			int32_t iCharPos = GetCharPos(iX);
			// was space? try left character
			if (IsWholeWordSpacer(Text[iCharPos]))
			{
				if (!iCharPos) break;
				if (IsWholeWordSpacer(Text[--iCharPos])) break;
			}
			// search ending of word left and right
			// bounds-check is done by zero-char at end, which is regarded as a spacer
			iSelectionStart = iCharPos; iSelectionEnd = iCharPos + 1;
			while (iSelectionStart > 0 && !IsWholeWordSpacer(Text[iSelectionStart-1])) --iSelectionStart;
			while (!IsWholeWordSpacer(Text[iSelectionEnd])) ++iSelectionEnd;
			// set cursor pos to end of selection
			iCursorPos = iSelectionEnd;
			// ignore last btn-down-selection
			fLeftBtnDown = false;
		}
		break;
		case C4MC_Button_MiddleDown:
			// set selection start
			iSelectionStart = iSelectionEnd = GetCharPos(iX);
			// set cursor pos here, too
			iCursorPos = iSelectionStart;
#ifndef _WIN32
			// Insert primary selection
			InsertText(Application.Paste(false).c_str(), true);
#endif
			break;
		};
		// scroll cursor in view
		if (iPrevCursorPos != iCursorPos) ScrollCursorInView();
	}

	void Edit::DoDragging(CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam)
	{
		// update cursor pos
		int32_t iPrevCursorPos = iCursorPos;
		iCursorPos = iSelectionEnd = GetCharPos(iX);
		// scroll cursor in view
		if (iPrevCursorPos != iCursorPos) ScrollCursorInView();
	}

	void Edit::OnGetFocus(bool fByMouse)
	{
		// inherited
		Control::OnGetFocus(fByMouse);
		// select all
		iSelectionStart=0; iSelectionEnd=iCursorPos=SLen(Text);
		// begin with a flashing cursor
		tLastInputTime = C4TimeMilliseconds::Now();
	}

	void Edit::OnLooseFocus()
	{
		// clear selection
		iSelectionStart = iSelectionEnd = 0;
		// inherited
		Control::OnLooseFocus();
	}

	void Edit::DrawElement(C4TargetFacet &cgo)
	{
		// draw background
		pDraw->DrawBoxDw(cgo.Surface, cgo.TargetX+rcBounds.x,cgo.TargetY+rcBounds.y,rcBounds.x+rcBounds.Wdt+cgo.TargetX-1,rcClientRect.y+rcClientRect.Hgt+cgo.TargetY, dwBGClr);
		// draw frame
		if (dwBorderColor)
		{
			int32_t x1=cgo.TargetX+rcBounds.x,y1=cgo.TargetY+rcBounds.y,x2=x1+rcBounds.Wdt,y2=y1+rcBounds.Hgt;
			pDraw->DrawFrameDw(cgo.Surface, x1, y1, x2, y2-1, dwBorderColor);
			pDraw->DrawFrameDw(cgo.Surface, x1+1, y1+1, x2-1, y2-2, dwBorderColor);
		}
		else
			// default frame color
			Draw3DFrame(cgo);
		// clipping
		int cx0,cy0,cx1,cy1; bool fClip, fOwnClip;
		fClip = pDraw->GetPrimaryClipper(cx0,cy0,cx1,cy1);
		float nclx1 = rcClientRect.x+cgo.TargetX-2, ncly1 = rcClientRect.y+cgo.TargetY, nclx2 = rcClientRect.x+rcClientRect.Wdt+cgo.TargetX+1, ncly2 = rcClientRect.y+rcClientRect.Hgt+cgo.TargetY;
		pDraw->ApplyZoom(nclx1, ncly1);
		pDraw->ApplyZoom(nclx2, ncly2);
		fOwnClip = pDraw->SetPrimaryClipper(nclx1, ncly1, nclx2, ncly2);
		// get usable height of edit field
		int32_t iHgt = pFont->GetLineHeight(), iY0;
		if (rcClientRect.Hgt <= iHgt)
		{
			// very narrow edit field: use all of it
			iHgt=rcClientRect.Hgt;
			iY0=rcClientRect.y;
		}
		else
		{
			// normal edit field: center text vertically
			iY0 = rcClientRect.y+(rcClientRect.Hgt-iHgt)/2+1;
			// don't overdo it with selection mark
			iHgt-=2;
		}
		// get text to draw, apply password mask if neccessary
		StdStrBuf Buf; char *pDrawText;
		if (cPasswordMask)
		{
			Buf.AppendChars(cPasswordMask, SLen(Text));
			pDrawText = Buf.getMData();
		}
		else
			pDrawText = Text;
		// draw selection
		if (iSelectionStart != iSelectionEnd)
		{
			// get selection range
			int32_t iSelBegin = std::min(iSelectionStart, iSelectionEnd);
			int32_t iSelEnd = std::max(iSelectionStart, iSelectionEnd);
			// get offsets in text
			int32_t iSelX1, iSelX2, h;
			char c = pDrawText[iSelBegin]; pDrawText[iSelBegin]=0; pFont->GetTextExtent(pDrawText, iSelX1, h, false); pDrawText[iSelBegin]=c;
			c = pDrawText[iSelEnd]; pDrawText[iSelEnd]=0; pFont->GetTextExtent(pDrawText, iSelX2, h, false); pDrawText[iSelEnd]=c;
			iSelX1 -= iXScroll; iSelX2 -= iXScroll;
			// draw selection box around it
			pDraw->DrawBoxDw(cgo.Surface, cgo.TargetX+rcClientRect.x+iSelX1,cgo.TargetY+iY0,rcClientRect.x+iSelX2-1+cgo.TargetX,iY0+iHgt-1+cgo.TargetY,0x7f7f7f00);
		}
		// draw edit text
		pDraw->TextOut(pDrawText, *pFont, 1.0f, cgo.Surface, rcClientRect.x + cgo.TargetX - iXScroll, iY0 + cgo.TargetY - 1, dwFontClr, ALeft, false);
		// draw cursor
		bool fBlink = ((tLastInputTime - C4TimeMilliseconds::Now())/500)%2 == 0;
		if (HasDrawFocus() && fBlink)
		{
			char cAtCursor = pDrawText[iCursorPos]; pDrawText[iCursorPos]=0; int32_t w,h,wc;
			pFont->GetTextExtent(pDrawText, w, h, false);
			pDrawText[iCursorPos] = cAtCursor;
			pFont->GetTextExtent(CursorRepresentation, wc, h, false); wc/=2;
			pDraw->TextOut(CursorRepresentation, *pFont, 1.5f, cgo.Surface, rcClientRect.x + cgo.TargetX + w - wc - iXScroll, iY0 + cgo.TargetY - h/3, dwFontClr, ALeft, false);
		}
		// unclip
		if (fOwnClip)
		{
			if (fClip) pDraw->SetPrimaryClipper(cx0,cy0,cx1,cy1);
			else pDraw->NoPrimaryClipper();
		}
	}

	void Edit::SelectAll()
	{
		// safety: no text?
		if (!Text) return;
		// select all
		iSelectionStart = 0;
		iSelectionEnd = iCursorPos = SLen(Text);
	}

	ContextMenu *Edit::OnContext(C4GUI::Element *pListItem, int32_t iX, int32_t iY)
	{
		// safety: no text?
		if (!Text) return nullptr;
		// create context menu
		ContextMenu *pCtx = new ContextMenu();
		// fill with any valid items
		// get selected range
		uint32_t iSelBegin = std::min(iSelectionStart, iSelectionEnd), iSelEnd = std::max(iSelectionStart, iSelectionEnd);
		bool fAnythingSelected = (iSelBegin != iSelEnd);
		if (fAnythingSelected)
		{
			pCtx->AddItem(LoadResStr("IDS_DLG_CUT"), LoadResStr("IDS_DLGTIP_CUT"), Ico_None, new CBMenuHandler<Edit>(this, &Edit::OnCtxCut));
			pCtx->AddItem(LoadResStr("IDS_DLG_COPY"), LoadResStr("IDS_DLGTIP_COPY"), Ico_None, new CBMenuHandler<Edit>(this, &Edit::OnCtxCopy));
		}
		if (Application.IsClipboardFull())
			pCtx->AddItem(LoadResStr("IDS_DLG_PASTE"), LoadResStr("IDS_DLGTIP_PASTE"), Ico_None, new CBMenuHandler<Edit>(this, &Edit::OnCtxPaste));

		if (fAnythingSelected)
			pCtx->AddItem(LoadResStr("IDS_DLG_CLEAR"), LoadResStr("IDS_DLGTIP_CLEAR"), Ico_None, new CBMenuHandler<Edit>(this, &Edit::OnCtxClear));
		if (*Text && (iSelBegin!=0 || iSelEnd!=SLen(Text)))
			pCtx->AddItem(LoadResStr("IDS_DLG_SELALL"), LoadResStr("IDS_DLGTIP_SELALL"), Ico_None, new CBMenuHandler<Edit>(this, &Edit::OnCtxSelAll));
		// return ctx menu
		return pCtx;
	}

	bool Edit::GetCurrentWord(char *szTargetBuf, int32_t iMaxTargetBufLen)
	{
		// get word before cursor pos (for nick completion)
		if (!Text || iCursorPos<=0) return false;
		int32_t iPos = iCursorPos;
		while (iPos>0)
				if (IsWholeWordSpacer(Text[iPos-1])) break; else --iPos;
		SCopy(Text + iPos, szTargetBuf, std::min(iCursorPos - iPos, iMaxTargetBufLen));
		return !!*szTargetBuf;
	}


// ----------------------------------------------------
// RenameEdit

	RenameEdit::RenameEdit(Label *pLabel) : Edit(pLabel->GetBounds(), true), fFinishing(false), pForLabel(pLabel)
	{
		// ctor - construct for label
		assert(pForLabel);
		pForLabel->SetVisibility(false);
		InsertText(pForLabel->GetText(), true);
		// put self into place
		Container *pCont = pForLabel->GetParent();
		assert(pCont);
		pCont->AddElement(this);
		Dialog *pDlg = GetDlg();
		if (pDlg)
		{
			pPrevFocusCtrl = pDlg->GetFocus();
			pDlg->SetFocus(this, false);
		}
		else pPrevFocusCtrl=nullptr;
		// key binding for rename abort
		C4CustomKey::CodeList keys;
		keys.push_back(C4KeyCodeEx(K_ESCAPE));
		if (Config.Controls.GamepadGuiControl)
		{
			ControllerKeys::Cancel(keys);
		}
		pKeyAbort = new C4KeyBinding(keys, "GUIRenameEditAbort", KEYSCOPE_Gui,
		                             new ControlKeyCB<RenameEdit>(*this, &RenameEdit::KeyAbort), C4CustomKey::PRIO_FocusCtrl);
	}

	RenameEdit::~RenameEdit()
	{
		delete pKeyAbort;
	}

	void RenameEdit::Abort()
	{
		OnCancelRename();
		FinishRename();
	}

	Edit::InputResult RenameEdit::OnFinishInput(bool fPasting, bool fPastingMore)
	{
		// any text?
		if (!Text || !*Text)
		{
			// OK without text is regarded as abort
			OnCancelRename();
			FinishRename();
		}
		else switch (OnOKRename(Text))
			{
			case RR_Invalid:
			{
				// new name was not accepted: Continue editing
				Dialog *pDlg = GetDlg();
				if (pDlg) if (pDlg->GetFocus() != this) pDlg->SetFocus(this, false);
				SelectAll();
				break;
			}

			case RR_Accepted:
				// okay, rename to that text
				FinishRename();
				break;

			case RR_Deleted:
				// this is invalid; don't do anything!
				break;
			}
		return IR_Abort;
	}

	void RenameEdit::FinishRename()
	{
		// done: restore stuff
		fFinishing = true;
		pForLabel->SetVisibility(true);
		Dialog *pDlg = GetDlg();
		if (pDlg && pPrevFocusCtrl) pDlg->SetFocus(pPrevFocusCtrl, false);
		delete this;
	}

	void RenameEdit::OnLooseFocus()
	{
		Edit::OnLooseFocus();
		// callback when control looses focus: OK input
		if (!fFinishing) OnFinishInput(false, false);
	}



// ----------------------------------------------------
// LabeledEdit

	LabeledEdit::LabeledEdit(const C4Rect &rcBounds, const char *szName, bool fMultiline, const char *szPrefText, CStdFont *pUseFont, uint32_t dwTextClr)
			: C4GUI::Window()
	{
		if (!pUseFont) pUseFont = &(::GraphicsResource.TextFont);
		SetBounds(rcBounds);
		ComponentAligner caMain(GetClientRect(), 0,0, true);
		int32_t iLabelWdt=100, iLabelHgt=24;
		pUseFont->GetTextExtent(szName, iLabelWdt, iLabelHgt, true);
		C4Rect rcLabel, rcEdit;
		if (fMultiline)
		{
			rcLabel = caMain.GetFromTop(iLabelHgt);
			caMain.ExpandLeft(-2);
			caMain.ExpandTop(-2);
			rcEdit = caMain.GetAll();
		}
		else
		{
			rcLabel = caMain.GetFromLeft(iLabelWdt);
			caMain.ExpandLeft(-2);
			rcEdit = caMain.GetAll();
		}
		AddElement(new Label(szName, rcLabel, ALeft, dwTextClr, pUseFont, false));
		AddElement(pEdit = new C4GUI::Edit(rcEdit, false));
		pEdit->SetFont(pUseFont);
		if (szPrefText) pEdit->InsertText(szPrefText, false);
	}

	bool LabeledEdit::GetControlSize(int *piWdt, int *piHgt, const char *szForText, CStdFont *pForFont, bool fMultiline)
	{
		CStdFont *pUseFont = pForFont ? pForFont : &(::GraphicsResource.TextFont);
		int32_t iLabelWdt=100, iLabelHgt=24;
		pUseFont->GetTextExtent(szForText, iLabelWdt, iLabelHgt, true);
		int32_t iEditWdt = 100, iEditHgt = Edit::GetCustomEditHeight(pUseFont);
		if (fMultiline)
		{
			iEditWdt += 2; // indent edit a bit
			if (piWdt) *piWdt = std::max<int32_t>(iLabelWdt, iEditWdt);
			if (piHgt) *piHgt = iLabelHgt + iEditHgt + 2;
		}
		else
		{
			iLabelWdt += 2; // add a bit of spacing between label and edit
			if (piWdt) *piWdt = iLabelWdt + iEditWdt;
			if (piHgt) *piHgt = std::max<int32_t>(iLabelHgt, iEditHgt);
		}
		return true;
	}

} // end of namespace

