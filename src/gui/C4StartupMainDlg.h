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
// Startup screen for non-parameterized engine start

#ifndef INC_C4StartupMainDlg
#define INC_C4StartupMainDlg

#include "gui/C4Startup.h"

class C4StartupMainDlg : public C4StartupDlg
{
private:
	C4KeyBinding *pKeyDown, *pKeyUp, *pKeyEnter;
	C4GUI::Label *pParticipantsLbl;
	C4GUI::Button *pStartButton;
	bool fFirstShown;

protected:

	void ConfirmMoveKey(const char *strKeyFilename);

	void DrawElement(C4TargetFacet &cgo) override;
	void OnClosed(bool fOK) override;    // callback when dlg got closed: Abort startup
	C4GUI::ContextMenu *OnPlayerSelContext(C4GUI::Element *pBtn, int32_t iX, int32_t iY); // preliminary player selection via simple context menu
	C4GUI::ContextMenu *OnPlayerSelContextAdd(C4GUI::Element *pBtn, int32_t iX, int32_t iY);
	C4GUI::ContextMenu *OnPlayerSelContextRemove(C4GUI::Element *pBtn, int32_t iX, int32_t iY);
	void OnPlayerSelContextAddPlr(C4GUI::Element *pTarget, const StdCopyStrBuf &rsFilename);
	void OnPlayerSelContextRemovePlr(C4GUI::Element *pTarget, const int &iIndex);
	void UpdateParticipants();
	void ShowParticipantCreationDlg();

	void OnStartBtn(C4GUI::Control *btn); // callback: run default start button pressed
	void OnPlayerSelectionBtn(C4GUI::Control *btn); // callback: player selection (preliminary version via context menus...)
	void OnNetJoinBtn(C4GUI::Control *btn); // callback: join net work game (direct join only for now)
	void OnNetJoin(const StdStrBuf &rsHostAddress);
	void OnOptionsBtn(C4GUI::Control *btn); // callback: Show options screen
	void OnEditorBtn(C4GUI::Control *btn); // callback: Editor
	void OnModsBtn(C4GUI::Control *btn); // callback: Mods
	void OnAboutBtn(C4GUI::Control *btn); // callback: Show about screen
	void OnExitBtn(C4GUI::Control *btn); // callback: exit button pressed
	void OnTODO(C4GUI::Control *btn); // button not yet implemented

	bool KeyEnterDown(); // return pressed -> reroute as space
	bool KeyEnterUp(); // return released -> reroute as space

	void OnShown() override; // callback when shown: Show log if restart after failure; show player creation dlg on first start

public:
	C4StartupMainDlg(); // ctor
	~C4StartupMainDlg() override; // dtor
};

#endif // INC_C4StartupMainDlg
