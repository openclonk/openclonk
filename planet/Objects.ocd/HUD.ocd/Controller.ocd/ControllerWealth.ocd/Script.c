/**
	ControllerWealth

	(Optionally) shows an icon and the current player's wealth in the top right corner.
	Use GUI_Controller->ShowWealth(); to show the wealth display to all players.
	Or call it individually on particular controllers.
	Use GUI_Controller->HideWealth(); to hide the display.

	@authors Maikel, Clonkonaut
*/

// HUD margin and size in tenths of em.
static const GUI_Controller_Wealth_IconSize = 25;
static const GUI_Controller_Wealth_IconMargin = 5;

static GUI_Controller_Wealth_shown; // shown or hidden for newly joining players?

local wealth_gui_menu;
local wealth_gui_id;

local wealth_display = false;

/* Showing / Hiding */

public func ShowWealth()
{
	// Definition call
	if (GetType(this) == C4V_Def)
	{
		GUI_Controller_Wealth_shown = true; // for players joining later
		for (var i = 0; i<GetPlayerCount(C4PT_User); ++i)
		{
			var plr = GetPlayerByIndex(i, C4PT_User);
			var controller = FindObject(Find_ID(GUI_Controller), Find_Owner(plr));
			if (controller) controller->ShowWealth();
		}
	} else {
		if (wealth_display) return;
		wealth_gui_id = GuiOpen(wealth_gui_menu);
		wealth_display = true;
		this->~ShiftGoal();
	}
}

public func HideWealth()
{
	// Definition call
	if (GetType(this) == C4V_Def)
	{
		GUI_Controller_Wealth_shown = false; // for players joining later
		for (var i = 0; i<GetPlayerCount(C4PT_User); ++i)
		{
			var plr = GetPlayerByIndex(i, C4PT_User);
			var controller = FindObject(Find_ID(Library_HUDController->GetGUIControllerID()), Find_Owner(plr));
			if (controller) controller->HideWealth();
		}
	} else {
		if (!wealth_display) return;
		GuiClose(wealth_gui_id);
		wealth_gui_id = nil;
		wealth_display = false;
		this->~UnshiftGoal();
	}
}

public func IsShowingWealth() { return wealth_display; }

/* Creation / Destruction */

private func Construction()
{
	wealth_display = GUI_Controller_Wealth_shown; // initial show/hide setting
	var plr = GetOwner();
	var wealth = GetWealth(plr);

	var margin = GUI_Controller_Wealth_IconMargin;
	var whole = margin + GUI_Controller_Wealth_IconSize;

	wealth_gui_menu =
	{
		Target = this,
		Player = plr,
		Style = GUI_Multiple | GUI_TextHCenter | GUI_TextBottom,
		Left = Format("100%%%s", ToEmString(-whole)),
		Right = Format("100%%%s", ToEmString(-margin)),
		Top = ToEmString(margin),
		Bottom = ToEmString(whole),
		Priority = 1,
		Symbol = Icon_Wealth,
		GraphicsName = GetGraphicsName(wealth),
		Text = Format("%d", wealth),
	};

	if (wealth_display) wealth_gui_id = GuiOpen(wealth_gui_menu);

	return _inherited(...);
}

private func Destruction()
{
	if (wealth_gui_id) GuiClose(wealth_gui_id);

	_inherited(...);
}

/* Callbacks */

public func OnWealthChanged(int plr)
{
	// Only update wealth when it is the right player.
	if (plr == GetOwner())
	{
		var wealth = GetWealth(plr);
		wealth_gui_menu.GraphicsName = GetGraphicsName(wealth);
		wealth_gui_menu.Text = Format("%d", wealth);
		GuiUpdate(wealth_gui_menu, wealth_gui_id);
	}

	return _inherited(plr, ...);
}

// Graphics changes in accordance to accumulated wealth
private func GetGraphicsName(int wealth)
{
	var num = "0";
	if (wealth >= 10) num = "1";
	if (wealth >= 30) num = "2";
	if (wealth >= 70) num = "3";
	if (wealth >= 120) num = "4";
	return num;
}
