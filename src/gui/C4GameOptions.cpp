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

#include "C4Include.h"
#include "gui/C4GameOptions.h"

#include "game/C4Application.h"
#include "game/C4Game.h"
#include "control/C4GameControl.h"
#include "graphics/C4GraphicsResource.h"
#include "gui/C4GameLobby.h"
#include "gui/C4Startup.h"

// ----------- C4GameOptionsList::Option ----------------------------------------------------------------

C4GameOptionsList::Option::Option(C4GameOptionsList *pForDlg) :
	BaseClass(C4Rect(0, 0, 0, 0)), pForDlg(pForDlg), pPrimarySubcomponent(nullptr)
{}

void C4GameOptionsList::Option::InitOption(C4GameOptionsList *pForDlg)
{
	// post-call after initialization: Adds to list box and does initial update
	// add to listbox (will eventually get moved)
	pForDlg->AddElement(this);
	// first-time update
	Update();
}



// ----------- C4GameOptionsList::OptionDropdown ----------------------------------------------------------------

C4GameOptionsList::OptionDropdown::OptionDropdown(class C4GameOptionsList *pForDlg, const char *szCaption, bool fReadOnly)
		: Option(pForDlg), fReadOnly(fReadOnly)
{
	bool fIsPreGame = pForDlg->IsPreGame();
	CStdFont &rUseFont = fIsPreGame ? C4Startup::Get()->Graphics.BookFont : ::GraphicsResource.TextFont;
	uint32_t dwFontClr = fIsPreGame ? C4StartupFontClr : 0xffffffff;
	// get size of caption label
	bool fTabular = pForDlg->IsTabular();
	int32_t iCaptWidth, iCaptHeight;
	if (fTabular)
	{
		// tabular layout: Caption label width by largest caption on runtime; fixed 1/3rd on startup
		rUseFont.GetTextExtent(LoadResStr("IDS_NET_RUNTIMEJOIN"), iCaptWidth, iCaptHeight, true);
		if (pForDlg->IsPreGame())
		{
			iCaptWidth = pForDlg->GetItemWidth() * 1 / 3;
		}
		else
		{
			iCaptWidth = iCaptWidth * 5 / 4;
		}
	}
	else
	{
		rUseFont.GetTextExtent(szCaption, iCaptWidth, iCaptHeight, true);
	}
	// calc total height for component
	int iHorizontalMargin = 1;
	int iVerticalMargin = 1;
	int iComboMargin = 5;
	int iSelComboHgt = C4GUI::ComboBox::GetDefaultHeight();
	SetBounds(C4Rect(0, 0, pForDlg->GetItemWidth(), (!fTabular) * (iCaptHeight + iVerticalMargin*2) + iVerticalMargin*2 + iSelComboHgt));
	C4GUI::ComponentAligner ca(GetContainedClientRect(), iHorizontalMargin, iVerticalMargin);
	// create subcomponents
	AddElement(pCaption = new C4GUI::Label(FormatString("%s:", szCaption).getData(), fTabular ? ca.GetFromLeft(iCaptWidth, iCaptHeight) : ca.GetFromTop(iCaptHeight), ALeft, dwFontClr, &rUseFont, false, false));
	ca.ExpandLeft(-iComboMargin);
	AddElement(pPrimarySubcomponent = pDropdownList = new C4GUI::ComboBox(ca.GetAll()));
	pDropdownList->SetReadOnly(fReadOnly);
	pDropdownList->SetComboCB(new C4GUI::ComboBox_FillCallback<C4GameOptionsList::OptionDropdown>(this, &C4GameOptionsList::OptionDropdown::OnDropdownFill, &C4GameOptionsList::OptionDropdown::OnDropdownSelChange));
	if (fIsPreGame)
	{
		pDropdownList->SetColors(C4StartupFontClr, C4StartupEditBGColor, C4StartupEditBorderColor);
		pDropdownList->SetFont(&rUseFont);
		pDropdownList->SetDecoration(&(C4Startup::Get()->Graphics.fctContext));
	}
	// final init
	InitOption(pForDlg);
}



// ----------- C4GameOptionsList::OptionScenarioParameter----------------------------------------------------------------

C4GameOptionsList::OptionScenarioParameter::OptionScenarioParameter(class C4GameOptionsList *pForDlg, const class C4ScenarioParameterDef *parameter_def)
	: C4GameOptionsList::OptionDropdown(pForDlg, parameter_def->GetName(), !pForDlg->IsPreGame() && (!::Control.isCtrlHost() || ::Game.C4S.Head.SaveGame)), ParameterDef(parameter_def), LastValue(0), LastValueValid(false)
{
	SetToolTip(parameter_def->GetDescription());
}

void C4GameOptionsList::OptionScenarioParameter::DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// Fill dropdown menuy with known possible options for this parameter
	size_t idx=0; const C4ScenarioParameterDef::Option *option;
	while ((option = ParameterDef->GetOptionByIndex(idx++)))
	{
		pFiller->AddEntry(option->Name.getData(), option->Value);
	}
}

void C4GameOptionsList::OptionScenarioParameter::DoDropdownSelChange(int32_t idNewSelection)
{
	// runtime change needs to be synchronized
	if (!pForDlg->IsPreGame())
	{
		// change possible?
		if (!::Control.isCtrlHost()) return;
		// Then initiate an update of the parameters on all clients
		C4GameLobby::C4PacketSetScenarioParameter pck(ParameterDef->GetID(), idNewSelection);
		::Network.Clients.BroadcastMsgToClients(MkC4NetIOPacket(PID_SetScenarioParameter, pck));
	}
	// also process on host (and standalone pre-game)
	pForDlg->GetParameters()->SetValue(ParameterDef->GetID(), idNewSelection, false);
	if (pForDlg->IsPreGame()) Update();
}

void C4GameOptionsList::OptionScenarioParameter::Update()
{
	int32_t val=0;
	// display forced league value?
	bool fLeagueReadOnly = false;
	if (::Config.Network.LeagueServerSignUp && !fReadOnly && !pForDlg->IsPreGameSingle()) val = ParameterDef->GetLeagueValue();
	if (val)
		fLeagueReadOnly = true;
	else
		val = pForDlg->GetParameters()->GetValueByID(ParameterDef->GetID(), ParameterDef->GetDefault());
	if (!fReadOnly) pDropdownList->SetReadOnly(fLeagueReadOnly);
	// update data to currently set option
	if (LastValueValid && val == LastValue) return;
	const C4ScenarioParameterDef::Option *option = ParameterDef->GetOptionByValue(val);
	if (option)
		pDropdownList->SetText(option->Name.getData());
	else
		pDropdownList->SetText(FormatString("%d", (int)val).getData());
	LastValueValid = true;
	LastValue = val;
}



// ----------- C4GameOptionsList::OptionControlMode ----------------------------------------------------------------

// Unfortunately, the control mode cannot be changed in the lobby
C4GameOptionsList::OptionControlMode::OptionControlMode(class C4GameOptionsList *pForDlg)
		: C4GameOptionsList::OptionDropdown(pForDlg, LoadResStr("IDS_TEXT_CONTROLMODE"), !::Control.isCtrlHost() || !::Control.isNetwork() || !::Control.Network.IsEnabled())
{
	SetToolTip(LoadResStr("IDS_DESC_CHANGESTHEWAYCONTROLDATAI"));
}

void C4GameOptionsList::OptionControlMode::DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller)
{
	// change possible?
	if (!::Control.isNetwork() || !::Control.Network.IsEnabled() || !::Control.isCtrlHost()) return;
	// add possible modes
	pFiller->AddEntry(LoadResStr("IDS_NET_CTRLMODE_CENTRAL"), CNM_Central);
	pFiller->AddEntry(LoadResStr("IDS_NET_CTRLMODE_DECENTRAL"), CNM_Decentral);
}

void C4GameOptionsList::OptionControlMode::DoDropdownSelChange(int32_t idNewSelection)
{
	// change possible?
	if (!::Control.isNetwork() || !::Control.Network.IsEnabled() || !::Control.isCtrlHost()) return;
	// perform it
	::Network.SetCtrlMode(idNewSelection);
	// update for clients done by packet; host needs to set it manually
	Update();
}

void C4GameOptionsList::OptionControlMode::Update()
{
	const char *szControlMode;
	if (!::Control.isNetwork() || !::Control.Network.IsEnabled())
		szControlMode = LoadResStr("IDS_NET_NONET");
	else
	{
		switch (::Network.Status.getCtrlMode())
		{
		case CNM_Central: szControlMode = LoadResStr("IDS_NET_CTRLMODE_CENTRAL"); break;
		case CNM_Decentral: szControlMode = LoadResStr("IDS_NET_CTRLMODE_DECENTRAL"); break;
		default: szControlMode = LoadResStr("IDS_NET_CTRLMODE_NONE"); break;
		}
	}
	pDropdownList->SetText(szControlMode);
}


// ----------- C4GameOptionsList::OptionControlRate ----------------------------------------------------------------

C4GameOptionsList::OptionControlRate::OptionControlRate(class C4GameOptionsList *pForDlg)
		: C4GameOptionsList::OptionDropdown(pForDlg, LoadResStr("IDS_CTL_CONTROLRATE"), !::Control.isCtrlHost())
{
	SetToolTip(LoadResStr("IDS_CTL_CONTROLRATE_DESC"));
}

void C4GameOptionsList::OptionControlRate::DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller)
{
	for (int i = 1; i < std::min(C4MaxControlRate, 10); ++i)
		pFiller->AddEntry(FormatString("%d", i).getData(), i);
}

void C4GameOptionsList::OptionControlRate::DoDropdownSelChange(int32_t idNewSelection)
{
	// adjust rate
	int32_t iNewRate = idNewSelection;
	if (!iNewRate || iNewRate == ::Control.ControlRate) return;
	::Control.AdjustControlRate(iNewRate - ::Control.ControlRate);
}

void C4GameOptionsList::OptionControlRate::Update()
{
	if (atoi(pDropdownList->GetText().getData()) == ::Control.ControlRate) return;
	pDropdownList->SetText(FormatString("%d", ::Control.ControlRate).getData());
}


// ----------- C4GameOptionsList::OptionRuntimeJoin ----------------------------------------------------------------

C4GameOptionsList::OptionRuntimeJoin::OptionRuntimeJoin(class C4GameOptionsList *pForDlg)
		: C4GameOptionsList::OptionDropdown(pForDlg, LoadResStr("IDS_NET_RUNTIMEJOIN"), !::Network.isHost())
{
	SetToolTip(LoadResStr("IDS_NET_RUNTIMEJOIN_DESC"));
}

void C4GameOptionsList::OptionRuntimeJoin::DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller)
{
	pFiller->AddEntry(LoadResStr("IDS_NET_RUNTIMEJOINBARRED"), 0);
	pFiller->AddEntry(LoadResStr("IDS_NET_RUNTIMEJOINFREE"), 1);
}

void C4GameOptionsList::OptionRuntimeJoin::DoDropdownSelChange(int32_t idNewSelection)
{
	// adjust mode
	bool fAllowed = !!idNewSelection;
	Config.Network.NoRuntimeJoin = !fAllowed;
	if (Game.IsRunning) ::Network.AllowJoin(fAllowed);
}

void C4GameOptionsList::OptionRuntimeJoin::Update()
{
	const char *szText;
	if (Config.Network.NoRuntimeJoin)
		szText = LoadResStr("IDS_NET_RUNTIMEJOINBARRED");
	else
		szText = LoadResStr("IDS_NET_RUNTIMEJOINFREE");
	pDropdownList->SetText(szText);
}


// ----------- C4GameOptionsList::OptionTeamDist ----------------------------------------------------------------

C4GameOptionsList::OptionTeamDist::OptionTeamDist(class C4GameOptionsList *pForDlg)
		: C4GameOptionsList::OptionDropdown(pForDlg, LoadResStr("IDS_MSG_TEAMDIST"), !::Control.isCtrlHost())
{
	SetToolTip(LoadResStr("IDS_MSG_TEAMDIST_DESC"));
}

void C4GameOptionsList::OptionTeamDist::DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller)
{
	Game.Teams.FillTeamDistOptions(pFiller);
}

void C4GameOptionsList::OptionTeamDist::DoDropdownSelChange(int32_t idNewSelection)
{
	// adjust team distribution
	Game.Teams.SendSetTeamDist(C4TeamList::TeamDist(idNewSelection));
}

void C4GameOptionsList::OptionTeamDist::Update()
{
	StdStrBuf sOption; sOption.Take(Game.Teams.GetTeamDistString());
	pDropdownList->SetText(sOption.getData());
}


// ----------- C4GameOptionsList::OptionTeamColors ----------------------------------------------------------------

C4GameOptionsList::OptionTeamColors::OptionTeamColors(class C4GameOptionsList *pForDlg)
		: C4GameOptionsList::OptionDropdown(pForDlg, LoadResStr("IDS_MSG_TEAMCOLORS"), !::Control.isCtrlHost())
{
	SetToolTip(LoadResStr("IDS_MSG_TEAMCOLORS_DESC"));
}

void C4GameOptionsList::OptionTeamColors::DoDropdownFill(C4GUI::ComboBox_FillCB *pFiller)
{
	pFiller->AddEntry(LoadResStr("IDS_MSG_ENABLED"), 1);
	pFiller->AddEntry(LoadResStr("IDS_MSG_DISABLED"), 0);
}

void C4GameOptionsList::OptionTeamColors::DoDropdownSelChange(int32_t idNewSelection)
{
	bool fEnabled = !!idNewSelection;
	Game.Teams.SendSetTeamColors(fEnabled);
}

void C4GameOptionsList::OptionTeamColors::Update()
{
	pDropdownList->SetText(Game.Teams.IsTeamColors() ? LoadResStr("IDS_MSG_ENABLED") : LoadResStr("IDS_MSG_DISABLED"));
}

// ----------- C4GameOptionsList -----------------------------------------------------------------------

C4GameOptionsList::C4GameOptionsList(const C4Rect &rcBounds, bool fActive, C4GameOptionsListSource source, C4ScenarioParameterDefs *param_defs, C4ScenarioParameters *params)
	: C4GUI::ListBox(rcBounds), source(source), param_defs(param_defs), params(params)
{
	// default parameter defs
	if (!IsPreGame())
	{
		if (!this->param_defs) this->param_defs = &::Game.ScenarioParameterDefs;
		if (!this->params) this->params = &::Game.Parameters.ScenarioParameters;
	}
	// initial option fill
	InitOptions();
	if (fActive) Activate();
}

void C4GameOptionsList::InitOptions()
{
	// create options for custom scenario parameters
	if (param_defs)
	{
		size_t idx = 0; const C4ScenarioParameterDef *def;
		while ((def = param_defs->GetParameterDefByIndex(idx++)))
			if (!def->IsAchievement()) // achievements are displayed in scenario selection. no need to repeat them here
				new OptionScenarioParameter(this, def);
	}
	// create lobby and runtime option selection components
	if (!IsPreGame())
	{
		new OptionControlMode(this);
		new OptionControlRate(this);
		if (::Network.isHost()) new OptionRuntimeJoin(this);
		if (!IsRuntime())
		{
			if (Game.Teams.HasTeamDistOptions()) new OptionTeamDist(this);
			if (Game.Teams.IsMultiTeams()) new OptionTeamColors(this);
		}
	}
}

void C4GameOptionsList::ClearOptions()
{
	C4GUI::Element *pFirst;
	while ((pFirst = GetFirst())) delete pFirst;
}

void C4GameOptionsList::Update()
{
	// update all option items
	for (Option *pItem = static_cast<Option *>(pClientWindow->GetFirst()); pItem; pItem = pItem->GetNext())
		pItem->Update();
}

void C4GameOptionsList::Activate()
{
	// register timer if necessary
	Application.Add(this);
	// force an update
	Update();
}

void C4GameOptionsList::Deactivate()
{
	// release timer if set
	Application.Remove(this);
}

void C4GameOptionsList::SetParameters(C4ScenarioParameterDefs *param_defs, C4ScenarioParameters *params)
{
	// update to new parameter set
	ClearOptions();
	this->param_defs = param_defs;
	this->params = params;
	InitOptions();
}

