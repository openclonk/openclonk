/**
	Surrounding
	Enables you to pick up objects from your surrounding.

	@author Zapper 
*/

// Search for new objects only once per frame.
local last_search_frame;
local current_objects;

// Overload contents callback for the interaction menu.
public func Contents(int index)
{
	RefreshIfNecessary();
	if (index < 0 || index >= GetLength(current_objects)) return nil;
	return current_objects[index];
}

public func ContentsCount(id definition)
{
	RefreshIfNecessary();
	
	if (definition == nil)
	{
		return GetLength(current_objects);
	}
	else
	{
		var count = 0;
		for (var index = 0; index < GetLength(current_objects); ++index)
			if (current_objects[index] && current_objects[index]->GetID() == definition)
			{
				count += 1;
			}
		
		return count;
	}
}

private func RefreshIfNecessary()
{
	if (last_search_frame != FrameCounter())
	{
		current_objects = GetPossibleObjects();
		last_search_frame = FrameCounter();
	}
}

private func GetPossibleObjects(id limit_definition)
{
	var sort = nil, check_id = nil;
	if (limit_definition != nil)
	{
		sort = Sort_Distance();
		check_id = Find_ID(limit_definition);
	}
	return FindObjects(Find_Distance(Radius), Find_NoContainer(), Find_Property("Collectible"), Find_Layer(GetActionTarget()->GetObjectLayer()), check_id, sort);
}

// Find first contents object of type. Used by interaction menu.
public func FindContents(id idcont)
{
	var objs = GetPossibleObjects(idcont);
	if (!GetLength(objs)) return nil;
	return objs[0];
}

// Called by the Clonk when an interaction menu is opened.
public func InitFor(object clonk, object menu)
{
	SetAction("Attach", clonk);
	SetOwner(clonk->GetOwner());
	this.Visibility = VIS_Owner;
	
	// The effects will remove this object once the interaction menu is closed.
	AddEffect("KeepAlive", menu, 1, 0, this);
}

// You can transfer items to the environment, which will then be placed on the ground.
public func Collection2(object obj)
{
	var clonk = GetActionTarget();
	obj->Exit(0, clonk->GetDefBottom() - clonk->GetY());
	OnDropped(clonk, obj);
}

// When the item is moved into this object via script, we don't even have to collect it in the first place.
public func Collect(object obj)
{
	var clonk = GetActionTarget();
	var container = obj->Contained();
	if (container)
	{
		// Special treatment for objects that the Clonk holds. Just use the appropriate library function.
		if (container->~IsClonk() && !obj->~IsCarryHeavy())
		{
			container->DropInventoryItem(container->GetItemPos(obj));
		}
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

private func OnDropped(object clonk, object obj)
{
	// Make the Clonk not directly re-collect the object;
	clonk->~OnDropped(obj);
}

public func AttachTargetLost()
{
	RemoveObject();
}

private func FxKeepAliveStop(object target, proplist effect, int reason, int temp)
{
	if (temp) return;
	RemoveObject();
}

public func IsObjectContained(object obj)
{
	// Callback from menu if obj is still "in" this container
	// Since we're hacking our contents, just do the distance check
	return (obj && !obj->Contained() && ObjectDistance(obj) <= this.Radius);
}

private func GetSurroundingMessageObjects(clonk)
{
	return FindObjects(Find_Distance(Radius), Find_NoContainer(), Find_Exclude(clonk), Find_Func("GetSurroundingEntryMessage", clonk), Find_Layer(clonk->GetObjectLayer()), Sort_Distance());
}

private func AddSurroundingObjectMessages(object clonk)
{
	var message_objects = GetSurroundingMessageObjects(clonk);
	var menu_entries = [], i = 0;
	for (var obj in message_objects)
	{
		var entry = {Symbol = Clonk, Bottom = "2em",
			custom =
			{
				Right = "100%", Bottom = "2em",
				Bottom = "1.2em",
				Priority = i,
				BackgroundColor = 0x80804040,
				image = {Right = "2em", Symbol = obj},
				text = {Left = "2em", Text = obj->GetSurroundingEntryMessage(clonk) }
			}};
		menu_entries[i++] = entry;
	}
	return menu_entries;
}

public func GetInteractionMenus(object clonk)
{
	if (!GetLength(GetSurroundingMessageObjects(clonk))) return nil;
	return [{
		entries_callback = this.AddSurroundingObjectMessages,
		BackgroundColor = 0x705050,
		Priority = 100 // high priority items are actually at the bottom
	}];
}

public func IsContainer() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local Plane = 1;
local Radius = 20;

local ActMap =
{
	Attach = 
	{
		Prototype = Action,
		Name="Attach",
		Procedure = DFA_ATTACH,
		NextAction="Hold",
		Length = 1,
		FacetBase = 1,
		AbortCall = "AttachTargetLost"
	}
};
