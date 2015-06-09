/**
	Surrounding
	Enables you to pick up objects from your surrounding.

	@author Zapper 
*/

// proplist {id = <id>, amount = <amount>}
local current_object_info;

public func GetObjectsMenuEntries()
{
	var menu_entries = [];
	for (var info in current_object_info)
	{
		var text = "";
		if (info.amount > 1) text = Format("%dx", info.amount);
		PushBack(menu_entries, {symbol = info.id, text = text});
	}
	return menu_entries;
}

public func GetInteractionMenus(object clonk)
{
	var menus = _inherited() ?? [];		
	var menu =
	{
		title = "$ObjectsAroundYou$",
		entries_callback = this.GetObjectsMenuEntries,
		callback = "OnPickUpObject",
		callback_target = this,
		BackgroundColor = RGB(0, 50, 0),
		Priority = 20
	};
	PushBack(menus, menu);
	return menus;
}

public func OnPickUpObject(id symbol)
{
	var found = nil;
	for (var obj in FindObjects(Find_Distance(Radius), Find_ID(symbol), Find_NoContainer(), Sort_Distance()))
	{
		if (obj->GBackSolid() && obj->Stuck()) continue;
		found = obj;
		break;
	}
	// Maybe something else was quicker?
	if (!found) return;
	
	var clonk = GetActionTarget();
	clonk->Collect(found);
	// It is possible that the Clonk removed the object.
	if (!found || found->Contained())
		Refresh();
}

func InitFor(object clonk, object menu)
{
	SetAction("Attach", clonk);
	SetOwner(clonk->GetOwner());
	
	// The effects will remove this object once the interaction menu is closed.
	AddEffect("KeepAlive", menu, 1, 0, this);
	
	// Refresh the objects regularly.
	AddEffect("CheckRefresh", this, 1, 5, this);
	
	// And refresh one time.
	Refresh();
}

func Refresh()
{
	// Look for all objects in the vicinity that can be accessed by the Clonk (aka non-stuck).
	var objects = FindObjects(Find_Distance(Radius), Find_NoContainer(), Find_Category(C4D_Object));
	var new_object_info = [];
	for (var obj in objects)
	{
		if (!obj.Collectible) continue;
		// Use GBackSolid and Stuck, because items dropped by the Clonk are regularly in earth with their bottom vertex (shovel e.g.).
		// So use only don't collect when both the center is not free and the object is stuck.
		if (obj->GBackSolid() && obj->Stuck()) continue;
		// Already in the list? Just increase the amount.
		var found = false;
		for (var old_data in new_object_info)
		{
			if (old_data.id != obj->GetID()) continue;
			++old_data.amount;
			found = true;
			break;
		}
		if (found) continue;
		// Otherwise, just add to list.
		PushBack(new_object_info, {id = obj->GetID(), amount = 1});
	}
	if (new_object_info == current_object_info) return;
	current_object_info = new_object_info;
	UpdateInteractionMenus(this.GetObjectsMenuEntries);
}

func FxCheckRefreshTimer(object target, proplist effect)
{
	Refresh();
}

func OnDropped(object clonk, object obj)
{
	// Make the Clonk not directly re-collect the object;
	clonk->~OnDropped(obj);
}

// You can transfer items to the environment, which will then be placed on the ground.
func Collection2(object obj)
{
	var clonk = GetActionTarget();
	obj->Exit(0, clonk->GetDefBottom() - clonk->GetY());
	OnDropped(clonk, obj);
}

// When the item is moved into this object via script, we don't even have to collect it in the first place.
func Collect(object obj)
{
	var clonk = GetActionTarget();
	var container = obj->Contained();
	if (container)
	{
		// Special treatment for objects that the Clonk holds. Just use the appropriate library function.
		if (container->~IsClonk())
			container->DropInventoryItem(container->GetItemPos(obj));
		else
		{
			// Otherwise, just force-drop it at the clonks's bottom.
			obj->Exit();
			obj->SetPosition(clonk->GetX(), clonk->GetDefBottom() - obj->GetDefHeight() - obj->GetDefOffset(1));
			OnDropped(clonk, obj);
		}
	}
	else
	{
		obj->SetPosition(clonk->GetX(), clonk->GetDefBottom() - obj->GetDefHeight() - obj->GetDefOffset(1));
		OnDropped(clonk, obj);
	}
	return true;
}



func AttachTargetLost()
{
	RemoveObject();
}

func FxKeepAliveStop(object target, proplist effect, int reason, int temp)
{
	if (temp) return;
	RemoveObject();
}

public func HasInteractionMenu() { return true; }

public func AllowsGrabAll() { return true; }

public func GetGrabAllObjects()
{
	var objects = FindObjects(Find_Distance(Radius), Find_NoContainer(), Find_Category(C4D_Object));
	for (var index = GetLength(objects) - 1; index >= 0; index--)
	{
		var obj = objects[index];
		if (!obj.Collectible || (obj->GBackSolid() && obj->Stuck()))
			RemoveArrayIndex(objects, index);
	}
	return objects;
}


local Name = "$Name$";
local Description = "$Description$";
local Plane = 1;
local Radius = 40;

local ActMap =
{
	Attach = 
	{
		Prototype = Action,
		Name="Attach",
		Procedure=DFA_ATTACH,
		NextAction="Hold",
		Length=1,
		FacetBase=1,
		AbortCall = "AttachTargetLost"
	}
};
