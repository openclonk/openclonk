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
// Custom game options and configuration dialog

#ifndef INC_C4GameOptions
#define INC_C4GameOptions

#include "gui/C4Gui.h"

// options dialog: created as listbox inside another dialog
// used to configure some standard runtime options, as well as custom game options
class C4GameOptionsList : public C4GUI::ListBox, private C4ApplicationSec1Timer
{
public:
	enum { IconLabelSpacing = 2 }; // space between an icon and its text

private:
	class Option : public C4GUI::Control
	{
	protected:
		typedef C4GUI::Control BaseClass;
		class C4GameOptionsList *pForDlg;

		// primary subcomponent: forward focus to this element
		C4GUI::Control *pPrimarySubcomponent;

		virtual bool IsFocused(C4GUI::Control *pCtrl)
		{
			// also forward own focus to primary control
			return BaseClass::IsFocused(pCtrl) || (HasFocus() && pPrimarySubcomponent == pCtrl);
		}

	public:
		Option(class C4GameOptionsList *pForDlg); // ctor - adds to list
		void InitOption(C4GameOptionsList *pForDlg); // add to list and do initial update

		virtual void Update() {}; // update data

		Option *GetNext() { return static_cast<Option *>(BaseClass::GetNext()); }
	};

	// dropdown list option
	class OptionDropdown : public Option
	{
	public:
		OptionDropdown(class C4GameOptionsList *pForDlg, const char *szCaption, bool fReadOnly);

	protected:
		C4GUI::Label *pCaption;
		C4GUI::ComboBox *pDropdownList;
		bool fReadOnly;

		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller) = 0;
		void OnDropdownFill(C4GUI::ComboBox_FillCB *pFiller)
		{ DoDropdownFill(pFiller); }
		virtual void DoDropdownSelChange(int32_t idNewSelection) = 0;
		bool OnDropdownSelChange(C4GUI::ComboBox *pForCombo, int32_t idNewSelection)
		{ DoDropdownSelChange(idNewSelection); Update(); return true; }
	};

	// drop down list to specify a custom scenario parameter
	class OptionScenarioParameter : public OptionDropdown
	{
		const class C4ScenarioParameterDef *ParameterDef;
		int32_t LastValue; bool LastValueValid;

	public:
		OptionScenarioParameter(class C4GameOptionsList *pForDlg, const class C4ScenarioParameterDef *parameter_def);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
		virtual void DoDropdownSelChange(int32_t idNewSelection);

		virtual void Update(); // update data to currently set option
	};


	// drop down list to specify central/decentral control mode
	class OptionControlMode : public OptionDropdown
	{
	public:
		OptionControlMode(class C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
		virtual void DoDropdownSelChange(int32_t idNewSelection);

		virtual void Update(); // update data to current control rate
	};

	// drop down list option to adjust control rate
	class OptionControlRate : public OptionDropdown
	{
	public:
		OptionControlRate(class C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
		virtual void DoDropdownSelChange(int32_t idNewSelection);

		virtual void Update(); // update data to current control rate
	};

	// drop down list option to adjust team usage
	class OptionTeamDist : public OptionDropdown
	{
	public:
		OptionTeamDist(class C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
		virtual void DoDropdownSelChange(int32_t idNewSelection);

		virtual void Update(); // update data to current team mode
	};

	// drop down list option to adjust team color state
	class OptionTeamColors : public OptionDropdown
	{
	public:
		OptionTeamColors(class C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
		virtual void DoDropdownSelChange(int32_t idNewSelection);

		virtual void Update(); // update data to current team color mode
	};

	// drop down list option to adjust control rate
	class OptionRuntimeJoin : public OptionDropdown
	{
	public:
		OptionRuntimeJoin(class C4GameOptionsList *pForDlg);

	protected:
		virtual void DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller);
		virtual void DoDropdownSelChange(int32_t idNewSelection);

		virtual void Update(); // update data to current runtime join state
	};

public:
	enum C4GameOptionsListSource
	{
		GOLS_PreGameSingle,
		GOLS_PreGameNetwork,
		GOLS_Lobby,
		GOLS_Runtime
	};

	C4GameOptionsList(const C4Rect &rcBounds, bool fActive, C4GameOptionsListSource source, class C4ScenarioParameterDefs *param_defs=nullptr, class C4ScenarioParameters *params=nullptr);
	~C4GameOptionsList() { Deactivate(); }

private:
	C4GameOptionsListSource source; // where to draw options from. e.g. lobby options such as team colors aren't presented at run-time
	class C4ScenarioParameterDefs *param_defs; // where to pull parameters and parameter definitions from
	class C4ScenarioParameters *params;

	void InitOptions(); // creates option selection components
	void ClearOptions(); // remove all option components

public:
	// update all option flags by current game state
	void Update();
	void OnSec1Timer() { Update(); }

	// update to new parameter set. recreates option fields. set parameters to nullptr for no options
	void SetParameters(C4ScenarioParameterDefs *param_defs, C4ScenarioParameters *params);

	// activate/deactivate periodic updates
	void Activate();
	void Deactivate();

	// config
	bool IsRuntime() const { return source==GOLS_Runtime; }
	bool IsTabular() const { return IsRuntime() || IsPreGame(); } // low lobby space doesn't allow tabular layout
	bool IsPreGame() const { return source==GOLS_PreGameSingle || source==GOLS_PreGameNetwork; }
	bool IsPreGameSingle() const { return source==GOLS_PreGameSingle; }

	C4ScenarioParameters *GetParameters() { return params; } // used by children
};
#endif //INC_C4GameOptions
