/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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
// Input to player control mapping

#ifndef INC_C4PlayerControl
#define INC_C4PlayerControl

#ifndef BIG_C4INCLUDE
#include <C4KeyboardInput.h>
#endif

// one control definition, e.g. "Left", "Throw", etc.
class C4PlayerControlDef
	{
	private:
		StdCopyStrBuf sIdentifier; // name as seen in script and config
		StdCopyStrBuf sGUIName;    // name as displayed to player
		StdCopyStrBuf sGUIDesc;    // key description displayed to player in config dialog
		bool fIsHoldKey;          // if true, the control can be in down and up state
		int32_t iRepeat;          // if >0, the key will generate successive events when held down
		bool fDefaultDisabled;    // if true, the control is disabled by default and needs to be enabled by script

	public:
		C4PlayerControlDef() : fIsHoldKey(false), fDefaultDisabled(false) {} 
		~C4PlayerControlDef() {};

		void CompileFunc(StdCompiler *pComp);

		const char *GetIdentifier() const { return sIdentifier.getData(); }
		const char *GetGUIName() const { return sGUIName.getData(); }
		const char *GetGUIDesc() const { return sGUIDesc.getData(); }

		//C4PlayerControlDef &operator =(const C4PlayerControlDef &src);
		bool operator ==(const C4PlayerControlDef &cmp) const;
	};

// CON_* constants are indices into the C4PlayerControlDefs list
enum { CON_None = -1 }; // No/undefined control

// list of all known player control definitions
class C4PlayerControlDefs
	{
	private:
		typedef std::vector<C4PlayerControlDef> DefVecImpl;
		DefVecImpl Defs;

	public:
		void CompileFunc(StdCompiler *pComp);
		void MergeFrom(const C4PlayerControlDefs &Src); // copy all defs from source file; overwrite defs of same name if found

		C4PlayerControlDef *GetControlByIndex(int32_t idx);
		int32_t GetControlIndexByIdentifier(const char *szIdentifier) const; // return CON_None for not found
	};

// a key/mouse/gamepad assignment to a PlayerControlDef
class C4PlayerControlAssignment
	{
	private:
		// KeyCombo list:
		// if size()>1, the control is triggered only if this combo is fulfilled
		// used for simultanuous keypresses or sequences
		struct KeyComboItem
		{
			C4KeyCodeEx Key;
			StdCopyStrBuf sKeyName;
			void CompileFunc(StdCompiler *pComp);
			bool operator ==(const KeyComboItem &cmp) const { return sKeyName==cmp.sKeyName; }
		};
		typedef std::vector<KeyComboItem> KeyComboVec;
		KeyComboVec KeyCombo;
		bool fComboIsSequence; // if true, the keys must be pressed in sequence. Otherwise, they must be pressed simultanuously

		// trigger key: key/mouse/gamepad event triggering this assignment. For combinations, the last key of the combo.
		C4KeyCodeEx TriggerKey;

		StdCopyStrBuf sControlName; // name of the control to be executed on this key
		int32_t iControl; // the control to be executed on this key, i.e. the resolved sControlName
		bool fAlwaysUnhandled;      // if true, the key will not block handling of other keys even if it got handled

		// action to be performed on the control upon this key
		enum TriggerModes
			{
			CTM_Default=0,          // standard behaviour: The control will be triggered
			CTM_Hold,               // the control will be put into "down"-mode
			CTM_Release,            // the hold mode of the control will be released
			} eTriggerMode;

	public:
		C4PlayerControlAssignment() : TriggerKey(), iControl(CON_None), fAlwaysUnhandled(false), eTriggerMode(CTM_Default) {}
		~C4PlayerControlAssignment();

		void CompileFunc(StdCompiler *pComp);
		void ResolveRefs(C4PlayerControlDefs *pControlDefs); // resolve references between assignments

		bool operator ==(const C4PlayerControlAssignment &cmp) const; // doesn't compare resolved TriggerKey/iControl
		const char *GetControlName() const { return sControlName.getData(); }
	};

// a set of key/mouse/gamepad assignments to all controls
class C4PlayerControlAssignmentSet
	{
	private:
		StdCopyStrBuf sName;
		typedef std::vector<C4PlayerControlAssignment> AssignmentsVec;
		AssignmentsVec Assignments;

	public:
		C4PlayerControlAssignmentSet() {}
		~C4PlayerControlAssignmentSet() {}

		void CompileFunc(StdCompiler *pComp);
		void ResolveRefs(C4PlayerControlDefs *pControlDefs); // resolve references between assignments

		void MergeFrom(const C4PlayerControlAssignmentSet &Src, bool fLowPrio); // take over all assignments defined in Src

		const char *GetName() const { return sName.getData(); }

		C4PlayerControlAssignment *GetAssignmentByControlName(const char *szControlName) const;
	};

// list of C4PlayerControlAssignmentSet
class C4PlayerControlAssignmentSets
	{
	private:
		typedef std::list<C4PlayerControlAssignmentSet> AssignmentSetList;

	public:
		C4PlayerControlAssignmentSets() {}
		~C4PlayerControlAssignmentSets() {}

		void CompileFunc(StdCompiler *pComp);

		void MergeFrom(const C4PlayerControlAssignmentSets &Src, bool fLowPrio); // take over all assignments in known sets and new sets defined in Src
	};

// contents of one PlayerControls.txt file
class C4PlayerControlFile
	{
	private:
		C4PlayerControlDef ControlDefs;
		C4PlayerControlAssignmentSets AssignmentSets;
	public:
		bool Load(C4Group &hGroup, const char *szFilename);

		const C4PlayerControlDef &GetControlDefs() const { return ControlDefs; }
		const C4PlayerControlAssignmentSets &GetAssignmentSets() const { return AssignmentSets; }
	};

// runtime information about a player's control
class C4PlayerControl
	{
	private:
		struct RecentKey
			{
			C4KeyCodeEx Key;
			int32_t iFrame;
			};
		// shortcut
		C4PlayerControlDefs &ControlDefs;

		// async values
		C4PlayerControlAssignmentSet *pControlSet; // the control set used by this player
		std::list<C4KeyBinding *> KeyBindings;     // keys registered into Game.KeyboardInput
		std::list<RecentKey> RecentKeys;           // keys pressed recently; for combinations
		std::vector<C4KeyCodeEx> DownKeys;         // keys currently held down

		// sync values
		struct
			{
			std::vector<int32_t> ControlDownStates;           // indexed by C4PlayerControlID: Down-state of a control. 0=up, 100=down; values inbetween e.g. for gamepad sticks
			std::vector<int32_t> ControlDisableStates;        // indexed by C4PlayerControlID: Disable-states of controls. >0 is disabled.

			void CompileFunc(StdCompiler *pComp);
			} Sync;

		// callbacks from Game.KeyboardInput
		bool ProcessKeyPress(C4KeyCodeEx key, int32_t iKeyIndex);
		bool ProcessKeyDown(C4KeyCodeEx key, int32_t iKeyIndex);
		bool ProcessKeyUp(C4KeyCodeEx key, int32_t iKeyIndex);

	public:
		C4PlayerControl();
		~C4PlayerControl() { Clear(); }
		void Clear();

		void RegisterKeyset(C4PlayerControlAssignmentSet *pKeyset); // register all keys into Game.KeyboardInput creating KeyBindings
	};


#endif // INC_C4PlayerControl