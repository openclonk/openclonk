/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2007-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// IRC client dialog

#ifndef INC_C4ChatDlg
#define INC_C4ChatDlg

#include "gui/C4Gui.h"
#include "network/C4InteractiveThread.h"

// a GUI control to chat in
class C4ChatControl : public C4GUI::Window, private C4InteractiveThread::Callback
{
private:
	// one open channel or query
	class ChatSheet : public C4GUI::Tabular::Sheet
	{
	public:
		// one item in the nick list
		class NickItem : public C4GUI::Window
		{
		private:
			C4GUI::Icon *pStatusIcon; // status icon indicating channel status (e.g. operator)
			C4GUI::Label *pNameLabel; // the nickname
			bool fFlaggedExisting; // flagged for existance; used by user update func
			int32_t iStatus;

		public:
			NickItem(class C4Network2IRCUser *pByUser);

		protected:
			void UpdateOwnPos() override;

		public:
			const char *GetNick() const { return pNameLabel->GetText(); }
			int32_t GetStatus() const { return iStatus; }

			void Update(class C4Network2IRCUser *pByUser);
			void SetFlaggedExisting(bool fToVal) { fFlaggedExisting = fToVal; }
			bool IsFlaggedExisting() const { return fFlaggedExisting; }

			static int32_t SortFunc(const C4GUI::Element *pEl1, const C4GUI::Element *pEl2, void *);

		};

		enum SheetType { CS_Server, CS_Channel, CS_Query };
	private:
		C4ChatControl *pChatControl;
		C4GUI::TextWindow *pChatBox;
		C4GUI::ListBox *pNickList;
		C4GUI::WoodenLabel *pInputLbl;
		C4GUI::Edit *pInputEdit;
		int32_t iBackBufferIndex;   // chat message history index
		SheetType eType;
		StdStrBuf sIdent;
		bool fHasUnread;
		StdStrBuf sChatTitle; // topic for channels; name+ident for queries; server name for server sheet

		C4KeyBinding *pKeyHistoryUp, *pKeyHistoryDown; // keys used to scroll through chat history

	public:
		ChatSheet(C4ChatControl *pChatControl, const char *szTitle, const char *szIdent, SheetType eType);
		~ChatSheet() override;

		C4GUI::Edit *GetInputEdit() const { return pInputEdit; }
		SheetType GetSheetType() const { return eType; }
		const char *GetIdent() const { return sIdent.getData(); }
		void SetIdent(const char *szToIdent) { sIdent.Copy(szToIdent); }
		const char *GetChatTitle() const { return sChatTitle.getData(); }
		void SetChatTitle(const char *szNewTitle) { sChatTitle.Copy(szNewTitle); }

		void AddTextLine(const char *szText, uint32_t dwClr);
		void DoError(const char *szError);
		void Update(bool fLock);
		void UpdateUsers(C4Network2IRCUser *pUsers);
		void ResetUnread(); // mark messages as read

	protected:
		void UpdateSize() override;
		void OnShown(bool fByUser) override;
		void UserClose() override; // user pressed close button: Close queries, part channels, etc.

		C4GUI::Edit::InputResult OnChatInput(C4GUI::Edit *edt, bool fPasting, bool fPastingMore);
		bool KeyHistoryUpDown(bool fUp);
		void OnNickDblClick(class C4GUI::Element *pEl);

	private:
		NickItem *GetNickItem(const char *szByNick);
		NickItem *GetFirstNickItem() { return pNickList ? static_cast<NickItem *>(pNickList->GetFirst()) : nullptr; }
		NickItem *GetNextNickItem(NickItem *pPrev) { return static_cast<NickItem *>(pPrev->GetNext()); }
	};

private:
	C4GUI::Tabular *pTabMain, *pTabChats;
	// login controls
	C4GUI::Label *pLblLoginNick, *pLblLoginPass, *pLblLoginRealName, *pLblLoginChannel;
	C4GUI::Edit *pEdtLoginNick, *pEdtLoginPass, *pEdtLoginRealName, *pEdtLoginChannel;
	C4GUI::Button *pBtnLogin;

	StdStrBuf sTitle;
	C4GUI::BaseInputCallback *pTitleChangeBC;

	C4Network2IRCClient *pIRCClient;
	bool fInitialMessagesReceived; // set after initial update call, which fetches all messages. Subsequent calls fetch only unread messages

public:
	C4ChatControl(C4Network2IRCClient *pIRC);
	~C4ChatControl() override;

protected:
	void UpdateSize() override;
	C4GUI::Edit::InputResult OnLoginDataEnter(C4GUI::Edit *edt, bool fPasting, bool fPastingMore); // advance focus when user presses enter in one of the login edits
	void OnConnectBtn(C4GUI::Control *btn); // callback: connect button pressed

public:
	virtual class C4GUI::Control *GetDefaultControl();
	C4Network2IRCClient *getIRCClient() { return pIRCClient; }

	void SetTitleChangeCB(C4GUI::BaseInputCallback *pNewCB);
	virtual void OnShown(); // callback when shown
	void UpdateShownPage();
	void Update();
	void UpdateTitle();
	bool DlgEnter();
	void OnSec1Timer() { Update(); }              // timer proc
	bool ProcessInput(const char *szInput, ChatSheet *pChatSheet); // process chat input - return false if no more pasting is to be done (i.e., on /quit or /part in channel)
	const char *GetTitle() { return sTitle.getData(); }
	void UserQueryQuit();
	ChatSheet *OpenQuery(const char *szForNick, bool fSelect, const char *szIdentFallback);

private:
	static bool IsServiceName(const char *szName);
	ChatSheet *GetActiveChatSheet();
	ChatSheet *GetSheetByTitle(const char *szTitle, C4ChatControl::ChatSheet::SheetType eType);
	ChatSheet *GetSheetByIdent(const char *szIdent, C4ChatControl::ChatSheet::SheetType eType);
	ChatSheet *GetServerSheet();
	void ClearChatSheets();

	// IRC event hook
	void OnThreadEvent(C4InteractiveEventType eEvent, void *pEventData) override { if (pEventData == pIRCClient) Update(); }

};

// container dialog for the C4ChatControl
class C4ChatDlg : public C4GUI::Dialog
{
private:
	static C4ChatDlg *pInstance;
	class C4ChatControl *pChatCtrl;

public:
	C4ChatDlg();
	~C4ChatDlg() override;

	static C4ChatDlg *ShowChat();
	static void StopChat();
	static bool IsChatActive();
	static bool IsChatEnabled();
	static bool ToggleChat();

protected:
	// default control to be set if unprocessed keyboard input has been detected
	class C4GUI::Control *GetDefaultControl() override;

	// true for dialogs that should span the whole screen
	// not just the mouse-viewport
	bool IsFreePlaceDialog() override { return true; }

	// true for dialogs that receive keyboard input even in shared mode
	bool IsExclusiveDialog() override { return true; }

	// for custom placement procedures; should call SetPos
	bool DoPlacement(C4GUI::Screen *pOnScreen, const C4Rect &rPreferredDlgRect) override;

	void OnClosed(bool fOK) override;    // callback when dlg got closed
	void OnShown() override;             // callback when shown - should not delete the dialog

	void UpdateSize() override;

	void OnExitBtn(C4GUI::Control *btn); // callback: exit button pressed
	void OnChatTitleChange(const StdStrBuf &sNewTitle);
};

#endif // INC_C4ChatDlg
