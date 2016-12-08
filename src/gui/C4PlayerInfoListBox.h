/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2008-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// player listbox used in lobby and game over dlg

#ifndef INC_C4PlayerInfoListBox
#define INC_C4PlayerInfoListBox

#include "gui/C4Gui.h"

class C4PlayerInfoListBox : public C4GUI::ListBox
{
public:
	enum Spacings
	{
		// some spacings
		IconLabelSpacing     = 2, // space between an icon and its text
		ClientListBoxSpacing = 8, // space between two clients in the list
		PlayerListBoxIndent  = 3, // indent of player list box items
		NoneLabelSpacing     = 20, // indent of "- none -"-label in list box

		SoundIconShowTime    = 1  // seconds. min time a sound icon is shown
	};

	// list box mode
	enum Mode
	{
		PILBM_LobbyClientSort,
		PILBM_LobbyTeamSort,
		PILBM_Evaluation,
		PILBM_EvaluationNoWinners
	};

private:
	// generic player list item
	class ListItem : public C4GUI::Window
	{
	protected:
		C4PlayerInfoListBox *pList;
		uint32_t dwBackground; // not drawn if 0

	public:
		struct ID
		{
			enum IDType
			{
				PLI_NONE=0,
				PLI_SCRIPTPLR,    // script player caption (ID=0) - script players themselbed are regular PLI_PLAYER
				PLI_SAVEGAMEPLR,  // restore savegame player (ID>0), or caption (ID=0)
				PLI_PLAYER,       // player
				PLI_CLIENT,       // client label
				PLI_TEAM,         // team label
				PLI_REPLAY        // replay player (ID>0), or caption (ID=0)
			} idType;

			int32_t id; // player file ID or team ID or client ID

			ID() : idType(PLI_NONE), id(0) {}
			ID(IDType eType, int32_t id) : idType(eType), id(id) {}

			inline bool operator ==(const ID &r2)
			{
				return idType == r2.idType && id == r2.id;
			}
		};

		ID idListItemID;

		C4GameLobby::MainDlg *GetLobby() const;
		virtual void Update() {} // periodic update callback

	protected:
		virtual void DrawElement(C4TargetFacet &cgo); // draw background
		bool CanLocalChooseTeams(int32_t idPlayer=0) const; // whether the local client can change any teams

	public:
		ListItem(C4PlayerInfoListBox *pList) : C4GUI::Window(), pList(pList), dwBackground(0) {}
	};

	// lobby information and display of joined players
	class PlayerListItem : public ListItem
	{
	private:
		// subcomponents
		C4GUI::Icon *pIcon;       // player icon
		C4GUI::Label *pNameLabel; // label indicating player name
		C4GUI::Label *pScoreLabel; // label showing some player score (league)
		C4GUI::Label *pTimeLabel; // evaluation only: label showing total playing time
		C4GUI::Label *pExtraLabel; // evaluation only: label showing extra data set by script
		C4GUI::Icon *pRankIcon;    // league rank icon
		C4GUI::ComboBox *pTeamCombo; // team selection combo - nullptr for no-team-scens
		C4GUI::Picture *pTeamPic; // evaluation only: Team icon spec

		bool fIconSet;            // whether custom icon has been set
		bool fJoinedInfoSet;      // join info for savegame recreation
		DWORD dwJoinClr, dwPlrClr;//colors currently reflected in icon

	protected:
		int32_t idClient, idPlayer;                     // referenced IDs
		bool fFreeSavegamePlayer;                   // if set, the player is an (unassociated) savegame player
		bool fShownCollapsed;                       // true if small view is shown

	protected:
		virtual void UpdateOwnPos(); // recalculate item positioning
		virtual int32_t GetListItemTopSpacing();

	public:
		PlayerListItem(C4PlayerInfoListBox *pForListBox, int32_t idClient, int32_t idPlayer, bool fSavegamePlayer, C4GUI::Element *pInsertBeforeElement); // ctor
		~PlayerListItem() {};                         // dtor

		void UpdateIcon(C4PlayerInfo *pInfo, C4PlayerInfo *pJoinedInfo); // update player icon
		void UpdateTeam();
		void UpdateScoreLabel(C4PlayerInfo *pInfo); // update league score labels and icons
		void UpdateCollapsed();

	public:
		C4GUI::ContextMenu *OnContext(C4GUI::Element *pListItem, int32_t iX, int32_t iY); // open context menu
		C4GUI::ContextMenu *OnContextTakeOver(C4GUI::Element *pListItem, int32_t iX, int32_t iY); // takeover savegame player submenu
		void OnCtxTakeOver(C4GUI::Element *pListItem, const int32_t &idPlayer);
		void OnCtxRemove(C4GUI::Element *pListItem);
		void OnCtxNewColor(C4GUI::Element *pListItem);

		void OnTeamComboFill(C4GUI::ComboBox_FillCB *pFiller);
		bool OnTeamComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);

		virtual void Update(); // update icon and team

		int32_t GetInfoID() const { return idPlayer; }
		C4Network2Client *GetNetClient() const; // return associated network client
		C4PlayerInfo *GetPlayerInfo() const;

		C4PlayerInfo *GetJoinedInfo() const; // if this player is joined or associated to a joined info, return the joined info

		bool IsLocalClientPlayer() const; // whether this player is going to join locally
		bool CanLocalChooseTeam() const;  // whether the local client can change team for this player

	};

	// lobby information and display of connected clients
	class ClientListItem : public ListItem
	{
	private:
		// subcomponents
		C4GUI::Icon *pStatusIcon; // icon indicating client status (host, etc.)
		C4GUI::Label *pNameLabel; // label indicating client name
		C4GUI::Label *pPingLabel; // label indicating ping to client - may be nullptr

	protected:
		int32_t idClient;                     // associated network interface ID
		DWORD dwClientClr;                    // client color used for chatting

		bool fIsShownActive;   // whether client was active in last update
		time_t tLastSoundTime; // now() when the client last issued a sound (display as sound icon). 0 for no sound.

	public:
		ClientListItem(C4PlayerInfoListBox *pForListBox, const C4ClientCore &rClientInfo, ListItem *pInsertBefore); // ctor
		~ClientListItem() {}                         // dtor

		void SetColor(DWORD dwToClr)              // update color of client name label
		{ pNameLabel->SetColor((dwClientClr=dwToClr) | C4GUI_MessageFontAlpha); }
		void SetStatus(C4GUI::Icons icoNewStatus) // set new status
		{ pStatusIcon->SetIcon(icoNewStatus); }
		void SetPing(int32_t iToPing);                // update ping label; iToPing=-1 removes the label
		void UpdateCore(const C4ClientCore &rCore); // core update: re-set name and indicate whether active or not
		void SetSoundIcon();                        // sets the sound icon as current icon and schedules reset after some time

		// spacing inserted between two client list items
		virtual int32_t GetListItemTopSpacing() { return ClientListBoxSpacing; }
		virtual bool GetListItemTopSpacingBar() { return true; }

	public:
		void Init(const C4ClientCore &rClientInfo); // init members

		void UpdateInfo();                                  // update for changed player info

		DWORD GetColor() const { return dwClientClr; } // client chat color
		C4Client *GetClient() const; // get client associated with this list item
		bool IsLocalClientPlayer() const; // whether this player is going to join locally
		C4GUI::Icons GetCurrentStatusIcon();              // get status icon that shows the current client state
		class C4Network2Client *GetNetClient() const; // return assicuated network client; nullptr for local
		bool IsLocal() const;

		virtual void Update() { UpdatePing(); UpdateInfo(); }
		void UpdatePing();                     // update ping label

		C4GUI::ContextMenu *OnContext(C4GUI::Element *pListItem, int32_t iX, int32_t iY); // open context menu
		void OnCtxKick(C4GUI::Element *pListItem); // kick item selected in client ctx menu
		void OnCtxActivate(C4GUI::Element *pListItem); // toggle player/observer
		void OnCtxInfo(C4GUI::Element *pListItem); // show info dlg (modal)
		void OnCtxIgnore(C4GUI::Element *pListItem);
		void OnBtnAddPlr(C4GUI::Control *btn);
	};

	// team label
	class TeamListItem : public ListItem
	{
	private:
		// subcomponents
		C4GUI::Icon *pIcon;
		C4GUI::Label *pNameLabel;

		int32_t idTeam; // team ID

	protected:
		virtual void UpdateOwnPos(); // recalculate item positioning

	public:
		TeamListItem(C4PlayerInfoListBox *pForListBox, int32_t idTeam, ListItem *pInsertBefore); // ctor

		virtual void MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam); // input: mouse movement or buttons

		// spacing inserted between of those list items; usually this is top anyway...
		virtual bool GetListItemTopSpacingBar() { return true; }
		virtual int32_t GetListItemTopSpacing() { return ClientListBoxSpacing; }

		virtual void Update();

	private:
		void MoveLocalPlayersIntoTeam(); // move all local players into team marked by this label
	};

	// list of unassigned savegame recreation players
	class FreeSavegamePlayersListItem : public ListItem
	{
	private:
		// subcomponents
		C4GUI::Icon *pIcon;
		C4GUI::Label *pNameLabel;

	public:
		FreeSavegamePlayersListItem(C4PlayerInfoListBox *pForListBox, ListItem *pInsertBefore); // ctor

		// spacing inserted between of those list items; usually this is top anyway...
		virtual bool ListItemTopSpacingBar() { return true; }
		virtual int32_t GetListItemTopSpacing() { return ClientListBoxSpacing; }

	public:
		virtual void Update();
	};

	// list of script players (both savegame recreation and regular)
	class ScriptPlayersListItem : public ListItem
	{
	private:
		// subcomponents
		C4GUI::Icon *pIcon;
		C4GUI::Label *pNameLabel;
		C4GUI::IconButton *btnAddPlayer;

	public:
		ScriptPlayersListItem(C4PlayerInfoListBox *pForListBox, ListItem *pInsertBefore); // ctor

		// spacing inserted between of those list items; usually this is top anyway...
		virtual bool ListItemTopSpacingBar() { return true; }
		virtual int32_t GetListItemTopSpacing() { return ClientListBoxSpacing; }

	protected:
		void OnBtnAddPlr(C4GUI::Control *btn);

	public:
		virtual void Update();
	};

	// list of players in record (currently not used, because replays in network are not allowed :C)
	class ReplayPlayersListItem : public ListItem
	{
	private:
		// subcomponents
		C4GUI::Icon *pIcon;
		C4GUI::Label *pNameLabel;

	public:
		ReplayPlayersListItem(C4PlayerInfoListBox *pForListBox, ListItem *pInsertBefore); // ctor

		// spacing inserted between of those list items; usually this is top anyway...
		virtual bool GetListItemTopSpacingBar() { return true; }
		virtual int32_t GetListItemTopSpacing() { return ClientListBoxSpacing; }
	};

	// -----------------------------------------------------------------------------------------------
private:
	Mode eMode;
	int32_t iMaxUncollapsedPlayers; // maximum number of players that can be displayed without collapse - valid only if fIsCollapsed
	bool fIsCollapsed;
	int32_t iTeamFilter; // if nonzero, only playeers of this team are shown in the listbox

	uint32_t dwTextColor;
	CStdFont *pCustomFont;

	enum AddMode
	{
		AM_Winners,
		AM_Losers,
		AM_All
	};

	void UpdateSavegamePlayers(ListItem **ppCurrInList);
	void UpdateReplayPlayers(ListItem **ppCurrInList);
	void UpdateScriptPlayers(ListItem **ppCurrInList);
	void UpdatePlayersByTeam(ListItem **ppCurrInList);
	void UpdatePlayersByRandomTeam(ListItem **ppCurrInList);
	void UpdatePlayersByClient(ListItem **ppCurrInList);
	void UpdatePlayersByEvaluation(ListItem **ppCurrInList, bool fShowWinners);
	void UpdatePlayersByEvaluation(ListItem **ppCurrInList, C4Team *pTeam, AddMode eAddMode);

	ListItem *GetPlayerListItem(ListItem::ID::IDType eType, int32_t id); // search for a player list item
	bool PlrListItemUpdate(ListItem::ID::IDType eType, int32_t id, class ListItem **pEnsurePos); // search for player list item with given ID in the list starting at ensurepos; ensure it's positioned at given pos; update and return true if found

	bool IsEvaluation() const { return eMode == PILBM_Evaluation || eMode == PILBM_EvaluationNoWinners; }
	bool IsLobby() const { return eMode == PILBM_LobbyClientSort || eMode == PILBM_LobbyTeamSort; }
	bool IsTeamFilter() const { return !!iTeamFilter; }

protected:
	bool IsPlayerItemCollapsed(PlayerListItem *pItem); // CB from list item: return true if it should be shown small
	void OnPlrListSelChange(class C4GUI::Element *pEl) { Update(); }

	uint32_t GetTextColor() const { return dwTextColor; }
	CStdFont *GetCustomFont() const { return pCustomFont; }

public:
	C4PlayerInfoListBox(const C4Rect &rcBounds, Mode eMode, int32_t iTeamFilter=0);
	virtual ~C4PlayerInfoListBox() {}

	void Update(); // update player list
	void SetClientSoundIcon(int32_t iForClientID);
	void SetMode(Mode eNewMode);
	void SetTeamFilter(int32_t idNewTeamFilter) { iTeamFilter = idNewTeamFilter; }
	void SetCustomFont(CStdFont *pNewFont, uint32_t dwTextColor);

	Mode GetMode() const { return eMode; }

	friend class PlayerListItem;
	friend class TeamListItem;
};


#endif // INC_C4PlayerInfoListBox
