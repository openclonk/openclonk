/*
	This file contains functions that are used for layouting custom menus.
*/

global func CreateCustomMenu(id menuStyle)
{
	var menu = CreateObject(menuStyle);
	menu->SetPosition(1, 1);
	return menu;
}

global func GuiAction_Call(proplist target, string function, value)
{
	return [GUI_Call, target, function, value];
}

global func GuiAction_SetTag(object target, int subwindow, string tag)
{
	return [GUI_SetTag, target, subwindow, tag];
}

global func Gui_AddMargin(proplist submenu, int marginX, int marginY)
{
	submenu.X = submenu.X ?? [0, 0];
	submenu.Y = submenu.Y ?? [0, 0];
	submenu.Wdt = submenu.Wdt ?? [1000, 0];
	submenu.Hgt = submenu.Hgt ?? [1000, 0];
	
	// safety, the coordinates could be a single value
	if (GetType(submenu.X) != C4V_Array) submenu.X = [submenu.X, 0];
	if (GetType(submenu.Y) != C4V_Array) submenu.Y = [submenu.Y, 0];
	if (GetType(submenu.Wdt) != C4V_Array) submenu.Wdt = [submenu.Wdt, 0];
	if (GetType(submenu.Hgt) != C4V_Array) submenu.Hgt = [submenu.Hgt, 0];
	
	submenu.X[1] += marginX;
	submenu.Y[1] += marginY;
	submenu.Wdt[1] -= marginX;
	submenu.Hgt[1] -= marginY;
	return true;
}

global func Gui_AddCloseButton(proplist menu, proplist target, string callback, parameter)
{
	var close_button =
	{
		Priority = 0x0fffff,
		X = [1000, -32], Y = 0,
		Wdt = 1000, Hgt = [0, 32],
		Symbol = Icon_Cancel,
		BackgroundColor = {Std = 0, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag(nil, nil, "Hover"),
		OnMouseOut = GuiAction_SetTag(nil, nil, "Std"),
		OnClick = GuiAction_Call(target, callback, parameter)
	};
	Gui_AddSubwindow(close_button, menu);
	return close_button;
}

global func Gui_UpdateText(string text, int menu, int submenu, object target)
{
	var update = {Text = text};
	CustomGuiUpdate(update, menu, submenu, target);
	return true;
}

// adds proplist /submenu/ as a new property to /menu/
global func Gui_AddSubwindow(proplist submenu, proplist menu)
{
	do
	{
		var uniqueID = Format("child%d", RandomX(10000, 0xffffff));
		if (menu[uniqueID] != nil) continue;
		menu[uniqueID] = submenu;
		return true;
	} while (true);
}