/*
		Shows an icon for the ringmenu.
		Author: Mimmo, Clonkonaut
*/

local selected;
local position;
local controller;

public func SetHUDController(object c) { controller = c; }

protected func Initialize()
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
	if(nr == 0)
		SetGraphics("LeftHand", GUI_Backpack_Slot_Icon, 4, GFXOV_MODE_Base);
	if(nr == 1)
		SetGraphics("RightHand", GUI_Backpack_Slot_Icon, 5, GFXOV_MODE_Base);
	//SetGraphics(nil, nil, MI_SELECTION_LAYER);
	//SetClrModulation(HSL(255-nr*86, 200, 150));
}

public func SetUnselected()
{
	SetGraphics(nil,nil,4);
	SetGraphics(nil,nil,5);
}


// SetSymbol from GUI_RingMenu_Icon
public func SetSymbol(obj)
{
	this.Visibility = VIS_Owner;
		
	if(!obj)
	{
		SetGraphics(nil, nil, MI_ICON_LAYER);
		SetName("");
		this.MouseDragImage = nil;
	}
	else
	{
		if (GetType(obj) == C4V_C4Object)
			SetGraphics(nil, nil, MI_ICON_LAYER, GFXOV_MODE_ObjectPicture, 0, 0, obj);
		else
			SetGraphics(nil,obj,MI_ICON_LAYER,GFXOV_MODE_IngamePicture);
		
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