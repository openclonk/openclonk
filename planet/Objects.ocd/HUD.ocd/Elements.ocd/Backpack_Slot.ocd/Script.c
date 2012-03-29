/*
		Shows an icon for the ringmenu.
		Author: Mimmo, Clonkonaut
*/

local selected;
local position;
local controller;

public func SetHUDController(object c) { controller = c; }

protected func Construction()
{
	// Visibility
	this.Visibility = VIS_Owner;
	// Parallaxity
	this.Parallaxity = [0, 0];
	// Mouse drag image
	this.MouseDragImage = nil;
	this.MouseDrag = MD_DragSource | MD_DropTarget;

	return;
}

public func SetSlotId(int i) { position = i; }
public func GetSlotId() { return position; }

public func SetSelected(int nr)
{
	SetGraphics("Hand", GUI_Backpack_Slot_Icon, 4, GFXOV_MODE_Base);
	
	if(nr == 0)
		SetObjDrawTransform(700, 0, -17000, 0, 700, 17000, 4);
	if(nr == 1)
		SetObjDrawTransform(-700, 0, 17000, 0, 700, 17000, 4);
}

public func SetUnselected()
{
	SetGraphics(nil,nil,4);
}


// SetSymbol from GUI_RingMenu_Icon
public func SetSymbol(obj)
{
	this.Visibility = VIS_Owner;
		
	if(!obj)
	{
		SetGraphics(nil, nil, 1);
		SetGraphics(nil, nil, 2);
		//SetGraphics(nil, nil, 3);
		SetName("");
		this.MouseDragImage = nil;
	}
	else
	{
		if (GetType(obj) == C4V_C4Object)
			SetGraphics(nil, nil, 2, GFXOV_MODE_ObjectPicture, 0, 0, obj);
		else
			SetGraphics(nil,obj, 2, GFXOV_MODE_IngamePicture);
		
		// if object has extra slot, show it
		if(obj->~HasExtraSlot() && obj->Contents())
		{
			SetGraphics(nil, nil, 1, GFXOV_MODE_ObjectPicture, nil, nil, obj->Contents());
			SetClrModulation(RGBa(255,255,255,200),1);
			SetObjDrawTransform(900,0,0,0,900,0,1);
			//SetObjDrawTransform(1000,0,32000,0,1000,-28000,2);
		}
		// or otherwise, remove it
		else
		{
			SetGraphics(nil,nil, 1);
		}
		
		SetName(obj->GetName());
		this.MouseDragImage = obj->GetID();
	}
}

// Warning: Mouse-Drag'n'Drop still is really weird here

// Called when this object is selected with the left mouse button.
public func MouseSelection(int plr)
{
	if(plr != GetOwner())
		return nil;
		
	var c = GetCursor(GetOwner());
	if(!c) return 1;
	
	// set 1st mouse button to this slot/item
	var old = c->GetHandItemPos(0);
	c->SetHandItemPos(0, position);
	controller->OnHandSelectionChange(old, position, 0);
	
	return true;
}

// Called when this object is selected with the right mouse button.
public func MouseSelectionAlt(int plr)
{
	if(plr != GetOwner())
		return nil;
		
	var c = GetCursor(GetOwner());
	if(!c) return 1;
	
	// set 2nd mouse button to this slot/item
	var old = c->GetHandItemPos(1);
	c->SetHandItemPos(1, position);
	controller->OnHandSelectionChange(old, position, 1);

	return true;	
}

public func MouseDrag(int plr)
{
	if(plr != GetOwner())
		return;
		
	return this;
}

public func MouseDrop(int plr, object src)
{
	if(src->GetOwner() != GetOwner())
		return false;
	
	if(src->GetID() == this->GetID())
		return true;
	
	return false;
}

// Called after this object has been dragged onto another one.
public func MouseDragDone(self, object target)
{
	var c = GetCursor(GetOwner());
	if(!c) return;
	
	if(target == nil)
	{
		c->DropInventoryItem(position);
		return true;
	}
	
	if(target->GetOwner() != GetOwner())
		return false;
		
	if(target->GetID() == this->GetID())
	{
		c->Switch2Items(position, target->GetSlotId());
		return true;
	}
}

// highlight and block hiding
public func OnMouseOver(int plr)
{
	if(!controller || GetOwner() == NO_OWNER)
		return nil;
	
	SetGraphics("Focussed", GUI_Backpack_Slot_Icon);
	
	controller->ShowInventory();
}

public func OnMouseOut(int plr)
{
	if(!controller || GetOwner() == NO_OWNER)
		return nil;
	
	SetGraphics(nil, nil);
	
	controller->ScheduleHideInventory();
}