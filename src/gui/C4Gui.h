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
// defines user controls

// 2do:
//   mouse wheel processing
//   disabled buttons

#ifndef INC_C4Gui
#define INC_C4Gui

#include "graphics/C4FacetEx.h"
#include "graphics/C4FontLoader.h"
#include "gui/C4KeyboardInput.h"
#include "lib/C4Rect.h"
#include "lib/C4LogBuf.h"
#include "object/C4Id.h"
#include "platform/C4Window.h"
#include "platform/StdScheduler.h"

// consts (load those from a def file some time)
// font colors - alpha is font alpha, which is inversed opaque
#define C4GUI_CaptionFontClr     0xffffffff
#define C4GUI_Caption2FontClr    0xffffff00
#define C4GUI_InactCaptionFontClr 0xffafafaf
#define C4GUI_ButtonFontClr      0xffffff00
#define C4GUI_ButtonFontShadowClr 0xff000000
#define C4GUI_StatusFontClr      0xffffffff
#define C4GUI_MessageFontClr     0xffffffff
#define C4GUI_MessageFontAlpha   0xff000000
#define C4GUI_InactMessageFontClr 0xffafafaf
#define C4GUI_NotifyFontClr      0xffff0000
#define C4GUI_ComboFontClr       0xffffffff
#define C4GUI_CheckboxFontClr    0xffffffff
#define C4GUI_SmallCheckboxFontClr 0xffffffff
#define C4GUI_CheckboxDisabledFontClr 0xffafafaf
#define C4GUI_LogFontClr         0xffafafaf
#define C4GUI_LogFontClr2        0xffff1f1f
#define C4GUI_ErrorFontClr       0xffff1f1f
#define C4GUI_ProgressBarFontClr 0xffffffff
#define C4GUI_ContextFontClr     0xffffffff
#define C4GUI_GfxTabCaptActiveClr 0xff000000
#define C4GUI_GfxTabCaptInactiveClr 0xff000000

// other colors
#define C4GUI_ImportantBGColor   0x2f00007f
#define C4GUI_ProgressBarColor   0x4fffffff
#define C4GUI_ListBoxSelColor    0x4faf0000
#define C4GUI_ListBoxInactSelColor  0x4f7f7f7f
#define C4GUI_ContextSelColor    0x4faf0000
#define C4GUI_ContextBGColor     0xaf3f1a00
#define C4GUI_StandardBGColor    0x9f000000
#define C4GUI_ActiveTabBGColor   C4GUI_StandardBGColor
#define C4GUI_ListBoxBarColor    0x7f772200
#define C4GUI_EditBGColor        0x7f000000
#define C4GUI_EditFontColor      0xffffffff

#define C4GUI_ToolTipBGColor     0xFFF1EA78
#define C4GUI_ToolTipFrameColor  0x7f000000
#define C4GUI_ToolTipColor       0xFF483222

// winner/loser color marking
#define C4GUI_WinningTextColor        0xffffdf00
#define C4GUI_WinningBackgroundColor  0xafaf7a00
#define C4GUI_LosingTextColor         0xffffffff
#define C4GUI_LosingBackgroundColor   0x7fafafaf


// border colors for 3D-frames
#define C4GUI_BorderAlpha        0x4f
#define C4GUI_BorderColor1       0x772200
#define C4GUI_BorderColor2       0x331100
#define C4GUI_BorderColor3       0xaa4400
#define C4GUI_BorderColorA1      (C4GUI_BorderAlpha<<24 | C4GUI_BorderColor1)
#define C4GUI_BorderColorA2      (C4GUI_BorderAlpha<<24 | C4GUI_BorderColor2)
#define C4GUI_BorderColorA3      (C4GUI_BorderAlpha<<24 | C4GUI_BorderColor3)

// GUI icon sizes
#define C4GUI_IconWdt   40
#define C4GUI_IconHgt   40
#define C4GUI_IconExWdt 64
#define C4GUI_IconExHgt 64
#define C4GUI_ControllerIconWdt 100
#define C4GUI_ControllerIconHgt 100

#define C4GUI_IconLabelSpacing   2 // space between an icon and its text

// scroll bar size
#define C4GUI_ScrollBarWdt   16
#define C4GUI_ScrollBarHgt   16
#define C4GUI_ScrollArrowHgt 16
#define C4GUI_ScrollArrowWdt 16
#define C4GUI_ScrollThumbHgt 16 // only for non-dynamic scroll thumbs
#define C4GUI_ScrollThumbWdt 16 // only for non-dynamic scroll thumbs

// button size
#define C4GUI_ButtonHgt         32 // height of buttons
#define C4GUI_BigButtonHgt      40 // height of bigger buttons (main menu)
#define C4GUI_ButtonAreaHgt     40 // height of button areas
#define C4GUI_DefButtonWdt     140 // width of default buttons
#define C4GUI_DefButton2Wdt    120 // width of default buttons if there are two of them
#define C4GUI_DefButton2HSpace  10 // horzontal space between two def dlg buttons

// default checkbox height
#define C4GUI_CheckBoxHgt         32
#define C4GUI_CheckBoxLabelSpacing 4 // pixels between checkbox box and label

// list box item spacing
#define C4GUI_DefaultListSpacing  1 // 1 px of free space between two list items
#define C4GUI_ListBoxBarIndent   10

// default dialog box sizes
#define C4GUI_MessageDlgWdt    500 // width of message dialog
#define C4GUI_MessageDlgWdtMedium 360 // width of message dialog w/o much text
#define C4GUI_MessageDlgWdtSmall 300 // width of message dialog w/o much text
#define C4GUI_ProgressDlgWdt   500 // width of progress dialog
#define C4GUI_InputDlgWdt      300
#define C4GUI_DefDlgIndent      10 // indent for default dlg items
#define C4GUI_DefDlgSmallIndent  4 // indent for dlg items that are grouped
#define C4GUI_MessageDlgVRoom  100 // height added to text height in message dialog
#define C4GUI_ProgressDlgVRoom 150 // height added to text height in progress dialog
#define C4GUI_InputDlgVRoom    150
#define C4GUI_ProgressDlgPBHgt  30 // height of progress bar in progress dlg
#define C4GUI_InfoDlgWdt       620 // width of info dialog
#define C4GUI_InfoDlgVRoom     100 // height added to text height in info dialog
#define C4GUI_MaxToolTipWdt    500 // maximum width for tooltip boxes

// time for tooltips to appear (msecs) -evaluated while drawing
#define C4GUI_ToolTipShowTime 500 // 0.5 seconds

// time for title bars to start scrolling to make longer text visible -evaluated while drawing
#define C4GUI_TitleAutoScrollTime 3000 // 3 seconds

// time interval for tab caption scrolling
#define C4GUI_TabCaptionScrollTime 500 // 0.5 seconds

// Z-ordering of dialogs
#define C4GUI_Z_CHAT            +2 // chat input dialog more important than other input dialogs
#define C4GUI_Z_INPUT           +1 // input dialogs on top of others
#define C4GUI_Z_DEFAULT          0 // normal placement on top of viewports
#define C4GUI_Z_PLAYERMENU      -1 // inside viewport: player menu
#define C4GUI_Z_OBJECTMENU      -2 // inside viewport: cursor menu

#define C4GUI_MinWoodBarHgt     23

#define C4GUI_FullscreenDlg_TitleHeight      C4UpperBoardHeight // pixels reserved for top of fullscreen dialog title
#define C4GUI_FullscreenCaptionFontClr       0xffffff00

namespace C4GUI
{

	// some class predefs

	// C4Gui.cpp
	class Element; class Screen; class CMouse;
	class ComponentAligner;

	// C4GuiLabels.cpp
	class Label; class WoodenLabel; class MultilineLabel;
	class HorizontalLine; class ProgressBar;
	class Picture; class Icon; class PaintBox;
	class TextWindow;

	// C4GuiContainers.cpp
	class Container; class Window; class GroupBox; class Control;
	class ScrollBar; class ScrollWindow;

	// C4GuiButton.cpp
	class Button; template <class CallbackDlg, class Base> class CallbackButton;
	class IconButton;
	class CloseButton; class OKButton; class CancelButton;
	class CloseIconButton; class OKIconButton; class CancelIconButton;

	// C4GuiEdit.cpp
	class Edit;

	// C4GuiCheckBox.cpp
	class CheckBox;

	// C4GuiListBox.cpp
	class ListBox;

	// C4GuiTabular.cpp
	class Tabular;

	// C4GuiMenu.cpp
	class ContextMenu;
	class ContextButton;

	// C4GUIComboBox.cpp
	class ComboBox;

	// C4GuiDialog.cpp
	class Dialog; class MessageDialog; class ProgressDialog;
	class InputDialog; class InfoDialog;

	// inline
	class MenuHandler; class ContextHandler;


	// expand text like "Te&xt" to "Te<c ffff00>x</c>t". Color yellow for normal hotkey and red for tooltip.
	bool ExpandHotkeyMarkup(StdStrBuf &sText, uint32_t &rcHotkey, bool for_tooltip = false);

	// make color readable on black: max alpha to 0x1f, max color hues
	DWORD MakeColorReadableOnBlack(DWORD &rdwClr);

	// menu handler: generic context menu callback
	class MenuHandler
	{
	public:
		MenuHandler() = default; //ctor
		virtual ~MenuHandler() = default; // dtor

		virtual void OnOK(Element *pTarget) = 0; // menu item selected
	};

	// context handler: opens context menu on right-click or menu key
	class ContextHandler
	{
	private:
		int32_t iRefs{0};
	public:
		ContextHandler() = default; //ctor
		virtual ~ContextHandler() = default; // dtor

		virtual bool OnContext(Element *pOnElement, int32_t iX, int32_t iY) = 0; // context queried - ret true if handled
		virtual ContextMenu *OnSubcontext(Element *pOnElement) = 0; // subcontext queried

		inline void Ref() { ++iRefs; }
		inline void DeRef() { if (!--iRefs) delete this; }
	};

	// generic callback handler
	class BaseCallbackHandler
	{
	private:
		int32_t iRefs{0};
	public:
		BaseCallbackHandler()  = default;
		virtual ~BaseCallbackHandler() = default;

		inline void Ref() { ++iRefs; }
		inline void DeRef() { if (!--iRefs) delete this; }

		virtual void DoCall(class Element *pElement) = 0;
	};

	template <class CB> class CallbackHandler : public BaseCallbackHandler
	{
	public:
		typedef void (CB::*Func)(class Element *pElement);

	private:
		CB *pCBClass;
		Func CBFunc;

	public:
		void DoCall(class Element *pElement) override
		{
			((pCBClass)->*CBFunc)(pElement);
		}

		CallbackHandler(CB *pTarget, Func rFunc) : pCBClass(pTarget), CBFunc(rFunc) {}
	};

	template <class CB> class CallbackHandlerNoPar : public BaseCallbackHandler
	{
	public:
		typedef void (CB::*Func)();

	private:
		CB *pCBClass;
		Func CBFunc;

	public:
		void DoCall(class Element *pElement) override
		{
			((pCBClass)->*CBFunc)();
		}

		CallbackHandlerNoPar(CB *pTarget, Func rFunc) : pCBClass(pTarget), CBFunc(rFunc) {}
	};

	template <class CB, class ParType> class CallbackHandlerExPar : public BaseCallbackHandler
	{
	public:
		typedef void (CB::*Func)(ParType);

	private:
		CB *pCBClass;
		ParType par;
		Func CBFunc;

	public:
		void DoCall(class Element *pElement) override
		{
			((pCBClass)->*CBFunc)(par);
		}

		CallbackHandlerExPar(CB *pTarget, Func rFunc, ParType par) : pCBClass(pTarget), par(par), CBFunc(rFunc) {}
	};

	// callback with parameter coming from calling class
	template <class ParType> class BaseParCallbackHandler : public BaseCallbackHandler
	{
	protected:
		void DoCall(class Element *pElement) override {assert(false);} // no-par: Not to be called
	public:
		BaseParCallbackHandler() = default;

		virtual void DoCall(ParType par) = 0;
	};

	template <class CB, class ParType> class ParCallbackHandler : public BaseParCallbackHandler<ParType>
	{
	public:
		typedef void (CB::*Func)(ParType par);

	private:
		CB *pCBClass;
		Func CBFunc;


	protected:
		// not to be called, but avoid warning for hiding base class functions
		using BaseParCallbackHandler<ParType>::DoCall;

	public:
		void DoCall(ParType par) override { ((pCBClass)->*CBFunc)(par); }

		ParCallbackHandler(CB *pTarget, Func rFunc) : pCBClass(pTarget), CBFunc(rFunc) {}
	};

	// three facets for left/top, middle and right/bottom of an auto-sized bar
	struct DynBarFacet
	{
		C4Facet fctBegin, fctMiddle, fctEnd;

		void SetHorizontal(C4Surface &rBySfc, int iHeight=0, int iBorderWidth=0);
		void SetHorizontal(C4Facet &rByFct, int32_t iBorderWidth=0);
		void Clear() { fctBegin.Default(); fctMiddle.Default(); fctEnd.Default(); }
	};

	// facets used to draw a scroll bar
	struct ScrollBarFacets
	{
		DynBarFacet barScroll;
		C4Facet fctScrollDTop, fctScrollPin, fctScrollDBottom;

		void Set(const C4Facet &rByFct, int32_t iPinIndex=0);
		void Clear() { barScroll.Clear(); fctScrollDTop.Default(); fctScrollPin.Default(); fctScrollDBottom.Default(); }
	};

	// a generic gui-element
	class Element
	{
	private:
		StdStrBuf ToolTip; // MouseOver - status text
		bool is_immediate_tooltip{false};

	protected:
		Container *pParent{nullptr};     // owning container
		Element *pPrev, *pNext; // previous and next element of same container
		Window *pDragTarget{nullptr};    // target that is dragged around when the user drags this element
		int32_t iDragX, iDragY;     // drag start pos
		bool fDragging{false};         // if set, mouse is down on component and dragging enabled
		ContextHandler *pContextHandler{nullptr}; // context handler to be called upon context menu request
	public:
		bool fVisible{true}; // if false, component (and subcomponents) are not drawn
	protected:
		C4Rect rcBounds; // element bounds

		virtual void Draw(C4TargetFacet &cgo) { DrawElement(cgo); } // draw this class (this + any contents)
		virtual void DrawElement(C4TargetFacet &cgo) { };           // draw element itself

		virtual void RemoveElement(Element *pChild); // clear ptrs

		virtual void UpdateSize();                // called when own size changed
		virtual void UpdatePos();                 // called when own position changed

		void Draw3DFrame(C4TargetFacet &cgo, bool fUp=false, int32_t iIndent=1, BYTE byAlpha=C4GUI_BorderAlpha, bool fDrawTop=true, int32_t iTopOff=0, bool fDrawLeft=true, int32_t iLeftOff=0); // draw frame around element
		void DrawBar(C4TargetFacet &cgo, DynBarFacet &rFacets); // draw gfx bar within element bounds
		void DrawVBar(C4TargetFacet &cgo, DynBarFacet &rFacets); // draw gfx bar within element bounds
		void DrawHBarByVGfx(C4TargetFacet &cgo, DynBarFacet &rFacets);  // draw horizontal gfx bar within element bounds, using gfx of vertical one
		void DrawHVBar(C4TargetFacet &cgo, DynBarFacet &rFacets, C4DrawTransform &trf, int32_t iMiddleLength);

		virtual bool IsOwnPtrElement() { return false; } // if true is returned, item will not be deleted when container is cleared
		virtual bool IsExternalDrawDialog() { return false; }
		virtual bool IsMenu() { return false; }
		virtual class DialogWindow* GetDialogWindow() { return nullptr; } // return DialogWindow if this element is a dialog

		// for listbox-selection by character input
		virtual bool CheckNameHotkey(const char *) { return false; }

	public:
		virtual Container *GetContainer() { return pParent; } // returns parent for elements; this for containers

		virtual void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam); // input: mouse movement or buttons
		virtual void MouseEnter(CMouse &rMouse) {}; // called when mouse cursor enters element region
		virtual void MouseLeave(CMouse &rMouse) {}; // called when mouse cursor leaves element region

		virtual void StartDragging(CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam); // called by element in MouseInput: start dragging
		virtual void DoDragging(CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam);   // called by mouse: dragging process
		virtual void StopDragging(CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam); // called by mouse: mouse released after dragging process

		virtual bool OnHotkey(uint32_t cHotkey) { return false; } // return true when hotkey has been processed

	public:
		bool DoContext(); // open context menu if assigned

	public:
		Element();  // ctor
		virtual ~Element(); // dtor

		Container *GetParent() { return pParent; } // get owning container
		virtual class Dialog *GetDlg(); // return contained dialog
		virtual Screen *GetScreen(); // return contained screen
		virtual Control *IsFocusElement() { return nullptr; }; // return control to gain focus in search-cycle

		virtual void UpdateOwnPos() { };          // called when element bounds were changed externally
		void ScreenPos2ClientPos(int32_t &riX, int32_t &riY); // transform screen coordinates to element coordinates
		void ClientPos2ScreenPos(int32_t &riX, int32_t &riY); // transform element coordinates to screen coordinates

		void SetToolTip(const char *szNewTooltip, bool is_immediate = false); // update used tooltip
		const char *GetToolTip();                  // return tooltip const char* (own or fallback to parent)
		const char *GetOwnToolTip() { return ToolTip.getData(); } // return tooltip const char*, without fallback to parent
		bool IsImmediateToolTip() const { return is_immediate_tooltip; }

		int32_t GetWidth() { return rcBounds.Wdt; }
		int32_t GetHeight() { return rcBounds.Hgt; }
		C4Rect &GetBounds() { return rcBounds; }
		void SetBounds(const C4Rect &rcNewBound) { rcBounds=rcNewBound; UpdatePos(); UpdateSize(); }
		virtual C4Rect &GetClientRect() { return rcBounds; }
		C4Rect GetContainedClientRect() { C4Rect rc=GetClientRect(); rc.x=rc.y=0; return rc; }
		Element *GetNext() const { return pNext; }
		Element *GetPrev() const { return pPrev; }
		virtual Element *GetFirstNestedElement(bool fBackwards) { return this; }
		virtual Element *GetFirstContained() { return nullptr; }
		bool IsInActiveDlg(bool fForKeyboard);
		virtual bool IsParentOf(Element *pEl) { return false; } // whether this is the parent container (directly or recursively) of the passed element

		C4Rect GetToprightCornerRect(int32_t iWidth=16, int32_t iHeight=16, int32_t iHIndent=4, int32_t iVIndent=4, int32_t iIndexX=0); // get rectangle to be used for context buttons and stuff

		bool IsVisible();
		virtual void SetVisibility(bool fToValue);

		virtual int32_t GetListItemTopSpacing() { return C4GUI_DefaultListSpacing; }
		virtual bool GetListItemTopSpacingBar() { return false; }

		void SetDragTarget(Window *pToWindow) { pDragTarget = pToWindow; }
		void SetContextHandler(ContextHandler *pNewHd) // takes over ptr
		{
			if (pContextHandler) pContextHandler->DeRef();
			if ((pContextHandler = pNewHd)) pNewHd->Ref();
		}
		virtual ContextHandler *GetContextHandler(); // get context handler to be used (own or parent)

		friend class Container; friend class TextWindow; friend class ListBox;
	};

	// a simple text label on the screen
	class Label : public Element
	{
	protected:
		StdStrBuf sText; // label text
		DWORD dwFgClr; // text color
		int32_t x0, iAlign; // x-textstart-pos; horizontal alignment
		CStdFont *pFont;
		uint32_t cHotkey;   // hotkey for this label
		bool fAutosize;
		bool fMarkup;

		Control *pClickFocusControl; // control that gets focus if the label is clicked or hotkey is pressed

		void DrawElement(C4TargetFacet &cgo) override; // output label
		void UpdateOwnPos() override;

		bool OnHotkey(uint32_t cHotkey) override; // focus control on correct hotkey

		virtual int32_t GetLeftIndent() { return 0; }

	public:
		Label(const char *szLblText, int32_t iX0, int32_t iTop, int32_t iAlign=ALeft, DWORD dwFClr=0xffffffff, CStdFont *pFont=nullptr, bool fMakeReadableOnBlack = true, bool fMarkup=true); // ctor
		Label(const char *szLblText, const C4Rect &rcBounds, int32_t iAlign=ALeft, DWORD dwFClr=0xffffffff, CStdFont *pFont=nullptr, bool fMakeReadableOnBlack = true, bool fAutosize = true, bool fMarkup=true); // ctor

		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse

		void SetText(const char *szToText, bool fAllowHotkey=true); // update text
		const char *GetText() { return sText.getData(); } // retrieve current text
		void SetClickFocusControl(Control *pToCtrl) { pClickFocusControl=pToCtrl; }
		void SetColor(DWORD dwToClr, bool fMakeReadableOnBlack=true) { dwFgClr = fMakeReadableOnBlack ? MakeColorReadableOnBlack(dwToClr) : dwToClr; } // update label color
		void SetX0(int32_t iToX0);
		void SetAutosize(bool fToVal) { fAutosize = fToVal; }
	};

	// a label with some wood behind
	// used for captions
	class WoodenLabel : public Label
	{
	private:
		uint32_t iAutoScrollDelay; // if set and text is longer than would fit, the label will automatically start moving if not changed and displayed for a while

		// Time when the label text was changed last. nullptr if not initialized; set upon first drawing
		C4TimeMilliseconds tLastChangeTime;  
		int32_t iScrollPos, iScrollDir;
		int32_t iRightIndent;
	protected:
		void DrawElement(C4TargetFacet &cgo) override; // output label

		C4Facet fctIcon; // icon shown at left-side of label; if set, text is aligned to left

		int32_t GetLeftIndent() override { return fctIcon.Surface ? rcBounds.Hgt : 0; }
		int32_t GetRightIndent() const { return iRightIndent; }

	public:
		WoodenLabel(const char *szLblText, const C4Rect &rcBounds, DWORD dwFClr=0xffffffff, CStdFont *pFont=nullptr, int32_t iAlign=ACenter, bool fMarkup=true) // ctor
				: Label(szLblText, rcBounds, iAlign, dwFClr, pFont, true, true, fMarkup), iAutoScrollDelay(0), tLastChangeTime(C4TimeMilliseconds::Now()), iScrollPos(0), iScrollDir(0), iRightIndent(0)
		{ SetAutosize(false); this->rcBounds=rcBounds; }// ctor - re-sets bounds after SetText

		static int32_t GetDefaultHeight(CStdFont *pUseFont=nullptr);

		void SetIcon(const C4Facet &rfctIcon);
		void SetAutoScrollTime(uint32_t tDelay) { iAutoScrollDelay=tDelay; ResetAutoScroll(); }
		void ResetAutoScroll();

		void SetRightIndent(int32_t iNewIndent) { iRightIndent = iNewIndent; }
	};

	// a multiline label with automated text clipping
	// used for display of log buffers
	class MultilineLabel : public Element
	{
	protected:
		C4LogBuffer Lines;
		bool fMarkup;

	protected:
		void DrawElement(C4TargetFacet &cgo) override; // output label
		void UpdateHeight();
		void UpdateSize() override;                // update label height

	public:
		MultilineLabel(const C4Rect &rcBounds, int32_t iMaxLines, int32_t iMaxBuf, const char *szIndentChars, bool fAutoGrow, bool fMarkup); // ctor

		void AddLine(const char *szLine, CStdFont *pFont, DWORD dwClr, bool fDoUpdate, bool fMakeReadableOnBlack, CStdFont *pCaptionFont); // add line of text
		void Clear(bool fDoUpdate);                     // clear lines

		friend class TextWindow;
	};

	// a bar that show progress
	class ProgressBar : public Element
	{
	protected:
		int32_t iProgress, iMax;

		void DrawElement(C4TargetFacet &cgo) override; // draw progress bar

	public:
		ProgressBar(C4Rect &rrcBounds, int32_t iMaxProgress=100) // progress bar ctor
				: Element(), iProgress(0), iMax(iMaxProgress)
		{ rcBounds=rrcBounds; UpdatePos(); }

		void SetProgress(int32_t iToProgress) { iProgress = iToProgress; }
	};

	// Auxiliary design gfx: a horizontal line
	class HorizontalLine : public Element
	{
	protected:
		uint32_t dwClr, dwShadowClr;

		void DrawElement(C4TargetFacet &cgo) override; // draw horizontal line
	public:
		HorizontalLine(C4Rect &rrcBounds, uint32_t dwClr=0x000000, uint32_t dwShadowClr=0xaf7f7f7f)
				: Element(), dwClr(dwClr), dwShadowClr(dwShadowClr)
		{ SetBounds(rrcBounds); }
	};

	// picture displaying a FacetEx
	class Picture : public Element
	{
	protected:
		C4FacetSurface Facet; // picture facet
		bool fAspect;    // preserve width/height-ratio when drawing
		bool fCustomDrawClr; // custom drawing color for clrbyowner-surfaces
		uint32_t dwDrawClr;
		bool fAnimate; // if true, the picture is animated. Whoaa!
		int32_t iPhaseTime, iAnimationPhase, iDelay; // current animation phase - undefined if not animated

		void DrawElement(C4TargetFacet &cgo) override; // draw the image

	public:
		Picture(const C4Rect &rcBounds, bool fAspect); // ctor - does not load image

		const C4FacetSurface &GetFacet() const { return Facet; } // get picture facet
		C4FacetSurface &GetMFacet() { return Facet; } // get picture facet
		void SetFacet(const C4Facet &fct) { Facet.Clear(); Facet.Set(fct); }
		bool EnsureOwnSurface();                // create an own surface, if it's just a link
		void SetDrawColor(uint32_t dwToClr) { dwDrawClr = dwToClr; fCustomDrawClr = true; }
		void SetAnimated(bool fEnabled, int iDelay); // starts/stops cycling through all phases of the specified facet
	};

	// picture displaying two facets
	class OverlayPicture : public Picture
	{
	protected:
		int iBorderSize; // border of overlay image if not zoomed
		C4Facet OverlayImage; // image to be displayed on top of the actual picture

		void DrawElement(C4TargetFacet &cgo) override; // draw the image

	public:
		OverlayPicture(const C4Rect &rcBounds, bool fAspect, const C4Facet &rOverlayImage, int iBorderSize); // ctor - does not load image
	};

	// icon indices
	enum
	{
		Ico_Extended   = 0x100, // icon index offset for extended icons
		Ico_Controller = 0x200,
	};
	enum Icons
	{
		Ico_Empty          = -2, // for context menus only
		Ico_None           = -1,
		Ico_Clonk          = 0,
		Ico_Notify         = 1,
		Ico_Wait           = 2,
		Ico_NetWait        = 3,
		Ico_Host           = 4,
		Ico_Client         = 5,
		Ico_UnknownClient  = 6,
		Ico_UnknownPlayer  = 7,
		Ico_ObserverClient = 8,
		Ico_Player         = 9,
		Ico_Resource       = 10,
		Ico_Error          = 11,
		Ico_SavegamePlayer = 12,
		Ico_Save           = 13,
		Ico_Active         = 14,
		Ico_Options        = 14,
		Ico_Editor         = 14,
		Ico_Inactive       = 15,
		Ico_Kick           = 16,
		Ico_Loading        = 17,
		Ico_Confirm        = 18,
		Ico_Team           = 19,
		Ico_AddPlr         = 20,
		Ico_Record         = 21,
		Ico_Chart          = 21,
		Ico_Gfx            = 22,
		Ico_Sound          = 23,
		Ico_Keyboard       = 24,
		Ico_Gamepad        = 25,
		Ico_MouseOff       = 26,
		Ico_MouseOn        = 27,
		Ico_Help           = 28,
		Ico_Definition     = 29,
		Ico_GameRunning    = 30,
		Ico_Lobby          = 31,
		Ico_RuntimeJoin    = 32,
		Ico_Exit           = 33,
		Ico_Close          = 34,
		Ico_Rank1          = 35,
		Ico_Rank2          = 36,
		Ico_Rank3          = 37,
		Ico_Rank4          = 38,
		Ico_Rank5          = 39,
		Ico_Rank6          = 40,
		Ico_Rank7          = 41,
		Ico_Rank8          = 42,
		Ico_Rank9          = 43,
		Ico_OfficialServer = 44,
		Ico_Surrender      = 45,
		Ico_MeleeLeague    = 46,
		Ico_Ready          = 47,
		Ico_Star           = 48,
		Ico_Disconnect     = 49,
		Ico_View           = 50,
//		Ico_RegJoinOnly    = 51,
		Ico_Ignored        = 52,

		Ico_Ex_RecordOff     = Ico_Extended + 0,
		Ico_Ex_RecordOn      = Ico_Extended + 1,
//		Ico_Ex_FairCrew      = Ico_Extended + 2,
		Ico_Ex_NormalCrew    = Ico_Extended + 3,
		Ico_Ex_LeagueOff     = Ico_Extended + 4,
		Ico_Ex_LeagueOn      = Ico_Extended + 5,
		Ico_Ex_InternetOff   = Ico_Extended + 6,
		Ico_Ex_InternetOn    = Ico_Extended + 7,
		Ico_Ex_League        = Ico_Extended + 8,
//		Ico_Ex_FairCrewGray  = Ico_Extended + 9,
		Ico_Ex_NormalCrewGray= Ico_Extended + 10,
		Ico_Ex_Locked        = Ico_Extended + 11,
		Ico_Ex_Unlocked      = Ico_Extended + 12,
		Ico_Ex_LockedFrontal = Ico_Extended + 13,
		Ico_Ex_Update        = Ico_Extended + 14,
		Ico_Ex_Chat          = Ico_Extended + 15,
		Ico_Ex_GameList      = Ico_Extended + 16,
		Ico_Ex_Comment       = Ico_Extended + 17,

		Ico_Controller_A             = Ico_Controller + 0,
		Ico_Controller_B             = Ico_Controller + 3,
		Ico_Controller_X             = Ico_Controller + 17,
		Ico_Controller_Y             = Ico_Controller + 18,
		Ico_Controller_Back          = Ico_Controller + 1,
		Ico_Controller_Start         = Ico_Controller + 16,
		Ico_Controller_Dpad          = Ico_Controller + 6,
		Ico_Controller_DpadLeft      = Ico_Controller + 5,
		Ico_Controller_DpadRight     = Ico_Controller + 7,
		Ico_Controller_DpadDown      = Ico_Controller + 4,
		Ico_Controller_DpadUp        = Ico_Controller + 8,
		Ico_Controller_LeftShoulder  = Ico_Controller + 9,
		Ico_Controller_RightShoulder = Ico_Controller + 12,
		Ico_Controller_LeftTrigger   = Ico_Controller + 11,
		Ico_Controller_RightTrigger  = Ico_Controller + 14,
		Ico_Controller_LeftStick     = Ico_Controller + 10,
		Ico_Controller_RightStick    = Ico_Controller + 13,
	};

	// cute, litte, useless thingy
	class Icon : public Picture
	{
	public:
		Icon(const C4Rect &rcBounds, Icons icoIconIndex);

		void SetIcon(Icons icoNewIconIndex);
		static C4Facet GetIconFacet(Icons icoIconIndex);
	};

	// a collection of gui-elements
	class Container : public Element
	{
	protected:
		Element *pFirst, *pLast; // contained elements

		void Draw(C4TargetFacet &cgo) override; // draw all elements
		virtual void ElementSizeChanged(Element *pOfElement) { } // called when an element size is changed
		virtual void ElementPosChanged(Element *pOfElement) { }  // called when an element position is changed

		virtual void AfterElementRemoval()
		{ if (pParent) pParent->AfterElementRemoval(); } // called by ScrollWindow to parent after an element has been removed

		bool OnHotkey(uint32_t cHotkey) override; // check all contained elements for hotkey

	public:
		Container();  // ctor
		~Container() override; // dtor

		void Clear();                        // delete all child elements
		void ClearChildren();                // delete all child elements
		void RemoveElement(Element *pChild) override; // remove child element from container
		void MakeLastElement(Element *pChild);       // resort to the end of the list
		void AddElement(Element *pChild); // add child element to container
		void ReaddElement(Element *pChild); // resort child element to end of list
		void InsertElement(Element *pChild, Element *pInsertBefore); // add child element to container, ordered directly before given, other element
		Element *GetNextNestedElement(Element *pPrevElement, bool fBackwards); // get next element after given, applying recursion
		Element *GetFirstContained() override { return pFirst; }
		virtual Element *GetLastContained() { return pLast; }
		Element *GetFirstNestedElement(bool fBackwards) override;
		
		class Iterator
		{
		private:
			Element *current;
		public:
			Iterator(Element *element = nullptr) : current(element) { }

			Element * operator*() const { return current; }
			Element * operator->() const { return current; }
			void operator++() { current = current->GetNext(); };
			void operator++(int) { operator++(); }

			bool operator==(const Iterator & other) const
			{
				return (current == other.current);
			}

			bool operator!=(const Iterator & other) const
			{
				return !(*this == other);
			}
		};

		class ReverseIterator
		{
		private:
			Element *current;
		public:
			ReverseIterator(Element *element = nullptr) : current(element) { }

			Element * operator*() const { return current; }
			Element * operator->() const { return current; }
			void operator++() { current = current->GetPrev(); };
			void operator++(int) { operator++(); }

			bool operator==(const ReverseIterator & other) const
			{
				return (current == other.current);
			}

			bool operator!=(const ReverseIterator & other) const
			{
				return !(*this == other);
			}
		};
		
		// provide C++-style iterator interface
		Iterator begin() { return Iterator(pFirst); }
		Iterator end() { return Iterator(nullptr); }
		ReverseIterator rbegin() { return ReverseIterator(pLast); }
		ReverseIterator rend() { return ReverseIterator(nullptr); }

		Element *GetFirst() { return pFirst; }
		Element *GetLast() { return pLast; }
		Container *GetContainer() override { return this; } // returns parent for elements; this for containers
		Element *GetElementByIndex(int32_t i); // get indexed child element
		int32_t GetElementCount();

		void SetVisibility(bool fToValue) override;
		virtual bool IsFocused(Control *pCtrl) { return pParent ? pParent->IsFocused(pCtrl) : false; }
		virtual bool IsSelectedChild(Element *pChild) { return pParent ? pParent->IsSelectedChild(pChild) : true; } // whether the child element is selected - only false for list-box-containers which can have unselected children
		bool IsParentOf(Element *pEl) override; // whether this is the parent container (directly or recursively) of the passed element

		virtual void ApplyElementOffset(int32_t &riX, int32_t &riY) {} // apply child drawing offset
		virtual void ApplyInvElementOffset(int32_t &riX, int32_t &riY) {} // subtract child drawing offset


		friend class Element; friend class ScrollWindow;
	};

	// a rectangled control that contains other elements
	class Window : public Container
	{
	protected:
		C4Rect rcClientRect; // area for contained elements

		void Draw(C4TargetFacet &cgo) override; // draw this window

	public:
		Window(); // ctor

		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse movement or buttons

		void SetPos(int32_t iXPos, int32_t iYPos)
		{ rcBounds.x=iXPos; rcBounds.y=iYPos; UpdatePos(); }

		void UpdateOwnPos() override; // update client rect
		C4Rect &GetClientRect() override { return rcClientRect; }

		void ApplyElementOffset(int32_t &riX, int32_t &riY) override
		{ riX -= rcClientRect.x; riY -= rcClientRect.y; }
		void ApplyInvElementOffset(int32_t &riX, int32_t &riY) override
		{ riX += rcClientRect.x; riY += rcClientRect.y; }
		virtual bool IsComponentOutsideClientArea() { return false; } // if set, drawing routine of subcomponents will clip to Bounds rather than to ClientRect

		// get margins from bounds to client rect
		virtual int32_t GetMarginTop() { return 0; }
		virtual int32_t GetMarginLeft() { return 0; }
		virtual int32_t GetMarginRight() { return 0; }
		virtual int32_t GetMarginBottom() { return 0; }
	};

	// a scroll bar
	class ScrollBar : public Element
	{
	protected:
		bool fScrolling;        // if set, scrolling is currently enabled
		bool fAutoHide;         // if set, bar is made invisible if scrolling is not possible anyway
		int32_t iScrollThumbSize; // height(/width) of scroll thumb
		int32_t iScrollPos;         // y(/x) offset of scroll thumb
		bool fTopDown, fBottomDown; // whether scrolling buttons are pressed
		bool fHorizontal;           // if set, the scroll bar is horizontal instead of vertical
		int32_t iCBMaxRange;        // range for callback class

		ScrollWindow *pScrollWindow; // associated scrolled window - may be 0 for callback scrollbars
		BaseParCallbackHandler<int32_t> *pScrollCallback; // callback called when scroll pos changes

		ScrollBarFacets *pCustomGfx;

		void Update();       // update scroll bar according to window
		void OnPosChanged(); // update window according to scroll bar, and/or do callbacks

		void DrawElement(C4TargetFacet &cgo) override; // draw scroll bar

		// suppress scrolling pin for very narrow menus
		bool HasPin()
		{
			if (fHorizontal) return rcBounds.Wdt > (2*C4GUI_ScrollArrowWdt + C4GUI_ScrollThumbWdt);
			else             return rcBounds.Hgt > (2*C4GUI_ScrollArrowHgt + C4GUI_ScrollThumbHgt);
		}
		int32_t GetMaxScroll()
		{
			if (fHorizontal) return HasPin() ? GetBounds().Wdt - 2*C4GUI_ScrollArrowWdt - iScrollThumbSize : 100;
			else             return HasPin() ? GetBounds().Hgt - 2*C4GUI_ScrollArrowHgt - iScrollThumbSize : 100;
		}
		int32_t GetScrollByPos(int32_t iX, int32_t iY)
		{
			return Clamp<int32_t>((fHorizontal ? iX-C4GUI_ScrollArrowWdt : iY-C4GUI_ScrollArrowHgt)-iScrollThumbSize/2, 0, GetMaxScroll());
		}
		bool IsScrolling() { return fScrolling; }

	public:
		ScrollBar(C4Rect &rcBounds, ScrollWindow *pWin); // ctor for scroll window
		ScrollBar(C4Rect &rcBounds, bool fHorizontal, BaseParCallbackHandler<int32_t> *pCB, int32_t iCBMaxRange=256); // ctor for callback
		~ScrollBar() override; // dtor

		// mouse handling
		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse movement or buttons
		void DoDragging(CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam) override;   // dragging: allow dragging of thumb
		void MouseLeave(CMouse &rMouse) override;                                     // mouse leaves with button down: reset down state of buttons

		// change style
		void SetDecoration(ScrollBarFacets *pToGfx, bool fAutoHide)
		{ pCustomGfx = pToGfx; this->fAutoHide=fAutoHide; }
		// change scroll pos in a [0, iCBMaxRange-1] scale
		void SetScrollPos(int32_t iToPos) { iScrollPos = iToPos * GetMaxScroll() / (iCBMaxRange-1); }

		friend class ScrollWindow;
		friend class ::C4ScriptGuiWindow;
	};

	// a window that can be scrolled
	class ScrollWindow : public Window
	{
	protected:
		ScrollBar *pScrollBar; // vertical scroll bar associated with the window
		int32_t iScrollY;          // vertical scroll pos
		int32_t iClientHeight;     // client rect height
		bool fHasBar;
		int32_t iFrozen;           // if >0, no scrolling updates are done (used during window refill)

		// pass element updates through to parent window
		void ElementSizeChanged(Element *pOfElement) override // called when an element size is changed
		{
			Window::ElementSizeChanged(pOfElement);
			if (pParent) pParent->ElementSizeChanged(pOfElement);
		}
		void ElementPosChanged(Element *pOfElement) override  // called when an element position is changed
		{
			Window::ElementPosChanged(pOfElement);
			if (pParent) pParent->ElementPosChanged(pOfElement);
		}

	public:
		ScrollWindow(Window *pParentWindow); // create scroll window in client area of window
		~ScrollWindow() override { if (pScrollBar) pScrollBar->pScrollWindow = nullptr; }

		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override;

		bool IsComponentOutsideClientArea() override { return true; } // always clip drawing routine of subcomponents to Bounds
		void Update();               // update client rect and scroll bar according to window
		void UpdateOwnPos() override;
		void Freeze() { ++iFrozen; }
		void UnFreeze() { if (!--iFrozen) Update(); }
		bool IsFrozen() const { return !!iFrozen; }

		void SetClientHeight(int32_t iToHgt) // set new client height
		{ iClientHeight=iToHgt; Update(); }

		// change style
		void SetDecoration(ScrollBarFacets *pToGfx, bool fAutoScroll)
		{ if (pScrollBar) pScrollBar->SetDecoration(pToGfx, fAutoScroll); }

		void SetScroll(int32_t iToScroll); // sets new scrolling; does not update scroll bar
		void ScrollToBottom();    // set scrolling to bottom range; updates scroll bar
		void ScrollPages(int iPageCount);    // scroll down by multiples of visible height; updates scroll bar
		void ScrollBy(int iAmount);    // scroll down by vertical pixel amount; updates scroll bar
		void ScrollRangeInView(int32_t iY, int32_t iHgt); // sets scrolling so range is in view; updates scroll bar
		bool IsRangeInView(int32_t iY, int32_t iHgt); // returns whether scrolling range is in view

		int32_t GetScrollY() { return iScrollY; }

		void SetScrollBarEnabled(bool fToVal, bool noAutomaticPositioning = false);
		bool IsScrollBarEnabled() { return fHasBar; }

		bool IsScrollingActive() { return fHasBar && pScrollBar && pScrollBar->IsScrolling(); }
		bool IsScrollingNecessary() { return iClientHeight > rcBounds.Hgt; }

		friend class ScrollBar;
	};

	// a collection of components
	class GroupBox : public Window
	{
	private:
		StdStrBuf sTitle;
		CStdFont *pFont;
		uint32_t dwFrameClr, dwTitleClr, dwBackClr;
		int32_t iMargin;

		CStdFont *GetTitleFont() const;

	public:
		GroupBox(C4Rect &rtBounds) : Window(), pFont(nullptr), dwFrameClr(0u), dwTitleClr(C4GUI_CaptionFontClr), dwBackClr(0xffffffff), iMargin(4)
		{
			// init client rect
			SetBounds(rtBounds);
		} // ctor

		void SetFont(CStdFont *pToFont) { pFont = pToFont; }
		void SetColors(uint32_t dwFrameClr, uint32_t dwTitleClr, uint32_t dwBackClr=0xffffffff) { this->dwFrameClr = dwFrameClr; this->dwTitleClr = dwTitleClr; this->dwBackClr = dwBackClr; }
		void SetTitle(const char *szToTitle) { if (szToTitle && *szToTitle) sTitle.Copy(szToTitle); else sTitle.Clear(); UpdateOwnPos(); }
		void SetMargin(int32_t iNewMargin) { iMargin = iNewMargin; UpdateOwnPos(); }

		bool HasTitle() const { return !!sTitle.getLength(); }

		void DrawElement(C4TargetFacet &cgo) override; // draw frame

		int32_t GetMarginTop() override { return HasTitle() ? iMargin + GetTitleFont()->GetLineHeight() : iMargin; }
		int32_t GetMarginLeft() override { return iMargin; }
		int32_t GetMarginRight() override { return iMargin; }
		int32_t GetMarginBottom() override { return iMargin; }
	};

	// a drawing area
	class PaintBox : public Window
	{
	protected:
		C4FacetSurface fctPaint;

		void DrawElement(C4TargetFacet &cgo) override; // draw what has been painted
	public:
		PaintBox(C4Rect &rtBounds, int32_t iSfcWdt=-1, int32_t iSfcHgt=-1); // ctor
		~PaintBox() override;                // dtor

		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse
	};

	// a control that may have focus
	class Control : public Window
	{
	private:
		class C4KeyBinding *pKeyContext;
	protected:
		virtual bool CharIn(const char *) { return false; }         // input: character key pressed - should return false for none-character-inputs

		void DisableFocus(); // called when control gets disabled: Make sure it loses focus
		virtual bool IsFocusOnClick() { return true; } // defaultly, controls get focused on left-down
		Control *IsFocusElement() override { return this; }; // this control can gain focus
		virtual void OnGetFocus(bool fByMouse) {}; // callback when control gains focus
		virtual void OnLooseFocus() {}; // callback when control looses focus

		bool KeyContext() { return DoContext(); }

	public:
		Control(const C4Rect &rtBounds);  // ctor
		~Control() override;

		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse. left-click sets focus

		bool HasFocus() { return pParent && pParent->IsFocused(this); }
		bool HasDrawFocus();

		friend class Dialog; friend class ListBox;
	};

	// generic callback-functions to the dialog
	template <class CallbackDlg> class DlgCallback
	{
	public:
		typedef void (CallbackDlg::*Func)(Control *pFromControl);
		typedef bool (CallbackDlg::*BoolFunc)(Control *pFromControl);
		typedef bool (CallbackDlg::*Bool2Func)(Control *pFromControl, bool fBool, bool fBool2);
		typedef ContextMenu *(CallbackDlg::*ContextFunc)(Element *pFromElement, int32_t iX, int32_t iY);
		typedef void (CallbackDlg::*ContextClickFunc)(Element *pTargetElement);
	};

	// multi-param callback-functions to the dialog
	template <class CallbackDlg, class TEx> class DlgCallbackEx
	{
	public:
		typedef void (CallbackDlg::*ContextClickFunc)(Element *pTargetElement, TEx Extra);
	};

	// a button. may be pressed.
	class Button : public Control
	{
	private:
		class C4KeyBinding *pKeyButton;
		DynBarFacet *pCustomGfx, *pCustomGfxDown;

	protected:
		StdStrBuf sText;    // button label
		CStdFont *pCustomFont;    // custom font (if assigned)
		DWORD dwCustomFontClr;    // text font color (valid only if pCustomFont)
		bool fDown;         // if set, button is currently held down
		bool fMouseOver;    // if set, the mouse hovers over the button
		uint32_t cHotkey;   // hotkey for this button
		bool fEnabled;

		bool IsFocusOnClick() override { return false; } // buttons don't get focus on click (for easier input, e.g. in chatbox)

		void DrawElement(C4TargetFacet &cgo) override; // draw dlg bg

		virtual void OnPress(); // called when button is pressed

		bool KeyButtonDown();
		bool KeyButtonUp();
		void SetDown(); // mark down and play sound
		void SetUp(bool fPress);   // mark up and play sound

		bool OnHotkey(uint32_t cHotkey) override; // press btn on correct hotkey

	public:
		Button(const char *szBtnText, const C4Rect &rtBounds); // ctor
		~Button() override; // dtor

		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse movement or buttons
		void MouseEnter(CMouse &rMouse) override; // mouse re-enters with button down: set button down
		void MouseLeave(CMouse &rMouse) override; // mouse leaves with button down: reset down state

		void SetText(const char *szToText); // update button text (and hotkey)
		void SetCustomGraphics(DynBarFacet *pCustomGfx, DynBarFacet *pCustomGfxDown)
		{ this->pCustomGfx = pCustomGfx; this->pCustomGfxDown = pCustomGfxDown; }
		void SetEnabled(bool fToVal) { fEnabled=fToVal; if (!fEnabled) fDown=false; }
		void SetFont(CStdFont *pFont, DWORD dwCustomFontClr=C4GUI_CaptionFontClr) { this->pCustomFont = pFont; this->dwCustomFontClr=dwCustomFontClr; }
	};

	// button using icon image
	class IconButton : public Button
	{
	protected:
		C4Facet fctIcon;
		uint32_t dwClr;
		bool fHasClr;
		bool fHighlight; // if true, the button is highlighted permanently

		void DrawElement(C4TargetFacet &cgo) override; // draw icon and highlight if necessary

	public:
		IconButton(Icons eUseIcon, const C4Rect &rtBounds, char cHotkey='\0', const char *tooltip_text=nullptr); // ctor
		void SetIcon(Icons eUseIcon);
		void SetFacet(const C4Facet &rCpy, uint32_t dwClr=0u) { fctIcon = rCpy; }
		void SetColor(uint32_t dwClr) { fHasClr=true; this->dwClr=dwClr; }
		void SetHighlight(bool fToVal) { fHighlight=fToVal; }
	};

	// button using arrow image
	class ArrowButton : public Button
	{
	public:
		enum ArrowFct { Left=0, Right=1, Down=2 };
	protected:
		ArrowFct eDir;

		void DrawElement(C4TargetFacet &cgo) override; // draw arrow; highlight and down if necessary

	public:
		ArrowButton(ArrowFct eDir, const C4Rect &rtBounds, char cHotkey=0); // ctor

		static int32_t GetDefaultWidth();
		static int32_t GetDefaultHeight();
	};

	// button using facets for highlight
	class FacetButton : public Button
	{
	protected:
		C4Facet fctBase, fctHighlight;
		uint32_t dwTextClrInact, dwTextClrAct; // text colors for inactive/active button
		FLOAT_RECT rcfDrawBounds; // drawing bounds

		// title drawing parameters
		int32_t iTxtOffX, iTxtOffY;
		uint8_t byTxtAlign;         // ALeft, ACenter or ARight
		CStdFont *pFont; float fFontZoom;

		void DrawElement(C4TargetFacet &cgo) override; // draw base facet or highlight facet if necessary

	public:
		FacetButton(const C4Facet &rBaseFct, const C4Facet &rHighlightFct, const FLOAT_RECT &rtfBounds, char cHotkey); // ctor
		void SetTextColors(uint32_t dwClrInact, uint32_t dwClrAct)
		{ dwTextClrInact = dwClrInact; dwTextClrAct = dwClrAct; }
		void SetTextPos(int32_t iOffX, int32_t iOffY, uint8_t byAlign=ACenter)
		{ iTxtOffX=iOffX; iTxtOffY=iOffY; byTxtAlign=byAlign; }
		void SetTextFont(CStdFont *pFont, float fFontZoom=1.0f)
		{ this->pFont=pFont; this->fFontZoom=fFontZoom; }
	};

	// a button doing some callback...
	template <class CallbackDlg, class Base=Button> class CallbackButton : public Base
	{
	protected:
		CallbackDlg *pCB;

		typename DlgCallback<CallbackDlg>::Func pCallbackFn; // callback function
		void OnPress() override
		{
			if (pCallbackFn)
			{
				CallbackDlg *pC=pCB;
				if (!pC) if (!(pC=reinterpret_cast<CallbackDlg *>(Base::GetDlg()))) return;
				(pC->*pCallbackFn)(this);
			}
		}

	public:
		CallbackButton(ArrowButton::ArrowFct eDir, const C4Rect &rtBounds, typename DlgCallback<CallbackDlg>::Func pFn, CallbackDlg *pCB=nullptr) // ctor
				: Base(eDir, rtBounds, 0), pCB(pCB), pCallbackFn(pFn) { }
		CallbackButton(const char *szBtnText, C4Rect &rtBounds, typename DlgCallback<CallbackDlg>::Func pFn, CallbackDlg *pCB=nullptr) // ctor
				: Base(szBtnText, rtBounds), pCB(pCB), pCallbackFn(pFn) { }
		CallbackButton(Icons eUseIcon, const C4Rect &rtBounds, char cHotkey, typename DlgCallback<CallbackDlg>::Func pFn, CallbackDlg *pCB=nullptr) // ctor
				: Base(eUseIcon, rtBounds, cHotkey), pCB(pCB), pCallbackFn(pFn) { }
		CallbackButton(Icons eUseIcon, const C4Rect &rtBounds, const char *tooltip_text, typename DlgCallback<CallbackDlg>::Func pFn, CallbackDlg *pCB = nullptr) // ctor
				: Base(eUseIcon, rtBounds, '\0', tooltip_text), pCB(pCB), pCallbackFn(pFn) { }
		CallbackButton(int32_t iID, const C4Rect &rtBounds, char cHotkey, typename DlgCallback<CallbackDlg>::Func pFn, CallbackDlg *pCB=nullptr) // ctor
				: Base(iID, rtBounds, cHotkey), pCB(pCB), pCallbackFn(pFn) { }
	};

	// a button doing some callback to any class
	template <class CallbackDlg, class Base=Button> class CallbackButtonEx : public Base
	{
	protected:
		typedef CallbackDlg * CallbackDlgPointer;
		CallbackDlgPointer pCBTarget;                     // callback target
		typename DlgCallback<CallbackDlg>::Func pCallbackFn; // callback function
		void OnPress() override
		{ (pCBTarget->*pCallbackFn)(this); }

	public:
		CallbackButtonEx(const char *szBtnText, const C4Rect &rtBounds, CallbackDlgPointer pCBTarget, typename DlgCallback<CallbackDlg>::Func pFn) // ctor
				: Base(szBtnText, rtBounds), pCBTarget(pCBTarget), pCallbackFn(pFn) { }
		CallbackButtonEx(Icons eUseIcon, const C4Rect &rtBounds, char cHotkey, CallbackDlgPointer pCBTarget, typename DlgCallback<CallbackDlg>::Func pFn) // ctor
				: Base(eUseIcon, rtBounds, cHotkey), pCBTarget(pCBTarget), pCallbackFn(pFn) { }
		CallbackButtonEx(const C4Facet &fctBase, const C4Facet &fctHighlight, const FLOAT_RECT &rtfBounds, char cHotkey, CallbackDlgPointer pCBTarget, typename DlgCallback<CallbackDlg>::Func pFn) // ctor
				: Base(fctBase, fctHighlight, rtfBounds, cHotkey), pCBTarget(pCBTarget), pCallbackFn(pFn) { }
	};

	// an edit control to type text in
	class Edit : public Control
	{
	public:
		Edit(const C4Rect &rtBounds, bool fFocusEdit=false); // ctor
		~Edit() override;

		enum InputResult // action to be taken when text is confirmed with enter
		{
			IR_None=0,    // do nothing and continue pasting
			IR_CloseDlg,  // stop any pastes and close parent dialog successfully
			IR_CloseEdit, // stop any pastes and remove this control
			IR_Abort      // do nothing and stop any pastes
		};

	private:
		enum CursorOperation { COP_BACK, COP_DELETE, COP_LEFT, COP_RIGHT, COP_HOME, COP_END };
		static const char *CursorRepresentation;

		bool KeyCursorOp(const C4KeyCodeEx &key, const CursorOperation &op);
		bool KeyEnter();
		bool KeyCopy() { Copy(); return true; }
		bool KeyPaste() { Paste(); return true; }
		bool KeyCut() { Cut(); return true; }
		bool KeySelectAll() { SelectAll(); return true; }

		class C4KeyBinding *RegisterCursorOp(CursorOperation op, C4KeyCode key, const char *szName, C4CustomKey::Priority eKeyPrio);

		class C4KeyBinding *pKeyCursorBack, *pKeyCursorDel, *pKeyCursorLeft, *pKeyCursorRight, *pKeyCursorHome, *pKeyCursorEnd,
					*pKeyEnter, *pKeyCopy, *pKeyPaste, *pKeyCut, *pKeySelAll;

	protected:
		// context callbacks
		ContextMenu *OnContext(C4GUI::Element *pListItem, int32_t iX, int32_t iY);
		void OnCtxCopy(C4GUI::Element *pThis) { Copy(); };
		void OnCtxPaste(C4GUI::Element *pThis) { Paste(); };
		void OnCtxCut(C4GUI::Element *pThis) { Cut(); };
		void OnCtxClear(C4GUI::Element *pThis) { DeleteSelection(); };
		void OnCtxSelAll(C4GUI::Element *pThis) { SelectAll(); };

	private:
		void Deselect(); // clear selection range

	public:
		bool InsertText(const char *szText, bool fUser); // insert text at cursor pos (returns whether all text could be inserted)
		void ClearText();    // remove all the text
		void DeleteSelection(); // deletes the selected text. Adjust cursor position if necessary
		bool SetText(const char *szText, bool fUser) { ClearText(); return InsertText(szText, fUser); }
		void SetPasswordMask(char cNewPasswordMask) { cPasswordMask = cNewPasswordMask; } // mask edit box contents using the given character

	private:
		int32_t GetCharPos(int32_t iControlXPos); // get character index of pixel position; always resides within current text length
		void EnsureBufferSize(int32_t iMinBufferSize); // ensure buffer has desired size
		void ScrollCursorInView();                 // ensure cursor pos is visible in edit control
		bool DoFinishInput(bool fPasting, bool fPastingMore); // do OnFinishInput callback and process result - returns whether pasting operation should be continued

		bool Copy(); bool Cut(); bool Paste();     // clipboard operations

	protected:
		CStdFont *pFont;           // font for edit
		char *Text;                // edit text
		uint32_t dwBGClr, dwFontClr, dwBorderColor; // drawing colors for edit box
		int32_t iBufferSize;           // size of current buffer
		int32_t iCursorPos;            // cursor position: char, before which the cursor is located
		int32_t iSelectionStart, iSelectionEnd; // selection range (start may be larger than end)
		int32_t iMaxTextLength;        // maximum number of characters to be input here
		C4TimeMilliseconds tLastInputTime;     // time of last input (for cursor flashing)
		int32_t iXScroll;              // horizontal scrolling
		char cPasswordMask;         // character to be used for masking the contents. 0 for none.

		bool fLeftBtnDown;         // flag whether left mouse button is down or not

		bool CharIn(const char * c) override;                                            // input: character key pressed - should return false for none-character-inputs
		void DoDragging(CMouse &rMouse, int32_t iX, int32_t iY, DWORD dwKeyParam) override;   // dragging: allow text selection outside the component
		bool IsFocusOnClick() override { return true; } // edit fields do get focus on click
		void OnGetFocus(bool fByMouse) override;                     // edit control gets focus
		void OnLooseFocus() override;                   // edit control looses focus

		void DrawElement(C4TargetFacet &cgo) override; // draw edit control

		// called when user presses enter in single-line edit control - closes the current dialog
		virtual InputResult OnFinishInput(bool fPasting, bool fPastingMore) { return IR_CloseDlg; }
		virtual void OnAbortInput() {}

		// get margins from bounds to client rect
		int32_t GetMarginTop() override { return 2; }
		int32_t GetMarginLeft() override { return 4; }
		int32_t GetMarginRight() override { return 4; }
		int32_t GetMarginBottom() override { return 2; }

	public:
		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse movement or buttons

		const char *GetText() { return Text; }
		void SelectAll(); // select all the text

		static int32_t GetDefaultEditHeight();
		static int32_t GetCustomEditHeight(CStdFont *pUseFont);

		bool GetCurrentWord(char *szTargetBuf, int32_t iMaxTargetBufLen); // get word before cursor pos (for nick completion)

		// layout
		void SetFont(CStdFont *pToFont) { pFont=pToFont; ScrollCursorInView(); }
		void SetColors(uint32_t dwNewBGClr, uint32_t dwNewFontClr, uint32_t dwNewBorderColor)
		{ dwBGClr = dwNewBGClr; dwFontClr = dwNewFontClr; dwBorderColor = dwNewBorderColor; }

		void SetMaxText(int32_t iTo) { iMaxTextLength = iTo; }
	};

	// an edit doing some callback
	template <class CallbackCtrl> class CallbackEdit : public Edit
	{
	private:
		CallbackCtrl *pCBCtrl;

	protected:
		typedef InputResult (CallbackCtrl::*CBFunc)(Edit *, bool, bool);
		typedef void (CallbackCtrl::*CBAbortFunc)();
		CBFunc pCBFunc; CBAbortFunc pCBAbortFunc;
		InputResult OnFinishInput(bool fPasting, bool fPastingMore) override
		{ if (pCBFunc && pCBCtrl) return (pCBCtrl->*pCBFunc)(this, fPasting, fPastingMore); else return IR_CloseDlg; }
		void OnAbortInput() override
		{ if (pCBAbortFunc && pCBCtrl) (pCBCtrl->*pCBAbortFunc)(); }

	public:
		CallbackEdit(const C4Rect &rtBounds, CallbackCtrl * pCBCtrl, CBFunc pCBFunc, CBAbortFunc pCBAbortFunc=nullptr) // ctor
				: Edit(rtBounds), pCBCtrl(pCBCtrl), pCBFunc(pCBFunc), pCBAbortFunc(pCBAbortFunc) { }
	};

	// an edit control that renames a label - some less decoration; abort on Escape and focus loss
	class RenameEdit : public Edit
	{
	private:
		C4KeyBinding *pKeyAbort; // key bindings
		bool fFinishing;  // set during deletion process
		Label *pForLabel; // label that is being renamed
		Control *pPrevFocusCtrl; // previous focus element to be restored after rename

	public:
		enum RenameResult
		{
			RR_Invalid=0, // rename not accepted; continue editing
			RR_Accepted,  // rename accepted; delete control
			RR_Deleted    // control deleted - leave everything
		};

	public:
		RenameEdit(Label *pLabel); // ctor - construct for label; add element; set focus
		~RenameEdit() override;

		void Abort();

	private:
		void FinishRename(); // renaming aborted or finished - remove this element and restore label

	protected:
		bool KeyAbort() { Abort(); return true; }
		InputResult OnFinishInput(bool fPasting, bool fPastingMore) override; // forward last input to OnOKRename
		void OnLooseFocus() override; // callback when control looses focus: OK input

		virtual void OnCancelRename() {} // renaming was aborted
		virtual RenameResult OnOKRename(const char *szNewName) = 0; // rename performed - return whether name was accepted
	};

	template <class CallbackDlg, class ParType> class CallbackRenameEdit : public RenameEdit
	{
	public:
	protected:
		typedef void (CallbackDlg::*CBCancelFunc)(ParType);
		typedef RenameResult (CallbackDlg::*CBOKFunc)(ParType, const char *);

		CBCancelFunc pCBCancelFunc; CBOKFunc pCBOKFunc;
		CallbackDlg *pDlg; ParType par;

		void OnCancelRename() override { if (pDlg && pCBCancelFunc) (pDlg->*pCBCancelFunc)(par); }
		RenameResult OnOKRename(const char *szNewName) override { return (pDlg && pCBOKFunc) ? (pDlg->*pCBOKFunc)(par, szNewName) : RR_Accepted; }

	public:
		CallbackRenameEdit(Label *pForLabel, CallbackDlg *pDlg, const ParType &par, CBOKFunc pCBOKFunc, CBCancelFunc pCBCancelFunc) // ctor
				: RenameEdit(pForLabel), pCBCancelFunc(pCBCancelFunc), pCBOKFunc(pCBOKFunc), pDlg(pDlg), par(par) { }
	};

	// editbox below descriptive label sharing one window for common tooltip
	class LabeledEdit : public C4GUI::Window
	{
	public:
		LabeledEdit(const C4Rect &rcBounds, const char *szName, bool fMultiline, const char *szPrefText=nullptr, CStdFont *pUseFont=nullptr, uint32_t dwTextClr = C4GUI_CaptionFontClr);
	private:
		C4GUI::Edit *pEdit;
	public:
		const char *GetText() const { return pEdit->GetText(); }
		C4GUI::Edit *GetEdit() const { return pEdit; }
		static bool GetControlSize(int *piWdt, int *piHgt, const char *szForText, CStdFont *pForFont, bool fMultiline);
	};

	// checkbox with a text label right of it
	class CheckBox : public Control
	{
	private:
		bool fChecked;
		class C4KeyBinding *pKeyCheck;
		StdStrBuf sCaption;
		bool fMouseOn;
		BaseCallbackHandler *pCBHandler; // callback handler called if check state changes
		bool fEnabled;
		CStdFont *pFont;
		uint32_t dwEnabledClr, dwDisabledClr;
		uint32_t cHotkey;

	public:
		CheckBox(const C4Rect &rtBounds, const char *szCaption, bool fChecked); // ctor
		~CheckBox() override;

	private:
		bool KeyCheck() { ToggleCheck(true); return true; }

	public:
		void ToggleCheck(bool fByUser); // check on/off; do callback

	protected:
		void UpdateOwnPos() override;
		bool IsFocusOnClick() override { return false; } // just check/uncheck on click; do not gain keyboard focus as well
		Control *IsFocusElement() override { return fEnabled ? Control::IsFocusElement() : nullptr; }; // this control can gain focus if enabled
		void DrawElement(C4TargetFacet &cgo) override; // draw checkbox
		bool OnHotkey(uint32_t cHotkey) override; // return true when hotkey has been processed

	public:
		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse movement or buttons
		void MouseEnter(CMouse &rMouse) override;
		void MouseLeave(CMouse &rMouse) override;

		void SetChecked(bool fToVal) { fChecked = fToVal; } // set w/o callback
		bool GetChecked() const { return fChecked; }
		void SetOnChecked(BaseCallbackHandler *pCB);
		void SetEnabled(bool fToVal) { if (!(fEnabled=fToVal)) DisableFocus(); }

		const char *GetText() { return sCaption.getData(); }

		void SetFont(CStdFont *pFont, uint32_t dwEnabledClr, uint32_t dwDisabledClr)
		{ this->pFont=pFont; this->dwEnabledClr=dwEnabledClr; this->dwDisabledClr=dwDisabledClr; }

		static bool GetStandardCheckBoxSize(int *piWdt, int *piHgt, const char *szForCaptionText, CStdFont *pUseFont); // get needed size to construct a checkbox
	};

	// a vertical list of elements
	class ListBox : public Control
	{
	private:
		class C4KeyBinding *pKeyContext, *pKeyUp, *pKeyDown, *pKeyPageUp, *pKeyPageDown, *pKeyHome, *pKeyEnd, *pKeyActivate, *pKeyLeft, *pKeyRight;

		bool KeyContext();
		bool KeyUp();
		bool KeyDown();
		bool KeyLeft();
		bool KeyRight();
		bool KeyPageUp();
		bool KeyPageDown();
		bool KeyHome();
		bool KeyEnd();
		bool KeyActivate();

	protected:
		int32_t iMultiColItemWidth; // if nonzero, the listbox is multicolumn and the column count depends on how many items fit in
		int32_t iColCount; // number of columns (usually 1)
		ScrollWindow *pClientWindow; // client scrolling window
		Element *pSelectedItem;      // selected list item
		BaseCallbackHandler *pSelectionChangeHandler, *pSelectionDblClickHandler;
		bool fDrawBackground; // whether darker background is to be drawn
		bool fDrawBorder;     // whether 3D frame around box shall be drawn or nay
		bool fSelectionDisabled; // if set, no entries can be selected

		void DrawElement(C4TargetFacet &cgo) override; // draw listbox

		bool IsFocused(Control *pCtrl) override
		{
			// subcontrol also counts as focused if the list has focus and the subcontrol is selected
			return Control::IsFocused(pCtrl) || (HasFocus() && pSelectedItem == pCtrl);
		}
		bool IsFocusOnClick() override { return true; } // list boxes do get focus on click
		Control *IsFocusElement() override { return this; }; // this control can gain focus
		void OnGetFocus(bool fByMouse) override; // callback when control gains focus - select a list item if none are selected
		bool CharIn(const char * c) override; // character input for direct list element selection

		void AfterElementRemoval() override
		{ Container::AfterElementRemoval(); UpdateElementPositions(); } // called by ScrollWindow to parent after an element has been removed

		void UpdateColumnCount();

	public:
		ListBox(const C4Rect &rtBounds, int32_t iMultiColItemWidth=0); // ctor
		~ListBox() override; // dtor

		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse movement or buttons

		void RemoveElement(Element *pChild) override; // remove child component
		bool AddElement(Element *pChild, int32_t iIndent=0); // add element and adjust its pos
		bool InsertElement(Element *pChild, Element *pInsertBefore, int32_t iIndent=0); // insert element and adjust its pos
		void ElementSizeChanged(Element *pOfElement) override; // called when an element size is changed
		void ElementPosChanged(Element *pOfElement) override;  // called when an element position is changed

		int32_t GetItemWidth() { return iMultiColItemWidth ? iMultiColItemWidth : pClientWindow->GetClientRect().Wdt; }

		void SelectionChanged(bool fByUser); // pSelectedItem changed: sound, tooltip, etc.
		void SetSelectionChangeCallbackFn(BaseCallbackHandler *pToHandler) // update selection change handler
		{
			if (pSelectionChangeHandler) pSelectionChangeHandler->DeRef();
			if ((pSelectionChangeHandler = pToHandler)) pToHandler->Ref();
		}
		void SetSelectionDblClickFn(BaseCallbackHandler *pToHandler) // update selection doubleclick handler
		{
			if (pSelectionDblClickHandler) pSelectionDblClickHandler->DeRef();
			if ((pSelectionDblClickHandler = pToHandler)) pSelectionDblClickHandler->Ref();
		}

		void ScrollToBottom() // set scrolling to bottom range
		{ if (pClientWindow) pClientWindow->ScrollToBottom(); }
		void ScrollItemInView(Element *pItem); // set scrolling so a specific item is visible
		void FreezeScrolling() { pClientWindow->Freeze(); }
		void UnFreezeScrolling() { pClientWindow->UnFreeze(); }

		// change style
		void SetDecoration(bool fDrawBG, ScrollBarFacets *pToGfx, bool fAutoScroll, bool fDrawBorder=false)
		{ fDrawBackground=fDrawBG; this->fDrawBorder=fDrawBorder; if (pClientWindow) pClientWindow->SetDecoration(pToGfx, fAutoScroll); }
		void SetSelectionDiabled(bool fToVal=true) { fSelectionDisabled = fToVal; }

		// get head and tail list items
		Element *GetFirst() { return pClientWindow ? pClientWindow->GetFirst() : nullptr; }
		Element *GetLast() { return pClientWindow ? pClientWindow->GetLast() : nullptr; }

		// get margins from bounds to client rect
		int32_t GetMarginTop() override { return 3; }
		int32_t GetMarginLeft() override { return 3; }
		int32_t GetMarginRight() override { return 3; }
		int32_t GetMarginBottom() override { return 3; }

		Element *GetSelectedItem() { return pSelectedItem; } // get focused listbox item
		bool IsScrollingActive() { return pClientWindow && pClientWindow->IsScrollingActive(); }
		bool IsScrollingNecessary() { return pClientWindow && pClientWindow->IsScrollingNecessary(); }
		void SelectEntry(Element *pNewSel, bool fByUser);
		void SelectFirstEntry(bool fByUser) { SelectEntry(GetFirst(), fByUser); }
		void SelectNone(bool fByUser) { SelectEntry(nullptr, fByUser); }
		bool IsMultiColumn() const { return iColCount > 1; }
		int32_t ContractToElementHeight(); // make smaller if elements don't use up all of the available height. Return amount by which list box got contracted

		void UpdateElementPositions();            // reposition list items so they are stacked vertically
		void UpdateElementPosition(Element *pOfElement, int32_t iIndent); // update pos for one specific element
		void UpdateSize() override { Control::UpdateSize(); if (pClientWindow) { pClientWindow->UpdateSize(); UpdateElementPositions(); } }

		bool IsSelectedChild(Element *pChild) override { return pChild == pSelectedItem || (pSelectedItem && pSelectedItem->IsParentOf(pChild)); }

		typedef int32_t (*SortFunction)(const Element *pEl1, const Element *pEl2, void *par);
		void SortElements(SortFunction SortFunc, void *par); // sort list items
	};

	// tabbing panel
	class Tabular : public Control
	{
	public:
		// sheet covering the client area of a tabular
		class Sheet : public Window
		{
		protected:
			StdStrBuf sTitle; // sheet label
			int32_t icoTitle; // sheet label icon
			uint32_t cHotkey;
			uint32_t dwCaptionClr; // caption color - default if 0
			bool fHasCloseButton, fCloseButtonHighlighted;
			bool fTitleMarkup;

			Sheet(const char *szTitle, const C4Rect &rcBounds, int32_t icoTitle = Ico_None, bool fHasCloseButton=false, bool fTitleMarkup=true); // ctor; expands hotkey markup in title

			void DrawCaption(C4TargetFacet &cgo, int32_t x, int32_t y, int32_t iMaxWdt, bool fLarge, bool fActive, bool fFocus, C4Facet *pfctClip, C4Facet *pfctIcon, CStdFont *pUseFont);
			void GetCaptionSize(int32_t *piWdt, int32_t *piHgt, bool fLarge, bool fActive, C4Facet *pfctClip, C4Facet *pfctIcon, CStdFont *pUseFont);
			virtual void OnShown(bool fByUser) { } // calklback from tabular after sheet has been made visible
			void SetCloseButtonHighlight(bool fToVal) { fCloseButtonHighlighted = fToVal; }
			bool IsPosOnCloseButton(int32_t x, int32_t y, int32_t iCaptWdt, int32_t iCaptHgt, bool fLarge);
			bool IsActiveSheet();

		public:
			const char *GetTitle() { return sTitle.getData(); }
			char GetHotkey() { return cHotkey; }
			void SetTitle(const char *szNewTitle);
			void SetCaptionColor(uint32_t dwNewClr=0) { dwCaptionClr=dwNewClr; }
			virtual void UserClose() { delete this; } // user pressed close button
			bool HasCloseButton() const { return fHasCloseButton; }

			friend class Tabular;
		};

		enum TabPosition
		{
			tbNone=0, // no tabs
			tbTop,    // tabs on top
			tbLeft  // tabs to the left
		};

	private:
		Sheet *pActiveSheet; // currently selected sheet
		TabPosition eTabPos; // whither tabs shalt be shown or nay, en where art thy shown?
		int32_t iMaxTabWidth; // maximum tab length; used when tabs are positioned left and do not have gfx
		int32_t iSheetSpacing, iSheetOff; // distances of sheet captions
		int32_t iCaptionLengthTotal, iCaptionScrollPos; // scrolling in captions (top only)
		bool fScrollingLeft, fScrollingRight, fScrollingLeftDown, fScrollingRightDown; // scrolling in captions (top only)
		C4TimeMilliseconds tLastScrollTime; // set when fScrollingLeftDown or fScrollingRightDown are true: Time for next scrolling if mouse is held down
		int iSheetMargin;
		bool fDrawSelf; // if border and bg shall be drawn

		C4Facet *pfctBack, *pfctClip, *pfctIcons; // set for tabulars that have custom gfx
		CStdFont *pSheetCaptionFont;                // font to be used for caption drawing; nullptr if default GUI font is to be used

		C4KeyBinding *pKeySelUp, *pKeySelDown, *pKeySelUp2, *pKeySelDown2, *pKeyCloseTab; // key bindings

		void SelectionChanged(bool fByUser); // pActiveSheet changed: sound, tooltip, etc.
		void SheetsChanged(); // update iMaxTabWidth by new set of sheet labels
		void UpdateScrolling();
		void DoCaptionScroll(int32_t iDir);


	private:
		bool HasGfx() { return pfctBack && pfctClip && pfctIcons; } // whether the control uses custom graphics

	protected:
		bool KeySelUp(); // keyboard callback: Select previous sheet
		bool KeySelDown(); // keyboard callback: Select next sheet
		bool KeyCloseTab(); // keyboard callback: Close current sheet if possible

		void DrawElement(C4TargetFacet &cgo) override;
		void MouseLeaveCaptionArea();
		void MouseLeave(CMouse &rMouse) override;
		void OnGetFocus(bool fByMouse) override;

		Control *IsFocusElement() override { return eTabPos ? this : nullptr; }; // this control can gain focus only if tabs are enabled only
		bool IsFocusOnClick() override { return false; } // but never get focus on single mouse click, because this would de-focus any contained controls!

		int32_t GetTopSize() { return (eTabPos == tbTop) ? 20 : 0; } // vertical size of tab selection bar
		int32_t GetLeftSize() { return (eTabPos == tbLeft) ? (HasGfx() ? GetLeftClipSize(pfctClip) : 20+iMaxTabWidth) : 0; } // horizontal size of tab selection bar
		bool HasLargeCaptions() { return eTabPos == tbLeft; }

		int32_t GetMarginTop() override { return iSheetMargin+GetTopSize() + (HasGfx() ? (rcBounds.Hgt-GetTopSize())*30/483 : 0); }
		int32_t GetMarginLeft() override { return iSheetMargin+GetLeftSize() + (HasGfx() ? (rcBounds.Wdt-GetLeftSize())*13/628 : 0); }
		int32_t GetMarginRight() override { return iSheetMargin + (HasGfx() ? (rcBounds.Wdt-GetLeftSize())*30/628 : 0); }
		int32_t GetMarginBottom() override { return iSheetMargin + (HasGfx() ? (rcBounds.Hgt-GetTopSize())*32/483 : 0); }

		void UpdateSize() override;

		bool IsSelectedChild(Element *pChild) override { return pChild == pActiveSheet || (pActiveSheet && pActiveSheet->IsParentOf(pChild)); }

	public:
		Tabular(C4Rect &rtBounds, TabPosition eTabPos); // ctor
		~Tabular() override; // dtor

		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override;

		void RemoveElement(Element *pChild) override; // clear ptr
		Sheet *AddSheet(const char *szTitle, int32_t icoTitle = Ico_None);
		void AddCustomSheet(Sheet *pAddSheet);
		void ClearSheets(); // del all sheets
		void SelectSheet(int32_t iIndex, bool fByUser);
		void SelectSheet(Sheet *pSelSheet, bool fByUser);


		Sheet *GetSheet(int32_t iIndex) { return (Sheet *) GetElementByIndex(iIndex); }
		Sheet *GetActiveSheet() { return pActiveSheet; }
		int32_t GetActiveSheetIndex();
		int32_t GetSheetCount() { return GetElementCount(); }

		void SetGfx(C4Facet *pafctBack, C4Facet *pafctClip, C4Facet *pafctIcons, CStdFont *paSheetCaptionFont, bool fResizeByAspect);
		static int32_t GetLeftClipSize(C4Facet *pfctForClip) { return pfctForClip->Wdt*95/120; } // left clip area size by gfx
		void SetSheetMargin(int32_t iMargin) { iSheetMargin = iMargin; UpdateOwnPos(); }
		void SetDrawDecoration(bool fToVal) { fDrawSelf = fToVal; }

		friend class Sheet;
	};

	// scrollable text box
	class TextWindow : public Control
	{
	protected:
		ScrollWindow *pClientWindow; // client scrolling window
		Picture *pTitlePicture;      // [optional]: Picture shown atop the text
		MultilineLabel *pLogBuffer;  // buffer holding text data
		bool fDrawBackground, fDrawFrame;        // whether dark background should be drawn (default true)
		size_t iPicPadding;

		void DrawElement(C4TargetFacet &cgo) override; // draw text window

		void ElementSizeChanged(Element *pOfElement) override; // called when an element size is changed
		void ElementPosChanged(Element *pOfElement) override;  // called when an element position is changed
		void UpdateSize() override;

		Control *IsFocusElement() override { return nullptr; }; // no focus element for now, because there's nothing to do (2do: scroll?)

	public:
		TextWindow(C4Rect &rtBounds, size_t iPicWdt=0, size_t iPicHgt=0, size_t iPicPadding=0, size_t iMaxLines=100, size_t iMaxTextLen=4096, const char *szIndentChars="    ", bool fAutoGrow=false, const C4Facet *pOverlayPic=nullptr, int iOverlayBorder=0, bool fMarkup=false); // ctor

		void AddTextLine(const char *szText, CStdFont *pFont, DWORD dwClr, bool fDoUpdate, bool fMakeReadableOnBlack, CStdFont *pCaptionFont=nullptr) // add text in a new line
		{ if (pLogBuffer) pLogBuffer->AddLine(szText, pFont, dwClr, fDoUpdate, fMakeReadableOnBlack, pCaptionFont); }
		void ScrollToBottom() // set scrolling to bottom range
		{ if (pClientWindow) pClientWindow->ScrollToBottom(); }
		void ClearText(bool fDoUpdate)
		{ if (pLogBuffer) pLogBuffer->Clear(fDoUpdate); }
		int32_t GetScrollPos()
		{ return pClientWindow ? pClientWindow->GetScrollY() : 0; }
		void SetScrollPos(int32_t iPos)
		{ if (pClientWindow) pClientWindow->ScrollRangeInView(iPos, 0); }
		void UpdateHeight()
		{ if (pLogBuffer) pLogBuffer->UpdateHeight(); }
		void SetDecoration(bool fDrawBG, bool fDrawFrame, ScrollBarFacets *pToGfx, bool fAutoScroll)
		{ fDrawBackground=fDrawBG; this->fDrawFrame=fDrawFrame; if (pClientWindow) pClientWindow->SetDecoration(pToGfx, fAutoScroll); }
		void SetPicture(const C4Facet &rNewPic);

		// get margins from bounds to client rect
		int32_t GetMarginTop() override { return 8; }
		int32_t GetMarginLeft() override { return 10; }
		int32_t GetMarginRight() override { return 5; }
		int32_t GetMarginBottom() override { return 8; }
	};

	// context menu opened on right-click
	class ContextMenu : public Window
	{
	private:
		// key bindings
		class C4KeyBinding *pKeySelUp, *pKeySelDown, *pKeySubmenu, *pKeyBack, *pKeyAbort, *pKeyConfirm, *pKeyHotkey;

		bool KeySelUp();
		bool KeySelDown();
		bool KeySubmenu();
		bool KeyBack();
		bool KeyAbort();
		bool KeyConfirm();
		bool KeyHotkey(const C4KeyCodeEx &key);

	private:
		static int32_t iGlobalMenuIndex;
		int32_t iMenuIndex;

		void UpdateElementPositions(); // stack all menu elements

		// element adding made private: May only add entries
		bool AddElement(Element *pChild);
		bool InsertElement(Element *pChild, Element *pInsertBefore);

	public:
		// one text entry (icon+text+eventually right-arrow)
		class Entry : public Element
		{
		private:
			int32_t GetIconIndent() { return (icoIcon==-1) ? 0 : rcBounds.Hgt+2; }

		protected:
			StdStrBuf sText; // entry text
			uint32_t cHotkey;       // entry hotkey
			Icons icoIcon;      // icon to be drawn left of text
			MenuHandler *pMenuHandler; // callback when item is selected
			ContextHandler *pSubmenuHandler; // callback when submenu is opened

		protected:
			void DrawElement(C4TargetFacet &cgo) override; // draw element

			MenuHandler *GetAndZeroCallback()
			{ MenuHandler *pMH = pMenuHandler; pMenuHandler=nullptr; return pMH; }

			void MouseLeave(CMouse &rMouse) override
			{ if (GetParent()) ((ContextMenu *) GetParent())->MouseLeaveEntry(rMouse, this); }

			bool OnHotkey(uint32_t cKey) override { return cKey==cHotkey; }

			bool IsMenu() override { return true; }

		public:
			Entry(const char *szText, Icons icoIcon=Ico_None, MenuHandler *pMenuHandler=nullptr, ContextHandler *pSubmenuHandler=nullptr); // ctor
			~Entry() override
			{
				if (pMenuHandler) delete pMenuHandler;
				if (pSubmenuHandler) delete pSubmenuHandler;
			}

			const char *GetText() { return sText.getData(); }

			friend class ContextMenu;
		};

	protected:
		Element *pTarget{nullptr}; // target element; close menu if this is lost
		Element *pSelectedItem{nullptr}; // currently highlighted menu item

		ContextMenu *pSubmenu{nullptr}; // currently open submenu

		// mouse movement or buttons forwarded from screen:
		// Check bounds and forward as MouseInput to context menu or submenus
		// return whether inside context menu bounds
		bool CtxMouseInput(CMouse &rMouse, int32_t iButton, int32_t iScreenX, int32_t iScreenY, DWORD dwKeyParam);
		void RemoveElement(Element *pChild) override; // clear ptr - abort menu if target is destroyed

		virtual bool CharIn(const char * c);                                            // input: character key pressed - should return false for none-character-inputs
		void MouseLeaveEntry(CMouse &rMouse, Entry *pOldEntry); // callback: mouse leaves - deselect menu item if no submenu is open (callback done by menu item)

		void DrawElement(C4TargetFacet &cgo) override; // draw BG
		void Draw(C4TargetFacet &cgo) override;        // draw inherited (element+contents) plus submenu

		bool IsMenu() override { return true; }

		Screen *GetScreen() override; // return screen by static var

		int32_t GetMarginTop() override { return 5; }
		int32_t GetMarginLeft() override { return 5; }
		int32_t GetMarginRight() override { return 5; }
		int32_t GetMarginBottom() override { return 5; }

		void SelectionChanged(bool fByUser); // other item selected: Close any submenu; do callbacks

		void ElementSizeChanged(Element *pOfElement) override; // called when an element size is changed
		void ElementPosChanged(Element *pOfElement) override;  // called when an element position is changed

		void CheckOpenSubmenu(); // open submenu if present for selected item
		bool IsSubmenu(); // return whether it's a submenu
		void DoOK();      // do OK for selected item

	public:
		ContextMenu();  // ctor
		~ContextMenu() override; // dtor

		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse movement or buttons

		void Open(Element *pTarget, int32_t iScreenX, int32_t iScreenY);
		void Abort(bool fByUser);

		void AddItem(const char *szText, const char *szToolTip=nullptr, Icons icoIcon=Ico_None, MenuHandler *pMenuHandler=nullptr, ContextHandler *pSubmenuHandler=nullptr)
		{
			Element *pNew = new ContextMenu::Entry(szText, icoIcon, pMenuHandler, pSubmenuHandler);
			AddElement(pNew); pNew->SetToolTip(szToolTip);
		}

		Entry *GetIndexedEntry(int32_t iIndex) { return static_cast<Entry *>(GetElementByIndex(iIndex)); }

		void SelectItem(int32_t iIndex);

		int32_t GetMenuIndex() { return iMenuIndex; }
		static int32_t GetLastMenuIndex() { return iGlobalMenuIndex; }

		Dialog *GetTargetDialog() const { return pTarget ? pTarget->GetDlg() : nullptr; }

		friend class Screen; friend class Entry; friend class Dialog;
	};

	// a button to open a context menu
	class ContextButton : public Control
	{
	private:
		C4KeyBinding *pKeyContext;
		int32_t iOpenMenu; // associated menu (used to flag button down)
		bool fMouseOver;

	public:
		ContextButton(C4Rect &rtBounds); // ctor
		ContextButton(Element *pForEl, bool fAdd, int32_t iHIndent=4, int32_t iVIndent=4);  // ctor creating at topright pos of element
		~ContextButton() override;

	private:
		bool DoContext(int32_t iX=-1, int32_t iY=-1); // open context menu
		bool KeyContext() { return DoContext(); }
		void RegisterContextKey();

	protected:
		void DrawElement(C4TargetFacet &cgo) override;    // draw btn

		bool IsFocusOnClick() override { return false; } // don't select control on click

	public:
		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse. left-click opens menu
		void MouseEnter(CMouse &rMouse) override;
		void MouseLeave(CMouse &rMouse) override;
	};


	// combo box filler
	// should be ComboBox::FillCB; but nested class will cause internal compiler faults
	class ComboBox_FillCB
	{
	private:
		ComboBox *pCombo;
		ContextMenu *pDrop;
	public:
		virtual ~ComboBox_FillCB() = default;
		void FillDropDown(ComboBox *pComboBox, ContextMenu *pDropdownList)
		{ pCombo = pComboBox; pDrop = pDropdownList; FillDropDownCB(); }
		virtual void FillDropDownCB() = 0;
		virtual bool OnComboSelChange(ComboBox *pForCombo, int32_t idNewSelection) = 0;

		// to be used in fill-callback only (crash otherwise!)
		void AddEntry(const char *szText, int32_t id);
		bool FindEntry(const char *szText);
		void ClearEntries();
		void SelectEntry(int32_t iEntry); // select entry by index
	};

	template <class CB> class ComboBox_FillCallback : public ComboBox_FillCB
	{
	public:
		typedef void (CB::*ComboFillFunc)(ComboBox_FillCB *pFiller);
		typedef bool (CB::*ComboSelFunc)(ComboBox *pForCombo, int32_t idNewSelection);

	private:
		CB *pCBClass;
		ComboFillFunc FillFunc;
		ComboSelFunc SelFunc;
	protected:
		void FillDropDownCB() override
		{ if (pCBClass && FillFunc) (pCBClass->*FillFunc)(this); }
		bool OnComboSelChange(ComboBox *pForCombo, int32_t idNewSelection) override
		{ if (pCBClass && SelFunc) return (pCBClass->*SelFunc)(pForCombo, idNewSelection); else return false; }

	public:
		ComboBox_FillCallback(CB * pCBClass, ComboFillFunc FillFunc, ComboSelFunc SelFunc) :
				pCBClass(pCBClass), FillFunc(FillFunc), SelFunc(SelFunc) { }
	};


	// dropdown box to select elements from a list
	class ComboBox : public Control
	{
	public:
		struct ComboMenuCBStruct // struct used as menu callback parameter for dropdown menu
		{
			StdCopyStrBuf sText;
			int32_t id{0};

			ComboMenuCBStruct() : sText() {}
			ComboMenuCBStruct(const char *szText, int32_t id) : sText(szText), id(id) {}
		};
	private:
		class C4KeyBinding *pKeyOpenCombo, *pKeyCloseCombo;
	private:
		int32_t iOpenMenu; // associated menu (used to flag dropped down)
		ComboBox_FillCB *pFillCallback; // callback used to display the dropdown
		char Text[C4MaxTitle+1]; // currently selected item
		bool fReadOnly;  // merely a label in ReadOnly-mode
		bool fSimple;    // combo without border and stuff
		bool fMouseOver; // mouse hovering over this?
		CStdFont *pUseFont;                       // font used to draw this control
		uint32_t dwFontClr, dwBGClr, dwBorderClr; // colors used to draw this control
		C4Facet *pFctSideArrow;                   // arrow gfx used to start combo-dropdown

	private:
		bool DoDropdown(); // open dropdown menu (context menu)
		bool KeyDropDown() { return DoDropdown(); }
		bool KeyAbortDropDown() { return AbortDropdown(true); }
		bool AbortDropdown(bool fByUser); // abort dropdown menu, if it's open

	protected:
		void DrawElement(C4TargetFacet &cgo) override;    // draw combo box

		bool IsFocusOnClick() override { return false; } // don't select control on click
		Control *IsFocusElement() override { return fReadOnly ? nullptr : this; }; // this control can gain focus if not readonly

		void OnCtxComboSelect(C4GUI::Element *pListItem, const ComboMenuCBStruct &rNewSel);

	public:
		ComboBox(const C4Rect &rtBounds);
		~ComboBox() override;

		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse. left-click opens menu
		void MouseEnter(CMouse &rMouse) override; // called when mouse cursor enters element region
		void MouseLeave(CMouse &rMouse) override; // called when mouse cursor leaves element region

		void SetComboCB(ComboBox_FillCB *pNewFillCallback);
		static int32_t GetDefaultHeight();
		void SetText(const char *szToText);
		void SetReadOnly(bool fToVal) { if ((fReadOnly = fToVal)) AbortDropdown(false); }
		void SetSimple(bool fToVal) { fSimple = fToVal; }
		const StdStrBuf GetText() { return StdStrBuf(Text, false); }
		void SetFont(CStdFont *pToFont) { pUseFont=pToFont; }
		void SetColors(uint32_t dwFontClr, uint32_t dwBGClr, uint32_t dwBorderClr)
		{ this->dwFontClr=dwFontClr; this->dwBGClr=dwBGClr; this->dwBorderClr=dwBorderClr; }
		void SetDecoration(C4Facet *pFctSideArrow)
		{ this->pFctSideArrow = pFctSideArrow; }

		friend class ComboBox_FillCB;
	};

	class Dialog;

	// EM window class
	class DialogWindow : public C4Window
	{
	public:
		Dialog* pDialog{nullptr};
		DialogWindow(): C4Window() {}
		using C4Window::Init;
		C4Window * Init(C4AbstractApp * pApp, const char * Title, const C4Rect &rcBounds, const char *szID);
		void Close() override;
		void PerformUpdate() override;
	};

	// information on how to draw dialog borders and face
	class FrameDecoration
	{
	private:
		int iRefCount{0};

		bool SetFacetByAction(C4Def *pOfDef, class C4TargetFacet &rfctTarget, const char *szFacetName);

	public:
		C4Def *pSourceDef;
		C4ID idSourceDef;
		uint32_t dwBackClr; // background face color
		C4TargetFacet fctTop, fctTopRight, fctRight, fctBottomRight, fctBottom, fctBottomLeft, fctLeft, fctTopLeft;
		int iBorderTop, iBorderLeft, iBorderRight, iBorderBottom;
		bool fHasGfxOutsideClientArea;

		FrameDecoration()  { Clear(); }
		void Clear(); // zero data

		// create from ActMap and graphics of a definition (does some script callbacks to get parameters)
		bool SetByDef(C4Def *pSrcDef);
		bool SetByDef(C4ID idSourceDef); // a wrapper for the above method
		bool UpdateGfx(); // update Surface, e.g. after def reload

		void Ref() { ++iRefCount; }
		void Deref() { if (!--iRefCount) delete this; }

		void Draw(C4TargetFacet &cgo, C4Rect &rcDrawArea); // draw deco for given rect (rect includes border)
	};

	// a dialog
	class Dialog: public Window
	{
	private:
		enum Fade { eFadeNone=0, eFadeOut, eFadeIn };

		C4KeyBinding *pKeyAdvanceControl, *pKeyAdvanceControlB, *pKeyHotkey, *pKeyEnter, *pKeyEscape, *pKeyFocusDefControl;
	protected:
		WoodenLabel *pTitle; // title bar text
		CallbackButton<Dialog, C4GUI::IconButton> *pCloseBtn;
		Control *pActiveCtrl; // control that has focus
		bool fShow;    // if set, the dlg is shown
		bool fOK;      // if set, the user pressed OK
		int32_t iFade;     // dlg fade (percent)
		Fade eFade;    // fading mode
		bool fDelOnClose; // auto-delete when closing
		StdStrBuf TitleString;
		bool fViewportDlg; // set in ctor: if true, dlg is not independant, but drawn ad controlled within viewports
		DialogWindow *pWindow;  // window in console mode
		FrameDecoration *pFrameDeco;

		bool CreateConsoleWindow();
		void DestroyConsoleWindow();

		void UpdateSize() override;                // called when own size changed - update assigned pWindow
		void UpdatePos() override; // Dialogs with their own windows can only be at 0/0

	public:
		Dialog(int32_t iWdt, int32_t iHgt, const char *szTitle, bool fViewportDlg); // ctor
		~Dialog() override; // dtor

		void RemoveElement(Element *pChild) override; // clear ptr

		void Draw(C4TargetFacet &cgo) override; // render dialog (published)
		void DrawElement(C4TargetFacet &cgo) override; // draw dlg bg
		bool IsComponentOutsideClientArea() override { return !!pTitle; } // pTitle lies outside client area

		virtual const char *GetID() { return nullptr; }

		// special handling for viewport dialogs
		void ApplyElementOffset(int32_t &riX, int32_t &riY) override;
		void ApplyInvElementOffset(int32_t &riX, int32_t &riY) override;

		bool IsFocused(Control *pCtrl) override { return pCtrl == pActiveCtrl; }
		void SetFocus(Control *pCtrl, bool fByMouse);
		Control *GetFocus() { return pActiveCtrl; }
		Dialog *GetDlg() override { return this; } // this is the dialog
		DialogWindow* GetDialogWindow() override { return pWindow; }

		virtual bool CharIn(const char * c);                                 // input: character key pressed - should return false for none-character-inputs  (forward to focused control)

	private:
		bool KeyHotkey(const C4KeyCodeEx &key);
		bool KeyFocusDefault();

	public:
		void MouseInput(CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override; // input: mouse. forwards to child controls

		// default control to be set if unprocessed keyboard input has been detected
		virtual class Control *GetDefaultControl() { return nullptr; }

		// default dlg actions for enter/escape
		virtual bool OnEnter() { UserClose(true); return true; }
		bool KeyEnter() { return OnEnter(); }
		virtual bool OnEscape() { UserClose(false); return true; }
		bool KeyEscape() { return OnEscape(); }

		void AdvanceFocus(bool fBackwards); // change focus to next component
		bool KeyAdvanceFocus(bool fBackwards) { AdvanceFocus(fBackwards); return true; }

		int32_t GetMarginTop() override { return (pTitle ? pTitle->GetBounds().Hgt : 0) + (pFrameDeco ? pFrameDeco->iBorderTop : Window::GetMarginTop()); }
		int32_t GetMarginLeft() override { return pFrameDeco ? pFrameDeco->iBorderLeft : Window::GetMarginLeft(); }
		int32_t GetMarginRight() override { return pFrameDeco ? pFrameDeco->iBorderRight: Window::GetMarginRight(); }
		int32_t GetMarginBottom() override { return pFrameDeco ? pFrameDeco->iBorderBottom: Window::GetMarginBottom(); }

		static int32_t GetDefaultTitleHeight();

		bool IsShown() { return fShow; } // returns whether dlg is on screen (may be invisible)
		bool IsOK() { return fOK; }      // returns whether user pressed OK
		bool IsAborted() { return !fShow && !fOK; } // returns whether dialog has been aborted
		bool IsActive(bool fForKeyboard); // return whether dlg has mouse control focus
		bool IsFading() { return eFade != eFadeNone; }

		virtual bool IsFullscreenDialog() { return false; }
		virtual bool HasBackground() { return false; } // true if dlg draws screen background (fullscreen dialogs only)

		// true for dialogs that should span the whole screen
		// not just the mouse-viewport
		virtual bool IsFreePlaceDialog() { return false; }

		// true for dialogs that should be placed at the bottom of the screen (chat)
		virtual bool IsBottomPlacementDialog() { return false; }

		// true for dialogs that receive full keyboard and mouse input even in shared mode
		virtual bool IsExclusiveDialog() { return false; }

		// some dialogs, like menus or chat control, don't really need a mouse
		// so do not enable it for those in shared mode if mouse control is disabled
		virtual bool IsMouseControlled() { return true; }

		// For dialogs associated to a viewport: Return viewport (for placement)
		virtual C4Viewport *GetViewport() { return nullptr; }
		bool IsViewportDialog() { return fViewportDlg; }

		// for custom placement procedures; should call SetPos
		virtual bool DoPlacement(Screen *pOnScreen, const C4Rect &rPreferredDlgRect)  { return false; }

		// true for dialogs drawn externally
		bool IsExternalDrawDialog() override { return false; }

		// z-ordering used for dialog placement
		virtual int32_t GetZOrdering() { return C4GUI_Z_DEFAULT; }

		bool Show(Screen *pOnScreen, bool fCB); // show dialog on screen - default to last created screen
		void Close(bool fOK);         // close dlg
		bool FadeIn(Screen *pOnScreen); // fade dlg into screen
		void FadeOut(bool fCloseWithOK); // fade out dlg
		bool DoModal();               // execute message loop until dlg is closed (or GUI destructed) - returns whether dlg was OK
		bool Execute();               // execute dialog - does message handling, gfx output and idle proc; return false if dlg got closed or GUI deleted
		bool Execute2();              // execute dialog - does message handling, gfx output and idle proc; return false and deletes self if dlg got closed or GUI deleted
		void SetDelOnClose(bool fToVal=true) { fDelOnClose=fToVal; } // dialog will delete itself when closed

		void SetTitle(const char *szToTitle, bool fShowCloseButton = true); // change title text; creates or removes title bar if necessary
		void SetFrameDeco(FrameDecoration *pNewDeco) // change border decoration
		{
			if (pFrameDeco) pFrameDeco->Deref();
			if ((pFrameDeco = pNewDeco)) pNewDeco->Ref();
			UpdateOwnPos(); // margin may have changed; might need to reposition stuff
		}
		void ClearFrameDeco() // clear border decoration; no own pos update!
		{if (pFrameDeco) pFrameDeco->Deref(); pFrameDeco = nullptr; }
		FrameDecoration *GetFrameDecoration() const { return pFrameDeco; }
		void SetClientSize(int32_t iToWdt, int32_t iToHgt); // resize dialog so its client area has the specified size

		void OnUserClose(C4GUI::Control *btn) // user presses close btn: Usually close dlg with abort
		{ UserClose(false); }
		virtual void UserClose(bool fOK) { Close(fOK); }
		virtual void OnClosed(bool fOK);    // callback when dlg got closed
		virtual void OnShown() {}           // callback when shown - should not delete the dialog
		virtual void OnIdle() {}            // idle proc in DoModal

		ContextHandler *GetContextHandler() override // always use own context handler only (no fall-through to screen)
		{ return pContextHandler; }
#ifdef _WIN32
		static bool RegisterWindowClass(HINSTANCE hInst); // registers WNDCLASS for console mode dialogs
#endif
		friend class Screen;
	};

	// a dialog covering the whole screen (using UpperBoard-caption)
	class FullscreenDialog : public Dialog
	{
	protected:
		Label *pFullscreenTitle, *pSubTitle; // subtitle to be put in upper-right corner
		int32_t iDlgMarginX, iDlgMarginY; // dialog margin set by screen size

		const char *GetID() override { return nullptr; } // no ID needed, because it's never created as a window

	public:
		FullscreenDialog(const char *szTitle, const char *szSubtitle); // ctor

		void SetTitle(const char *szToTitle); // change title text; creates or removes title bar if necessary

	protected:
		void DrawElement(C4TargetFacet &cgo) override; // draw dlg bg

		// fullscreen dialogs are not closed on Enter
		bool OnEnter() override { return false; }

		bool IsComponentOutsideClientArea() override { return true; }

		virtual bool HasUpperBoard() { return false; } // standard fullscreen dialog: UpperBoard no longer present

		bool IsFullscreenDialog() override { return true; }
		bool DoPlacement(Screen *pOnScreen, const C4Rect &rPreferredDlgRect) override  { return true; } // fullscreen dlg already placed

		int32_t GetMarginTop() override { return (HasUpperBoard() ? C4UpperBoardHeight : C4GUI_FullscreenDlg_TitleHeight) + iDlgMarginY; }
		int32_t GetMarginLeft() override { return iDlgMarginX; }
		int32_t GetMarginRight() override { return iDlgMarginX; }
		int32_t GetMarginBottom() override { return iDlgMarginY; }
		void UpdateOwnPos() override; // called when element bounds were changed externally

		// helper func: draw facet to screen background
		void DrawBackground(C4TargetFacet &cgo, C4Facet &rFromFct);
	};

	// a button closing the Dlg
	class CloseButton : public Button
	{
	protected:
		bool fCloseResult;

		void OnPress() override
		{ Dialog *pDlg; if ((pDlg = GetDlg())) pDlg->UserClose(fCloseResult); }

	public:
		CloseButton(const char *szBtnText, const C4Rect &rtBounds, bool fResult) // ctor
				: Button(szBtnText, rtBounds), fCloseResult(fResult) { }
	};
	class CloseIconButton : public IconButton
	{
	protected:
		bool fCloseResult;

		void OnPress() override
		{ Dialog *pDlg; if ((pDlg = GetDlg())) pDlg->UserClose(fCloseResult); }

	public:
		CloseIconButton(const C4Rect &rtBounds, Icons eIcon, bool fResult) // ctor
				: IconButton(eIcon, rtBounds, 0), fCloseResult(fResult) { }
	};

	// OK button
	class OKButton : public CloseButton
	{
	public: OKButton(const C4Rect &rtBounds) // ctor
				: CloseButton(LoadResStr("IDS_DLG_OK"), rtBounds, true) {} };
	class OKIconButton : public CloseIconButton
	{
	public: OKIconButton(const C4Rect &rtBounds, Icons eIcon) // ctor
				: CloseIconButton(rtBounds, eIcon, true) {} };
	// cancel button
	class CancelButton : public CloseButton
	{
	public: CancelButton(const C4Rect &rtBounds) // ctor
				: CloseButton(LoadResStr("IDS_DLG_CANCEL"), rtBounds, false) {} };
	class CancelIconButton : public CloseIconButton
	{
	public: CancelIconButton(const C4Rect &rtBounds, Icons eIcon) // ctor
				: CloseIconButton(rtBounds, eIcon, false) {} };
	// regular close button
	class DlgCloseButton : public CloseButton
	{
	public: DlgCloseButton(const C4Rect &rtBounds) // ctor
				: CloseButton(LoadResStr("IDS_DLG_CLOSE"), rtBounds, true) {} };
	// Yes button
	class YesButton : public CloseButton
	{
	public: YesButton(const C4Rect &rtBounds) // ctor
				: CloseButton(LoadResStr("IDS_DLG_YES"), rtBounds, true) {} };
	// No button
	class NoButton : public CloseButton
	{
	public: NoButton(const C4Rect &rtBounds) // ctor
				: CloseButton(LoadResStr("IDS_DLG_NO"), rtBounds, false) {} };
	// Retry button
	class RetryButton : public CloseButton
	{
	public: RetryButton(const C4Rect &rtBounds) // ctor
				: CloseButton(LoadResStr("IDS_BTN_RETRY"), rtBounds, true) {} };
	// Reset button
	class ResetButton : public CloseButton
	{
	public: ResetButton(const C4Rect &rtBounds) // ctor
				: CloseButton(LoadResStr("IDS_BTN_RESET"), rtBounds, true) {} };

	// a simple message dialog
	class MessageDialog : public Dialog
	{
	private:
		bool fHasOK;
		int32_t *piConfigDontShowAgainSetting;
		class C4KeyBinding *pKeyCopy;
		std::string sCopyText; // text that goes into clipboard if user presses Ctrl+C on this window
	public:
		enum Buttons { btnOK=1, btnAbort=2, btnYes=4, btnNo=8, btnRetry=16, btnReset=32,
		               btnOKAbort=btnOK|btnAbort, btnYesNo=btnYes|btnNo, btnRetryAbort=btnRetry|btnAbort
		             };
		enum DlgSize { dsRegular=C4GUI_MessageDlgWdt, dsMedium=C4GUI_MessageDlgWdtMedium, dsSmall=C4GUI_MessageDlgWdtSmall };
		MessageDialog(const char *szMessage, const char *szCaption, DWORD dwButtons, Icons icoIcon, DlgSize eSize=dsRegular, int32_t *piConfigDontShowAgainSetting=nullptr, bool fDefaultNo=false);
		~MessageDialog() override;

	protected:
		bool OnEnter() override { if (!fHasOK) return false; Close(true); return true; }
		void OnDontShowAgainCheck(C4GUI::Element *pCheckBox)
		{ if (piConfigDontShowAgainSetting) *piConfigDontShowAgainSetting = static_cast<C4GUI::CheckBox *>(pCheckBox)->GetChecked(); }
		bool KeyCopy();

		const char *GetID() override { return "MessageDialog"; }
		int32_t GetZOrdering() override { return C4GUI_Z_INPUT; }
	};

	// a confirmation dialog, which performs a callback after confirmation
	class ConfirmationDialog : public MessageDialog
	{
	private:
		BaseCallbackHandler *pCB;

	public:
		ConfirmationDialog(const char *szMessage, const char *szCaption, BaseCallbackHandler *pCB, DWORD dwButtons=MessageDialog::btnOKAbort, bool fSmall=false, Icons icoIcon=Ico_Confirm);
		~ConfirmationDialog() override { if (pCB) pCB->DeRef(); }

	protected:
		void OnClosed(bool fOK) override;    // callback when dlg got closed
	};

	// a simple progress dialog
	class ProgressDialog : public Dialog
	{
	protected:
		ProgressBar *pBar; // progress bar component
		Label *pLblMessage; // message label

		const char *GetID() override { return "ProgressDialog"; }

	public:
		ProgressDialog(const char *szMessage, const char *szCaption, int32_t iMaxProgress, int32_t iInitialProgress, Icons icoIcon);

		void SetProgress(int32_t iToProgress) { pBar->SetProgress(iToProgress); } // change progress
		bool OnEnter() override { return false; } // don't close on Enter!
		void SetMessage(const char *szMessage);
	};


	class BaseInputCallback
	{
	public:
		virtual void OnOK(const StdStrBuf &sText) = 0;
		virtual ~BaseInputCallback() = default;
	};

	template <class T> class InputCallback : public BaseInputCallback
	{
	public:
		typedef void (T::*CBFunc)(const StdStrBuf &);
	private:
		T *pTarget;
		CBFunc pCBFunc;
	public:
		InputCallback(T *pTarget, CBFunc pCBFunc) : pTarget(pTarget), pCBFunc(pCBFunc) {}

		void OnOK(const StdStrBuf &sText) override { (pTarget->*pCBFunc)(sText); }
	};

	// a dialog for a one-line text input; contains OK and cancel button
	class InputDialog : public Dialog
	{
	protected:
		Edit *pEdit; // edit for text input
		BaseInputCallback *pCB;
		C4Rect rcEditBounds;
		bool fChatLayout;
		Label *pChatLbl;

		const char *GetID() override { return "InputDialog"; }

	public:
		InputDialog(const char *szMessage, const char *szCaption, Icons icoIcon, BaseInputCallback *pCB, bool fChatLayout=false);
		~InputDialog() override { if (pCB) delete pCB; }

		void OnClosed(bool fOK) override { if (pCB && fOK) pCB->OnOK(StdStrBuf(pEdit->GetText())); Dialog::OnClosed(fOK); } // close CB
		void SetMaxText(int32_t iMaxLen) { pEdit->SetMaxText(iMaxLen); }
		void SetInputText(const char *szToText);
		const char *GetInputText() { return pEdit->GetText(); }
		void SetCustomEdit(Edit *pCustomEdit);
	};

	// a dialog showing some information text
	class InfoDialog : public Dialog, private C4ApplicationSec1Timer
	{
	private:
		int32_t iScroll;      // current scroll pos; backup for text update

	protected:
		TextWindow *pTextWin;

		void CreateSubComponents(); // ctor func

		// add text to the info window
		void AddLine(const char *szText);
		void AddLineFmt(const char *szFmtString, ...) GNUC_FORMAT_ATTRIBUTE_O;

		void BeginUpdateText(); // backup scrolling and clear text window
		void EndUpdateText();   // restore scroll pos; set last update time

		virtual void UpdateText() {}; // function to be overwritten for timed dlgs: Update window text

		void OnSec1Timer() override; // idle proc: update text if necessary
	public:
		InfoDialog(const char *szCaption, int32_t iLineCount); // ctor
		InfoDialog(const char *szCaption, int iLineCount, const StdStrBuf &sText); // ctor - init w/o timer
		~InfoDialog() override;
	};

	// a keyboard event that's only executed if the dialog is activated
	template <class TargetClass> class DlgKeyCB : public C4KeyCB<TargetClass>
	{
	private:
		typedef C4KeyCB<TargetClass> Base;
		typedef bool(TargetClass::*CallbackFunc)();
	public:
		DlgKeyCB(TargetClass &rTarget, CallbackFunc pFuncDown, CallbackFunc pFuncUp=nullptr, CallbackFunc pFuncPressed=nullptr)
				: Base(rTarget, pFuncDown, pFuncUp, pFuncPressed) {}

		bool CheckCondition() override { return Base::rTarget.IsInActiveDlg(true) && Base::rTarget.IsVisible(); }
	};

	template <class TargetClass> class DlgKeyCBPassKey : public C4KeyCBPassKey<TargetClass>
	{
	private:
		typedef C4KeyCBPassKey<TargetClass> Base;
		typedef bool(TargetClass::*CallbackFunc)(const C4KeyCodeEx &key);
	public:
		DlgKeyCBPassKey(TargetClass &rTarget, CallbackFunc pFuncDown, CallbackFunc pFuncUp=nullptr, CallbackFunc pFuncPressed=nullptr)
				: Base(rTarget, pFuncDown, pFuncUp, pFuncPressed) {}

		bool CheckCondition() override { return Base::rTarget.IsInActiveDlg(true) && Base::rTarget.IsVisible(); }
	};

	template <class TargetClass, class ParameterType> class DlgKeyCBEx : public C4KeyCBEx<TargetClass, ParameterType>
	{
	private:
		typedef C4KeyCBEx<TargetClass, ParameterType> Base;
		typedef bool(TargetClass::*CallbackFunc)(ParameterType par);
	public:
		DlgKeyCBEx(TargetClass &rTarget, const ParameterType &par, CallbackFunc pFuncDown, CallbackFunc pFuncUp=nullptr, CallbackFunc pFuncPressed=nullptr)
				: Base(rTarget, par, pFuncDown, pFuncUp, pFuncPressed) {}

		bool CheckCondition() override { return Base::rTarget.IsInActiveDlg(true) && Base::rTarget.IsVisible(); }
	};

	// a keyboard event that's only executed if the control has focus
	template <class TargetClass> class ControlKeyCB : public C4KeyCB<TargetClass>
	{
	private:
		typedef C4KeyCB<TargetClass> Base;
		typedef bool(TargetClass::*CallbackFunc)();
	public:
		ControlKeyCB(TargetClass &rTarget, CallbackFunc pFuncDown, CallbackFunc pFuncUp=nullptr, CallbackFunc pFuncPressed=nullptr)
				: Base(rTarget, pFuncDown, pFuncUp, pFuncPressed) {}

		bool CheckCondition() override { return Base::rTarget.HasDrawFocus(); }
	};

	template <class TargetClass, class ParameterType> class ControlKeyCBExPassKey : public C4KeyCBExPassKey<TargetClass, ParameterType>
	{
	private:
		typedef C4KeyCBExPassKey<TargetClass, ParameterType> Base;
		typedef bool(TargetClass::*CallbackFunc)(const C4KeyCodeEx &key, const ParameterType &par);
	public:
		ControlKeyCBExPassKey(TargetClass &rTarget, const ParameterType &rPar, CallbackFunc pFuncDown, CallbackFunc pFuncUp=nullptr, CallbackFunc pFuncPressed=nullptr)
				: Base(rTarget, rPar, pFuncDown, pFuncUp, pFuncPressed) {}

		bool CheckCondition() override { return Base::rTarget.HasDrawFocus(); }
	};

	// key event that checks whether a control is active, but forwards to the dialog
	template <class TargetClass> class ControlKeyDlgCB : public C4KeyCB<TargetClass>
	{
	private:
		class Control *pCtrl;
		typedef C4KeyCB<TargetClass> Base;
		typedef bool(TargetClass::*CallbackFunc)();
	public:
		ControlKeyDlgCB(Control *pCtrl, TargetClass &rTarget, CallbackFunc pFuncDown, CallbackFunc pFuncUp=nullptr, CallbackFunc pFuncPressed=nullptr)
				: Base(rTarget, pFuncDown, pFuncUp, pFuncPressed), pCtrl(pCtrl) {}

		bool CheckCondition() override { return pCtrl && pCtrl->IsInActiveDlg(true) && pCtrl->IsVisible(); }
	};

	// mouse cursor control
	class CMouse
	{
	public:
		int32_t x,y; // cursor position
		bool LDown, MDown, RDown; // mouse button states
		int32_t LDownX, LDownY;       // position where left button was pressed last
		DWORD dwKeys;             // shift, ctrl, etc.
		bool fActive;
		C4TimeMilliseconds tLastMovementTime; // C4TimeMilliseconds::Now() when the mouse pos changed last

		// whether last input was done by mouse
		// set to true whenever mouse pos changes or buttons are pressed
		// reset by keyboard actions to avoid tooltips where the user isn't even doing anything
		bool fActiveInput;

		enum TooltipShowState
		{
			TTST_None = 0, // show no tooltips
			TTST_Immediate = 1, // show only tooltips of elements that require immediate show (i.e. otherwise unlabeled buttons)
			TTST_All = 2, // show all tooltips (mouse hovered on same element for some time)
		};

	public:
		Element *pMouseOverElement, *pPrevMouseOverElement; // elements at mouse position (after and before processing of mouse event)
		Element *pDragElement;                              // element at pos where left mouse button was clicked last

	public:
		CMouse(int32_t iX, int32_t iY);  // ctor
		~CMouse(); // dtor

		void Draw(C4TargetFacet &cgo, TooltipShowState draw_tool_tips); // draw cursor

		void Input(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam); // process mouse input
		bool IsLDown() { return LDown; }

		void GetLastXY(int32_t &rX, int32_t &rY, DWORD &rdwKeys) { rX=x; rY=y; rdwKeys=dwKeys; }

		void ResetElements() // reset MouseOver/etc.-controls
		{ pMouseOverElement=pPrevMouseOverElement=pDragElement=nullptr; }
		void ReleaseElements(); // reset MouseOver/etc.-controls, doing the appropriate callbacks
		void OnElementGetsInvisible(Element *pChild); // clear ptr

		void RemoveElement(Element *pChild); // clear ptr

		void SetOwnedMouse(bool fToVal) { fActive=fToVal; }

		void ResetToolTipTime() { tLastMovementTime = C4TimeMilliseconds::Now(); }
		bool IsMouseStill() { return C4TimeMilliseconds::Now()-tLastMovementTime >= C4GUI_ToolTipShowTime; }
		void ResetActiveInput() { fActiveInput = false; }
		bool IsActiveInput() { return fActiveInput; }

		void ReleaseButtons() { LDown = RDown = MDown = false; }
	};

	// the dialog client area
	class Screen : public Window
	{
	public:
		CMouse Mouse;

	protected:
		Dialog *pActiveDlg;    // current, active dialog
		ContextMenu *pContext{nullptr}; // currently opened context menu (lowest submenu)
		bool fExclusive{true};       // default true. if false, input is shared with the game
		C4Rect PreferredDlgRect; // rectangle in which dialogs should be placed
		float fZoom{1.0f};

		static Screen *pScreen; // static singleton var

	public:
		void RemoveElement(Element *pChild) override; // clear ptr
		const C4Rect &GetPreferredDlgRect() { return PreferredDlgRect; }

	protected:
		using Window::Draw;
		virtual void Draw(C4TargetFacet &cgo, bool fDoBG); // draw screen contents

		void ElementPosChanged(Element *pOfElement) override; // called when a dialog is moved

	public:
		void ShowDialog(Dialog *pDlg, bool fFade); // present dialog to user
		void CloseDialog(Dialog *pDlg, bool fFade);// hide dialog from user
		void ActivateDialog(Dialog *pDlg);         // set dlg as active
		void RecheckActiveDialog();

		Dialog *GetTopDialog(); // get topmost dlg
	public:
		Screen();
		~Screen() override;

		void Init(int32_t tx, int32_t ty, int32_t twdt, int32_t thgt);
		void Clear();

		void Render(bool fDoBG);                 // render to pDraw
		void RenderMouse(C4TargetFacet &cgo);        // draw mouse only

		Screen *GetScreen() override { return this; }; // return contained screen
		static Screen *GetScreenS() { return pScreen; } // get global screen

		float GetZoom() const { return fZoom; }; // get GUI zoom

		bool IsActive() { return !!GetTopDialog(); } // return whether GUI is active

		bool KeyAny(); // to be called on keystrokes; resets some tooltip-times
		virtual bool CharIn(const char * c);        // input: character key pressed - should return false for none-character-inputs
		using Window::MouseInput;
		bool MouseInput(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam, Dialog *pDlg, class C4Viewport *pVP); // input: mouse movement or buttons; sends MouseEnter/Leave; return whether inside dialog
		void MouseMove(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam, class C4Viewport *pVP); // pVP specified for console mode viewports only
		void SetMouseInGUI(bool fInGUI, bool fByMouse);
		bool RecheckMouseInput();                                       // do mouse movement iusing last input flags

		bool ShowMessage(const char *szMessage, const char *szCaption, Icons icoIcon, int32_t *piConfigDontShowAgainSetting=nullptr); // show message
		bool ShowErrorMessage(const char *szMessage); // show message: Error caption and icon
		bool ShowMessageModal(const char *szMessage, const char *szCaption, DWORD dwButtons, Icons icoIcon, int32_t *piConfigDontShowAgainSetting=nullptr); // show modal message dlg
		ProgressDialog *ShowProgressDlg(const char *szMessage, const char *szCaption, int32_t iMaxProgress=100, int32_t iInitialProgress=0, Icons icoIcon=Ico_Wait); // create and show a progress dialog
		bool ShowModalDlg(Dialog *pDlg, bool fDestruct=true); // show any dialog modal and destruct it afterwards
		bool ShowRemoveDlg(Dialog *pDlg); // show dialog, and flag it to delete itself when closed; return immediately

		void CloseAllDialogs(bool fWithOK); // close all dialogs on the screen; top dlgs first
		void SetPreferredDlgRect(const C4Rect &rtNewPref) { PreferredDlgRect = rtNewPref; }
#ifdef USE_WIN32_WINDOWS
		Dialog *GetDialog(HWND hWindow); // get console dialog
#endif
		Dialog *GetDialog(C4Window * pWindow); // get console dialog
		void DoContext(ContextMenu *pNewCtx, Element *pAtElement, int32_t iX, int32_t iY); // open context menu (closes any other contextmenu)
		void AbortContext(bool fByUser) { if (pContext) pContext->Abort(fByUser); } // close context menu
		int32_t GetContextMenuIndex() { return pContext ? pContext->GetMenuIndex() : 0; } // get current context-menu (lowest level)
		int32_t GetLastContextMenuIndex() { return ContextMenu::GetLastMenuIndex(); } // get last opened context-menu (lowest level)
		bool HasContext() { return !!pContext; }

		bool HasFullscreenDialog(bool fIncludeFading); // true if on fullscreen dialog is visible
		Dialog *GetFullscreenDialog(bool fIncludeFading); // return dlg pointer to first found fullscreen dialog

		// callback from C4Game from message-handling of WM_TIMER; forwards to dialog-timer
		void OnSec1Timer();

		// exclusive
		void SetExclusive(bool fToState) { fExclusive = fToState; Mouse.SetOwnedMouse(fExclusive); UpdateMouseFocus(); }
		bool IsExclusive() { return fExclusive; }
		bool HasKeyboardFocus()
		{
			// always hook keyboard in exclusive mode; only on exclusive top dialogs in shared mode
			if (IsExclusive()) return true;
			Dialog *pDlg = GetTopDialog();
			return pDlg && pDlg->IsExclusiveDialog();
		}
		bool HasMouseFocus() // return whether mouse controls GUI only
		{ return HasKeyboardFocus(); }
		int32_t GetMouseControlledDialogCount(); // count dlgs with that flag set
		void UpdateMouseFocus(); // when exclusive mode has changed: Make sure mouse clip is correct

		// draw a tooltip in a box
		static void DrawToolTip(const char *szTip, C4TargetFacet &cgo, float guix, float guiy);

		// gamepad
		void UpdateGamepadGUIControlEnabled(); // callback when gamepad gui option changed

		friend class Dialog; friend class ContextMenu;
	};

	// menu handler bound to member functions of another class
	template <class CBClass> class CBMenuHandler : public MenuHandler
	{
	private:
		CBClass *pCBTarget; // callback target
		typename DlgCallback<CBClass>::ContextClickFunc pCallbackFn; // callback function

	public:
		// ctor
		CBMenuHandler<CBClass>(CBClass *pCBTarget, typename DlgCallback<CBClass>::ContextClickFunc pCallbackFn, int32_t iaExtra=0)
				: MenuHandler(), pCBTarget(pCBTarget), pCallbackFn(pCallbackFn) { }

		void OnOK(Element *pTargetElement) override
		{
			if (!pCBTarget /* || !pCallbackFn */) return;
			// do callback
			(pCBTarget->*pCallbackFn)(pTargetElement);
		}
	};

	// menu handler bound to member functions of another class, providing extra storage for parameters
	template <class CBClass, class TEx> class CBMenuHandlerEx : public MenuHandler
	{
	private:
		CBClass *pCBTarget; // callback target
		typename DlgCallbackEx<CBClass, const TEx &>::ContextClickFunc pCallbackFn; // callback function
		TEx Extra;  // extra data
	public:
		// ctor
		CBMenuHandlerEx<CBClass, TEx>(CBClass *pCBTarget, typename DlgCallbackEx<CBClass, const TEx &>::ContextClickFunc pCallbackFn, TEx aExtra)
				: MenuHandler(), pCBTarget(pCBTarget), pCallbackFn(pCallbackFn), Extra(std::move(aExtra)) { }
		CBMenuHandlerEx<CBClass, TEx>(CBClass *pCBTarget, typename DlgCallbackEx<CBClass, const TEx &>::ContextClickFunc pCallbackFn)
				: MenuHandler(), pCBTarget(pCBTarget), pCallbackFn(pCallbackFn), Extra() { }

		const TEx &GetExtra() const { return Extra; }
		void SetExtra(const TEx &e) { Extra = e; }

		void OnOK(Element *pTargetElement) override
		{
			if (!pCBTarget || !pCallbackFn) return;
			// do callback
			(pCBTarget->*pCallbackFn)(pTargetElement, Extra);
		}
	};

	// context handler bound to member functions of another class
	template <class CBClass> class CBContextHandler : public ContextHandler
	{
	private:
		CBClass *pCBTarget; // callback target
		typename DlgCallback<CBClass>::ContextFunc pCallbackFn; // callback function

	public:
		// ctor
		CBContextHandler<CBClass>(CBClass *pCBTarget, typename DlgCallback<CBClass>::ContextFunc pCallbackFn)
				: ContextHandler(), pCBTarget(pCBTarget), pCallbackFn(pCallbackFn) { }

		bool OnContext(Element *pOnElement, int32_t iX, int32_t iY) override
		{
			DlgCallback<CBClass>();
			if (!pCBTarget) return false;
			// if (!pCallbackFn) return false;
			Screen *pScreen = pOnElement->GetScreen();
			if (!pScreen) return false;
			// let CB func create context menu
			ContextMenu *pNewMenu = (pCBTarget->*pCallbackFn)(pOnElement, iX, iY);
			if (!pNewMenu) return false;
			// open it on screen
			pScreen->DoContext(pNewMenu, pOnElement, iX, iY);
			// done, success
			return true;
		}

		C4GUI::ContextMenu *OnSubcontext(Element *pOnElement) override
		{
			// query directly
			return (pCBTarget->*pCallbackFn)(pOnElement, 0, 0);
		}
		friend class Dialog;
	};

	// a helper used to align elements
	class ComponentAligner
	{
	protected:
		C4Rect rcClientArea; // free client area
		int32_t iMarginX, iMarginY; // horizontal and vertical margins of components

		static C4Rect rcTemp; // temp rect

	public:
		ComponentAligner(const C4Rect &rcArea, int32_t iMarginX, int32_t iMarginY, bool fZeroAreaXY=false) // ctor
				: rcClientArea(rcArea), iMarginX(iMarginX), iMarginY(iMarginY)
		{ if (fZeroAreaXY) rcClientArea.x=rcClientArea.y=0; }

		bool GetFromTop(int32_t iHgt, int32_t iWdt, C4Rect &rcOut); // get a portion from the top
		bool GetFromLeft(int32_t iWdt, int32_t iHgt, C4Rect &rcOut); // get a portion from the left
		bool GetFromRight(int32_t iWdt, int32_t iHgt, C4Rect &rcOut); // get a portion from the right
		bool GetFromBottom(int32_t iHgt, int32_t iWdt, C4Rect &rcOut); // get a portion from the bottom
		void GetAll(C4Rect &rcOut); // get remaining client area
		bool GetCentered(int32_t iWdt, int32_t iHgt, C4Rect &rcOut); // get centered subregion within area

		C4Rect &GetFromTop(int32_t iHgt, int32_t iWdt=-1) { GetFromTop(iHgt, iWdt, rcTemp); return rcTemp; } // get a portion from the top
		C4Rect &GetFromLeft(int32_t iWdt, int32_t iHgt=-1) { GetFromLeft(iWdt, iHgt, rcTemp); return rcTemp; } // get a portion from the left
		C4Rect &GetFromRight(int32_t iWdt, int32_t iHgt=-1) { GetFromRight(iWdt, iHgt, rcTemp); return rcTemp; } // get a portion from the right
		C4Rect &GetFromBottom(int32_t iHgt, int32_t iWdt=-1) { GetFromBottom(iHgt, iWdt, rcTemp); return rcTemp; } // get a portion from the bottom
		C4Rect &GetAll() { GetAll(rcTemp); return rcTemp; } // get remaining client are
		C4Rect &GetCentered(int32_t iWdt, int32_t iHgt) { GetCentered(iWdt, iHgt, rcTemp); return rcTemp; } // get centered subregion within area (w/o size checks)

		C4Rect &GetGridCell(int32_t iSectX, int32_t iSectXMax, int32_t iSectY, int32_t iSectYMax, int32_t iSectSizeX=-1, int32_t iSectSizeY=-1, bool fCenterPos=false, int32_t iSectNumX=1, int32_t iSectNumY=1);

		int32_t GetWidth() const { return rcClientArea.Wdt; }
		int32_t GetHeight() const { return rcClientArea.Hgt; }
		int32_t GetHMargin() const { return iMarginX; }
		int32_t GetVMargin() const { return iMarginY; }

		int32_t GetInnerWidth() const { return rcClientArea.Wdt-2*iMarginX; }
		int32_t GetInnerHeight() const { return rcClientArea.Hgt-2*iMarginY; }

		void ExpandTop(int32_t iByHgt) { rcClientArea.y-=iByHgt; rcClientArea.Hgt+=iByHgt; } // enlarge client rect
		void ExpandLeft(int32_t iByWdt) { rcClientArea.x-=iByWdt; rcClientArea.Wdt+=iByWdt; } // enlarge client rect
		void ExpandRight(int32_t iByWdt) { rcClientArea.Wdt+=iByWdt; } // enlarge client rect
		void ExpandBottom(int32_t iByHgt) { rcClientArea.Hgt+=iByHgt; } // enlarge client rect

		void LogIt(const char *szName);
	};

	// shortcut for check whether GUI is active
	inline bool IsActive() { return Screen::GetScreenS()->IsActive(); }
	inline bool IsExclusive() { return Screen::GetScreenS()->IsExclusive(); }

	// shortcut for GUI screen size
	inline int32_t GetScreenWdt() { return Screen::GetScreenS()->GetBounds().Wdt; }
	inline int32_t GetScreenHgt() { return Screen::GetScreenS()->GetBounds().Hgt; }

	// sound effect in GUI: Only if enabled
	void GUISound(const char *szSound);

	// Zoom
	inline float GetZoom() { return Screen::GetScreenS()->GetZoom(); }
	inline void MouseMove(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam, class C4Viewport *pVP) // pVP specified for console mode viewports only
	{ Screen::GetScreenS()->MouseMove(iButton, iX, iY, dwKeyParam, pVP); }

	extern Screen TheScreen;
} // end of namespace

typedef C4GUI::Screen C4GUIScreen;
extern C4GUIScreen *pGUI;
#endif
