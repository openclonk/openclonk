/**
	Grid
	Shows a simple grid menu.
*/

#include MenuStyle_List

local Name = "Grid Menu";

func Construction()
{
	inherited(...);
	this.Style = GUI_GridLayout;
}

public func SetTightGridLayout(bool tight)
{
	tight = tight ?? true;
	if (tight)
		this.Style = GUI_TightGridLayout;
	else
		this.Style = GUI_GridLayout;
}

func MakeEntryProplist(symbol, text)
{
	var custom_entry = {Bottom = "+2em", Right = "+2em", desc = {Style = GUI_TextRight | GUI_TextBottom}};
	custom_entry.Symbol = symbol;
	custom_entry.desc.Text = text;
	custom_entry.BackgroundColor = {Std = 0, OnHover = 0x50ff0000};
	return custom_entry;
}
