/*
	This file contains functions that are used for layouting custom menus.
*/

global func GuiAction_Call(proplist target, string function, value)
{
	return [GUI_Call, target, function, value];
}

global func GuiAction_SetTag(string tag, int subwindow, object target)
{
	return [GUI_SetTag, tag, subwindow, target];
}

global func GuiAddCloseButton(proplist menu, proplist target, string callback, parameter)
{
	var close_button =
	{
		Priority = 0x0fffff,
		Left = "100%-2em", Top = "0%+0em",
		Right = "100%", Bottom = "0%+2em",
		Symbol = Icon_Cancel,
		BackgroundColor = {Std = 0, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(target, callback, parameter)
	};
	GuiAddSubwindow(close_button, menu);
	return close_button;
}

global func GuiUpdateText(string text, int menu, int submenu, object target)
{
	var update = {Text = text};
	CustomGuiUpdate(update, menu, submenu, target);
	return true;
}

// adds proplist /submenu/ as a new property to /menu/
global func GuiAddSubwindow(proplist submenu, proplist menu)
{
	do
	{
		// use an anonymous name starting with an underscore
		var uniqueID = Format("_child%d", RandomX(10000, 0xffffff));
		if (menu[uniqueID] != nil) continue;
		menu[uniqueID] = submenu;
		return true;
	} while (true);
}

// converts an integer into a "em"-value string that can be used as a position for a GUI. The value will be divided by "factor" which is 10 by default
global func ToEmString(int value, int factor)
{
	factor = factor ?? 10;
	return Format("%+d.%dem", value / factor, Abs(value % factor));
}