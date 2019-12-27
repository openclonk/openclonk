/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2013-2016, The OpenClonk Team and contributors
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
// handles input dialogs, last-message-buffer, MessageBoard-commands

#ifndef INC_C4MessageInput
#define INC_C4MessageInput

#include "gui/C4Gui.h"
#include "object/C4ObjectPtr.h"

const int32_t C4MSGB_BackBufferMax  = 20;

// chat input dialog
class C4ChatInputDialog : public C4GUI::InputDialog
{
private:
	typedef C4GUI::InputDialog BaseClass;
	class C4KeyBinding *pKeyHistoryUp, *pKeyHistoryDown, *pKeyAbort, *pKeyNickComplete, *pKeyPlrControl, *pKeyGamepadControl, *pKeyBackClose;

	bool fObjInput;    // input queried by script?
	bool fUppercase;   // script input converted to uppercase
	class C4Object *pTarget; // target object for script callback
	int32_t iPlr;      // target player for script callback

	// last message lookup
	int32_t BackIndex;

	bool fProcessed; // set if chat input has been processed

	static C4ChatInputDialog *pInstance; // singleton-instance

private:
	bool KeyHistoryUpDown(bool fUp);
	bool KeyCompleteNick(); // complete nick at cursor pos of edit
	bool KeyPlrControl(const C4KeyCodeEx &key);
	bool KeyGamepadControlDown(const C4KeyCodeEx &key);
	bool KeyGamepadControlUp(const C4KeyCodeEx &key);
	bool KeyGamepadControlPressed(const C4KeyCodeEx &key);
	bool KeyBackspaceClose(); // close if chat text box is empty (on backspace)

protected:
	// chat input callback
	C4GUI::Edit::InputResult OnChatInput(C4GUI::Edit *edt, bool fPasting, bool fPastingMore);
	void OnChatCancel();
	void OnClosed(bool fOK) override;

	const char *GetID() override { return "ChatDialog"; }
public:
	C4ChatInputDialog(bool fObjInput, C4Object *pScriptTarget, bool fUpperCase, bool fTeam, int32_t iPlr, const StdStrBuf &rsInputQuery); // ctor - construct by screen ratios
	~C4ChatInputDialog() override;

	// place on top of normal dialogs
	int32_t GetZOrdering() override { return C4GUI_Z_CHAT; }

	// align by screen, not viewport
	bool IsFreePlaceDialog() override { return true; }

	// place more to the bottom of the screen
	bool IsBottomPlacementDialog() override { return true; }

	// true for dialogs that receive full keyboard and mouse input even in shared mode
	bool IsExclusiveDialog() override { return true; }

	// don't enable mouse just for this dlg
	bool IsMouseControlled() override { return false; }

	// usually processed by edit;
	// but may reach this if the user managed to deselect the edit control
	bool OnEnter() override { OnChatInput(pEdit, false, false); return true; }

	static bool IsShown() { return !!pInstance; } // external query fn whether dlg is visible
	static C4ChatInputDialog *GetInstance() { return pInstance; }

	bool IsScriptQueried() const { return fObjInput; }
	class C4Object *GetScriptTargetObject() const { return pTarget; }
	int32_t GetScriptTargetPlayer() const { return iPlr; }
};


class C4MessageBoardCommand
{
public:
	C4MessageBoardCommand();

public:
	char Name[C4MaxName + 1];
	char Script[_MAX_FNAME_LEN+30];

	C4MessageBoardCommand *Next;
};


class C4MessageInput
{
public:
	C4MessageInput() { Default(); }
	~C4MessageInput() { Clear(); }
	void Default();
	void Clear();
	bool Init();

private:
	// last input messages to be accessed via 'up'/'down' in input dialog
	char BackBuffer[C4MSGB_BackBufferMax][C4MaxMessage];

	// MessageBoard-commands
private:
	class C4MessageBoardCommand *pCommands{nullptr};
public:
	void AddCommand(const char *strCommand, const char *strScript);
	class C4MessageBoardCommand *GetCommand(const char *strName);

	// Input
public:
	bool CloseTypeIn();
	bool StartTypeIn(bool fObjInput = false, C4Object *pObj = nullptr, bool fUpperCase = false, bool fTeam = false, int32_t iPlr = -1, const StdStrBuf &rsInputQuery = StdStrBuf());
	bool KeyStartTypeIn(bool fTeam);
	bool ToggleTypeIn();
	bool IsTypeIn();
	C4ChatInputDialog *GetTypeIn() { return C4ChatInputDialog::GetInstance(); }
	void StoreBackBuffer(const char *szMessage);
	const char *GetBackBuffer(int32_t iIndex);
	bool ProcessInput(const char *szText);
	bool ProcessCommand(const char *szCommand);

public:
	void ClearPointers(C4Object *pObj);
	void AbortMsgBoardQuery(C4Object *pObj, int32_t iPlr);

	friend class C4ChatInputDialog;
};

extern C4MessageInput MessageInput;

// script query to ask a player for a string
class C4MessageBoardQuery
{
public:
	C4ObjectPtr CallbackObj; // callback target object
	StdStrBuf sInputQuery;   // question being asked to the player
	bool fAnswered{false};          // if set, an answer packet is in the queue (NOSAVE, as the queue isn't saved either!)
	bool fIsUppercase{false};       // if set, any input is converted to uppercase be4 sending to script

	// linked list to allow for multiple queries
	C4MessageBoardQuery *pNext{nullptr};

	// ctors
	C4MessageBoardQuery(C4Object *pCallbackObj, const StdStrBuf &rsInputQuery, bool fIsUppercase)
			: CallbackObj(pCallbackObj), fIsUppercase(fIsUppercase), pNext(nullptr)
	{
		sInputQuery.Copy(rsInputQuery);
	}

	C4MessageBoardQuery() : CallbackObj(nullptr) {}

	// use default copy ctor


	// compilation
	void CompileFunc(StdCompiler *pComp);
};


#endif // INC_C4MessageInput
