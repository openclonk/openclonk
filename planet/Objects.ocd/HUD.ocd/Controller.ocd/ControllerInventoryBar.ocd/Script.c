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

static const GUI_Controller_InventoryBar_IconMarginScreenBottom = 5; // margin from left border of screen
static const GUI_Controller_InventoryBar_IconSize = 64;
static const GUI_Controller_InventoryBar_IconMargin = 20;

local inventory_slots;
local inventory_gui_target;
local inventory_gui_id;

local progress_bar_links;

func GetCurrentGuiID()
{
	if (inventory_gui_id) return inventory_gui_id;
	var menu =
	{
		Target = GetInventoryGuiTarget(),
		Style = GUI_Multiple | GUI_IgnoreMouse | GUI_NoCrop,
		Y = [1000, -(GUI_Controller_InventoryBar_IconMarginScreenBottom + GUI_Controller_InventoryBar_IconSize)],
		OnClose = GuiAction_Call(this, "OnGuiClose")
		
	};
	inventory_gui_id = CustomGuiOpen(menu);
	return inventory_gui_id;
}

func OnGuiClose()
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
	progress_bar_links = [];
	inventory_slots = [];
	return _inherited(...);
}

func Destruction()
{
	if (inventory_gui_target)
		inventory_gui_target->RemoveObject();
		
	var menu = GetCurrentGuiID();
	if (menu)
	{
		CustomGuiClose(menu, nil, this);
	}
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
	CustomGuiSetTag("Std", GetCurrentGuiID(), inventory_slots[old].ID + 1000, GetInventoryGuiTarget());
	CustomGuiSetTag("Selected", GetCurrentGuiID(), inventory_slots[new].ID + 1000, GetInventoryGuiTarget());
	
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

	// sort out old progress bars
	if(GetLength(progress_bar_links))
	{
		var old_progress_bar_links = progress_bar_links[:];
		progress_bar_links = [];
		
		for(var bar in old_progress_bar_links)
		{
			if(!bar.effect) continue;
			PushBack(progress_bar_links, bar);
		}
	}
	
	// update inventory-slots
	var hand_item_pos = clonk->GetHandItemPos(0);
	
	for (var slot_info in inventory_slots)
	{
		var item = clonk->GetItem(slot_info.slot);
		var needs_selection = hand_item_pos == slot_info.slot;
		if ((item != slot_info.obj) || (needs_selection != slot_info.hand))
		{
			var update = { Symbol = item };
			CustomGuiUpdate(update, GetCurrentGuiID(), 1000 + slot_info.ID, GetInventoryGuiTarget());
			var tag = "Std";
			if (needs_selection) tag = "Selected";
			CustomGuiSetTag(tag, GetCurrentGuiID(), slot_info.ID, GetInventoryGuiTarget());
			slot_info.hand = needs_selection;
		}
		
		//inventory[i]->ClearProgressBarLink();
		// re-add progress bar if possible
		for(var bar in progress_bar_links)
		{
			if(bar.obj != item) continue;
			//inventory[i]->SetProgressBarLink(bar.effect);
			break;
		}
	}
}	

// sets the link of the progress bar for a certain slot
// the link is an effect that has the properties "max" and "current"
func SetProgressBarLinkForObject(object what, proplist e)
{
	PushBack(progress_bar_links, {obj = what, effect = e});
	ScheduleUpdateInventory();
}


func CalculateButtonPosition(int slot_number, int max_slots)
{
	var pos_x_offset = -((GUI_Controller_InventoryBar_IconSize + GUI_Controller_InventoryBar_IconMargin) * max_slots - GUI_Controller_InventoryBar_IconMargin) / 2;
	var pos_x = pos_x_offset + (GUI_Controller_InventoryBar_IconSize + GUI_Controller_InventoryBar_IconMargin) * slot_number;
	var pos_y = - (GUI_Controller_InventoryBar_IconMarginScreenBottom + GUI_Controller_InventoryBar_IconSize);
	var pos =
	{
		X = [500, pos_x], Y = [1000, pos_y],
		Wdt = [500, pos_x + GUI_Controller_InventoryBar_IconSize],
		Hgt = [1000, pos_y + GUI_Controller_InventoryBar_IconSize],
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
		obj = nil
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
		X = [pos.X[0], {Std = pos.X[1], Selected = pos.X[1] - 16}],
		Y = [pos.Y[0], {Std = pos.Y[1], Selected = pos.Y[1] - 16}],
		Wdt = [pos.Wdt[0], {Std = pos.Wdt[1], Selected = pos.Wdt[1] + 16}],
		Hgt = [pos.Hgt[0], {Std = pos.Hgt[1], Selected = pos.Hgt[1] + 16}],
		Text = Format("%2d", slot_info.slot + 1),
		icon = 
		{
			Target = GetInventoryGuiTarget(),
			ID = 1000 + slot_info.ID,
			X = [0, {Std = 0, Selected = -16}],
			Y = [0, {Std = 0, Selected = -16}],
			Wdt = [1000, {Std = 0, Selected = 16}],
			Hgt = [1000, {Std = 0, Selected = 16}]
		}
	};
	CustomGuiUpdate({new_icon = icon}, GetCurrentGuiID(), 0);	
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
		CustomGuiClose(GetCurrentGuiID(), slot_info.ID, GetInventoryGuiTarget());
		SetLength(inventory_slots, GetLength(inventory_slots)-1);
	}
	
	// modifications occured? Adjust position of old slots
	if (old_count != max_contents_count)
	{
		var gui_id = GetCurrentGuiID();
		var gui_target = GetInventoryGuiTarget();
		for (var i = 0; i < old_count; ++i)
		{
			var slot_info = inventory_slots[i];
			var update = CalculateButtonPosition(i, max_contents_count);
			CustomGuiUpdate(update, gui_id, slot_info.ID, gui_target);
		}
	}
}
