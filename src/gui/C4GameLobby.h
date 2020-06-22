/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// the ingame-lobby

#ifndef INC_C4GameLobby
#define INC_C4GameLobby

#include "gui/C4Gui.h"
#include "network/C4PacketBase.h"

namespace C4GameLobby
{
	// countdown time from which the elevator sound starts and team selection becomes unavailable
	const int32_t AlmostStartCountdownTime = 10; // seconds

	extern bool UserAbort;

	// * PID_LobbyCountdown
	// initiates or aborts countdown
	class C4PacketCountdown : public C4PacketBase
	{
	private:
		int32_t iCountdown{Abort}; // countdown timer, or zero for abort
	public:
		enum { Abort = -1 };

		C4PacketCountdown(int32_t iaCountdown) : iCountdown(iaCountdown) { } // ctor
		C4PacketCountdown() = default; // std ctor

		bool IsAbort() const { return iCountdown == Abort; }
		int32_t GetCountdown() const { return iCountdown; }
		StdStrBuf GetCountdownMsg(bool fInitialMsg=false) const;

		void CompileFunc(StdCompiler *pComp) override;
	};

	class C4PacketSetScenarioParameter : public C4PacketBase
	{
	private:
		StdCopyStrBuf ID;
		int32_t Value{0};
	public:
		C4PacketSetScenarioParameter(const char *id, int32_t v) : ID(id), Value(v) {} // ctor
		C4PacketSetScenarioParameter() = default; // std ctor

		const char *GetID() const { return ID.getData(); }
		int32_t GetValue() const { return Value; }

		void CompileFunc(StdCompiler *pComp) override;
	};

	// scneario info tab: displays scenario description
	class ScenDesc : public C4GUI::Window, private C4ApplicationSec1Timer
	{
	private:
		C4GUI::TextWindow *pDescBox;     // scenario description box
		bool fDescFinished;              // if set, scenario resource has been loaded

		void Update();

	public:
		ScenDesc(const C4Rect &rcBounds, bool fActive); // ctor
		~ScenDesc() override { Deactivate(); }

		void OnSec1Timer() override { Update(); }
		// activate/deactivate periodic updates
		void Activate();
		void Deactivate();
	};

	class MainDlg : public C4GUI::FullscreenDialog, private C4ApplicationSec1Timer
	{
	private:
		enum CountdownState { CDS_None=0, CDS_LongCountdown=1, CDS_Countdown=2, CDS_Start=3 } eCountdownState; // nonzero when a packet was received that the game is about to start (starts elevator sound, etc.)
		int32_t iBackBufferIndex;   // chat message history index
		C4KeyBinding *pKeyHistoryUp, *pKeyHistoryDown; // keys used to scroll through chat history

		enum { SheetIdx_PlayerList = 0, SheetIdx_Res = 1, SheetIdx_Options = 2, SheetIdx_Scenario = 3 };
		C4PlayerInfoListBox *pPlayerList;
		C4Network2ResDlg *pResList;
		C4GameOptionButtons *pGameOptionButtons;
		C4GameOptionsList *pOptionsList;
		ScenDesc *pScenarioInfo;
		C4GUI::TextWindow *pChatBox;
		C4GUI::Label *pRightTabLbl;
		C4GUI::Tabular *pRightTab;
		C4GUI::Edit *pEdt; // chat input
		C4GUI::CallbackButton<MainDlg> *btnRun; // host only
		C4GUI::CallbackButton<MainDlg, C4GUI::IconButton> *btnPlayers, *btnResources, *btnTeams, *btnOptions, *btnScenario, *btnChat; // right list sheet selection
		C4GUI::CheckBox *checkReady;

	protected:
		void OnReadyCheck(C4GUI::Element *pCheckBox); // callback: checkbox ticked
		void OnRunBtn(C4GUI::Control *btn); // callback: run button pressed
		void OnExitBtn(C4GUI::Control *btn); // callback: exit button pressed
		bool KeyHistoryUpDown(bool fUp); // key callback
		C4GUI::Edit::InputResult OnChatInput(C4GUI::Edit *edt, bool fPasting, bool fPastingMore); // callback: chat input performed

		void OnClosed(bool fOK) override; // callback when dlg is closed
		void OnSec1Timer() override;              // timer proc; update pings

		C4GUI::ContextMenu *OnRightTabContext(C4GUI::Element *pLabel, int32_t iX, int32_t iY); // open context menu
		void OnCtxTabPlayers(C4GUI::Element *pListItem) { OnTabPlayers(nullptr); }
		void OnTabPlayers(C4GUI::Control *btn);
		void OnCtxTabTeams(C4GUI::Element *pListItem) { OnTabTeams(nullptr); }
		void OnTabTeams(C4GUI::Control *btn);
		void OnCtxTabRes(C4GUI::Element *pListItem) { OnTabRes(nullptr); }
		void OnTabRes(C4GUI::Control *btn);
		void OnCtxTabOptions(C4GUI::Element *pListItem) { OnTabOptions(nullptr); }
		void OnTabOptions(C4GUI::Control *btn);
		void OnCtxTabScenario(C4GUI::Element *pListItem) { OnTabScenario(nullptr); }
		void OnTabScenario(C4GUI::Control *btn);
		void UpdateRightTab(); // update label and tooltips for sheet change
		void OnBtnChat(C4GUI::Control *btn);

		class C4GUI::Control *GetDefaultControl() override { return pEdt; } // def focus chat input

	private:
		void SetCountdownState(CountdownState eToState, int32_t iTimer);
		int32_t ValidatedCountdownTime(int32_t iTimeout); // correct invalid timeout settings

		void UpdatePlayerList();

	public:
		MainDlg(bool fHost); // ctor
		~MainDlg() override; // dtor

		// callback by network system
		void OnClientJoin(C4Client *pNewClient); // called when a new client joined (connection not necessarily ready)
		void OnClientConnect(C4Client *pClient, C4Network2IOConnection *pConn); // called when new clinet connection is established (notice of lobby status)
		void OnClientPart(C4Client *pPartClient); // called when a client disconnects
		bool OnMessage(C4Client *pOfClient, const char *szMessage); // display message in chat window
		void OnClientSound(C4Client *pOfClient);                    // show that someone played a sound
		void OnLog(const char *szLogMsg, DWORD dwClr=C4GUI_LogFontClr); // log callback
		void OnError(const char *szErrMsg); // error sound + log in red
		void OnPlayersChange() { UpdatePlayerList(); }
		void OnClientReadyStateChange() { UpdatePlayerList(); }
		void OnClientAddPlayer(const char *szFilename, int32_t idClient);
		// packet callbacks from C4Network2
		void HandlePacket(char cStatus, const C4PacketBase *pBasePkt, C4Network2Client *pClient);
		void OnCountdownPacket(const C4PacketCountdown &Pkt); // called when a countdown packet is received: Update countdown state

		bool IsCountdown();
		void Start(int32_t iCountdownTime); // host only: Do game start with specified countdown time (forwards to network system)
		void UpdatePassword();
		void ClearLog();
	};

	// helper
	void LobbyError(const char *szErrorMsg);

	// lobby countdown: Moves game from lobby to go state. Only created by host.
	class Countdown : private C4ApplicationSec1Timer
	{
	private:
		int32_t iStartTimer;        // countdown timer for round start; 0 for not started, -1 for start overdue

	public:
		void OnSec1Timer() override;              // timer proc; count down; send important countdown packets

	public:
		Countdown(int32_t iStartTimer); // ctor: Init; sends initial countdown packet
		~Countdown() override;

		void Abort();
	};

}

#endif
