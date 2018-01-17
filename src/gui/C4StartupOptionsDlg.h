/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2011-2016, The OpenClonk Team and contributors
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
// Startup screen for non-parameterized engine start: Options dialog

#ifndef INC_C4StartupOptionsDlg
#define INC_C4StartupOptionsDlg

#include "gui/C4Startup.h"

// startup dialog: Options
class C4StartupOptionsDlg : public C4StartupDlg
{
	// main dlg stuff -----------------------------------------------------
public:
	C4StartupOptionsDlg(); // ctor
	~C4StartupOptionsDlg() override; // dtor

private:
	class C4KeyBinding *pKeyBack;
	bool fConfigSaved{false}; // set when config has been saved because dlg is closed; prevents double save
	bool fCanGoBack{true}; // set if dlg has not been recreated yet, in which case going back to a previous dialog is not possible
	C4GUI::Tabular *pOptionsTabular;

protected:
	bool OnEnter() override { return false; } // Enter ignored
	bool OnEscape() override { DoBack(); return true; }
	bool KeyBack() { DoBack(); return true; }
	void OnClosed(bool fOK) override;    // callback when dlg got closed - save config
	void UserClose(bool fOK) override   // callback when the user tried to close the dialog (e.g., by pressing Enter in an edit) - just ignore this and save config instead
	{ if (fOK) SaveConfig(false, true); }

	void OnBackBtn(C4GUI::Control *btn) { DoBack(); }

	bool SaveConfig(bool fForce, bool fKeepOpen); // save any config fields that are not stored directly; return whether all values are OK

public:
	void DoBack(); // back to main menu
	
	bool SetSubscreen(const char *szToScreen) override; // go to specified property sheet
	void OnKeyboardLayoutChanged() override; // keyboard layout changed: update keys from scan codes

public:
	void RecreateDialog(bool fFade);

	// program tab --------------------------------------------------------
private:
	// button without fancy background
	class SmallButton : public C4GUI::Button
	{
	protected:
		void DrawElement(C4TargetFacet &cgo) override; // draw the button

	public:
		SmallButton(const char *szText, const C4Rect &rtBounds) // ctor
				: C4GUI::Button(szText, rtBounds) {}
		static int32_t GetDefaultButtonHeight();
	};

	class C4GUI::ComboBox *pLangCombo;
	class C4GUI::Label *pLangInfoLabel;
	class C4GUI::ComboBox *pFontFaceCombo, *pFontSizeCombo;

	void OnLangComboFill(C4GUI::ComboBox_FillCB *pFiller);
	bool OnLangComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);
	void UpdateLanguage();
	void OnFontFaceComboFill(C4GUI::ComboBox_FillCB *pFiller);
	void OnFontSizeComboFill(C4GUI::ComboBox_FillCB *pFiller);
	bool OnFontComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);
	void UpdateFontControls();
	bool SetGameFont(const char *szFontFace, int32_t iFontSize);
	void OnResetConfigBtn(C4GUI::Control *btn);

	// graphics tab -------------------------------------------------------
private:

	// checkbox that changes a config boolean directly
	class BoolConfig : public C4GUI::CheckBox
	{
	private:
		bool *pbVal, fInvert; int32_t *piVal, *piRestartChangeCfgVal;
	protected:
		void OnCheckChange(C4GUI::Element *pCheckBox);
	public:
		BoolConfig(const C4Rect &rcBounds, const char *szName, bool *pbVal, int32_t *piVal, bool fInvert=false, int32_t *piRestartChangeCfgVal=nullptr);
	};
	// editbox below descriptive label sharing one window for common tooltip
	class EditConfig : public C4GUI::LabeledEdit
	{
	public:
		EditConfig(const C4Rect &rcBounds, const char *szName, ValidatedStdCopyStrBufBase *psConfigVal, int32_t *piConfigVal, bool fMultiline);
	private:
		ValidatedStdCopyStrBufBase *psConfigVal;
		int32_t *piConfigVal;
	public:
		void Save2Config(); // control to config
		static bool GetControlSize(int *piWdt, int *piHgt, const char *szForText, bool fMultiline);
		int32_t GetIntVal() { return atoi(GetEdit()->GetText()); }
		void SetIntVal(int32_t iToVal) { GetEdit()->SetText(FormatString("%d", (int) iToVal).getData(), false); }
	} *pNetworkNickEdit;
	// message dialog with a timer; used to restore the resolution if the user didn't press anything for a while
	class ResChangeConfirmDlg : public C4GUI::Dialog, private C4ApplicationSec1Timer
	{
	private:
		C4GUI::Label *pOperationCancelLabel; int32_t iResChangeSwitchTime;
	public:
		ResChangeConfirmDlg();
		~ResChangeConfirmDlg() override;
		void OnSec1Timer() override; // update timer label
	protected:
		bool OnEnter() override { return true; } // Pressing Enter does not confirm this dialog, so "blind" users are less likely to accept their change
		const char *GetID() override { return "ResChangeConfirmDialog"; }
	};

	void OnWindowedModeComboFill(C4GUI::ComboBox_FillCB *pFiller);
	bool OnWindowedModeComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);
	void OnGfxResComboFill(C4GUI::ComboBox_FillCB *pFiller);
	bool OnGfxResComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);
	void OnGfxMSComboFill(C4GUI::ComboBox_FillCB *pFiller);
	bool OnGfxMSComboSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection);
	bool TryNewResolution(int32_t iResX, int32_t iResY, int32_t iRefreshRate);
	StdStrBuf GetGfxResString(int32_t iResX, int32_t iResY); // convert resolution to string to be displayed in resolution choice combobox
	const char * GetWindowedName(int32_t mode = -1);

	int32_t iGfxTexIndent;

	// sound tab ----------------------------------------------------------
private:
	C4KeyBinding *pKeyToggleMusic; // extra key binding for music: Must also update checkbox in real-time
	BoolConfig *pFESoundCheck;
	C4GUI::CheckBox *pFEMusicCheck;

	void OnFEMusicCheck(C4GUI::Element *pCheckBox); // toggling frontend music is reflected immediately
	void OnRXSoundCheck(C4GUI::Element *pCheckBox); // toggling sounds on off must init/deinit sound system
	void OnMusicVolumeSliderChange(int32_t iNewVal);
	void OnSoundVolumeSliderChange(int32_t iNewVal);

protected:
	bool KeyMusicToggle();

	// keyboard and gamepad control tabs ----------------------------------
private:
	// dialog shown to user so he can select a key
	class KeySelDialog : public C4GUI::MessageDialog
	{
	private:
		std::unique_ptr<C4KeyBinding> KeyListeners;
		C4KeyCodeEx key;
		const class C4PlayerControlAssignment *assignment;
		const class C4PlayerControlAssignmentSet *assignment_set;

		static StdStrBuf GetDlgMessage(const class C4PlayerControlAssignment *assignment, const class C4PlayerControlAssignmentSet *assignment_set);
		static C4GUI::Icons GetDlgIcon(const class C4PlayerControlAssignmentSet *assignment_set);

	protected:
		bool KeyDown(const C4KeyCodeEx &key) { return KeyPress(key, true); };
		bool KeyUp(const C4KeyCodeEx &key) { return KeyPress(key, false); };
		bool KeyPress(const C4KeyCodeEx &key, bool fDown);
		void MouseInput(C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override;
	public:
		KeySelDialog(const class C4PlayerControlAssignment *assignment, const class C4PlayerControlAssignmentSet *assignment_set);
		~KeySelDialog() override = default;

		C4KeyCodeEx GetKeyCode() { return key; }

	};

	// list box of definable keys
	class ControlConfigListBox : public C4GUI::ListBox
	{
	private:
		// assignment key label - change key on click
		class ControlAssignmentLabel : public C4GUI::Label
		{
		private:
			class C4PlayerControlAssignment *assignment; // pointer into assignment set (not owned!)
			class C4PlayerControlAssignmentSet *assignment_set; // pointer to assignment set (not owned!)

			void UpdateAssignmentString();
		public:
			void MouseInput(class C4GUI::CMouse &rMouse, int32_t iButton, int32_t iX, int32_t iY, DWORD dwKeyParam) override;

			ControlAssignmentLabel(class C4PlayerControlAssignment *assignment, class C4PlayerControlAssignmentSet *assignment_set, const C4Rect &bounds);
		};

		// item in list box
		class ListItem : public C4GUI::Window
		{
		private:
			ControlConfigListBox *parent_list;
			ControlAssignmentLabel *assignment_label;
			bool has_extra_spacing; // if true, add a bit of spacing on top of this item to group elements

			int32_t GetListItemTopSpacing() override { return C4GUI::Window::GetListItemTopSpacing() + (has_extra_spacing*GetBounds().Hgt/2); }

		public:
			ListItem(ControlConfigListBox *parent_list, class C4PlayerControlAssignment *assignment, class C4PlayerControlAssignmentSet *assignment_set, bool first_of_group);
		};

	private:
		class C4PlayerControlAssignmentSet *set; // assignment set being configured by this box
		static bool sort_by_group (C4PlayerControlAssignment *i, C4PlayerControlAssignment *j) { return i->GetGUIGroup() < j->GetGUIGroup(); }

	public:
		ControlConfigListBox(const C4Rect &rcBounds, class C4PlayerControlAssignmentSet *set);

		void SetAssignmentSet(class C4PlayerControlAssignmentSet *new_set);

		static void SetUserKey(class C4PlayerControlAssignmentSet *assignment_set, class C4PlayerControlAssignment *assignment, C4KeyCodeEx &key);
	};

	// config area to define a keyboard set
	class ControlConfigArea : public C4GUI::Window
	{
	private:
		bool fGamepad;            // if set, gamepad control sets are being configured
		int32_t iMaxControlSets;  // number of control sets configured in this area - must be smaller or equal to C4MaxControlSet
		int32_t iSelectedCtrlSet; // keyboard or gamepad set that is currently being configured
		class C4GUI::IconButton ** ppKeyControlSetBtns; // buttons to select configured control set - array in length of iMaxControlSets
		class KeySelButton * KeyControlBtns[C4MaxKey];  // buttons to configure individual kbd set buttons
		C4StartupOptionsDlg *pOptionsDlg;
		ControlConfigListBox *control_list;
		class C4GUI::CheckBox *pGUICtrl;
	public:
		ControlConfigArea(const C4Rect &rcArea, int32_t iHMargin, int32_t iVMargin, bool fGamepad, C4StartupOptionsDlg *pOptionsDlg);
		~ControlConfigArea() override;

		void UpdateCtrlSet();

	protected:

		void OnCtrlSetBtn(C4GUI::Control *btn);
		void OnResetKeysBtn(C4GUI::Control *btn);
		void OnGUIGamepadCheckChange(C4GUI::Element *pCheckBox);
	};

	ControlConfigArea *pControlConfigArea;

	class C4GamePadControl *GamePadCon;

	// network tab --------------------------------------------------------
private:
	// checkbox to enable protocol and editbox to input port number
	class NetworkPortConfig : public C4GUI::Window
	{
	public:
		NetworkPortConfig(const C4Rect &rcBounds, const char *szName, int32_t *pConfigValue, int32_t iDefault); // ctor
	private:
		int32_t *pConfigValue;         // pointer into config set
		C4GUI::CheckBox *pEnableCheck; // check box for whether port is enabled
		C4GUI::Edit *pPortEdit;        // edit field for port number

	public:
		void OnEnabledCheckChange(C4GUI::Element *pCheckBox); // callback when checkbox is ticked
		void SavePort(); // controls to config
		int32_t GetPort(); // get port as currently configured by control (or -1 for deactivated)

		static bool GetControlSize(int *piWdt, int *piHgt);
	} *pPortCfgTCP, *pPortCfgUDP, *pPortCfgRef, *pPortCfgDsc;

	class NetworkServerAddressConfig : public C4GUI::Window
	{
	public:
		NetworkServerAddressConfig(const C4Rect &rcBounds, const char *szName, int32_t *piConfigEnableValue, char *szConfigAddressValue, int iTabWidth); // ctor
	private:
		int32_t *piConfigEnableValue; char *szConfigAddressValue;
		C4GUI::CheckBox *pEnableCheck;
		C4GUI::Edit *pAddressEdit;
	public:
		void OnEnabledCheckChange(C4GUI::Element *pCheckBox); // callback when checkbox is ticked
		void Save2Config(); // controls to config

		static bool GetControlSize(int *piWdt, int *piHgt, int *piTabPos, const char *szForText);
	} *pLeagueServerCfg;
};


#endif // INC_C4StartupOptionsDlg
