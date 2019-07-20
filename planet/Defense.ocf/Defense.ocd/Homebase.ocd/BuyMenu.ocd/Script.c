/**
	Buy Menu

	Shows icons to instantly buy + equip weapons

	@authors Sven2
*/

local container, container_id; // container window containing menu entries and other stuff
local menu; // the actual menu (a MenuStyle_Grid)
local description_box, description_name_id, description_desc_id, description_message_id; // box showing name + desc of currently hovered item
local homebase; // associated base
local buymenu_toggle_id; // button to toggle menu visibility
local hovered_entry_index; // keep track of hovered entry to update description box

/* Creation / Destruction */

public func Construction(...)
{
	// The menu. Cannot inherit from MenuStyle_Grid, because at the moment the menu host
	// is the object itself. Therefore, child elements such as the buy menu would become
	// invisible if the menu closes. So just create an extra object for now.
	menu = CreateObject(MenuStyle_Grid, 0, 0, GetOwner());
	menu->SetPermanent(true);
	menu->SetMouseOverCallback(this, "OnMenuEntryHover");
	menu->SetMouseOutCallback(this, "OnMenuEntryHoverExit");
	menu.Player = nil; // by visibility
	menu.Bottom = "100% - 5em";
	menu.BackgroundColor = 0x20000000;
	menu.Visibility = VIS_None;
	// Create description box
	description_box =
	{
		Top = menu.Bottom,
		Margin = ["0em", "0em"],
		BackgroundColor = 0x40000000,
		Target = this,
		ID = 0xffffef,
		name_part =
		{
			Bottom = "1em",
			Left = "0.2em",
			ID = description_name_id = 3,
			Target = this
		},
		desc_part =
		{
			Top = "1em",
			Bottom = "3em",
			ID = description_desc_id = 4,
			Target = this,
		},
		message_part =
		{
			Top = "3em",
			Bottom = "5em",
			ID = description_message_id = 5,
			Target = this,
		}
	};
	// overall container
	container =
	{
		Left = "100% - 14em",
		Top = "4em",
		Right = "100% - 1em",
		Bottom = "100% - 4em",
		Style = GUI_Multiple,
		Target = menu, // bind visibility to menu object
		ID = 0xffffee,
		
		menu = menu,
		description_box = description_box
	};
	// Create button to show/hide menu
	CreateToggleVisibilityButton();
	return true;
}

public func Destruction(...)
{
	if (container_id) GuiClose(container_id);
	if (menu) menu->RemoveObject();
	DestroyToggleVisibilityButton();
	return true;
}

public func SetHomebase(object to)
{
	homebase = [to];
	return true;
}

// Forward menu operations
public func Open()
{
	if (container_id) GuiClose(container_id);
	container_id = GuiOpen(container);
	hovered_entry_index = -1;
	return false;
}

public func RemoveItem(idx)
{
	OnMenuEntryHoverExit(idx, idx, GetOwner()); // remove description
	if (menu) return menu->RemoveItem(idx, container_id);
	
	return false;
}


/* Buy menu entries */

public func UpdateCaption(string title, bool available, proplist entry, int item_idx)
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
	menu->AddItem(GetID(), nil, item_idx, this, this.ClickCaption, item_idx, custom_entry, container_id, true);
	return true;
}

public func ClickCaption() { return true; } // nothing to be done here (maybe expand/collapse in the future)

public func UpdateBuyEntry(id buy_def, bool available, proplist entry, int callback_idx, bool was_last_selection)
{
	if (!menu) return false;
	var custom_entry = {Bottom = "+2em", Right = Format("+%dem", 2 + entry.extra_width), Symbol = buy_def, GraphicsName = entry.graphic }, fontclr, bgclr, bgclr_hover;
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
	if (entry.cost)
	{
		custom_entry.price = {
			Text = Format("{{Icon_Wealth}}<c %x>%d</c>", fontclr, entry.cost),
			Style = GUI_TextRight | GUI_TextBottom
		};
		custom_entry.Style = GUI_FitChildren;
	}
	if (entry.hotkey)
	{
		custom_entry.hotkey = {
			Text = Format("<c %x>[%s]</c>", 0xffff00, entry.hotkey),
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
	menu->AddItem(buy_def, nil, callback_idx, this, this.ClickBuyButton, callback_idx, custom_entry, container_id, true);
	return true;
}

public func ClickBuyButton(int callback_idx, entry_id, int player)
{
	if (homebase && homebase[0])
	{
		if (player != homebase[0]->GetOwner()) return false; // wat?
		if (homebase[0]->OnBuySelection(callback_idx)) return true;
		Sound("Hits::Materials::Metal::DullMetalHit3", true, nil, player);
	}
	return false;
}

public func OnMenuEntryHover(int entry_idx, int entry_id, int player)
{
	if (homebase && homebase[0])
	{
		if (player != homebase[0]->GetOwner()) return false; // wat?
		var info = homebase[0]->GetEntryInformation(entry_idx);
		if (!info) info = {}; // clear boxes on invalid
		GuiUpdateText(Format("<c ffff00>%s</c>", info.name ?? ""), container_id, description_name_id, this);
		GuiUpdateText(info.desc ?? "", container_id, description_desc_id, this);
		GuiUpdateText(info.message ?? "", container_id, description_message_id, this);
		hovered_entry_index = entry_idx;
		return true;
	}
	return false;
}

public func OnMenuEntryHoverExit(int entry_idx, int entry_id, int player)
{
	if (homebase && homebase[0])
	{
		if (player != homebase[0]->GetOwner()) return false; // wat?
		if (hovered_entry_index == entry_idx) // must check for index because next Hover callback might have arrived before HoverExit callback
		{
			GuiUpdateText("", container_id, description_name_id, this);
			GuiUpdateText("", container_id, description_desc_id, this);
			GuiUpdateText("", container_id, description_message_id, this);
		}
		return true;
	}
	return false;
}


/* Buy menu open/close button */

private func CreateToggleVisibilityButton()
{
	var plr = GetOwner();
	
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
		// Place the button just left of the wealth display
		Left = "100% - 9em",
		Right = "100% - 6.5em",
		Top = "0.5em",
		Bottom = "3em",
		Priority = 1, // Z order?
		// Hover child element because a root window cannot collect clicks properly
		hover = {
			OnMouseIn = GuiAction_SetTag("OnHover"),
			OnMouseOut = GuiAction_SetTag("Std"),
			OnClick = GuiAction_Call(this, "ToggleVisibility"),
			BackgroundColor = {Std = nil, OnHover = 0x50000000 },
			//GraphicsName = GetGraphicsName(is_open),
			Symbol = { Std = Icon_Buy, OnHover = Icon_Buy },
			Text = { Std = Format("$Buy$%s", hotkey_string), OnHover = Format("<c ffff00>$Buy$</c>%s", hotkey_string) },
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
		Sound("UI::Close", true, nil, GetOwner());
	}
	else
	{
		// Open menu
		menu.Visibility = VIS_Owner;
		Sound("UI::Open", true, nil, GetOwner());
	}
	return true;
}
