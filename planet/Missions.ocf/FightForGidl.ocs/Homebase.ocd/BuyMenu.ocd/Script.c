/**
	Buy Menu

	Shows icons to instantly buy+equip weapons

	@authors Sven2
*/

local menu; // the actual menu (a MenuStyle_Grid)
local homebase; // associated base
local buymenu_toggle_id; // button to toggle menu visibility

/* Creation / Destruction */

public func Construction(...)
{
	// The menu. Cannot inherit from MenuStyle_Grid, because at the moment the menu host
	// is the object itself. Therefore, child elements such as the buy menu would become
	// invisible if the menu closes. So just create an extra object for now.
	menu = CreateObject(MenuStyle_Grid, 0,0, GetOwner());
	menu->SetPermanent(true);
	menu.Style |= GUI_Multiple;
	menu.Player = nil; // by visibility
	menu.Visibility = VIS_None; // default off - enable through button.
	menu.Left = "100% - 12em";
	menu.Top = "4em";
	menu.Right = "100% - 4em";
	menu.Bottom = "90% - 12em";
	menu.BackgroundColor = 0x20000000;
	// Create button to show/hide menu
	CreateToggleVisibilityButton();
	return true;
}

public func Destruction(...)
{
	DestroyToggleVisibilityButton();
	return true;
}

public func SetHomebase(object to)
{
	homebase = [to];
	return true;
}

// Forward menu operations
public func Open(...)
{
	if (menu) return menu->Open(...);
	return false;
}

public func RemoveItem(idx)
{
	if (menu) return menu->RemoveItem(idx);
	return false;
}


/* Buy menu entries */

public func UpdateCaption(string title, bool available, int item_idx)
{
	if (!menu) return false;
	var custom_entry = { Bottom = "+1.3em" }, fontclr, bgclr, bgclr_hover;
	if (available)
	{
		fontclr = 0xffffff;
		bgclr = nil;
	}
	else
	{
		fontclr = 0x7f7f7f;
		bgclr = 0x50000000;
	}
	custom_entry.BackgroundColor = bgclr;
	custom_entry.Text = Format("  <c %x>%s</c>", fontclr, title);
	custom_entry.Style = GUI_IgnoreMouse | GUI_TextBottom;
	menu->AddItem(GetID(), nil, item_idx, this, this.ClickCaption, item_idx, custom_entry, nil, true);
	return true;
}

public func ClickCaption() { return true; } // nothing to be done here (maybe expand/collapse in the future)

public func UpdateBuyEntry(id buy_def, bool available, int price, int callback_idx, bool was_last_selection, int extra_width, string hotkey)
{
	if (!menu) return false;
	var custom_entry = {Bottom = "+2em", Right = Format("+%dem", 2+extra_width), Symbol = buy_def }, fontclr, bgclr, bgclr_hover;
	if (available)
	{
		fontclr = 0x00ff00;
		bgclr = 0x507f7f7f;
		bgclr_hover = 0x507fff7f;
	}
	else
	{
		fontclr = 0x7f7f7f;
		bgclr = 0x50000000;
		bgclr_hover = 0x50ff0000;
	}
	if (price)
	{
		custom_entry.price = {
			Text = Format("{{Icon_Wealth}}<c %x>%d</c>", fontclr, price),
			Style = GUI_TextRight | GUI_TextBottom
		};
		custom_entry.Style = GUI_FitChildren;
	}
	if (hotkey)
	{
		custom_entry.hotkey = {
			Text = Format("<c %x>[%s]</c>", 0xffff00, hotkey),
			Style = GUI_TextRight | GUI_TextTop
		};
	}
	if (was_last_selection)
	{
		custom_entry.selection_marker = {
			Top = "100% - 2em", Right = "+50%", Symbol = Icon_Ok
		};
	}
	custom_entry.BackgroundColor = {Std = bgclr, OnHover = bgclr_hover };
	menu->AddItem(buy_def, nil, callback_idx, this, this.ClickBuyButton, callback_idx, custom_entry, nil, true);
	return true;
}

public func ClickBuyButton(int callback_idx, entry_id, int player)
{
	if (homebase && homebase[0])
	{
		if (player != homebase[0]->GetOwner()) return false; // wat?
		if (homebase[0]->OnBuySelection(callback_idx)) return true;
		Sound("DullMetalHit3", true, nil, player);
	}
	return false;
}


/* Buy menu open/close button */

private func CreateToggleVisibilityButton()
{
	var plr = GetOwner();
	// Place the button just below the wealth display
	// (in tenths of em)
	var margin = 5;
	var whole = margin + 25;
	
	var hotkey_string = GetPlayerControlAssignment(GetOwner(), CON_ToggleShop, true);
	if (hotkey_string && GetLength(hotkey_string) > 0)
		hotkey_string = Format("|<c ffff00>[%s]</c>", hotkey_string);
	else
		hotkey_string = "";

	var buymenu_button_menu =
	{
		Target = this,
		Player = plr,
		Style = GUI_Multiple | GUI_TextHCenter | GUI_TextBottom,
		Left = Format("100%%%s", ToEmString(-whole)),
		Right = Format("100%%%s", ToEmString(-margin)),
		Top = ToEmString(margin+whole),
		Bottom = ToEmString(2*whole),
		Priority = 1, // Z order?
		// Hover child element because a root window cannot collect clicks properly
		hover = {
			OnMouseIn = GuiAction_SetTag("OnHover"),
			OnMouseOut = GuiAction_SetTag("Std"),
			OnClick = GuiAction_Call(this, "ToggleVisibility"),
			BackgroundColor = {Std = nil, OnHover = 0x50000000 },
			//GraphicsName = GetGraphicsName(is_open),
			Symbol = { Std=Library_Base, OnHover=Library_Base },
			Text = { Std=Format("$Buy$%s", hotkey_string), OnHover=Format("<c ffff00>$Buy$</c>%s", hotkey_string) },
		}
	};
	buymenu_toggle_id = GuiOpen(buymenu_button_menu);
	
	return true;
}

private func DestroyToggleVisibilityButton()
{
	GuiClose(buymenu_toggle_id);
	return true;
}

public func ToggleVisibility(int player, ...)
{
	if (!menu) return false;
	if (menu.Visibility == VIS_Owner)
	{
		// Close menu
		menu.Visibility = VIS_None;
		Sound("CloseBuyMenu", true, nil, GetOwner());
	}
	else
	{
		// Open menu
		menu.Visibility = VIS_Owner;
		Sound("OpenBuyMenu", true, nil, GetOwner());
	}
	return true;
}
