/*
	This file contains functions that are used for layouting custom menus.
*/

global func CreateCustomMenu(id menuStyle)
{
	var menu = CreateObject(menuStyle);
	menu->SetPosition(1, 1);
	return menu;
}

global func MenuAction_Call(proplist target, string function, value)
{
	return [MENU_Call, target, function, value];
}

global func MenuAction_SetTag(object target, int subwindow, string tag)
{
	return [MENU_SetTag, target, subwindow, tag];
}

global func Menu_AddMargin(proplist submenu, int marginX, int marginY)
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

global func Menu_UpdateText(string text, int menu, int submenu, object target)
{
	var update = {Text = text};
	CustomMenuUpdate(update, menu, submenu, target);
	return true;
}

// adds proplist /submenu/ as a new property to /menu/
global func Menu_AddSubmenu(proplist submenu, proplist menu)
{
	do
	{
		var uniqueID = Format("child%d", RandomX(10000, 0xffffff));
		if (menu[uniqueID] != nil) continue;
		menu[uniqueID] = submenu;
		return true;
	} while (true);
}