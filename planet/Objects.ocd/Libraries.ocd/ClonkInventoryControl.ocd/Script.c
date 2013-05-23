/**
	ClonkInventoryControl
	Handles the Clonk's interaction with the inventory.

*/


/*
	used properties:
	this.inventory.last_slot: last inventory-slot that has been selected. Used for QuickSwitching
	this.inventory.handslot_choice_pending: used to determine if a slot-hotkey (1-9) has already been handled by a mouseclick

	other used properties of "this.inventory" might have been declared in Inventory.ocd
	
	this.control.hotkeypressed: declared in ClonkControl.ocd
*/


func Construction()
{
	if(this.inventory == nil)
		this.inventory = {};
	this.inventory.last_slot = 0;
	this.inventory.handslot_choice_pending = false;
	return _inherited(...);
}

func RejectCollect(id objid, object obj)
{
	var rejected = _inherited(objid,obj,...);
	if(rejected) return rejected;
	
	// check if the hand slot is full. If the overloaded
	// Collect() is called, this check will be skipped
	if (!this.inventory.force_collection)
		if (this->GetHandItem(0))
			return true;
	
	return false;
}

public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (!this) 
		return inherited(plr, ctrl, x, y, strength, repeat, release, ...);
		
	// Quickswitch changes the active slot to the last selected one
	if (ctrl == CON_QuickSwitch)
	{
		// but ignore quickswitch if we have more than 1 hand-slot
		if(this->HandObjects() > 1)
			return inherited(plr, ctrl, x, y, strength, repeat, release, ...);;
		
		// select last slot
		SetHandItemPos(0, this.inventory.last_slot); // last_slot is updated in SetHandItemPos
		return true;
	}
	
	if (ctrl == CON_InventoryShiftForward)
	{
		ShiftContents();
		return true;
	}
	if (ctrl == CON_InventoryShiftBackward)
	{
		ShiftContents(true);
		return true;
	}
	
	// hotkeys (inventory, vehicle and structure control)
	var hot = 0;
	if (ctrl == CON_InteractionHotkey0) hot = 10;
	if (ctrl == CON_InteractionHotkey1) hot = 1;
	if (ctrl == CON_InteractionHotkey2) hot = 2;
	if (ctrl == CON_InteractionHotkey3) hot = 3;
	if (ctrl == CON_InteractionHotkey4) hot = 4;
	if (ctrl == CON_InteractionHotkey5) hot = 5;
	if (ctrl == CON_InteractionHotkey6) hot = 6;
	if (ctrl == CON_InteractionHotkey7) hot = 7;
	if (ctrl == CON_InteractionHotkey8) hot = 8;
	if (ctrl == CON_InteractionHotkey9) hot = 9;
	
	if (hot > 0)
	{
		this.control.hotkeypressed = true; // see ClonkControl.ocd
		this->~ControlHotkey(hot-1);
		return true;
	}
	
	// dropping items via hotkey
	hot = 0;
	if (ctrl == CON_DropHotkey0) hot = 10;
	if (ctrl == CON_DropHotkey1) hot = 1;
	if (ctrl == CON_DropHotkey2) hot = 2;
	if (ctrl == CON_DropHotkey3) hot = 3;
	if (ctrl == CON_DropHotkey4) hot = 4;
	if (ctrl == CON_DropHotkey5) hot = 5;
	if (ctrl == CON_DropHotkey6) hot = 6;
	if (ctrl == CON_DropHotkey7) hot = 7;
	if (ctrl == CON_DropHotkey8) hot = 8;
	if (ctrl == CON_DropHotkey9) hot = 9;
	
	if (hot > 0)
	{
		this->~DropInventoryItem(hot-1);
		return true;
	}
	
	// this wall of text is called when 1-0 is beeing held, and left or right mouse button is pressed.
	var hand = 0;
	hot = 0;
	if (ctrl == CON_Hotkey0Select) hot = 10;
	if (ctrl == CON_Hotkey1Select) hot = 1;
	if (ctrl == CON_Hotkey2Select) hot = 2;
	if (ctrl == CON_Hotkey3Select) hot = 3;
	if (ctrl == CON_Hotkey4Select) hot = 4;
	if (ctrl == CON_Hotkey5Select) hot = 5;
	if (ctrl == CON_Hotkey6Select) hot = 6;
	if (ctrl == CON_Hotkey7Select) hot = 7;
	if (ctrl == CON_Hotkey8Select) hot = 8;
	if (ctrl == CON_Hotkey9Select) hot = 9;
	
	if(hot > 0  && hot <= this->MaxContentsCount())
	{
		SetHandItemPos(hand, hot-1);
		this->~OnInventoryHotkeyRelease(hot-1);
		return true;
	}
	
	// inventory
	hot = 0;
	if (ctrl == CON_Hotkey0) hot = 10;
	if (ctrl == CON_Hotkey1) hot = 1;
	if (ctrl == CON_Hotkey2) hot = 2;
	if (ctrl == CON_Hotkey3) hot = 3;
	if (ctrl == CON_Hotkey4) hot = 4;
	if (ctrl == CON_Hotkey5) hot = 5;
	if (ctrl == CON_Hotkey6) hot = 6;
	if (ctrl == CON_Hotkey7) hot = 7;
	if (ctrl == CON_Hotkey8) hot = 8;
	if (ctrl == CON_Hotkey9) hot = 9;
	
	// only the last-pressed key is taken into consideration.
	// if 2 hotkeys are held, the earlier one is beeing treated as released
	if (hot > 0 && hot <= this->MaxContentsCount())
	{
		// if released, we chose, if not chosen already
		if(release)
		{
			if(this.inventory.handslot_choice_pending == hot)
			{
				SetHandItemPos(0, hot-1);
				this->~OnInventoryHotkeyRelease(hot-1);
			}
		}
		// else we just highlight
		else
		{
			if(this.inventory.handslot_choice_pending)
			{
				this->~OnInventoryHotkeyRelease(this.inventory.handslot_choice_pending-1);
			}
			this.inventory.handslot_choice_pending = hot;
			this->~OnInventoryHotkeyPress(hot-1);
		}
		
		return true;
	}	
	
	// Collecting
	if (ctrl == CON_Collect)
	{
		// only if not inside something
		if(Contained()) return false; // not handled
		
		var dx = -GetDefWidth()/2, dy = -GetDefHeight()/2;
		var wdt = GetDefWidth(), hgt = GetDefHeight()+2;
		var obj = FindObject(Find_InRect(dx,dy,wdt,hgt), Find_OCF(OCF_Collectible), Find_NoContainer());
		if(obj)
		{
			// hackery to prevent the clonk from rejecting the collect because it is not being
			// collected into the hands
			this->Collect(obj,nil,nil,true);
		}
		
		// return not handled to still receive other controls - collection should not block anything else
		return false;
	}
}

// used in Inventory.ocd
public func SetHandItemPos(int hand, int inv)
{
	// save current slot
	if(hand == 0)
		this.inventory.last_slot = this->GetHandItemPos(0);
		
	var r = _inherited(hand, inv, ...);
	this.inventory.handslot_choice_pending = false;
	return r;
}
/* Backpack control */
func Selected(object mnu, object mnu_item)
{
	var backpack_index = mnu_item->GetExtraData();
	var hands_index = 0;
	// Update menu
	var show_new_item = this->GetItem(hands_index);
	mnu_item->SetSymbol(show_new_item);
	// swap index with backpack index
	this->Switch2Items(hands_index, backpack_index);
}