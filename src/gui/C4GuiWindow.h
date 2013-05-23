/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2013  David Dormagen
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* a flexisble ingame menu system that can be composed out of multiple windows */

#ifndef INC_C4GuiWindow
#define INC_C4GuiWindow

#include <C4Surface.h>
#include <C4Gui.h>

#include <C4Value.h>

#include <map>

enum C4GuiWindowPropertyName
{
	left = 0,
	top,
	right,
	bottom,
	relLeft,
	relRight,
	relTop,
	relBottom,
	backgroundColor,
	frameDecoration,
	symbolObject,
	symbolDef,
	text,
	onClickAction,
	onMouseInAction,
	onMouseOutAction,
	onCloseAction,
	style,
	priority,
	_lastProp
};

enum C4GuiWindowActionID
{
	SetTag = 1,
	Call,
};

enum C4GuiWindowStyleFlag
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
};

class C4GuiWindow;

class C4GuiWindowAction
{
	friend class C4GuiWindow;

	private:
	// the ID is unique among all actions. It is used later to synchronize callbacks
	int32_t id;

	int32_t action;
	C4GuiWindowAction *nextAction; // a linked list of actions
	// note: depending on the action not all of the following attributes always have values
	C4PropList *target; // contains a valid C4Object in case of SetTag, a generic proplist in case of Call
	C4String *text; // can be either a function name to call or a tag to set
	C4Value value; // arbitrary value used for Call
	int32_t subwindowID;

	public:
	C4GuiWindowAction() : id(0), action(0), nextAction(0), target(0), text(0), value(0), subwindowID(0) { }
	~C4GuiWindowAction();
	void ClearPointers(C4Object *pObj);
	bool Init(C4ValueArray *array, int32_t index = 0); // index is the current action in an array of actions
	// executes non-synced actions and syncs the others
	// the tag and action type parameters are only used to be able to sync commands
	void Execute(C4GuiWindow *parent, int32_t player, unsigned int tag, int32_t actionType);
	// used to execute synced commands, explanation see C4GuiWindow::ExecuteCommand
	bool ExecuteCommand(int32_t actionID, C4GuiWindow *parent, int32_t player);
	// used for serialization. The "first" parameter is used so that chained actions are stored correctly into an array
	const C4Value ToC4Value(bool first = true);
};

class C4GuiWindowProperty
{
	friend class C4GuiWindow;

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
		C4GuiWindowAction *action;
	} Prop;

	Prop *current;
	// the last tag is used to be able to call the correct action on re-synchronizing commands
	unsigned int currentTag;

	std::map<unsigned int, Prop> taggedProperties;
	void CleanUp(Prop &prop);
	void CleanUpAll();

	int32_t type; // which property do I stand for?

	void SetInt(unsigned int hash, int32_t to) { taggedProperties[hash] = Prop(); current = &taggedProperties[hash]; current->d = to; }
	void SetFloat(unsigned int hash, float to) { taggedProperties[hash] = Prop(); current = &taggedProperties[hash]; current->f = to; }
	void SetNull(unsigned int hash) { taggedProperties[hash] = Prop(); current = &taggedProperties[hash]; current->data = 0; }

	public:
	~C4GuiWindowProperty();
	C4GuiWindowProperty() : current(0), currentTag(0), type(-1) {}
	void Set(const C4Value &value, unsigned int hash);

	int32_t GetInt() { return current->d; }
	float GetFloat() { return current->f; }
	C4Object *GetObject() { return current->obj; }
	C4Def *GetDef() { return current->def; }
	C4GUI::FrameDecoration *GetFrameDecoration() { return current->deco; }
	StdCopyStrBuf *GetStrBuf() { return current->strBuf; }
	C4GuiWindowAction *GetAction() { return current->action; }
	C4GuiWindowAction *GetActionForTag(unsigned int hash); // used to synchronize actions

	bool SwitchTag(C4String *tag);
	unsigned int GetCurrentTag() { return currentTag; }

	const C4Value ToC4Value();

	void ClearPointers(C4Object *pObj);
};

class C4GuiWindowScrollBar
{
	friend class C4GuiWindow;

	public:
	float offset;
	C4GuiWindowScrollBar();
	virtual ~C4GuiWindowScrollBar();
	void ScrollBy(float val) { offset += val; if (offset < 0.0f) offset = 0.0f; else if (offset > 1.0f) offset = 1.0f;	}
	void Draw(C4TargetFacet &cgo, int32_t player, float parentLeft, float parentTop, float parentRight, float parentBottom);
	virtual bool MouseInput(int32_t button, int32_t mouseX, int32_t mouseY, DWORD dwKeyParam);

	private:
	C4GUI::ScrollBarFacets *decoration;
	C4GuiWindow *parent;
};

class C4GuiWindow
{
	friend class C4GuiWindowAction;

	private:
	// the menu ID is always unique, however the sub-menu IDs do NOT have to be unique
	// they can be set from script and in combination with the target should suffice to identify windows
	int32_t id;
	// this is not only a window inside a menu but a top-level-window?
	// this does not mean the ::WindowMenuRoot but rather a player-created submenu
	bool isMainWindow;

	std::list<C4GuiWindow*> children;
	C4GuiWindow *parent;
	bool wasRemoved; // to notify the window that it should not inform its parent on Close() a second time
	bool closeActionWasExecuted; // to prevent a window from calling the close-callback twice even if f.e. closed in the close-callback..
	bool visible;
	C4Object *target;
	const C4Object *GetTarget() { return target; }
	C4GuiWindowScrollBar *scrollBar;

	// this remembers whether the window currently has mouse focus
	// all windows with this property set are remembered by their parents and notified when the mouse left
	bool hasMouseFocus; // this needs to be saved in savegames!!!
	// OnMouseOut() called by this window, sets "hasMouseFocus" from true to false
	// must notify children, too!
	void OnMouseOut(int32_t player);
	void OnMouseIn(int32_t player); // called by this window, sets "hasMouseFocus" from false to true

	// properties are stored extra to make "tags" possible
	C4GuiWindowProperty props[C4GuiWindowPropertyName::_lastProp];
	void Init();
	// withMultipleFlag is there to draw only the non-multiple or the multiple windows
	// withMultipleFlag == -1: all windows are drawn (standard)
	// withMultipleFlag ==  0: only one non-Multiple window is drawn
	// withMultipleFlag ==  1: only Multiple windows are drawn
	// returns whether at least one child was drawn
	bool DrawChildren(C4TargetFacet &cgo, int32_t player, float parentLeft, float parentTop, float parentRight, float parentBottom, int32_t withMultipleFlag = -1);
	// ID is set by parent, parent gives unique IDs to children
	void SetID(int32_t to) { id = to; }
	// to be used to generate the quick-access children map for main menus
	void ChildGotID(C4GuiWindow *child);
	void ChildWithIDRemoved(C4GuiWindow *child);
	std::multimap<int32_t, C4GuiWindow *> childrenIDMap;
	// should be called when the Priority property of a child changes
	// will sort the child correctly into the children list
	void ChildChangedPriority(C4GuiWindow *child);
	// helper function
	// sets property value from possible(!) array
	void SetArrayTupleProperty(const C4Value &property, C4GuiWindowPropertyName first, C4GuiWindowPropertyName second, unsigned int hash);

	// this is only supposed to be called at ::GuiWindowRoot since it uses the "ID" property
	// this is done to make saving easier. Since IDs do not need to be sequential, action&menu IDs can both be derived from "id"
	int32_t GenerateMenuID() { return ++id; }
	int32_t GenerateActionID() { return ++id; }

	void UpdateLayout();
	void UpdateLayoutGrid();
	void UpdateLayoutVertical();
	// children height should be set when enabling a scroll bar so that, with style FitChildren, the size can simply be changed
	void EnableScrollBar(bool enable = true, float childrenHeight = 0.0f);

	public:
	// used by mouse input, this is in screen coordinates
	struct _lastDrawPosition
	{
		float left, right;
		float top, bottom;

		float topMostChild, bottomMostChild;
		int32_t dirty; // indicates wish to update topMostChild and bottomMostChild asap
		_lastDrawPosition() : left(0.0f), right(0.0f), top(0.0f), bottom(0.0f), topMostChild(0.0f), bottomMostChild(0.0f), dirty(2) { }
	} lastDrawPosition;

	bool IsVisible() { return visible; }
	void SetVisible(bool f) { visible = f; }
	void SetTag(C4String *tag);

	C4GuiWindow();
	C4GuiWindow(float stdBorderX, float stdBorderY);
	virtual ~C4GuiWindow();

	int32_t GetID() { return id; }
	// finds a child with a certain ID, usually called on ::MainWindowRoot to get submenus
	C4GuiWindow *GetChildByID(int32_t child);
	// finds any fitting sub menu - not necessarily direct child
	// has to be called on children of ::MainWindowRoot, uses the childrenIDMap
	// note: always checks the target to avoid ambiguities, even if 0
	C4GuiWindow *GetSubWindow(int32_t childID, C4Object *childTarget);



	// pass a proplist to create a window + subwindows as specified
	// you can call this function on a window more than once
	// if isUpdate is true, all new children will have resetStdTag set
	bool CreateFromPropList(C4PropList *proplist, bool resetStdTag = false, bool isUpdate = false);

	// constructs a C4Value (proplist) that contains everything that is needed for saving this window
	const C4Value ToC4Value();

	// C4GuiWindow will delete its children on close. Make sure you don't delete anything twice
	C4GuiWindow *AddChild(C4GuiWindow *child);
	C4GuiWindow *AddChild() { return AddChild(new C4GuiWindow()); }

	void ClearChildren(bool close = true); // close: whether to properly "Close" them, alias for RemoveChild
	void RemoveChild(C4GuiWindow *child, bool close = true, bool all = false); // child = 0 & all = true to clear all
	void Close();
	void ClearPointers(C4Object *pObj);

	// Draw without parameters can be used for the root
	bool Draw(C4TargetFacet &cgo, int32_t player);
	bool Draw(C4TargetFacet &cgo, int32_t player, float parentLeft, float parentTop, float parentRight, float parentBottom);
	bool GetClippingRect(float &left, float &top, float &right, float &bottom);

	// used for commands that have been synchronized and are coming from the command queue
	// attention: calls to this need to be synchronized!
	bool ExecuteCommand(int32_t actionID, int32_t player, int32_t subwindowID, int32_t actionType, C4Object *target, unsigned int tag);
	virtual bool MouseInput(int32_t player, int32_t button, int32_t mouseX, int32_t mouseY, DWORD dwKeyParam);
};

extern C4GuiWindow GuiWindowRoot;

#endif
