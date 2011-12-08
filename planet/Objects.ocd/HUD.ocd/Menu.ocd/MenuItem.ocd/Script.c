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
local item_size; // The item_size of the menu item.
local item_data; // Extra data, unused currently.

protected func Initialize()
{
	// visibility
	this.Visibility = VIS_Owner;
	// parallaxity
	this.Parallaxity = [0, 0];
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
public func MouseDrag(int plr)
{
	if (plr == GetOwner())
		return this;
	// The object may not be dragged.
	return;
}

// Called when an object is dragged onto this one.
public func MouseDrop(int plr, obj)
{
	// Check if the owners match.
	if (plr != GetOwner())
		return false;
		
	// Check if this belongs to a menu.
	if (!item_menu)
		return false;	

	// Forward command to menu.
	item_menu->OnItemDropped(obj, this);

	return true;
}

// Called when this object is dragged onto another one.
public func MouseDragDone(obj, object target)
{
	// Check if this belongs to a menu.
	if (!item_menu)
		return;
		
	// Forward command to menu.
	item_menu->OnItemDragged(this, target);

	return;
}





/* Menu item properties */

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

// Sets the menu item size in px * 1000.
public func SetSize(int size) 
{
	item_size = size / 96;
	Update();
	return;
}

public func GetSize() { return item_size; }
public func ResetSize() { SetSize(64000); }

public func SetCount(int count)
{
	item_count = BoundBy(count, 0, 999); // No more supported currently.
	Update();
	return;
}

public func GetCount() { return item_count; }

public func SetExtraData(extradata)
{
	item_data = extradata;
}

public func GetExtraData() { return item_data; }

// Updates the graphics according to size, object and count.
public func Update()
{
	// Set item size.
	SetObjDrawTransform(item_size, 0, 0, 0, item_size, 0, 0);	
	
	// Set item graphics.
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
			SetGraphics(nil, nil, 1, GFXOV_MODE_ObjectPicture, 0, 0, item_object);
			SetObjDrawTransform(item_size, 0, 0, 0, item_size, 0, 1);
			if (item_object->~HasExtraSlot())
			{
				SetGraphics(nil, GUI_ExtraSlot, 2, GFXOV_MODE_Base);
				SetObjDrawTransform(item_size, 0, 16*item_size, 0, item_size, 16*item_size, 2);
				var content = item_object->Contents(0);
				if (content)
				{
					SetGraphics(nil, nil, 3, GFXOV_MODE_ObjectPicture, 0, 0, content);
					SetObjDrawTransform(item_size/3, 0, 16*item_size, 0, item_size/3, 16*item_size, 3);
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
	
	// Set item count.
	if (item_count == 1) 
		return;
		
	var one = item_count % 10;
	var ten = (item_count / 10) % 10;
	var hun = (item_count / 100) % 10;
	var s = (200 * item_size) / 1000;
	var yoffs = (10000 * item_size) / 1000;
	var xoffs = (13000 * item_size) / 1000;
	var spacing = (5000 * item_size) / 1000;
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

	return;
}
