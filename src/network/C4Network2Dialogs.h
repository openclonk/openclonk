/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2004-2009, RedWolf Design GmbH, http://www.clonk.de/
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
// dialogs for network information

#ifndef INC_C4Network2Dialogs
#define INC_C4Network2Dialogs

#include "gui/C4Gui.h"

#include "landscape/C4Scenario.h"
#include "network/C4Network2Res.h"

class C4Graph;

// dialog showing info about a connected client
class C4Network2ClientDlg : public C4GUI::InfoDialog
{
protected:
	int iClientID; // ID of client info is shown about

protected:
	void UpdateText() override; // compose message text
	virtual int GetUpdateInterval() { return 500; } // update every 0.5 seconds

	const char *GetID() override { return "ClientDialog"; }

public:
	C4Network2ClientDlg(int iForClientID); // ctor
};

// resource dialog: created as listbox inside another dialog
class C4Network2ResDlg : public C4GUI::ListBox, private C4ApplicationSec1Timer
{
public:
	enum { IconLabelSpacing = 2 }; // space between an icon and its text

private:
	class ListItem : public C4GUI::Window
	{
	private:
		// subcomponents
		C4GUI::Icon *pFileIcon;     // icon indicating file type
		C4GUI::Label *pLabel; // label indicating file name
		C4GUI::Label *pProgress; // label indicating file progress
		C4GUI::IconButton *pSaveBtn; // file save button
		int iResID, iProgress;

		void LocalSaveResource(bool fDoOverwrite); // called by OnButtonSave*: queried local saving of resource

	public:
		ListItem(class C4Network2ResDlg *pForResDlg, const class C4Network2Res *pByRes); // ctor

		void Update(const class C4Network2Res *pByRes); // update data

		int GetResID() const { return iResID; }
		bool IsSavePossible();

		C4GUI::ContextMenu *OnContext(C4GUI::Element *pListItem, int iX, int iY); // open context menu
		void OnButtonSave(C4GUI::Control *pButton);      // queried resource save
		void OnButtonSaveConfirm(C4GUI::Element *pNull); // confirmed file overwrite

		C4Network2Res::Ref GetRefRes(); // get associated resource
	};

public:
	C4Network2ResDlg(const C4Rect &rcBounds, bool fActive);
	~C4Network2ResDlg() override { Deactivate(); }

	// enable/disable updates by timer calls
	void Activate(); void Deactivate();

	// update by resources
	void OnSec1Timer() override { Update(); }
	void Update();
};

// client list dialog: created as listbox inside another dialog
class C4Network2ClientListBox : public C4GUI::ListBox, private C4ApplicationSec1Timer
{
public:
	enum {
		IconLabelSpacing  = 2,
		SoundIconShowTime = 1 // seconds. min time a sound icon is shown}; // space between an icon and its text
	};

private:
	class ListItem : public C4GUI::Window
	{
	protected:
		int32_t iClientID;
		C4Network2ClientListBox *pForDlg;

	public:
		ListItem(class C4Network2ClientListBox *pForDlg, int32_t iClientID) // ctor
				: iClientID(iClientID), pForDlg(pForDlg) {}

		virtual void Update() = 0;
		int32_t GetClientID() const { return iClientID; }
		virtual int32_t GetConnectionID() const { return -1; }
	};

	class ClientListItem : public ListItem
	{
	private:
		// subcomponents
		C4GUI::Icon *pStatusIcon;  // client network status
		C4GUI::Label *pName;       // client name
		C4GUI::Label *pPing;       // client control ping
		C4GUI::IconButton *pActivateBtn, *pKickBtn; // buttons for host
		bool fShownActive;
		time_t last_sound_time; // now() when the client last issued a sound (display as sound icon). 0 for no sound.

	public:
		ClientListItem(class C4Network2ClientListBox *pForDlg, int iClientID); // ctor

		void Update() override; // update data
		const C4Client *GetClient() const; // get client by associated ID

		void OnButtonActivate(C4GUI::Control *pButton);
		void OnButtonKick(C4GUI::Control *pButton);

		void SetSoundIcon();
	};

	class ConnectionListItem : public ListItem
	{
	private:
		int32_t iConnID; // connection ID
		// subcomponents
		C4GUI::Label *pDesc;       // connection description
		C4GUI::Label *pPing;       // connection ping
		C4GUI::IconButton *pDisconnectBtn; // button to destroy connection

	public:
		ConnectionListItem(class C4Network2ClientListBox *pForDlg, int32_t iClientID, int32_t iConnectionID); // ctor

		void Update() override; // update data
		C4Network2IOConnection *GetConnection() const; // get connection by connection ID
		int32_t GetConnectionID() const override { return iConnID; }

		void OnButtonReconnect(C4GUI::Control *pButton);
		void OnButtonDisconnect(C4GUI::Control *pButton);
	};

private:
	bool fStartup;

	ClientListItem *GetClientListItem(int32_t iForClientID);

public:
	C4Network2ClientListBox(C4Rect &rcBounds, bool fStartup);
	~C4Network2ClientListBox() override;

	// update by client list
	void OnSec1Timer() override { Update(); }
	void Update();

	bool IsStartup() { return fStartup; }

	void SetClientSoundIcon(int32_t client_id);
};

// dialog framing the C4Network2ClientListBox and a game option list and a status label
class C4Network2ClientListDlg : public C4GUI::Dialog, private C4ApplicationSec1Timer
{
private:
	class C4GameOptionsList *pGameOptions;
	C4GUI::Label *pStatusLabel;
	C4Network2ClientListBox *pListBox;

	static C4Network2ClientListDlg *pInstance; // singleton-instance

	const char *GetID() override { return "ClientListDialog"; }
public:
	C4Network2ClientListDlg();
	~C4Network2ClientListDlg() override;

	void OnSec1Timer() override { Update(); }
	void Update();
	void OnSound(class C4Client *singer); // mark the specified client

	static bool Toggle(); // toggle dlg on/off
	static C4Network2ClientListDlg *GetInstance() { return pInstance; }
};

// host dialog shown at initial wait
class C4Network2StartWaitDlg : public C4GUI::Dialog
{
private:
	C4Network2ClientListBox *pClientListBox{nullptr};

	enum { DialogWidth = 250, DialogHeight = 300 };

protected:
	const char *GetID() override { return "NetStartWaitDialog"; }

public:
	C4Network2StartWaitDlg();
	~C4Network2StartWaitDlg() override = default;
};

// button area for some game options during lobby and scenario selection time
class C4GameOptionButtons : public C4GUI::Window
{
private:
	C4GUI::IconButton *btnInternet, *btnLeague, *btnPassword, *btnComment, *btnRecord;
	bool fNetwork, fHost, fLobby, fCountdown;

public:
	C4GameOptionButtons(const C4Rect &rcBounds, bool fNetwork, bool fHost, bool fLobby);
	~C4GameOptionButtons() override = default;

	void SetCountdown(bool fToVal);
protected:
	void OnBtnInternet(C4GUI::Control *btn);
	void OnBtnLeague(C4GUI::Control *btn);
	void OnBtnRecord(C4GUI::Control *btn);
	void OnBtnPassword(C4GUI::Control *btn);
	void OnPasswordSet(const StdStrBuf &rsNewPassword);
	void OnBtnComment(C4GUI::Control *btn);
	void OnCommentSet(const StdStrBuf &rsNewComment);

public:
	void UpdatePasswordBtn();
};

// graph display component
class C4Chart : public C4GUI::Element
{
protected:
	const C4Graph *pDisplayGraph;
	bool fOwnGraph;

	void DrawElement(C4TargetFacet &cgo) override; // draw the chart

public:
	C4Chart(C4Rect &rcBounds);
	~C4Chart() override;

	void SetGraph(const C4Graph *pNewGraph, bool fOwn) { pDisplayGraph = pNewGraph; fOwnGraph = fOwn; }
};

// C4Chart framing dialog
class C4ChartDialog : public C4GUI::Dialog
{
private:
	C4GUI::Tabular *pChartTabular{nullptr};

	// singleton-stuff
	static C4ChartDialog *pChartDlg;

	enum { DialogWidth = 400, DialogHeight = 300 };

	void AddChart(const StdStrBuf &rszName);

protected:
	const char *GetID() override { return "ChartDialog"; }

public:
	C4ChartDialog();

	// singleton-stuff
	~C4ChartDialog() override { if (pChartDlg==this) pChartDlg = nullptr; }
	static void Toggle();
};

#endif // INC_C4Network2Dialogs
