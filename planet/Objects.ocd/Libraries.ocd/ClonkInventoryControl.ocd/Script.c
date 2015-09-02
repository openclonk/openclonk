/**
	ClonkInventoryControl
	Handles the Clonk's interaction with the inventory.

*/


/*
	used properties:
	this.inventory.last_slot: last inventory-slot that has been selected. Used for QuickSwitching

	other used properties of "this.inventory" might have been declared in Inventory.ocd
	
	this.control.hotkeypressed: declared in ClonkControl.ocd
*/


func Construction()
{
	if(this.inventory == nil)
		this.inventory = {};
	this.inventory.last_slot = 0;
	return _inherited(...);
}

// Called by other libraries and objects when the Clonk has forcefully dropped (not thrown) an object.
func OnDropped(object obj)
{
	return _inherited(obj, ...);
}

func RejectCollect(id objid, object obj)
{
	var rejected = _inherited(objid, obj, ...);
	if(rejected) return rejected;
	
	// Allow collection only if called via clonk->Collect, to prevent picking up stuff on the ground.
	// Make an exception for containers, though.
	if (!this.inventory.force_collection && !obj->Contained()) return true;
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
	
	// Begin picking up objects.
	if (ctrl == CON_PickUp && !release)
	{
		BeginPickingUp();
		return true;
	}
	
	// Switching pickup object or finish pickup?
	if (this.inventory.is_picking_up)
	{
		// Stop picking up.
		if (ctrl == CON_PickUpNext_Stop)
		{
			this.inventory.pickup_item = nil;
			EndPickingUp();
			return true;
		}
		
		// Finish picking up (aka "collect").
		if (ctrl == CON_PickUp && release)
		{
			EndPickingUp();
			return true;
		}
		
		// Switch left/right through objects.
		var dir = nil;
		if (ctrl == CON_PickUpNext_Left) dir = -1;
		else if (ctrl == CON_PickUpNext_Right) dir = 1;
		
		if (dir != nil)
		{
			var item = FindNextPickupObject(this.inventory.pickup_item, dir);
			if (item)
				SetNextPickupItem(item);
			return true;
		}
	}
	
	
	// shift inventory
	var inventory_shift = 0;
	if (ctrl == CON_InventoryShiftForward) inventory_shift = 1;
	else if (ctrl == CON_InventoryShiftBackward) inventory_shift = -1;
	
	if (inventory_shift)
	{
		var current = (this->GetHandItemPos(0) + inventory_shift) % this->MaxContentsCount();
		if (current < 0) current = this->MaxContentsCount() + current;
		this->SetHandItemPos(0, current);
		return true;
	}
	
	var hot;
	
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
	// if 2 hotkeys are held, the earlier one is being treated as released
	if (hot > 0 && hot <= this->MaxContentsCount())
	{
		SetHandItemPos(0, hot-1);
		return true;
	}
	
	return inherited(plr, ctrl, x, y, strength, repeat, release, ...);
}

private func FxIntHighlightItemStart(object target, proplist fx, temp, object item)
{
	if (temp) return;
	fx.item = item;
	
	fx.dummy = CreateObject(Dummy, item->GetX() - GetX(), item->GetY() - GetY(), GetOwner());
	fx.dummy.ActMap = 
	{
		Attach = 
		{
			Name = "Attach",
			Procedure = DFA_ATTACH,
			FaceBase = 1
		}
	};
	fx.dummy.Visibility = VIS_Owner;
	fx.dummy.Plane = 1000;
	fx.dummy->Message("@%s", item->GetName());
	
	// Center dummy!
	fx.dummy->SetVertexXY(0, item->GetVertex(0, VTX_X), item->GetVertex(0, VTX_Y));
	fx.dummy->SetAction("Attach", item);
	
	fx.width  = item->GetDefWidth();
	fx.height = item->GetDefHeight();
	
	// Draw the item's graphics in front of it again to achieve a highlighting effect.
	fx.dummy->SetGraphics(nil, nil, 1, GFXOV_MODE_Object, nil, GFX_BLIT_Additive, item);
	
	// Draw a nice selector particle on item change.
	var selector =
	{
		Size = PV_Step(3, 2, 1, Max(fx.width, fx.height)),
		Attach = ATTACH_Front,
		Rotation = PV_Step(1, PV_Random(0, 360), 1),
		Alpha = 200
	};
	
	fx.dummy->CreateParticle("Selector", 0, 0, 0, 0, 0, Particles_Colored(selector, GetPlayerColor(GetOwner())), 1); 
}

private func FxIntHighlightItemTimer(object target, proplist fx, int time)
{
	if (!fx.dummy) return -1;
	if (!fx.item) return -1;
	if (ObjectDistance(this, fx.item) > 20) return -1;
	if (fx.item->Contained()) return -1;
}

private func FxIntHighlightItemStop(object target, proplist fx, int reason, temp)
{
	if (temp) return;
	if (fx.dummy) fx.dummy->RemoveObject();
	if (!this) return;
	if (fx.item == this.inventory.pickup_item)
		this.inventory.pickup_item = nil;
} 

private func SetNextPickupItem(object to)
{
	// Clear all old markers.
	var e = nil;
	while (e = GetEffect("IntHighlightItem", this))
		RemoveEffect(nil, this, e);
	// And set & mark new one.
	this.inventory.pickup_item = to;
	if (to)
		AddEffect("IntHighlightItem", this, 1, 2, this, nil, to);
}

private func FindNextPickupObject(object start_from, int x_dir)
{
	if (!start_from) start_from = this;
	var sort = Sort_Func("Library_ClonkInventoryControl_Sort_Priority", start_from->GetX()); 
	var objects = FindObjects(Find_Distance(20), Find_Or(Find_Category(C4D_Object), Find_Category(C4D_Living)), Find_NoContainer(), Find_Func("GetProperty", "Collectible"), sort);
	var len = GetLength(objects);
	// Find object next to the current one.
	var index = GetIndexOf(objects, start_from);
	if (index != nil)
	{
		index = index + x_dir;
		if (index < 0) index += len;
		index = index % len;
	}
	else index = 0;
	
	if (index >= len) return nil;
	var next = objects[index];
	if (next == start_from) return nil;
	return next;
}

private func BeginPickingUp()
{
	this.inventory.is_picking_up = true;
	
	var dir = -1;
	if (GetDir() == DIR_Right) dir = 1;
	var obj = FindNextPickupObject(this, dir);
	if (obj)
		SetNextPickupItem(obj);
}

private func EndPickingUp()
{
	this.inventory.is_picking_up = false;

	if (this.inventory.pickup_item)
	{
		// Remember stuff for a possible message - the item might have removed itself later.
		var item = this.inventory.pickup_item;
		var x = item->GetX();
		var y = item->GetY();
		var name = item->GetName();
		// Try to collect the item.
		Collect(item);
		
		// If anything happened, assume collection.
		if (!item || item->Contained())
		{
			var message = CreateObject(FloatingMessage, AbsX(x), AbsY(y), GetOwner());
			message.Visibility = VIS_Owner;
			message->SetMessage(name);
			message->SetYDir(-10);
			message->FadeOut(1, 20);
		}
	}
	
	var e = nil;
	while (e = GetEffect("IntHighlightItem", this))
	{
		RemoveEffect(nil, this, e);
	}
	
	this.inventory.pickup_item = nil;
}

// used in Inventory.ocd
public func SetHandItemPos(int hand, int inv)
{
	// save current slot
	if(hand == 0)
		this.inventory.last_slot = this->GetHandItemPos(0);
		
	return _inherited(hand, inv, ...);
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