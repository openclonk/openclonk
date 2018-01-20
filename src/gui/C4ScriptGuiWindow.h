/*
* OpenClonk, http://www.openclonk.org
*
* Copyright (c) 2014-2016, The OpenClonk Team and contributors
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

/* a flexisble ingame menu system that can be composed out of multiple windows */

#ifndef INC_C4ScriptGuiWindow
#define INC_C4ScriptGuiWindow

#include "graphics/C4Surface.h"
#include "gui/C4Gui.h"
#include "script/C4Value.h"

namespace C4ScriptGuiWindowPropertyName {
	enum type
	{
		left = 0,
		top,
		right,
		bottom,

		relLeft,
		relRight,
		relTop,
		relBottom,

		leftMargin,
		topMargin,
		rightMargin,
		bottomMargin,

		relLeftMargin,
		relRightMargin,
		relTopMargin,
		relBottomMargin,

		backgroundColor,
		frameDecoration,
		symbolObject,
		symbolDef,
		symbolGraphicsName,
		text,
		onClickAction,
		onMouseInAction,
		onMouseOutAction,
		onCloseAction,
		style,
		priority,
		player,
		tooltip,
		_lastProp
	};
}

namespace C4ScriptGuiWindowActionID {
	enum type
	{
		SetTag = 1,
		Call,
	};
}

namespace C4ScriptGuiWindowStyleFlag {
	enum type
	{
		None = 0,
		GridLayout = 1,
		VerticalLayout = 2,
		TextVCenter = 4,
		TextHCenter = 8,
		TextRight = 16,
		TextBottom = 32,
		FitChildren = 64,
		Multiple = 128,
		IgnoreMouse = 256,
		NoCrop = 512,
		TightGridLayout = 1024,
	};
}

class C4ScriptGuiWindow;

class C4ScriptGuiWindowAction
{
	friend class C4ScriptGuiWindow;

	private:
	// the ID is unique among all actions. It is used later to synchronize callbacks
	int32_t id{0};

	int32_t action{0};
	C4ScriptGuiWindowAction *nextAction{nullptr}; // a linked list of actions
	// note: depending on the action not all of the following attributes always have values
	C4PropList *target{nullptr}; // contains a valid C4Object in case of SetTag, a generic proplist in case of Call
	C4String *text{nullptr}; // can be either a function name to call or a tag to set
	C4Value value; // arbitrary value used for Call
	int32_t subwindowID{0};

	public:
	C4ScriptGuiWindowAction() : value(0) { }
	~C4ScriptGuiWindowAction();
	void ClearPointers(C4Object *pObj);
	bool Init(C4ValueArray *array, int32_t index = 0); // index is the current action in an array of actions
	// executes non-synced actions and syncs the others
	// the action type parameters is only used to be able to sync commands
	void Execute(C4ScriptGuiWindow *parent, int32_t player, int32_t actionType);
	// used to execute synced commands, explanation see C4ScriptGuiWindow::ExecuteCommand
	bool ExecuteCommand(int32_t actionID, C4ScriptGuiWindow *parent, int32_t player);
	// used for serialization. The "first" parameter is used so that chained actions are stored correctly into an array
	const C4Value ToC4Value(bool first = true);
};

class C4ScriptGuiWindowProperty
{
	friend class C4ScriptGuiWindow;

	private:
	typedef union
	{
		void *data;
		float f;
		int32_t d;
		C4Object *obj;
		C4Def *def;
		C4GUI::FrameDecoration *deco;
		StdCopyStrBuf *strBuf;
		C4ScriptGuiWindowAction *action;
	} Prop;

	Prop *current{nullptr};
	// the last tag is used to be able to call the correct action on re-synchronizing commands
	C4String* currentTag{nullptr};

	std::map<C4String*, Prop> taggedProperties;
	void CleanUp(Prop &prop);
	void CleanUpAll();

	int32_t type{-1}; // which property do I stand for?

	// the following methods directly set values (default Std tag)
	// note that for int/floats no cleanup is necessary as it would be for the more general Set method
	void SetInt(int32_t to, C4String *tag = nullptr);
	void SetFloat(float to, C4String *tag = nullptr);
	void SetNull(C4String *tag = nullptr);

	public:
	~C4ScriptGuiWindowProperty();
	C4ScriptGuiWindowProperty() = default;
	void Set(const C4Value &value, C4String *tag);

	int32_t GetInt() { return current->d; }
	float GetFloat() { return current->f; }
	C4Object *GetObject() { return current->obj; }
	C4Def *GetDef() { return current->def; }
	C4GUI::FrameDecoration *GetFrameDecoration() { return current->deco; }
	StdCopyStrBuf *GetStrBuf() { return current->strBuf; }
	C4ScriptGuiWindowAction *GetAction() { return current->action; }
	std::list<C4ScriptGuiWindowAction*> GetAllActions(); // used to synchronize actions

	bool SwitchTag(C4String *tag);
	C4String *GetCurrentTag() { return currentTag; }

	const C4Value ToC4Value();

	void ClearPointers(C4Object *pObj);
};

class C4ScriptGuiWindow : public C4GUI::ScrollWindow
{
	friend class C4ScriptGuiWindowAction;
	friend class C4ScriptGuiWindowScrollBar;
public:
	// the size of the screen that is covered by a centered "main menu"
	static const float standardWidth;
	static const float standardHeight;

	private:
	// the "main" menu ID is always unique, however the sub-menu IDs do NOT have to be unique
	// they can be set from script and in combination with the target should suffice to identify windows
	int32_t id;
	// The name of a window is used when updating a window to identify the correct child;
	// however, it is NOT generally used to identify windows, f.e. to close them.
	// The reasoning behind that is that EVERY window needs a name (in the defining proplist) but only windows that need to be addressed later need an ID;
	// so separating the identifying property from the name hopefully reduces clashes - especially since names will often be "left"/"right" etc.
	// Note that a window does not necessarily have a name and names starting with an underscore are never saved (to be able to f.e. add new windows without knowing the names of the existing windows).
	C4String *name;
	// this is not only a window inside a menu but a top-level-window?
	// this does not mean the ::WindowMenuRoot but rather a player-created submenu
	bool isMainWindow;
	// whether this menu is the root of all script-created menus (aka of the isMainWindow windows)
	bool IsRoot();
	bool mainWindowNeedsLayoutUpdate;

	bool wasRemoved; // to notify the window that it should not inform its parent on Close() a second time
	bool closeActionWasExecuted; // to prevent a window from calling the close-callback twice even if f.e. closed in the close-callback..
	C4Object *target;
	const C4Object *GetTarget() { return target; }

	// properties are stored extra to make "tags" possible
	C4ScriptGuiWindowProperty props[C4ScriptGuiWindowPropertyName::_lastProp];
	void Init();
	// ID is set by parent, parent gives unique IDs to children
	void SetID(int32_t to) { id = to; }
	// to be used to generate the quick-access children map for main menus
	void ChildGotID(C4ScriptGuiWindow *child);
	void ChildWithIDRemoved(C4ScriptGuiWindow *child);
	std::multimap<int32_t, C4ScriptGuiWindow *> childrenIDMap;
	// should be called when the Priority property of a child changes
	// will sort the child correctly into the children list
	void ChildChangedPriority(C4ScriptGuiWindow *child);
	// helper function to extract relative and absolute position values from a string
	void SetPositionStringProperties(const C4Value &property, C4ScriptGuiWindowPropertyName::type relative, C4ScriptGuiWindowPropertyName::type absolute, C4String *tag);
	C4Value PositionToC4Value(C4ScriptGuiWindowPropertyName::type relative, C4ScriptGuiWindowPropertyName::type absolute);
	// sets all margins either from a string or from an array
	void SetMarginProperties(const C4Value &property, C4String *tag);
	C4Value MarginsToC4Value();

	// this is only supposed to be called at ::Game.GuiWindowRoot since it uses the "ID" property
	// this is done to make saving easier. Since IDs do not need to be sequential, action&menu IDs can both be derived from "id"
	int32_t GenerateMenuID() { return ++id; }
	int32_t GenerateActionID() { return ++id; }


	// children height should be set when enabling a scroll bar so that, with style FitChildren, the size can simply be changed
	void EnableScrollBar(bool enable = true, float childrenHeight = 0.0f);

	public:
	// used by mouse input, this is in screen coordinates
	/*struct _lastDrawPosition
	{
		float left, right;
		float top, bottom;

		float topMostChild, bottomMostChild;
		int32_t dirty; // indicates wish to update topMostChild and bottomMostChild asap
		bool needLayoutUpdate;
		_lastDrawPosition() : left(0.0f), right(0.0f), top(0.0f), bottom(0.0f), topMostChild(0.0f), bottomMostChild(0.0f), dirty(2), needLayoutUpdate(false){}
	} lastDrawPosition;*/

	void SetTag(C4String *tag);

	C4ScriptGuiWindow();
	~C4ScriptGuiWindow() override;

	int32_t GetID() { return id; }
	// finds a child with a certain ID, usually called on ::MainWindowRoot to get submenus
	C4ScriptGuiWindow *GetChildByID(int32_t child);
	// finds a child by name, usually called when updating a window with a new proplist
	C4ScriptGuiWindow *GetChildByName(C4String *childName);
	// finds any fitting sub menu - not necessarily direct child
	// has to be called on children of ::MainWindowRoot, uses the childrenIDMap
	// note: always checks the target to avoid ambiguities, even if 0
	C4ScriptGuiWindow *GetSubWindow(int32_t childID, C4Object *childTarget);



	// pass a proplist to create a window + subwindows as specified
	// you can call this function on a window more than once
	// if isUpdate is true, all new children will have resetStdTag set
	bool CreateFromPropList(C4PropList *proplist, bool resetStdTag = false, bool isUpdate = false, bool isLoading = false);

	// constructs a C4Value (proplist) that contains everything that is needed for saving this window
	const C4Value ToC4Value();
	// this MUST only be called when loading
	void SetEnumeratedID(int enumID) { id = enumID; }
	void Denumerate(C4ValueNumbers *numbers);
	// C4ScriptGuiWindow will delete its children on close. Make sure you don't delete anything twice
	C4ScriptGuiWindow *AddChild(C4ScriptGuiWindow *child);
	C4ScriptGuiWindow *AddChild() { return AddChild(new C4ScriptGuiWindow()); }

	void ClearChildren(bool close = true); // close: whether to properly "Close" them, alias for RemoveChild
	void RemoveChild(C4ScriptGuiWindow *child, bool close = true, bool all = false); // child = 0 & all = true to clear all. Also deletes the child(ren).
	void Close();
	void ClearPointers(C4Object *pObj);

	// calculate the width/height based on a certain property (f.e. leftMargin and relLeftMargin) and the parent's width/height
	float CalculateRelativeSize(float parentWidthOrHeight, C4ScriptGuiWindowPropertyName::type absoluteProperty, C4ScriptGuiWindowPropertyName::type relativeProperty);

	// schedules a layout update for the next drawing step
	void RequestLayoutUpdate();
	// this updates the window's layout and also propagates to all children
	bool UpdateLayout(C4TargetFacet &cgo);
	bool UpdateLayout(C4TargetFacet &cgo, float parentWidth, float parentHeight);
	bool UpdateChildLayout(C4TargetFacet &cgo, float parentWidth, float parentHeight);
	// special layouts that are set by styles
	void UpdateLayoutGrid();
	void UpdateLayoutTightGrid();
	void UpdateLayoutVertical();
	// the window will be drawn in the context of a viewport BY the viewport
	// so just do nothing when TheScreen wants to draw the window
	void Draw(C4TargetFacet &cgo) override {}
	// Draw without parameters can be used for the root
	bool DrawAll(C4TargetFacet &cgo, int32_t player);
	// the clipping rectangle has already been set, but currentClippingRect must be passed to DrawChildren
	bool Draw(C4TargetFacet &cgo, int32_t player, C4Rect *currentClippingRect);
	bool GetClippingRect(int32_t &left, int32_t &top, int32_t &right, int32_t &bottom);

	// withMultipleFlag is there to draw only the non-multiple or the multiple windows
	// withMultipleFlag == -1: all windows are drawn (standard)
	// withMultipleFlag ==  0: only one non-Multiple window is drawn
	// withMultipleFlag ==  1: only Multiple windows are drawn
	// returns whether at least one child was drawn
	bool DrawChildren(C4TargetFacet &cgo, int32_t player, int32_t withMultipleFlag = -1, C4Rect *currentClippingRect = nullptr);

	// used for commands that have been synchronized and are coming from the command queue
	// attention: calls to this need to be synchronized!
	bool ExecuteCommand(int32_t actionID, int32_t player, int32_t subwindowID, int32_t actionType, C4Object *target);
	
	// virtual bool MouseInput(int32_t player, int32_t button, int32_t mouseX, int32_t mouseY, DWORD dwKeyParam);
	// this is called only on the root menu
	using C4GUI::ScrollWindow::MouseInput;
	virtual bool MouseInput(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam);
	// this is then called on the child windows, note the return value
	virtual bool ProcessMouseInput(int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam, int32_t parentOffsetX, int32_t parentOffsetY);
	// called when mouse cursor enters element region
	void MouseEnter(C4GUI::CMouse &rMouse) override;
	// called when mouse cursor leaves element region
	void MouseLeave(C4GUI::CMouse &rMouse) override;
	// This remembers whether the window currently has mouse focus and whether it has been mouse-down-ed.
	// All windows with mouse focus set are remembered by their parents and notified when the mouse left.
	// The state is also used to make sure that button-up events without button-downs are not caught by the UI.
	enum MouseState // values of this enum will be bit-wise combined
	{
		None = 0,
		Focus = 1,
		MouseDown = 2
	};
	int32_t currentMouseState; // this needs to be saved in savegames!!!
	// OnMouseOut() called by this window, unsets the mouse focus
	// must notify children, too!
	void OnMouseOut(int32_t player);
	// called by this window, sets the mouse focus; the offset is used to set the correct tooltip rectangle for ::MouseControl
	void OnMouseIn(int32_t player, int32_t parentOffsetX, int32_t parentOffsetY);
	bool HasMouseFocus() { return currentMouseState & MouseState::Focus; }
	// Returns whether the menu can be seen (and interacted with) by a player. This includes checking the target's visibility.
	bool IsVisibleTo(int32_t player);
private:
	// Use the currently loaded font to determine on-screen size of 1 EM.
	static float Em2Pix(float em);
	static float Pix2Em(float pix);
};

#endif
