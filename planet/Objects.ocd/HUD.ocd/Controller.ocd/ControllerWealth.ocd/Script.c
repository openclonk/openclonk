/**
	ControllerWealth

	Shows an icon and the current player's wealth in the top right corner.

	@authors Maikel, Clonkonaut
*/

// HUD margin and size in tenths of em.
static const GUI_Controller_Wealth_IconSize = 25;
static const GUI_Controller_Wealth_IconMargin = 5;

local wealth_gui_menu;
local wealth_gui_id;

/* Creation / Destruction */

private func Construction()
{
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
	wealth_gui_id = GuiOpen(wealth_gui_menu);

	return _inherited(...);
}

private func Destruction()
{
	GuiClose(wealth_gui_id);

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
