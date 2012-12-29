/**
	Inventory
	Functions to handle a multi-slot, multi-hand inventory.
*/

/*
	The inventory management:
	The objects in the inventory are saved (parallel to Contents()) in the
	array 'inventory'. They are accessed via GetItem(i) and GetItemPos(obj).
	Other functions are MaxContentsCount() (defines the maximum number of
	contents)
	
	Furthermore the clonk has a defined amount of "hands", defined by HandObjects().
	The array 'use_objects' is a mapping of "hands" onto the inventory-slots.
	The functions GetHandItem(i) returns the object in the "i"th hand.
	
	used properties:
	this.inventory.objects: items in the inventory, array
	this.inventory.hand_objects: items in the hands, array
	this.inventory.disableautosort: used to get default-Collection-behaviour (see Collection2)
	this.inventory.force_collection: used to pick stuff up, even though the hand-slots are all full (see RejectCollect + Collect with CON_Collect)
	this.inventory.forced_ejection: used to recognize if an object was thrown out with or without the force-key (Shift). If not, next hand slot will be selected.
*/


/* Item limit */
public func MaxContentsCount() { return 10; } // Size of the inventory
public func HandObjects() { return 1; } // Amount of hands to select items

func Construction()
{
	if(this.inventory == nil)
		this.inventory = {};
	this.inventory.objects = [];
	this.inventory.disableautosort = false;
	this.inventory.hand_objects = [];
	this.inventory.force_collection = false;
	this.inventory.forced_ejection = nil;

	for(var i=0; i < HandObjects(); i++)
		this.inventory.hand_objects[i] = i;
	return _inherited(...);
}

/** Get the 'i'th item in the inventory */
public func GetItem(int i)
{
	if (i >= GetLength(this.inventory.objects))
		return nil;
	if (i < 0) return nil;
		
	return this.inventory.objects[i];
}

/** Returns all items in the inventory */
public func GetItems()
{
	var inv = this.inventory.objects[:];
	RemoveHoles(inv);
	return inv;
}

/** Returns how many items are in the clonks inventory
    Does not have to be the same as ContentCounts() because of objects with special handling, like CarryHeavy */
public func GetItemCount()
{
	var count = 0;
	for(var i=0; i < GetLength(this.inventory.objects); i++)
		if(this.inventory.objects[i])
			count++;
	
	return count;
}

/** Get the 'i'th item in hands.
    These are the items that will be used with use-commands. (Left mouse click, etc...) */
public func GetHandItem(int i)
{
	// i is valid range
	if (i >= GetLength(this.inventory.hand_objects))
		return nil;
	if (i < 0) return nil;	
	return GetItem(this.inventory.hand_objects[i]);
}

/** Set the 'hand'th use-item to the 'inv'th slot */
public func SetHandItemPos(int hand, int inv)
{
	// indices are in range?	
	if(hand >= HandObjects() || inv >= MaxContentsCount())
		return nil;
	if(hand < 0 || inv < 0) return nil;

	// changing slots cancels using, if the slot with the used object is contained
	if(this.control.current_object) // declared in ClonkControl.ocd
	{
		var used_slot = GetItemPos(this.control.current_object);
		if(used_slot != nil)
			if(used_slot == GetHandItemPos(hand) || used_slot == inv)
				this->~CancelUseControl(0,0);
	}

	// If the item is already selected, we can't hold it in another one too.
	var hand2 = GetHandPosByItemPos(inv);
	if(hand2 != nil)
	{
		// switch places
		this.inventory.hand_objects[hand2] = this.inventory.hand_objects[hand];
		this.inventory.hand_objects[hand] = inv;
		
		// additional callbacks
		var hand_item;
		if(hand_item = GetHandItem(hand2))
		{
			this->~OnSlotFull(hand2);
			// OnSlotFull might have done something to the item
			if(GetHandItem(hand2) == hand_item)
				hand_item->~Selection(this, hand2);
		}
		else
			this->~OnSlotEmpty(hand2);
	}
	else
		this.inventory.hand_objects[hand] = inv;
	
	// call callbacks
	var item;
	if(item = GetItem(inv))
	{
		this->~OnSlotFull(hand);
		// OnSlotFull might have done something to the item
		if(GetItem(inv) == item)
			GetItem(inv)->~Selection(this, hand);
	}
	else
	{
		this->~OnSlotEmpty(hand);
	}
}

/** Returns the position in the inventory of the 'i'th use item */
public func GetHandItemPos(int i)
{
	if (i >= GetLength(this.inventory.hand_objects))
		return nil;
	if (i < 0) return nil;
	
	return this.inventory.hand_objects[i];
}


/** Returns in which hand-slot the inventory-slot is */
private func GetHandPosByItemPos(int o) // sorry for the horribly long name --boni
{
	for(var i=0; i < GetLength(this.inventory.hand_objects); i++)
		if(this.inventory.hand_objects[i] == o)
			return i;

	return nil;
}

/** Drops the item in the inventory slot, if any */
public func DropInventoryItem(int slot)
{
	var obj = GetItem(slot);
	if(!obj)
		return nil;
	
	this->SetCommand("Drop",obj);
}

/** Search for the index of an item */
public func GetItemPos(object item)
{
	if (item)
		if (item->Contained() == this)
		{
			var i = 0;
			for(var obj in this.inventory.objects)
			{
				if (obj == item) return i;
				++i;
			}
		}
	return nil;
}

/** Switch two items in the clonk's inventory */
public func Switch2Items(int one, int two)
{
	// no valid inventory index: cancel
	if (!Inside(one,0,MaxContentsCount()-1)) return;
	if (!Inside(two,0,MaxContentsCount()-1)) return;

	// switch them around
	var temp = this.inventory.objects[one];
	this.inventory.objects[one] = this.inventory.objects[two];
	this.inventory.objects[two] = temp;
	
	// callbacks: cancel use, variable declared in ClonkControl.ocd
	if (this.control.current_object == this.inventory.objects[one] || this.control.current_object == this.inventory.objects[two])
		this->~CancelUse();
	
	var handone, handtwo;
	handone = GetHandPosByItemPos(one);
	handtwo = GetHandPosByItemPos(two);
	
	// callbacks: (de)selection
	if (handone != nil)
		if (this.inventory.objects[two]) this.inventory.objects[two]->~Deselection(this,one);
	if (handtwo != nil)
		if (this.inventory.objects[one]) this.inventory.objects[one]->~Deselection(this,two);
		
	if (handone != nil)
		if (this.inventory.objects[one]) this.inventory.objects[one]->~Selection(this,one);
	if (handtwo != nil)
		if (this.inventory.objects[two]) this.inventory.objects[two]->~Selection(this,two);
	
	// callbacks: to self, for HUD
	if (handone != nil)
	{
		if (this.inventory.objects[one])
			this->~OnSlotFull(handone);
		else
			this->~OnSlotEmpty(handone);
	}
	if (handtwo != nil)
	{
		if (this.inventory.objects[two])
			this->~OnSlotFull(handtwo);
		else
			this->~OnSlotEmpty(handtwo);
	}
	
	this->~OnInventoryChange(one, two);
}


/* Overload of Collect function
   Allows inventory/hands-Handling with forced-collection
*/
public func Collect(object item, bool ignoreOCF, int pos, bool force)
{
	this.inventory.force_collection = force;
	var success = false;
	if (pos == nil || item->~IsCarryHeavy())
	{
		success = _inherited(item,ignoreOCF);
		this.inventory.force_collection = false;
		return success;
	}
	// fail if the specified slot is full
	if (GetItem(pos) == nil && pos >= 0 && pos < MaxContentsCount())
	{
		if (item)
		{
			this.inventory.disableautosort = true;
			// collect but do not sort in_
			// Collection2 will be called which attempts to automatically sort in
			// the collected item into the next free inventory slot. Since 'pos'
			// is given as a parameter, we don't want that to happen and sort it
			// in manually afterwards
			var success = _inherited(item);
			this.inventory.disableautosort = false;
			if (success)
			{
				this.inventory.objects[pos] = item;
				var handpos = GetHandPosByItemPos(pos); 
				// if the slot was a selected hand slot -> update it
				if(handpos != nil)
				{
					this->~OnSlotFull(handpos);
				}
			}
		}
	}
	
	this.inventory.force_collection = false;
	return success;
}

protected func Collection2(object obj)
{
	var sel = 0;

	// See Collect()
	if (this.inventory.disableautosort) return _inherited(obj,...);
	
	var success = false;
	var i;
	
	// sort into selected hands if empty
	for(i = 0; i < HandObjects(); i++)
		if(!GetHandItem(i))
		{
			sel = GetHandItemPos(i);
			this.inventory.objects[sel] = obj;
			success = true;
			break;
		}
		
	// otherwise, first empty slot
	if(!success)
	{
		for(var i = 0; i < MaxContentsCount(); ++i)
		{
			if (!GetItem(i))
			{
				sel = i;
				this.inventory.objects[sel] = obj;
				success = true;
				break;
			}
		}
	}
	
	// callbacks
	if (success)
	{
		var handpos = GetHandPosByItemPos(sel); 
		// if the slot was a selected hand slot -> update it
		if(handpos != nil)
		{
			this->~OnSlotFull(handpos);
			// OnSlotFull might have done something to obj
			if(GetHandItem(handpos) == obj)
				obj->~Selection(this, handpos);
		}
	}
		

	return _inherited(obj,...);
}

func Ejection(object obj)
{
	// if an object leaves this object
	// find obj in array and delete (cancel using too)
	var i = 0;
	var success = false;
	
	for(var item in this.inventory.objects)
    {
	   if (obj == item)
	   {
			this.inventory.objects[i] = nil;
			success = true;
			break;
	   }
	   ++i;
    }
    
    // variable declared in ClonkControl.ocd
	if (this.control.current_object == obj) this->~CancelUse();

	// callbacks
	if (success)
	{
		var handpos = GetHandPosByItemPos(i); 
		// if the slot was a selected hand slot -> update it
		if(handpos != nil)
		{
			// if it was a forced ejection, the hand will remain empty
			if(this.inventory.forced_ejection == obj)
			{
				this->~OnSlotEmpty(handpos);
				obj->~Deselection(this, handpos);
			}
			// else we'll select the next full slot
			else
			{
				// look for following non-selected non-free slots
				var found_slot = false;
				for(var j=i; j < MaxContentsCount(); j++)
					if(GetItem(j) && !GetHandPosByItemPos(j))
					{
						found_slot = true;
						break;
					}
				
				if(found_slot)
					SetHandItemPos(handpos, j); // SetHandItemPos handles the missing callbacks
				// no full next slot could be found. we'll stay at the same, and empty.
				else
				{
					this->~OnSlotEmpty(handpos);
					obj->~Deselection(this, handpos);
				}
			}
		}
	}
	
	// we have over-weight? Put the next unindexed object inside that slot
	// this happens if the clonk is stuffed full with items he can not
	// carry via Enter, CreateContents etc.
	var inventory_count = 0;
	for(var io in this.inventory.objects)
		if(io != nil)
			inventory_count++;
			
	if (ContentsCount() > inventory_count && !GetItem(i))
	{
		for(var c = 0; c < ContentsCount(); ++c)
		{
			var o = Contents(c);
			if(o->~IsCarryHeavy())
				continue;
			if (GetItemPos(o) == nil)
			{
				// found it! Collect it properly
				this.inventory.objects[i] = o;
				
				var handpos = GetHandPosByItemPos(i); 
				// if the slot was a selected hand slot -> update it
				if(handpos != nil)
				{
					this->~OnSlotFull(handpos);
					// OnSlotFull might have done something to o
					if(GetHandItem(handpos) == o)
						o->~Selection(this, handpos);
				}
					
				break;
			}
		}
	}
	
	_inherited(obj,...);
}

func ContentsDestruction(object obj)
{
	// tell the Hud that something changed
	this->~OnInventoryChange();
	_inherited(obj, ...);
}

protected func RejectCollect(id objid, object obj)
{
	// collection of that object magically disabled?
	if(GetEffect("NoCollection", obj)) return true;
	
	// try to stuff obj into an object with an extra slot
	for(var i=0; Contents(i); ++i)
		if (Contents(i)->~HasExtraSlot())
			if (!(Contents(i)->Contents(0)))
				if (Contents(i)->Collect(obj,true))
					return true;
					
	// try to stuff an object in clonk into obj if it has an extra slot
	if (obj->~HasExtraSlot())
		if (!(obj->Contents(0)))
			for(var i=0; Contents(i); ++i)
				if (obj->Collect(Contents(i),true))
					return false;

	// check max contents
	if (ContentsCount() >= MaxContentsCount()) return true;
	
	return _inherited(objid,obj,...);
}
