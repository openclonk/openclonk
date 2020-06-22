/**
	GUI.c
	This file contains functions that are used for layouting custom menus.

	@author Zapper
*/


// documented in /docs/sdk/script/fn
global func GuiAction_Call(proplist target, string function, value)
{
	return [GUI_Call, target, function, value];
}

// documented in /docs/sdk/script/fn
global func GuiAction_SetTag(string tag, int subwindow, object target)
{
	return [GUI_SetTag, tag, subwindow, target];
}

global func GuiAddCloseButton(proplist menu, proplist target, string callback, parameter)
{
	var close_button =
	{
		Tooltip = "$TooltipGUIClose$",
		Priority = 0x0fffff,
		Left = "100%-2em", Top = "0%+0em",
		Right = "100%", Bottom = "0%+2em",
		Symbol = GetDefaultCancelSymbol(),
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
	GuiUpdate(update, menu, submenu, target);
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

// Converts an integer into a floating "em"-value, as the given value is divided by the given factor (10 by default).
global func ToEmString(int value, int factor)
{
	// Make sure factor is a power of ten.
	factor = factor ?? 10;
	var power_of_ten = 0;
	while (10**power_of_ten != factor)
	{
		if (10**power_of_ten > factor)
		{
			Log("WARNING: factor in ToEmString(%d, %d) is not a multiple of ten, falling back to default", value, factor);
			factor = 10;
			power_of_ten = 1;
			break;
		}
		power_of_ten++;	
	}
	// Construct the string using sign, value and decimal notation.
	var em_sign = "+";
	if (value < 0)
		em_sign = "-";
	var em_value = Format("%d", Abs(value / factor));
	var em_decimal = Format("%011dem", Abs(value % factor));
	em_decimal = TakeString(em_decimal, GetLength(em_decimal) - power_of_ten - 2);
	if (power_of_ten == 0)
		em_decimal = "0";
	return Format("%s%s.%s", em_sign, em_value, em_decimal);
}

// Converts an integer into a floating percent value, as the given value is divided by the given factor (10 by default).
global func ToPercentString(int value, int factor)
{
	// Make sure factor is a power of ten.
	factor = factor ?? 10;
	var power_of_ten = 0;
	while (10**power_of_ten != factor)
	{
		if (10**power_of_ten > factor)
		{
			Log("WARNING: factor in ToPercentString(%d, %d) is not a multiple of ten, falling back to default", value, factor);
			factor = 10;
			power_of_ten = 1;
			break;
		}
		power_of_ten++;	
	}
	// Construct the string using sign, value and decimal notation.
	var percent_sign = "+";
	if (value < 0)
		percent_sign = "-";
	var percent_value = Format("%d", Abs(value / factor));
	var percent_decimal = Format("%011d%%", Abs(value % factor));
	percent_decimal = TakeString(percent_decimal, GetLength(percent_decimal) - power_of_ten - 1);
	if (power_of_ten == 0)
		percent_decimal = "0";
	return Format("%s%s.%s", percent_sign, percent_value, percent_decimal);
}

/*
Returns true if /this/ object is allowed to be displayed on the same stack as the /other/ object in a GUI.
*/
global func CanBeStackedWith(object other)
{
	return this->GetID() == other->GetID();
}

// Returns the default symbol used for the "cancel" icon displayed e.g. in the top-right corner of menus.
global func GetDefaultCancelSymbol()
{
	return _inherited(...);
}
