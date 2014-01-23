/**
	Menu item for the circular menu.
	Overlays:
	0=Grey Circle
	1=Icon of the Object
	2,3=Extraslot
	10,11,12= Amount
	@author Mimmo, Clonkonaut, Newton, Maikel
*/

local item_menu; // The menu object controlling this menu item.
local item_object; // The object/symbol of this menu item.
local item_count; // The count of this menu item.
local item_data; // Extra data, unused currently.

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

/* Callbacks from the engine on mouse selection and drag & drop */

// Called when this object is selected with the left mouse button.
public func MouseSelection(int plr)
{
	// Check if this belongs to a menu.
	if (!item_menu)
		return;
	
	// Transfer result to menu.
	return item_menu->OnItemSelection(this, plr);	
}

// Called when this object is selected with the right mouse button.
public func MouseSelectionAlt(int plr)
{
	// Check if this belongs to a menu.
	if (!item_menu)
		return;
	
	// Transfer result to menu.
	return item_menu->OnItemSelectionAlt(this, plr);	
}

// Called to determine which object is dragged.
public func OnMouseDrag(int plr)
{
	// Check if the owners match.
	if (plr != GetOwner()) return;
		
	// Check if this belongs to a menu.
	if (!item_menu) return;	
	
	// Check if containing menu allows drag&drop
	if (!item_menu->IsDragDropMenu()) return;

	return this;
}

// Called when an object is dragged onto this one.
public func OnMouseDrop(int plr, other)
{
	// Check if the owners match.
	if (plr != GetOwner()) return false;
		
	// Check if this belongs to a menu.
	if (!item_menu) return false;	
	
	// Check if containing menu allows drag&drop
	if (!item_menu->IsDragDropMenu()) return false;
	
	// Forward command to menu.
	return item_menu->OnItemDropped(other, this);
}

// Called after this object has been dragged onto another one.
public func OnMouseDragDone(self, object target)
{
	// Check if this belongs to a menu.
	if (!item_menu) return;
		
	// Forward command to menu.
	return item_menu->OnItemDragDone(self, target);
}

// Called if the mouse cursor starts hovering over this item.
public func OnMouseOver(int plr, object dragged)
{
	if (plr != GetOwner()) return;
	
	// Forward command to menu.
	return item_menu->OnMouseOverItem(this, dragged);
}

// Called if the mouse cursor stops hovering over this item.
public func OnMouseOut(int plr, object dragged)
{
	if (plr != GetOwner()) return;
	
	// Forward command to menu.
	return item_menu->OnMouseOutItem(this, dragged);
}

/* Menu item properties */

public func SetMenu(object menu)
{
	item_menu = menu;
	return;
}

public func GetMenu()
{
	return item_menu;
}

public func SetSymbol(obj)
{
	item_object = obj;
	Update();
	return;
}

public func GetSymbol()
{
	return item_object;
}

// Sets the menu item size.
public func SetSize(int size) 
{
	// The menu item size is handled over con since drag&drop needs to scale the same.
	SetCon(size);
	Update();
	return;
}

public func GetSize() { return GetCon(); }
public func ResetSize() { SetSize(100); }

public func SetCount(int count)
{
	item_count = count;
	if (item_count != nil)
		item_count = BoundBy(item_count, 0, 999); // No more supported currently.
	Update();
	return;
}

public func GetCount() { return item_count; }

public func SetData(data)
{
	item_data = data;
}

public func GetExtraData() { return item_data; }

// Updates the graphics according to size, object and count.
public func Update()
{
	// Set mouse drag image.
	this.MouseDragImage = item_object;
	
	// Update item symbol.
	UpdateSymbol();	
	
	// Update item amount.
	UpdateCount();
	
	// Update tooltip
	UpdateTooltip();
	
	return;
}	

private func UpdateSymbol()
{
	if (!item_object)
	{
		SetGraphics(nil, nil, 1);
		SetGraphics(nil, nil, 2);
		SetGraphics(nil, nil, 3);
	}
	else
	{
		if (GetType(item_object) == C4V_C4Object)
		{
			SetGraphics(nil, nil, 1, GFXOV_MODE_ObjectPicture, nil, 0, item_object);
			if (item_object->~HasExtraSlot())
			{
				SetGraphics(nil, GUI_ExtraSlot, 2, GFXOV_MODE_Base);
				SetObjDrawTransform(1000, 0, 16000, 0, 1000, 16000, 2);
				var content = item_object->Contents(0);
				if (content)
				{
					SetGraphics(nil, nil, 3, GFXOV_MODE_ObjectPicture, nil, 0, content);
					SetObjDrawTransform(1000/3, 0, 16000, 0, 1000/3, 16000, 3);
				}
				else
					SetGraphics(nil, nil, 3);
			}
			else
			{
				SetGraphics(nil, nil, 2);
				SetGraphics(nil, nil, 3);
			}
		}
		else
			SetGraphics(nil,item_object,1,GFXOV_MODE_IngamePicture);
		
		SetName(item_object->GetName());
	}
	return;
}

private func UpdateCount()
{	
	if (item_count == nil)
	{
		SetGraphics(nil, nil, 9);
		SetGraphics(nil, nil, 10);
		SetGraphics(nil, nil, 11);
		SetGraphics(nil, nil, 12);	
	}
	else
	{	
		// Set item count.
		var one = item_count % 10;
		var ten = (item_count / 10) % 10;
		var hun = (item_count / 100) % 10;
		var s = 300;
		var yoffs = 15000;
		var xoffs = 17000;
		var spacing = 7500;
		SetGraphics(Format("10"), Icon_SlimNumber, 9, GFXOV_MODE_IngamePicture); //10 == "x"
	
		SetGraphics(Format("%d", one), Icon_SlimNumber, 12, GFXOV_MODE_IngamePicture);
		SetObjDrawTransform(s, 0, xoffs-spacing-500, 0, s, yoffs+300, 9);
		SetObjDrawTransform(s, 0, xoffs, 0, s, yoffs, 12);
	
		if (ten > 0 || hun > 0)
		{
			SetGraphics(Format("%d", ten), Icon_SlimNumber, 11, GFXOV_MODE_IngamePicture);
			SetObjDrawTransform(s, 0, xoffs-spacing*2-500, 0, s, yoffs+300, 9);
			SetObjDrawTransform(s, 0, xoffs-spacing, 0, s, yoffs, 11);
		}
		else
			SetGraphics(nil, nil, 11);
			
		if (hun > 0)
		{
			SetGraphics(Format("%d", hun), Icon_SlimNumber, 10, GFXOV_MODE_IngamePicture);
			SetObjDrawTransform(s, 0, xoffs-spacing*3-500, 0, s, yoffs+300, 9);
			SetObjDrawTransform(s, 0, xoffs-spacing*2, 0, s, yoffs, 10);
		}
		else
			SetGraphics(nil, nil, 10);
	}
	return;
}

private func UpdateTooltip()
{
	if(!item_object)
		this.Tooltip = nil;
	else
	{
		this.Tooltip = item_object.Description;
	}
}
