/**
	ControllerWealth
	Controlls the wealth display.

	@author Maikel
*/


// HUD margin and size in tenths of em.
static const GUI_Controller_Wealth_IconSize = 60;
static const GUI_Controller_Wealth_IconMargin = 10;

// Local variables to keep track of the wealth HUD menu.
local wealth_gui_target;
local wealth_gui_menu;
local wealth_gui_id;

public func Construction()
{
	var plr = GetOwner();
	var wealth = GetWealth(plr);
	// Create a menu target.
	wealth_gui_target = CreateObject(Dummy, AbsX(0), AbsY(0), plr);
	wealth_gui_target.Visibility = VIS_Owner;
	// Create the wealth HUD menu.
	var margin = GUI_Controller_Wealth_IconMargin;
	var end = margin + GUI_Controller_Wealth_IconSize;
	wealth_gui_menu = 
	{
		Target = wealth_gui_target,
		Style = GUI_Multiple | GUI_TextHCenter | GUI_TextBottom,
		Left = Format("100%%%s", ToEmString(-end)),
		Right = Format("100%%%s", ToEmString(-margin)),
		Top = ToEmString(margin),
		Bottom = ToEmString(end),
		Symbol = Icon_Wealth,
		GraphicsName = GetGraphicsName(wealth),
		Text = Format("%d", wealth),
	};
	wealth_gui_id = GuiOpen(wealth_gui_menu);
	return _inherited(...);
}

public func Destruction()
{
	// This also closes the wealth HUD menu.
	if (wealth_gui_target)
		wealth_gui_target->RemoveObject();
	return _inherited(...);
}

// Callback when the wealth has changed: update the wealth HUD menu.
public func OnWealthChanged(int plr)
{
	var wealth = GetWealth(plr);
	wealth_gui_menu.GraphicsName = GetGraphicsName(wealth);
	wealth_gui_menu.Text = Format("%d", wealth);
	GuiUpdate(wealth_gui_menu, wealth_gui_id);
	return _inherited(plr, ...);
}

// Returns the graphics name for a specific wealth.
private func GetGraphicsName(int wealth)
{
	var num = 0;
	if (wealth >= 10) num = 1;
	if (wealth >= 30) num = 2;
	if (wealth >= 70) num = 3;
	if (wealth >= 120) num = 4;
	return Format("%d", num);
}
