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
// Input to player control mapping

#ifndef INC_C4PlayerControl
#define INC_C4PlayerControl

#include "gui/C4KeyboardInput.h"
#include "c4group/C4LangStringTable.h"
#include "object/C4Id.h"
#include "platform/C4TimeMilliseconds.h"

const float C4GFX_ZoomStep = 1.1040895f;

// one control definition, e.g. "Left", "Throw", etc.
class C4PlayerControlDef
{
public:
	enum CoordinateSpace // coordinate space for mouse position
	{
		COS_Game = 0,     // game (landscape) coordinates 
		COS_Viewport = 1  // viewport (GUI) coordinates
	};
	enum Actions //action to be performed when control is triggered
	{
		CDA_None = 0,          // do nothing
		CDA_Script,          // default: Script callback
		CDA_Menu,            // open player menu (async)
		CDA_MenuOK, CDA_MenuCancel, CDA_MenuLeft, CDA_MenuUp, CDA_MenuRight, CDA_MenuDown, // player menu controls (async)
		CDA_ObjectMenuTextComplete, // object menu fast-foward through text animation (async)
		CDA_ObjectMenuOK, CDA_ObjectMenuOKAll, CDA_ObjectMenuSelect, CDA_ObjectMenuCancel, CDA_ObjectMenuLeft, CDA_ObjectMenuUp, CDA_ObjectMenuRight, CDA_ObjectMenuDown, // object menu controls (sync)
		CDA_ZoomIn, CDA_ZoomOut // player viewport control (async)
	};

private:
	StdCopyStrBuf sIdentifier; // name as seen in script and config
	StdCopyStrBuf sGUIName;    // name as displayed to player
	StdCopyStrBuf sGUIDesc;    // key description displayed to player in config dialog
	bool fGlobal{false};             // if true, control can be bound to the global player only
	bool fIsHoldKey{false};          // if true, the control can be in down and up state
	bool fDefaultDisabled{false};    // if true, the control is disabled by default and needs to be enabled by script
	bool fSendCursorPos{false};      // if true, x/y parameters will be set by current GUI mouse cursor pos (or GetCursor()-GUI coordinate pos for gamepad)
	int32_t iRepeatDelay;     // if >0, the key will generate successive events when held down
	int32_t iInitialRepeatDelay; // delay after which KeyRepeat will be enabled
	C4ID idControlExtraData;  // extra data to be passed to script function
	CoordinateSpace eCoordSpace{COS_Game}; // coordinate space to be used for mouse coordinates when control is triggered by mouse
	Actions eAction{CDA_Script};

public:
	C4PlayerControlDef() :
		idControlExtraData(C4ID::None)
	{}
	~C4PlayerControlDef() = default;

	void CompileFunc(StdCompiler *pComp);

	const char *GetIdentifier() const { return sIdentifier.getData(); }
	const char *GetGUIName() const { return sGUIName.getData(); }
	const char *GetGUIDesc() const { return sGUIDesc.getData(); }
	Actions GetAction() const { return eAction; }
	bool IsHoldKey() const { return fIsHoldKey; }
	C4ID GetExtraData() const { return idControlExtraData; }
	bool IsGlobal() const { return fGlobal; }
	int32_t GetRepeatDelay() const { return iRepeatDelay; }
	int32_t GetInitialRepeatDelay() const { return iInitialRepeatDelay; }
	bool IsDefaultDisabled() const { return fDefaultDisabled; }
	CoordinateSpace GetCoordinateSpace() const { return eCoordSpace; }
	bool IsSendCursorPos() const { return fSendCursorPos; }

	bool operator ==(const C4PlayerControlDef &cmp) const;

	bool Execute(bool fUp, const C4KeyEventData &rKeyExtraData); // key was triggered - execute and return if handled
	bool IsSyncObjectMenuControl() const { return eAction>=CDA_ObjectMenuOK && eAction<=CDA_ObjectMenuDown; }
	bool IsAsync() const { return eAction != CDA_None && eAction != CDA_Script && !IsSyncObjectMenuControl(); } // true if to be executed directly when triggered
	bool IsSync() const { return eAction == CDA_Script || IsSyncObjectMenuControl(); } // true if to be executed via control queue
	bool IsValid() const { return eAction != CDA_None; }
};

// CON_* constants are indices into the C4PlayerControlDefs list
enum { CON_None = -1 }; // No/undefined control

// list of all known player control definitions
class C4PlayerControlDefs
{
private:
	typedef std::vector<C4PlayerControlDef> DefVecImpl;
	DefVecImpl Defs;
	bool clear_previous{false}; // if set is merged, all previous control defs are cleared - use 

public:
	struct CInternalCons
	{
		int32_t CON_ObjectMenuSelect{CON_None}, CON_ObjectMenuOKAll{CON_None}, CON_ObjectMenuOK{CON_None}, CON_ObjectMenuCancel{CON_None}, CON_CursorPos{CON_None};
		CInternalCons() = default;
	} InternalCons;

	void UpdateInternalCons();

public:
	C4PlayerControlDefs() = default;
	~C4PlayerControlDefs() = default;
	void Clear();

	void CompileFunc(StdCompiler *pComp);
	void MergeFrom(const C4PlayerControlDefs &Src); // copy all defs from source file; overwrite defs of same name if found

	void FinalInit(); // after all defs have been loaded: register script constants

	const C4PlayerControlDef *GetControlByIndex(int32_t idx) const;
	int32_t GetControlIndexByIdentifier(const char *szIdentifier) const; // return CON_None for not found
	size_t GetCount() const { return Defs.size(); }

	bool operator ==(const C4PlayerControlDefs &cmp) const { return Defs == cmp.Defs && clear_previous == cmp.clear_previous; }
};

struct C4PlayerControlRecentKey
{
	C4KeyCodeEx pressed_key, matched_key;
	C4TimeMilliseconds tTime;
	C4PlayerControlRecentKey(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key, C4TimeMilliseconds tTime) : pressed_key(pressed_key), matched_key(matched_key), tTime(tTime) {}
	bool operator ==(const C4KeyCodeEx &cmp) { return pressed_key==cmp; } // comparison op for finding items in lists: Search for the pressed key only
};

typedef std::list<C4PlayerControlRecentKey> C4PlayerControlRecentKeyList;

typedef std::vector<C4KeyCodeEx> C4KeyCodeExVec;

// a key/mouse/gamepad assignment to a PlayerControlDef
class C4PlayerControlAssignment
{
public:
	// action to be performed on the control upon this key
	enum TriggerModes
	{
		CTM_Default = 0,              // standard behaviour: The control will be triggered
		CTM_Hold = 1 << 0,        // the control will be put into "down"-mode
		CTM_Release = 1 << 1,        // the hold mode of the control will be released
		CTM_AlwaysUnhandled = 1 << 2,  // the key will not block handling of other keys even if it got handled
		CTM_HandleDownStatesOnly = 1 << 3, // used when an already handled release key is processed to reset down states of overridden keys only
		CTM_ClearRecentKeys = 1 << 4  // if this assignment is triggered, RecentKeys are reset so no more combos can be generated
	};

private:
	// KeyCombo list:
	// if size()>1, the control is triggered only if this combo is fulfilled
	// used for simultanuous keypresses or sequences
	struct KeyComboItem
	{
		C4KeyCodeEx Key;
		StdCopyStrBuf sKeyName;
		void CompileFunc(StdCompiler *pComp);
		void UpdateKeyName();
		bool operator ==(const KeyComboItem &cmp) const { return Key==cmp.Key; }
	};
	typedef std::vector<KeyComboItem> KeyComboVec;
	KeyComboVec KeyCombo;
	bool fComboIsSequence; // if true, the keys must be pressed in sequence. Otherwise, they must be pressed simultanuously

	// trigger key: key/mouse/gamepad event triggering this assignment. For combinations, the last key of the combo.
	C4KeyCodeEx TriggerKey;

	StdCopyStrBuf sControlName; // name of the control to be executed on this key
	StdCopyStrBuf sGUIName;    // name as displayed to player. If empty, name stored in control def should be used.
	StdCopyStrBuf sGUIDesc;    // key description displayed to player in config dialog. If empty, name stored in control def should be used.
	bool fGUIDisabled;   // whether this key can't be reassigned through the GUI dialogue
	bool fOverrideAssignments{false};  // override all other assignments to the same key?
	bool is_inherited{false}; // set for assignments that were copied from a parent set without modification
	bool fRefsResolved{false}; // set to true after sControlName and sKeyNames have been resolved to runtime values
	int32_t iGUIGroup{0};  // in which this control is grouped in the gui
	int32_t iControl{CON_None}; // the control to be executed on this key, i.e. the resolved sControlName
	int32_t iPriority{0};          // higher priority assignments get handled first
	int32_t iTriggerMode{CTM_Default};

	const C4PlayerControlAssignment *inherited_assignment{nullptr}; // valid for assignments that were copied from a parent: source assignment

public:
	C4PlayerControlAssignment() :
		TriggerKey()
	{}
	~C4PlayerControlAssignment() = default;

	void CompileFunc(StdCompiler *pComp);
	void CopyKeyFrom(const C4PlayerControlAssignment &src_assignment);
	bool ResolveRefs(class C4PlayerControlAssignmentSet *pParentSet, C4PlayerControlDefs *pControlDefs); // resolve references between assignments
	bool IsComboMatched(const C4PlayerControlRecentKeyList &DownKeys, const C4PlayerControlRecentKeyList &RecentKeys) const; // check if combo is currently fulfilled (assuming TriggerKey is already matched)
	void SetInherited(bool to_val) { is_inherited = to_val; }
	void SetInheritedAssignment(const C4PlayerControlAssignment *to_val) { inherited_assignment = to_val; }
	void ResetKeyToInherited();
	bool IsKeyChanged() const;
	void SetControlName(const char *control_name) { sControlName.Copy(control_name); }
	void SetKey(const C4KeyCodeEx &key);

	bool operator ==(const C4PlayerControlAssignment &cmp) const; // doesn't compare resolved TriggerKey/iControl
	bool operator <(const C4PlayerControlAssignment &cmp) const { return iPriority > cmp.iPriority; } // assignments are processed in DESCENDING priority!
	const char *GetControlName() const { return sControlName.getData(); }
	int32_t GetControl() const { return iControl; }
	const char *GetGUIName(const C4PlayerControlDefs &defs) const;
	const char *GetGUIDesc(const C4PlayerControlDefs &defs) const;
	bool IsGUIDisabled() const;
	int32_t GetGUIGroup() const; 
	bool IsRefsResolved() const { return fRefsResolved; }
	void ResetRefsResolved() { fRefsResolved = false; } // Mark references to other assignments as not resolved
	bool IsAlwaysUnhandled() const { return !!(iTriggerMode & CTM_AlwaysUnhandled); }
	int32_t GetTriggerMode() const { return iTriggerMode; }
	const C4KeyCodeEx &GetTriggerKey() const { return TriggerKey; }
	bool HasCombo() const { return KeyCombo.size()>1; }
	bool IsOverrideAssignments() const { return fOverrideAssignments; }
	bool IsInherited() const { return is_inherited; }
	const C4PlayerControlAssignment *GetInheritedAssignment() const { return inherited_assignment; }
	StdStrBuf GetKeysAsString(bool human_readable, bool short_name) const;
};

typedef std::vector<C4PlayerControlAssignment> C4PlayerControlAssignmentVec;
typedef std::vector<const C4PlayerControlAssignment *> C4PlayerControlAssignmentPVec;


// a set of key/mouse/gamepad assignments to all controls
class C4PlayerControlAssignmentSet
{
private:
	StdCopyStrBuf sName, sGUIName, sParentSetName;
	const C4PlayerControlAssignmentSet *parent_set{nullptr};
	C4PlayerControlAssignmentVec Assignments; // ordered by priority

	bool has_keyboard{true};
	bool has_mouse{true};
	bool has_gamepad{false};

public:
	C4PlayerControlAssignmentSet() = default;
	~C4PlayerControlAssignmentSet() = default;
	void InitEmptyFromTemplate(const C4PlayerControlAssignmentSet &template_set); // copy all fields except assignments

	void CompileFunc(StdCompiler *pComp);
	bool ResolveRefs(C4PlayerControlDefs *pControlDefs); // resolve references between assignments
	void SortAssignments();

	enum MergeMode { MM_Normal, MM_LowPrio, MM_Inherit, MM_ConfigOverload };

	void MergeFrom(const C4PlayerControlAssignmentSet &Src, MergeMode merge_mode); // take over all assignments defined in Src
	C4PlayerControlAssignment *CreateAssignmentForControl(const char *control_name);
	void RemoveAssignmentByControlName(const char *control_name);

	const char *GetName() const { return sName.getData(); }
	const char *GetGUIName() const { return sGUIName.getData(); }
	bool IsWildcardName() const { return IsWildcardString(sName.getData()); }

	C4PlayerControlAssignment *GetAssignmentByIndex(int32_t index); // assignments are ordered by priority
	C4PlayerControlAssignment *GetAssignmentByControlName(const char *szControlName);
	C4PlayerControlAssignment *GetAssignmentByControl(int32_t control);
	void GetAssignmentsByKey(const C4PlayerControlDefs &rDefs, const C4KeyCodeEx &key, bool fHoldKeysOnly, C4PlayerControlAssignmentPVec *pOutVec, const C4PlayerControlRecentKeyList &DownKeys, const C4PlayerControlRecentKeyList &RecentKeys) const; // match only by TriggerKey (last key of Combo) if fHoldKeysOnly
	void GetTriggerKeys(const C4PlayerControlDefs &rDefs, C4KeyCodeExVec *pRegularKeys, C4KeyCodeExVec *pHoldKeys) const; // put all trigger keys of keyset into output vectors

	bool operator ==(const C4PlayerControlAssignmentSet &cmp) const;

	C4Facet GetPicture() const; // get image to be drawn to represent this control set
	// todo
	bool HasKeyboard() const { return has_keyboard; }
	bool HasMouse() const { return has_mouse; }
	bool HasGamepad() const { return has_gamepad; }
	int32_t GetLayoutOrder() const { return 0; } // returns position on keyboard (increasing from left to right) for viewport sorting
	bool IsMouseControlAssigned(int32_t mouseevent) const;
};

// list of C4PlayerControlAssignmentSet
class C4PlayerControlAssignmentSets
{
private:
	typedef std::list<C4PlayerControlAssignmentSet> AssignmentSetList;
	AssignmentSetList Sets;
	bool clear_previous{false};

public:
	C4PlayerControlAssignmentSets() = default;
	~C4PlayerControlAssignmentSets() = default;
	void Clear();

	void CompileFunc(StdCompiler *pComp);
	bool operator ==(const C4PlayerControlAssignmentSets &cmp) const;
	bool ResolveRefs(C4PlayerControlDefs *pControlDefs); // resolve references between assignments
	void SortAssignments();

	void MergeFrom(const C4PlayerControlAssignmentSets &Src, C4PlayerControlAssignmentSet::MergeMode merge_mode); // take over all assignments in known sets and new sets defined in Src

	C4PlayerControlAssignmentSet *CreateEmptySetByTemplate(const C4PlayerControlAssignmentSet &template_set);
	void RemoveSetByName(const char *set_name);

	C4PlayerControlAssignmentSet *GetSetByName(const char *szName);
	C4PlayerControlAssignmentSet *GetDefaultSet();
	int32_t GetSetIndex(const C4PlayerControlAssignmentSet *set) const;
	C4PlayerControlAssignmentSet *GetSetByIndex(int32_t index);
	size_t GetSetCount() const { return Sets.size(); }
};

// contents of one PlayerControls.txt file
class C4PlayerControlFile
{
private:
	C4PlayerControlDefs ControlDefs;
	C4PlayerControlAssignmentSets AssignmentSets;
public:
	void Clear();
	void CompileFunc(StdCompiler *pComp);
	bool Load(C4Group &hGroup, const char *szFilename, C4LangStringTable *pLang);
	bool Save(C4Group &hGroup, const char *szFilename);

	const C4PlayerControlDefs &GetControlDefs() const { return ControlDefs; }
	const C4PlayerControlAssignmentSets &GetAssignmentSets() const { return AssignmentSets; }
};

// runtime information about a player's control
class C4PlayerControl
{
public:
	enum { MaxRecentKeyLookback = 3000, MaxSequenceKeyDelay = 800 }; // milliseconds: Time to press key combos

	enum ControlState {
		CONS_Down = 0,
		CONS_Up,
		CONS_Moved,
	};

private:
	C4PlayerControlDefs &ControlDefs; // shortcut

	// owner
	int32_t iPlr{-1};

	// async values
	C4PlayerControlAssignmentSet *pControlSet{nullptr}; // the control set used by this player - may be nullptr if the player cannot be controlled!
	typedef std::list<C4KeyBinding *> KeyBindingList;
	KeyBindingList KeyBindings;     // keys registered into Game.KeyboardInput
	C4PlayerControlRecentKeyList RecentKeys;           // keys pressed recently; for combinations
	C4PlayerControlRecentKeyList DownKeys;         // keys currently held down
	bool IsCursorPosRequested{false};                     // set to true when a SendCursorPos-control had been issued

public:
	// sync values
	struct CSync
	{
		struct ControlDownState
		{
			C4KeyEventData DownState, MovedState; // control is down if DownState.iStrength>0
			int32_t iDownFrame{0}, iMovedFrame; // frame when control was pressed
			bool fDownByUser{false};  // if true, the key is actually pressed. Otherwise, it's triggered as down by another key
			ControlDownState(const C4KeyEventData &rDownState, int32_t iDownFrame, bool fDownByUser)
					: DownState(rDownState), iDownFrame(iDownFrame), fDownByUser(fDownByUser) {}
			bool IsDown() const { return DownState.iStrength>0; }

			ControlDownState() : DownState() {}
			void CompileFunc(StdCompiler *pComp);
			bool operator ==(const ControlDownState &cmp) const;
		};
		typedef std::vector<ControlDownState> DownStateVec;
		DownStateVec ControlDownStates;           // indexed by C4PlayerControlID: Down-state of a control. 0=up, 100=down; values inbetween e.g. for gamepad sticks
		typedef std::vector<int32_t> DisableStateVec;
		DisableStateVec ControlDisableStates;        // indexed by C4PlayerControlID: Disable-states of controls. >0 is disabled.

		const ControlDownState *GetControlDownState(int32_t iControl) const;
		int32_t GetControlDisabled(int32_t iControl) const;
		bool IsControlDisabled(int32_t iControl) const { return GetControlDisabled(iControl)>0; }
		void SetControlDownState(int32_t iControl, const C4KeyEventData &rDownState, int32_t iDownFrame, bool fDownByUser);
		void SetControlMovedState(int32_t iControl, const C4KeyEventData &rMovedState, int32_t iMovedFrame);
		void ResetControlDownState(int32_t iControl);
		bool SetControlDisabled(int32_t iControl, int32_t iVal);

		void InitDefaults(const C4PlayerControlDefs &ControlDefs);
		void Clear();
		void CompileFunc(StdCompiler *pComp);
		bool operator ==(const CSync &cmp) const;
	};

private:
	CSync Sync;

	// callbacks from Game.KeyboardInput
	bool ProcessKeyEvent(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key, ControlState state, const C4KeyEventData &rKeyExtraData, bool reset_down_states_only=false, bool *clear_recent_keys=nullptr);
	bool ProcessKeyDown(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key);
	bool ProcessKeyUp(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key);
	bool ProcessKeyMoved(const C4KeyCodeEx &pressed_key, const C4KeyCodeEx &matched_key);

	// execute single control. return if handled.
	bool ExecuteControl(int32_t iControl, ControlState state, const C4KeyEventData &rKeyExtraData, int32_t iTriggerMode, bool fRepeated, bool fHandleDownStateOnly);
	bool ExecuteControlAction(int32_t iControl, C4PlayerControlDef::Actions eAction, C4ID idControlExtraData, ControlState state, const C4KeyEventData &rKeyExtraData, bool fRepeated);
	bool ExecuteControlScript(int32_t iControl, C4ID idControlExtraData, ControlState state, const C4KeyEventData &rKeyExtraData, bool fRepeated);

	// init
	void AddKeyBinding(const C4KeyCodeEx &key, bool fHoldKey, int32_t idx);

	// helper function: get current cursor position of controlling player in GUI coordinates
	// used e.g. to open menus at cursor pos
	bool GetCurrentPlayerCursorPos(int32_t *x_out, int32_t *y_out, int32_t *game_x_out, int32_t *game_y_out);

public:
	C4PlayerControl();
	~C4PlayerControl() { Clear(); }
	void Clear();

	// first-time init call after player join
	// not called again after control set change/savegame resume
	// does DefaultDisabled controls
	void Init();

	void CompileFunc(StdCompiler *pComp);

	void RegisterKeyset(int32_t iPlr, C4PlayerControlAssignmentSet *pKeyset); // register all keys into Game.KeyboardInput creating KeyBindings

	bool IsGlobal() const { return iPlr==-1; }
	const CSync::ControlDownState *GetControlDownState(int32_t iControl) const
	{ return Sync.GetControlDownState(iControl); }

	// callback from control queue
	void ExecuteControlPacket(const class C4ControlPlayerControl *pCtrl);

	// sync execution: Do keyrepeat, etc.
	void Execute();

	// mouse input
	bool DoMouseInput(uint8_t mouse_id, int32_t mouseevent, float game_x, float game_y, float gui_x, float gui_y, DWORD flags);

	// control enable/disable
	bool SetControlDisabled(int ctrl, bool is_disabled) { return Sync.SetControlDisabled(ctrl, is_disabled); }
	bool IsControlDisabled(int ctrl) const { return Sync.IsControlDisabled(ctrl); }

	// callback from C4GameControl when the next control packet is finalized
	void PrepareInput();

};


#endif // INC_C4PlayerControl
