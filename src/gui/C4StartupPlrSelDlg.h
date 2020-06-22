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
// Startup screen for non-parameterized engine start: Player selection dialog
// Also contains player creation, editing and crew management

#ifndef INC_C4StartupPlrSelDlg
#define INC_C4StartupPlrSelDlg

#include "gui/C4Startup.h"
#include "object/C4InfoCore.h"

// startup dialog: Player selection
class C4StartupPlrSelDlg : public C4StartupDlg
{
private:
	enum Mode { PSDM_Player=0, PSDM_Crew }; // player selection list, or crew editing mode
	enum { IconLabelSpacing = 2 }; // space between an icon and its text

private:
	// one item in the player or crew list
	class ListItem : public C4GUI::Control
	{
	private:
		typedef C4GUI::Window BaseClass;
		// subcomponents
	protected:
		C4GUI::CheckBox *pCheck;  // check box to mark participation
		C4GUI::Label *pNameLabel; // item caption
		class C4StartupPlrSelDlg *pPlrSelDlg;
		C4GUI::Icon *pIcon;    // item icon
	private:
		class C4KeyBinding *pKeyCheck; // space activates/deactivates selected player
		StdStrBuf Filename;       // file info was loaded from

	public:
		ListItem(C4StartupPlrSelDlg *pForDlg, C4GUI::ListBox *pForListBox, C4GUI::Element *pInsertBeforeElement=nullptr, bool fActivated=false);
		const C4FacetSurface &GetIconFacet() const { return pIcon->GetFacet(); }
		~ListItem() override;

	protected:
		virtual C4GUI::ContextMenu *ContextMenu() = 0;
		C4GUI::ContextMenu *ContextMenu(C4GUI::Element *pEl, int32_t iX, int32_t iY)
		{ return ContextMenu(); }

		void UpdateOwnPos() override; // recalculate item positioning
		bool KeyCheck() { pCheck->ToggleCheck(true); return true; }
		bool IsFocusOnClick() override { return false; } // do not focus; keep focus on listbox

		void SetName(const char *szNewName);
		void SetIcon(C4GUI::Icons icoNew);

		void SetFilename(const StdStrBuf &sNewFN);

	public:
		C4GUI::CheckBox *GetCheckBox() const { return pCheck; }
		ListItem *GetNext() const { return static_cast<ListItem *>(BaseClass::GetNext()); }
		virtual uint32_t GetColorDw() const = 0; // get drawing color for portrait
		bool IsActivated() const { return pCheck->GetChecked(); }
		void SetActivated(bool fToVal) { pCheck->SetChecked(fToVal); }
		const char *GetName() const;
		virtual void SetSelectionInfo(C4GUI::TextWindow *pSelectionInfo) = 0; // clears text field and writes selection info text into it
		const StdStrBuf &GetFilename() const { return Filename; }
		virtual StdStrBuf GetDelWarning() = 0;
		void GrabIcon(C4FacetSurface &rFromFacet);

		bool CheckNameHotkey(const char * c) override; // return whether this item can be selected by entering given char

		class LoadError : public StdStrBuf
		{
		public:
			LoadError(StdStrBuf &&rTakeFrom) { Take(std::move(rTakeFrom)); }
		}; // class thrown off load function if load failed
	};

public:
	// a list item when in player selection mode
	class PlayerListItem : public ListItem
	{
	private:
		C4PlayerInfoCore Core;    // player info core loaded from player file
		bool fHasCustomIcon;      // set for players with a BigIcon.png

	public:
		PlayerListItem(C4StartupPlrSelDlg *pForDlg, C4GUI::ListBox *pForListBox, C4GUI::Element *pInsertBeforeElement=nullptr, bool fActivated=false);
		~PlayerListItem() override = default;

		void Load(const StdStrBuf &rsFilename); // may throw LoadError

	protected:
		C4GUI::ContextMenu *ContextMenu() override;

	public:
		const C4PlayerInfoCore &GetCore() const { return Core; }
		void UpdateCore(C4PlayerInfoCore & NewCore); // Save Core to disk and update this item
		void GrabCustomIcon(C4FacetSurface &fctGrabFrom);
		void SetSelectionInfo(C4GUI::TextWindow *pSelectionInfo) override;
		uint32_t GetColorDw() const override { return Core.PrefColorDw; }
		StdStrBuf GetDelWarning() override;
		bool MoveFilename(const char *szToFilename); // change filename to given
	};

private:
	// a list item when in crew editing mode
	class CrewListItem : public ListItem
	{
	private:
		bool fLoaded;
		C4ObjectInfoCore Core;
		uint32_t dwPlrClr;
		C4Group *pParentGrp;

	public:
		CrewListItem(C4StartupPlrSelDlg *pForDlg, C4GUI::ListBox *pForListBox, uint32_t dwPlrClr);
		~CrewListItem() override = default;

		void Load(C4Group &rGrp, const StdStrBuf &rsFilename); // may throw LoadError

	protected:
		C4GUI::ContextMenu *ContextMenu() override;

		void RewriteCore();

		struct RenameParams { };
		void AbortRenaming(RenameParams par);
		C4GUI::RenameEdit::RenameResult DoRenaming(RenameParams par, const char *szNewName);

	public:
		void UpdateClonkEnabled();

		uint32_t GetColorDw() const override { return dwPlrClr; }; // get drawing color for portrait
		void SetSelectionInfo(C4GUI::TextWindow *pSelectionInfo) override; // clears text field and writes selection info text into it
		StdStrBuf GetDelWarning() override;
		const C4ObjectInfoCore &GetCore() const { return Core; }

		CrewListItem *GetNext() const { return static_cast<CrewListItem *>(ListItem::GetNext()); }

		void CrewRename(); // shows the edit-field to rename a crew member
		bool SetName(const char *szNewName); // update clonk name and core
		void OnDeathMessageCtx(C4GUI::Element *el);
		void OnDeathMessageSet(const StdStrBuf &rsNewMessage);
	};

public:
	C4StartupPlrSelDlg(); // ctor
	~C4StartupPlrSelDlg() override; // dtor

private:
	class C4KeyBinding *pKeyBack, *pKeyProperties, *pKeyCrew, *pKeyDelete, *pKeyRename, *pKeyNew;
	class C4GUI::ListBox *pPlrListBox;
	C4GUI::TextWindow *pSelectionInfo;
	Mode eMode{PSDM_Player};

	// in crew mode:
	struct CurrPlayer_t
	{
		C4PlayerInfoCore Core; // loaded player main core
		C4Group Grp;           // group to player file; opened when in crew mode
	}
	CurrPlayer;

private:
	C4Rect rcBottomButtons; int32_t iBottomButtonWidth;
	class C4GUI::Button *btnActivatePlr, *btnCrew, *btnProperties, *btnDelete, *btnBack, *btnNew;

	void UpdateBottomButtons(); // update command button texts and positions
	void UpdatePlayerList(); // refill pPlrListBox with players in player folder, or with crew in selected player
	void UpdateSelection();
	void OnSelChange(class C4GUI::Element *pEl) { UpdateSelection(); }
	void OnSelDblClick(class C4GUI::Element *pEl) { C4GUI::GUISound("UI::Click"); OnPropertyBtn(nullptr); }
	void UpdateActivatedPlayers(); // update Config.General.Participants by currently activated players
	void SelectItem(const StdStrBuf &Filename, bool fActivate); // find item by filename and select (and activate it, if desired)

	void SetPlayerMode(); // change view to listing players
	void SetCrewMode(PlayerListItem *pForPlayer);   // change view to listing crew of a player

	static int32_t CrewSortFunc(const C4GUI::Element *pEl1, const C4GUI::Element *pEl2, void *par);
	void ResortCrew();

protected:
	void OnItemCheckChange(C4GUI::Element *pCheckBox);
	static bool CheckPlayerName(const StdStrBuf &Playername, StdStrBuf &Filename, const StdStrBuf *pPrevFilename, bool fWarnEmpty);
	ListItem *GetSelection();
	void SetSelection(ListItem *pNewItem);

	C4GUI::RenameEdit *pRenameEdit{nullptr}; // hack: set by crew list item renaming. Must be cleared when something is done in the dlg
	void AbortRenaming();

	friend class ListItem; friend class PlayerListItem; friend class CrewListItem;
	friend class C4StartupPlrPropertiesDlg;

protected:
	int32_t GetMarginTop() override { return (rcBounds.Hgt/7); }
	bool HasBackground() override { return false; }
	void DrawElement(C4TargetFacet &cgo) override;

	bool OnEnter() override { return false; } // Enter ignored
	bool OnEscape() override { DoBack(); return true; }
	bool KeyBack() { DoBack(); return true; }
	bool KeyProperties() { OnPropertyBtn(nullptr); return true; }
	bool KeyCrew() { OnCrewBtn(nullptr); return true; }
	bool KeyDelete() { OnDelBtn(nullptr); return true; }
	bool KeyNew() { OnNewBtn(nullptr); return true; }

	void OnNewBtn(C4GUI::Control *btn);
	void OnNew(const StdStrBuf &Playername);
	void OnActivateBtn(C4GUI::Control *btn);
	void OnPropertyBtn(C4GUI::Control *btn);
	void OnPropertyCtx(C4GUI::Element *el) { OnPropertyBtn(nullptr); }
	void OnCrewBtn(C4GUI::Control *btn);
	void OnDelBtn(C4GUI::Control *btn);
	void OnDelCtx(C4GUI::Element *el) { OnDelBtn(nullptr); }
	void OnDelBtnConfirm(ListItem *pSel);
	void OnBackBtn(C4GUI::Control *btn) { DoBack(); }

public:
	void DoBack(); // back to main menu
};

// player creation or property editing dialog
class C4StartupPlrPropertiesDlg: public C4GUI::Dialog
{
protected:
	C4StartupPlrSelDlg *pMainDlg; // may be nullptr if shown as creation dialog in main menu!
	C4StartupPlrSelDlg::PlayerListItem * pForPlayer;
	C4GUI::Edit *pNameEdit; // player name edit box
	C4GUI::CheckBox *pAutoStopControl; // wether the player uses AutoStopControl
	C4GUI::IconButton *pClrPreview;
	C4GUI::Picture *pCtrlImg;
	C4GUI::Picture *pSkinImg;
	C4GUI::IconButton *pMouseBtn, *pJumpNRunBtn, *pClassicBtn, *pPictureBtn;
	C4GUI::Label *ctrl_name_lbl;
	C4PlayerInfoCore C4P; // player info core copy currently being edited
	C4FacetSurface fctOldBigIcon;
	C4FacetSurface fctNewBigIcon; // if assigned, save new picture/bigicon
	bool fClearBigIcon; // if true, delete current picture/bigicon
	const char *GetID() override { return "PlrPropertiesDlg"; }

	void DrawElement(C4TargetFacet &cgo) override;
	int32_t GetMarginTop() override { return 16; }
	int32_t GetMarginLeft() override { return 45; }
	int32_t GetMarginRight() override { return 55; }
	int32_t GetMarginBottom() override { return 30; }

	void UserClose(bool fOK) override; // OK only with a valid name
	bool IsComponentOutsideClientArea() override { return true; } // OK and close btn

	void OnClrChangeLeft(C4GUI::Control *pBtn);
	void OnClrChangeRight(C4GUI::Control *pBtn);
	void OnClrChangeCustom(C4GUI::Control *pBtn);
	void OnCtrlChangeLeft(C4GUI::Control *pBtn);
	void OnCtrlChangeRight(C4GUI::Control *pBtn);
	void OnSkinChangeLeft(C4GUI::Control *pBtn);
	void OnSkinChangeRight(C4GUI::Control *pBtn);
	void OnPictureBtn(C4GUI::Control *pBtn);

private:
	void UpdatePlayerColor(bool fUpdateSliders);
	void UpdatePlayerControl();
	void UpdatePlayerSkin();
	void UpdateBigIcon();

	bool SetNewPicture(C4Surface &srcSfc, C4FacetSurface *trgFct, int32_t iMaxSize, bool fColorize);
	void SetNewPicture(const char *szFromFilename); // set new bigicon by loading and scaling if necessary. If szFromFilename==nullptr, clear bigicon

public:
	C4StartupPlrPropertiesDlg(C4StartupPlrSelDlg::PlayerListItem * pForPlayer, C4StartupPlrSelDlg *pMainDlg);
	~C4StartupPlrPropertiesDlg() override = default;

	void OnClosed(bool fOK) override; // close CB
};

#endif // INC_C4StartupPlrSelDlg
