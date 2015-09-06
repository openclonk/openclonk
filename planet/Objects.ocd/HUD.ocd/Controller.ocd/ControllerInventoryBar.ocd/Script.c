/**
	ControllerInventoryBar
	Controlls the inventory bar interaction and layout.

	@author Zapper
*/

/*
	inventory_slot contains an array of proplists with the following attributes:
		ID: submenu ID. Unique in combination with the target == GetInventoryGuiTarget
		obj: last object that was shown here
		hand: bool, whether select with a hand
*/

// all values given in 10 em (-> 10 = 1.0em)
static const GUI_Controller_InventoryBar_IconMarginScreenBottom = 1; // margin from left border of screen
static const GUI_Controller_InventoryBar_IconSize = 20;
static const GUI_Controller_InventoryBar_IconMargin = 5;

local inventory_slots;
local inventory_gui_target;
local inventory_gui_id;

func GetInventoryGuiID()
{
	if (inventory_gui_id) return inventory_gui_id;
	var position_y_offset = -(GUI_Controller_InventoryBar_IconMarginScreenBottom + GUI_Controller_InventoryBar_IconSize);
	
	var menu =
	{
		Target = GetInventoryGuiTarget(),
		Style = GUI_Multiple | GUI_IgnoreMouse | GUI_NoCrop,
		Top = ToEmString(position_y_offset),
		OnClose = GuiAction_Call(this, "OnInventoryGuiClose")
		
	};
	inventory_gui_id = GuiOpen(menu);
	return inventory_gui_id;
}

func OnInventoryGuiClose()
{
	inventory_gui_id = nil;
}

// this function returns a dummy object that is used as the custom GUI target by the inventory menu
func GetInventoryGuiTarget()
{
	if (inventory_gui_target)
		return inventory_gui_target;
	inventory_gui_target = CreateObject(Dummy, AbsX(0), AbsY(0), GetOwner());
	inventory_gui_target.Visibility = VIS_Owner;
	return inventory_gui_target;
}

func Construction()
{
	inventory_gui_target = nil;
	inventory_slots = [];
	return _inherited(...);
}

func Destruction()
{
	// this also closes the menu
	if (inventory_gui_target)
		inventory_gui_target->RemoveObject();
		
	return _inherited(...);
}


func OnClonkRecruitment(object clonk, int plr)
{
	ScheduleUpdateInventory();
	return _inherited(clonk, plr, ...);
}

public func OnCrewDisabled(object clonk)
{
	ScheduleUpdateInventory();
	return _inherited(clonk, ...);
}

public func OnCrewSelection(object clonk, bool deselect)
{
	ScheduleUpdateInventory();
	return _inherited(clonk, deselect, ...);
}

// call from HUDAdapter or inventory-buttons
public func OnHandSelectionChange(int old, int new, int handslot)
{
	GuiUpdateTag("Std", GetInventoryGuiID(), inventory_slots[old].ID + 1000, GetInventoryGuiTarget());
	GuiUpdateTag("Selected", GetInventoryGuiID(), inventory_slots[new].ID + 1000, GetInventoryGuiTarget());
	
	OnSlotObjectChanged(handslot);
	return _inherited(old, new, handslot, ...);
}

// call from HUDAdapter (Clonk)
public func OnSlotObjectChanged(int slot)
{	
	// refresh inventory
	ScheduleUpdateInventory();
	return _inherited(slot, ...);
}

// Updates the Inventory in 1 frame
func ScheduleUpdateInventory()
{
	if (!GetEffect("UpdateInventory", this))
		AddEffect("UpdateInventory", this, 1, 1, this);
}

func FxUpdateInventoryTimer()
{
	UpdateInventory();
	return -1;
}

/* Inventory stuff */
func UpdateInventory()
{
	// only display if we have a clonk
	var clonk = GetCursor(GetOwner());
	if(!clonk)
	{
		GetInventoryGuiTarget().Visibility = VIS_None;
		return;
	}
	GetInventoryGuiTarget().Visibility = VIS_Owner;
	UpdateInventoryButtons(clonk);
	
	// update inventory-slots
	var hand_item_pos = clonk->GetHandItemPos(0);
	
	for (var slot_info in inventory_slots)
	{
		var item = clonk->GetItem(slot_info.slot);
		var needs_selection = hand_item_pos == slot_info.slot;
		var has_extra_slot = item && item->~HasExtraSlot();
		if ((!!item == slot_info.empty) || (item != slot_info.obj) || (needs_selection != slot_info.hand) || has_extra_slot)
		{
			// Hide or show extra-slot display?
			var extra_slot_player = NO_OWNER;
			var extra_symbol = nil;
			var contents = nil;
			var extra_slot_background_symbol = nil;
			if (has_extra_slot)
			{
				// Show!
				contents = item->Contents(0);
				if (contents)
					extra_symbol = contents->GetID();
				extra_slot_player = nil;
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
			if (extra_symbol && contents->~GetStackCount())
			{
				if (contents->IsInfiniteStackCount())
					number_symbol = Icon_Number;
				else extra_text = Format("%dx", contents->GetStackCount());
			}
			// If stackable itself, add count.
			/*
				Disabled for now, as the stackable objects add the number to their picture as an image.
				Reenable as soon as we can use different (bigger) font-sizes and the stackable objects do not need to hack their picture.
			var count = nil;
			if (item) count = item->~GetStackCount();
			var count_text = nil;
			if (count > 1)
			{
				count_text = Format("%dx", count);
			}*/

			// Compose the update!
			var update = 
			{
				icon = { Symbol = item/*, Text = count_text*/},
				extra_slot =
				{
					Player = extra_slot_player,
					text = {Text = extra_text, Symbol = number_symbol},
					circle =
					{
						Symbol = extra_slot_background_symbol,
						symbol = {Symbol = extra_symbol}
					}
				}
			};
			GuiUpdate(update, GetInventoryGuiID(), slot_info.ID, GetInventoryGuiTarget());
			var tag = "Std";
			if (needs_selection) tag = "Selected";
			GuiUpdateTag(tag, GetInventoryGuiID(), slot_info.ID, GetInventoryGuiTarget());
			slot_info.hand = needs_selection;
			slot_info.obj = item;
			slot_info.empty = !item;
		}
	}
}	

func FxExtraSlotUpdaterTimer(object target, proplist effect)
{
	if (!this) return -1;
	if (target->Contained() != GetCursor(GetOwner())) return -1;
	return 1;
}

func FxExtraSlotUpdaterUpdate(object target, proplist effect)
{
	ScheduleUpdateInventory();
}

// Calculates the position of a specific button and returns a proplist.
public func CalculateButtonPosition(int slot_number, int max_slots)
{
	var pos_x_offset = -((GUI_Controller_InventoryBar_IconSize + GUI_Controller_InventoryBar_IconMargin) * max_slots - GUI_Controller_InventoryBar_IconMargin) / 2;
	var pos_x = pos_x_offset + (GUI_Controller_InventoryBar_IconSize + GUI_Controller_InventoryBar_IconMargin) * slot_number;
	var pos_y = - (GUI_Controller_InventoryBar_IconMarginScreenBottom + GUI_Controller_InventoryBar_IconSize);
	var pos =
	{
		Left = Format("50%%%s", ToEmString(pos_x)),
		Top = Format("100%%%s", ToEmString(pos_y)),
		Right = Format("50%%%s", ToEmString(pos_x + GUI_Controller_InventoryBar_IconSize)),
		Bottom = Format("100%%%s", ToEmString(pos_y + GUI_Controller_InventoryBar_IconSize))
	};
	return pos;
}

// Insert an inventory slot into the inventory-bar
func CreateNewInventoryButton(int max_slots)
{
	var slot_number = GetLength(inventory_slots);
	var slot_info =
	{
		slot = slot_number,
		ID = slot_number + 1,
		hand = false,
		obj = nil,
		empty = true
	};
	PushBack(inventory_slots, slot_info);
	
	// the gui already exists, only update it with a new submenu
	var pos = CalculateButtonPosition(slot_number, max_slots);
	
	var icon = 
	{
		Target = GetInventoryGuiTarget(),
		Style = GUI_NoCrop,
		ID = slot_info.ID,
		Symbol = {Std = Icon_Menu_Circle, Selected = Icon_Menu_CircleHighlight},
		Left = pos.Left, Top = pos.Top, Right = pos.Right, Bottom = pos.Bottom,
		Text = Format("%2d", slot_info.slot + 1),
		icon = 
		{
			Target = GetInventoryGuiTarget(),
			ID = 1000 + slot_info.ID,
			Style = GUI_TextRight | GUI_TextBottom,
			Text = nil,
			Priority = 2
		},
		// Prepare (invisible) extra-slot display circle.
		extra_slot = 
		{
			Top = ToEmString(-GUI_Controller_InventoryBar_IconSize/2),
			Bottom = "0em",
			text =
			{
				Right = ToEmString(GUI_Controller_InventoryBar_IconSize/2),
				Style = GUI_TextRight,
				GraphicsName = "Inf", // sometimes used with Icon_Number
				Text = nil,
			},
			circle = 
			{
				Left = ToEmString(GUI_Controller_InventoryBar_IconSize/2),
				Symbol = nil,
				symbol = {}
			}
		}
	};
	GuiUpdate({_new_icon = icon}, GetInventoryGuiID(), 0);	
	//return bt;
}

// sets the inventory size to the currently selected clonk
private func UpdateInventoryButtons(object clonk)
{
	var max_contents_count = clonk->~MaxContentsCount();
	
	var old_count = GetLength(inventory_slots);
	// need to create more inventory buttons?
	while (max_contents_count > GetLength(inventory_slots))
		CreateNewInventoryButton(max_contents_count);

	// need to remove some inventory buttons?
	while (max_contents_count < GetLength(inventory_slots))
	{
		var slot_info = inventory_slots[-1];
		GuiClose(GetInventoryGuiID(), slot_info.ID, GetInventoryGuiTarget());
		SetLength(inventory_slots, GetLength(inventory_slots)-1);
	}
	
	// modifications occured? Adjust position of old slots
	if (old_count != max_contents_count)
	{
		var gui_id = GetInventoryGuiID();
		var gui_target = GetInventoryGuiTarget();
		for (var i = 0; i < old_count; ++i)
		{
			var slot_info = inventory_slots[i];
			var update = CalculateButtonPosition(i, max_contents_count);
			GuiUpdate(update, gui_id, slot_info.ID, gui_target);
		}
	}
}

// Shows the Carryheavy-Inventoryslot if obj is set
// Removes it if it's nil
func OnCarryHeavyChange(object obj)
{
	// TODO
	
	UpdateInventory();
}
