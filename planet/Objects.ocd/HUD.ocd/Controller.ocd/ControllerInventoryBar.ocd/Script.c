/**
	ControllerInventoryBar

	Displays inventory slots and extra information.

	@authors Zapper, Clonkonaut
*/

/*
	inventory_slot contains an array of proplists with the following attributes:
		ID: submenu ID. Unique in combination with the target == this
		obj: last object that was shown here
		hand: bool, whether select with a hand
		quick: bool, whether this is the quick switch slot
*/

// HUD margin and size in tenths of em.
static const GUI_Controller_InventoryBar_IconMarginScreenTop = 5;
static const GUI_Controller_InventoryBar_IconSize = 20;
static const GUI_Controller_InventoryBar_IconMargin = 5;

local inventory_slots;
local inventory_gui_menu;
local inventory_gui_id;

/* GUI creation */

// For custom HUD graphics overload the following function as deemed fit.

func AssembleInventoryButton(int max_slots, int slot_number, proplist slot_info)
{
	// The gui already exists, only update it with a new submenu
	var pos = CalculateButtonPosition(slot_number, max_slots);

	return
	{
		Target = this,
		slot_number =
		{
			Priority = 3, // Make sure the slot number is drawn above the icon.
			Style = GUI_TextTop,
			Text = Format("%2d", slot_info.slot + 1)
		},
		quick_switch = // Shows quick switch control key if this is the quick switch slot
		{
			Priority = 3,
			Style = GUI_NoCrop | GUI_TextHCenter | GUI_TextBottom,
			Left = "-50%",
			Right = "150%",
			Top = Format(" %s%s", "20%", ToEmString(-2)),
			Bottom = "20%",
			Text = { Std = "", Quick = Format("<c dddd00>[%s]</c>", GetPlayerControlAssignment(GetOwner(), CON_QuickSwitch, true)), Selected = "" }
		},
		Style = GUI_NoCrop,
		ID = slot_info.ID,
		Symbol = {Std = Icon_Menu_Circle, Quick = Icon_Menu_Circle, Selected = Icon_Menu_CircleHighlight},
		Left = pos.Left, Top = pos.Top, Right = pos.Right, Bottom = pos.Bottom,
		count =
		{
			ID = 1000 + slot_info.ID,
			Style = GUI_TextRight | GUI_TextBottom,
			Text = nil,
			Priority = 2
		},
		// Prepare (invisible) extra-slot display circle.
		extra_slot =
		{
			Top = ToEmString(GUI_Controller_InventoryBar_IconSize),
			Bottom = ToEmString(GUI_Controller_InventoryBar_IconSize + GUI_Controller_InventoryBar_IconSize/2),
			Style = GUI_TextLeft,
			Text = nil,
			symbol =// used to display an infinity sign if necessary (Icon_Number)
			{
				Right = ToEmString(GUI_Controller_InventoryBar_IconSize/2),
				GraphicsName = "Inf",
			},
			circle =// shows the item in the extra slot
			{
				Left = ToEmString(GUI_Controller_InventoryBar_IconSize/2),
				Symbol = nil,
				symbol = {}
			}
		},
		overlay = // Custom inventory overlays can be shown here.
		{
			ID = 2000 + slot_info.ID
		}
	};
}

/* Creation / Destruction */

private func Construction()
{
	inventory_slots = [];

	inventory_gui_menu =
	{
		Target = this,
		Player = NO_OWNER, // will be shown once a gui update occurs
		Style = GUI_Multiple | GUI_IgnoreMouse | GUI_NoCrop
	};
	inventory_gui_id = GuiOpen(inventory_gui_menu);

	return _inherited(...);
}

private func Destruction()
{
	GuiClose(inventory_gui_id);

	_inherited(...);
}

/* Callbacks */

public func OnCrewDisabled(object clonk)
{
	ScheduleUpdateInventory();

	return _inherited(clonk, ...);
}

public func OnCrewEnabled(object clonk)
{
	ScheduleUpdateInventory();

	return _inherited(clonk, ...);
}

public func OnCrewSelection(object clonk, bool deselect)
{
	ScheduleUpdateInventory();

	return _inherited(clonk, deselect, ...);
}

// call from HUDAdapter (Clonk)
public func OnSlotObjectChanged(int slot)
{
	// refresh inventory
	ScheduleUpdateInventory();

	return _inherited(slot, ...);
}

// Updates the Inventory in 1 frame
public func ScheduleUpdateInventory()
{
	if (!GetEffect("UpdateInventory", this))
		AddEffect("UpdateInventory", this, 1, 1, this);
}

private func FxUpdateInventoryTimer()
{
	UpdateInventory();
	return FX_Execute_Kill;
}

/* Display */

private func UpdateInventory()
{
	// only display if we have a clonk and it's not disabled
	var clonk = GetCursor(GetOwner());
	if(!clonk || !clonk->GetCrewEnabled())
	{
		if (inventory_gui_menu.Player != NO_OWNER)
		{
			inventory_gui_menu.Player = NO_OWNER;
			GuiUpdate(inventory_gui_menu, inventory_gui_id);
		}

		return;
	}

	// Make sure inventory is visible
	if (inventory_gui_menu.Player != GetOwner())
	{
		inventory_gui_menu.Player = GetOwner();
		GuiUpdate(inventory_gui_menu, inventory_gui_id);
	}

	UpdateInventoryButtons(clonk);

	// update inventory-slots
	var hand_item_pos = clonk->~GetHandItemPos(0);
	var quick_switch_slot = clonk->~GetQuickSwitchSlot();

	for (var slot_info in inventory_slots)
	{
		var item = clonk->GetItem(slot_info.slot);
		// Enable objects to provide a custom overlay for the icon slot.
		// This could e.g. be used by special scenarios or third-party mods.
		var custom_overlay = nil;
		// For stacked objects, we will have multiple virtual objects in one slot.
		var stack_count = nil;
		if (item)
		{
			stack_count = item->~GetStackCount();
			custom_overlay = item->~GetInventoryIconOverlay();
		}
		var needs_selection = hand_item_pos == slot_info.slot;
		var needs_quick_switch = quick_switch_slot == slot_info.slot;
		var has_extra_slot = item && item->~HasExtraSlot();
		if ((!!item == slot_info.empty) || (item != slot_info.obj) || (needs_selection != slot_info.hand) || (needs_quick_switch != slot_info.quick) || (stack_count != slot_info.last_count) || has_extra_slot || slot_info.had_custom_overlay || custom_overlay)
		{
			// Hide or show extra-slot display?
			var extra_slot_player = NO_OWNER;
			var extra_symbol = nil;
			var extra_symbol_stack_count = nil;
			var contents = nil;
			var extra_slot_background_symbol = nil;
			if (has_extra_slot)
			{
				// Show!
				contents = item->Contents(0);
				if (contents)
				{
					extra_symbol = contents;
					// Stack count: either actual stack count or stacked object count.
					extra_symbol_stack_count = contents->~GetStackCount();
					if (extra_symbol_stack_count == nil)
					{
					    // Stack count fallback to actually stacked objects
					    extra_symbol_stack_count = item->ContentsCount(contents->GetID());
					}
				}
				extra_slot_player = GetOwner();
				extra_slot_background_symbol = Icon_Menu_Circle;
				// And attach tracker..
				var i = 0, e = nil;
				var found = false;
				while (e = GetEffect("ExtraSlotUpdater", item, i++))
				{
					if (e.CommandTarget != this) continue;
					found = true;
					break;
				}
				if (!found) AddEffect("ExtraSlotUpdater", item, 1, 30 + Random(60), this);
			}
			// What to display in the extra slot?
			var extra_text = nil, number_symbol = nil;
			if (extra_symbol && extra_symbol_stack_count)
			{
				if (contents->~IsInfiniteStackCount())
					number_symbol = Icon_Number;
				else extra_text = Format("%dx", extra_symbol_stack_count);
			}
			
			// Close a possible lingering custom overlay for that slot.
			var custom_overlay_id = 2000 + slot_info.ID;
			GuiClose(inventory_gui_id, custom_overlay_id, nil);
			
			// Compose the update!
			var update =
			{
				slot = { Symbol = item },
				extra_slot =
				{
					Player = extra_slot_player,
					Text = extra_text,
					symbol =
					{
						Symbol = number_symbol
					},
					circle =
					{
						Symbol = extra_slot_background_symbol,
						symbol = { Symbol = extra_symbol }
					}
				},
				count = 
				{
					Text = ""
				}
			};
			
			if (item)
			{
				if (stack_count > 1 && !item->~IsInfiniteStackCount())
				{
					update.count.Text = Format("%dx", stack_count);
					slot_info.last_count = stack_count;
				}
			}
			else
			{
				slot_info.last_count = nil;
			}
			
			if (custom_overlay)
			{
				update.overlay = custom_overlay;
				update.overlay.ID = custom_overlay_id;
				slot_info.had_custom_overlay = true;
			}
			else
			{
				slot_info.had_custom_overlay = false;
			}
			
			GuiUpdate(update, inventory_gui_id, slot_info.ID, this);

			var tag = "Std";
			if (needs_quick_switch) tag = "Quick";
			if (needs_selection) tag = "Selected";
			GuiUpdateTag(tag, inventory_gui_id, slot_info.ID, this);

			slot_info.hand = needs_selection;
			slot_info.quick = needs_quick_switch;
			slot_info.obj = item;
			slot_info.empty = !item;
		}
	}
}

// Sets the inventory size to the currently selected clonk
private func UpdateInventoryButtons(object clonk)
{
	var max_contents_count = clonk.MaxContentsCount;

	var old_count = GetLength(inventory_slots);

	// need to create more inventory buttons?
	while (max_contents_count > GetLength(inventory_slots))
		CreateNewInventoryButton(max_contents_count);

	// need to remove some inventory buttons?
	while (max_contents_count < GetLength(inventory_slots))
	{
		var slot_info = inventory_slots[-1];
		GuiClose(inventory_gui_id, slot_info.ID, this);
		SetLength(inventory_slots, GetLength(inventory_slots)-1);
	}

	// modifications occured? Adjust position of old slots
	if (old_count != max_contents_count)
	{
		for (var i = 0; i < Min(old_count, max_contents_count); ++i)
		{
			var slot_info = inventory_slots[i];
			var update = CalculateButtonPosition(i, max_contents_count);
			GuiUpdate(update, inventory_gui_id, slot_info.ID, this);
		}
	}
}

// Insert an inventory slot into the inventory-bar
private func CreateNewInventoryButton(int max_slots)
{
	var slot_number = GetLength(inventory_slots);
	var slot_info =
	{
		slot = slot_number,
		ID = slot_number + 1,
		hand = false,
		quick = false,
		obj = nil,
		empty = true
	};
	PushBack(inventory_slots, slot_info);

	var slot = AssembleInventoryButton(max_slots, slot_number, slot_info);

	GuiUpdate({_new_icon = slot}, inventory_gui_id);
}

// Calculates the position of a specific button and returns a proplist.
private func CalculateButtonPosition(int slot_number, int max_slots)
{
	var pos_x_offset = -((GUI_Controller_InventoryBar_IconSize + GUI_Controller_InventoryBar_IconMargin) * max_slots - GUI_Controller_InventoryBar_IconMargin) / 2;
	var pos_x = pos_x_offset + (GUI_Controller_InventoryBar_IconSize + GUI_Controller_InventoryBar_IconMargin) * slot_number;
	var pos_y = GUI_Controller_InventoryBar_IconMarginScreenTop;
	var pos =
	{
		Left = Format("50%%%s", ToEmString(pos_x)),
		Top = Format("0%%%s", ToEmString(pos_y)),
		Right = Format("50%%%s", ToEmString(pos_x + GUI_Controller_InventoryBar_IconSize)),
		Bottom = Format("0%%%s", ToEmString(pos_y + GUI_Controller_InventoryBar_IconSize))
	};
	return pos;
}

private func FxExtraSlotUpdaterTimer(object target, proplist effect)
{
	if (!this) return FX_Execute_Kill;
	if (!target) return FX_Execute_Kill;
	if (target->Contained() != GetCursor(GetOwner())) return FX_Execute_Kill;
	return FX_OK;
}

private func FxExtraSlotUpdaterUpdate(object target, proplist effect)
{
	if (this) ScheduleUpdateInventory();
}