/**
	Control object for content menus.
	
	@author Newton, Maikel
*/

// this array contains all the menus
// index 0 = calling object
// index >0 = first crew, then containers
local circ_menus;
local menu_object;

local crew_count;
local container_count;

/** Creates a content menu for the calling object. This is supposed to be a crew member, 
	controlled by a player.
	@return a pointer to the created menu, or \c nil if failed.
*/
global func CreateContentsMenus()
{
	// Safety checks.
	if (!this) return;
	if (!(GetOCF() & OCF_CrewMember)) return;
	if (!(this->~HasMenuControl())) return;

	// Create the menu controller.
	var controller = CreateObject(GUI_Contents_Controller);
	controller->SetMenuObject(this);
	this->SetMenu(controller);
	
	var index = 0;
	
	// add the clonk itself
	controller->AddContentMenu(this, index, true);
	
	// add all nearby crewmembers
	var teammates = FindObjects(Find_Distance(20), Find_OCF(OCF_CrewMember), Find_Owner(GetOwner()), Find_Exclude(this));
	index = 1;
	for(var t in teammates)
		controller->AddContentMenu(t, index++, true);
		
	// Add content menus for all containers at the position of the caller.
	var containers = FindObjects(Find_AtPoint(), Find_NoContainer(), Find_OCF(OCF_Fullcon), Find_Func("IsContainer"), Sort_Func("SortInventoryObjs"));
	for(var c in containers)
		controller->AddContentMenu(c, index++, false);
	
	// Show and return content menu.
	controller->Show();
	return controller;
}

global func SortInventoryObjs()
{
	// left: crew members
	if (GetOCF() & OCF_CrewMember) return 0;
	// center: vehicles
	if (GetCategory() & C4D_Vehicle) return 10;
	// right: buildings
	if (GetCategory() & C4D_Structure) return 20;
}

public func IsContentMenu() { return true; }

func SetMenuObject(object menu_object)
{
	menu_object = menu_object;
}

func Construction()
{
	circ_menus = [];
	container_count = 0;
	crew_count = 0;
}

func Close() 
{
	if(menu_object)
		menu_object->~MenuClosed(this);
	RemoveObject();
}

func Destruction() 
{
	// remove all menu objects
	for(var prop in circ_menus)
	{
		if(prop.Object)
			prop.Object->~OnContentMenuClosed();
		prop.Menu->RemoveObject();
	}
}

func Show(bool no_update) 
{
	for (var prop in circ_menus)
	{
		if (!no_update)
			prop.Object->~OnContentMenuOpened();
		prop.Menu->Show();	
	}
}

func Hide() 
{
	for (var prop in circ_menus)
	{
		prop.Object->~OnContentMenuClosed();
		prop.Menu->Hide();
	}
}

func AddContentMenu(object container, int pos, bool isCrew)
{
	var menu = CreateObject(GUI_CircleMenu, 0, 0, GetOwner());

	menu->SetSymbol(container);
	menu->SetMenuObject(menu_object);
	menu->SetCommander(this);
	menu->SetDragDropMenu(true);

	var dist = ObjectDistance(menu_object, container);

	PutContentsIntoMenu(menu, container);
	circ_menus[pos] = {Object = container, Menu = menu, IsCrew = isCrew, Distance = dist};
	
	if(isCrew)
		crew_count++;
	else
		container_count++;
	
	// Track external changes in containers.
	AddEffect("ContainerTracker", container, 100, 1, this, nil, menu, container->GetPosition());
	
	UpdateContentMenus();
	return;
}

func RemoveContentMenu(int index)
{
	var length = GetLength(circ_menus);
	if(index >= length)
		return;
	
	// remove menu
	if(circ_menus[index].IsCrew)
		crew_count--;
	else
		container_count--;
	
	circ_menus[index].Menu->RemoveObject();
	
	// close the gap
	for(var i=index; i < length-1; i++)
		circ_menus[i] = circ_menus[i+1];
	
	SetLength(circ_menus, length-1);
	
	// and update
	UpdateContentMenus();
	for(var prop in circ_menus)
		prop.Menu->UpdateMenu();
}

// Draws the contents menus to the right positions.
private func UpdateContentMenus()
{
	// for positioning to the left
	var index_crew = 0;
	// for positioning to the right
	var index_containers = 1; // containers start with an offset of 1

	// calculate left edge
	var r = 160;
	var d = 2*r/3;
	var dx = Sqrt(r**2 - d**2);
	var x_base = 800 - dx*(container_count-1);
	var y_base = 320;
	
	for(var prop in circ_menus)
	{
		var menu = prop.Menu;
		
		var x,y;
		
		if(prop.IsCrew)
		{
			x = x_base - dx * (2*index_crew);
			y = y_base + 2*d - (-1)**index_crew * d;
			
			index_crew++;
		}
		else
		{
			// Determine x-position.
			//x = 800 + dx * (2*index_containers - container_count + 1);
			x = x_base + dx*(2*index_containers);
			// Alternate y-position.
			y = y_base + (-1)**(index_containers) * d;
			
			index_containers++;
		}
		// TODO: reduce size of menus to fit more into screen.
		menu->SetPosition(x, y);
	}
	return;
}

private func PutContentsIntoMenu(object menu, object container)
{
	if (container->~ NoStackedContentMenu())
	{
		var contents;
		// get contents in special order if possible
		if(!(contents = (container->~GetItems())))
			// else just take everything we find.
			contents = FindObjects(Find_Container(container));

		for (var content in contents)
			if (!AddContentsMenuItem(content, menu))
				return;
	} 
	else
	{
		var stacked_contents = container->GetStackedContents();
		for (var stack in stacked_contents)
			if (!AddContentsMenuItem(stack[0], menu, stack)) 
				return;
	}
	
	// TODO: find an extra-entry or something like that for this.
	if(container->~IsCarryingHeavy())
		AddContentsMenuItem(container->GetCarryHeavy(), menu);
}

private func AddContentsMenuItem(object obj, object menu, array stack)
{
	// Into the menu item, all the objects of the stack are saved as an array into it's extradata.
	var item = CreateObject(GUI_MenuItem);
	if (!menu->AddItem(item))
	{
		item->RemoveObject();
		return false;
	}
	item->SetSymbol(obj);
	if(obj.Description)
	{
		item->SetTooltip(obj.Description);
	}
	if (stack == nil)
	{
		item->SetData([obj]);
	}
	else
	{
		item->SetCount(GetLength(stack));
		item->SetData(stack);
	}
	return true;
}

global func GetStackedContents()
{
	if(!this) return nil;
	
	var contents = FindObjects(Find_Container(this));
	var stacked = [];
	
	// put all contents into stackcontents
	for(var content in contents)
	{
		// check if item of same ID already inside
		var has_stacked = false;
		for(var stackcontent in stacked)
		{
			if (!CanStackObjects(content,stackcontent[0])) continue;
			stackcontent[GetLength(stackcontent)] = content;
			has_stacked = true;
			break;
		}
		// otherwise, put new inside
		if (!has_stacked)
			stacked[GetLength(stacked)] = [content];
	}
	
	return stacked;
}

global func CanStackObjects(object first, object second)
{
	if(first->GetID() != second->GetID()) return false;
	if(first->~RejectStack(second)) return false;
	return true;
}

private func CanStackObjIntoMenuItem(object menu, object obj) {
	for(var item in menu->GetItems()) {
		if(!item) continue;
		
		var stack = item->GetExtraData();
		if(CanStackObjects(obj,stack[0])) {
			return item;
		}
	}
}

/*-- Content tracking --*/
// TODO: Implement this more carefully and cover all corner cases.

public func FxContainerTrackerStart(object target, proplist effect, int temporary, object menu, array position)
{
	if (temporary == 0)
	{
		effect.Menu = menu;
		effect.Position = position;
		// Initialize content list.
		effect.ContentList = [];
		var index = 0;
		while (target->Contents(index))
		{
			effect.ContentList[index] = target->Contents(index);
			index++;
		}
	}
	return 1;
}

public func FxContainerTrackerTimer(object target, proplist effect)
{
	// check if the target moved
	if(effect.Position[0] != target->GetX() || effect.Position[1] != target->GetY())
	{
		effect.CommandTarget->~OnContainerMovement(effect.Menu, target);
		effect.Position = target->GetPosition();

		return 1;
	}
	
	// Match current contents to actual list, first trivial test.
	if (GetLength(effect.ContentList) != target->ContentsCount())
		// Stop the effect, the contoller is notified in the stop call.
		return -1;
	
	// Test both ways around, cause either container can be empty.
	var index = 0;
	while (target->Contents(index))
	{
		if (effect.ContentList[index] != target->Contents(index))
		{
			// Stop the effect, the contoller is notified in the stop call.
			return -1;
		}
		index++;
	}
	for (index = 0; index < GetLength(effect.ContentList); index++)
	{
		if (effect.ContentList[index] != target->Contents(index))
		{
			// Stop the effect, the contoller is notified in the stop call.
			return -1;
		}
	}
	return 1;
}

public func FxContainerTrackerStop(object target, proplist effect, int reason, bool tmp)
{
	if(tmp)
		return;
	// Notify content menu if the effect has ended regularly, the menu will be updated
	// the effect will not be removed if the menu still exists
	if (!reason)
	{
		effect.CommandTarget->~OnContentChange(effect.Menu, target);
		if(effect.Menu)
		{
			// update list
			var index = 0;
			while (target->Contents(index))
			{
				effect.ContentList[index] = target->Contents(index);
				index++;
			}
			return -1;
		}
	}
	if(reason == 3 || reason == 4)
		effect.CommandTarget->~OnContainerRemoved(effect.Menu, target);

	return 1;
}

/** Called when the position of a container with an open menu changed.
    Checks if object still is in range, and removes menu if necessary.
*/
public func OnContainerMovement(object menu, object container)
{
	var index = FindMenuPos(menu);
	if(index < 0)
		return;
	
	// check distance
	if(ObjectDistance(menu_object, container) > circ_menus[index].Distance)
		RemoveContentMenu(index);
}

/** Called when a container with an open menu got removed.
    Removes the Menu and fixes ordering.
*/
public func OnContainerRemoved(object menu, object container)
{
	var index = FindMenuPos(menu);
	if(index < 0)
		return;
	
	// remove menu and reorder other menus
	RemoveContentMenu(index);
}

/** Called when the content of a container with an open menu changed.
    Updates menu.
*/
public func OnContentChange(object menu, object container)
{
	// Find changed menu and remove it.
	var index = FindMenuPos(menu);
	if(index < 0)
		return;
		
	menu->Clear();
	PutContentsIntoMenu(menu, container);

	return;
}

/** Transfers the objects contained in menu_item from the source to the target container.
	@param p_source proplist containing the source menu.
	@param p_target proplist containing the target menu.
	@param menu_item the menu item selected by the player.
	@param amount the amount of objects to be transfered.
	@return the actual count of objects that have successfully been transfered.
*/
private func TransferObjects(proplist p_source, proplist p_target, object menu_item, int amount)
{
	// Safety: target and source must be available.
	if (!p_source || !p_target)
		return 0;	
	
	// Determine actual amount that may be transfered.
	var objects = menu_item->GetExtraData(); // will always be at least [object]
	amount = BoundBy(amount, 0, GetLength(objects));
		
	// Move to object from source container to target container.
	// Memorize which objects have been put where in order to know which menu items to update
	// how (Because the collection can be rejected by RejectCollect/AllowTransfer).
	var moved_to_target = [];
	var moved_length = 0;
	for (var i = 0; i < amount; i++) 
	{
		var obj = objects[i];
		// Try to enter the object, but check for some collect callbacks.
		if (p_target.Object->~RejectCollect(obj->GetID(), obj) && !p_target.Object->~AllowTransfer(obj))
			continue;
		if (obj->Enter(p_target.Object))
		{				
			moved_to_target[GetLength(moved_to_target)] = obj;
			moved_length++;
		}
	}
	
	// Update target menu, source menu is updated elsewhere.
	if (moved_length > 0)
	{	
		// in case objects may not be stacked in the contents menu of the target object, add
		// each object as a new menu item
		if (p_target.Object->~NoStackedContentMenu())
		{
			for (var mov_obj in moved_to_target)
				AddContentsMenuItem(mov_obj, p_target.Menu);
		}
		// otherwise, add stacked
		else
		{
			// If menu item with same objects already exists: update menu item.
			var target_item = CanStackObjIntoMenuItem(p_target.Menu, moved_to_target[0]);
			if (target_item)
			{
				var new_extra_data = Concatenate(target_item->GetExtraData(), moved_to_target);
				target_item->SetData(new_extra_data);
				target_item->SetCount(GetLength(new_extra_data));
			}
			// Otherwise add a new menu item to containing menu.
			else
			{
				AddContentsMenuItem(moved_to_target[0], p_target.Menu, moved_to_target);
			}
		}
	}
	// Return the number of items that have been transfered.
	return moved_length;
}

/** Exchanges to menu items and their contents between the source and a target menu.

*/
private func ExchangeObjects() 
{
	// TODO: Implement.

}

private func UpdateAfterTakenObjects(proplist p_source, object menuItem)
{
	var objects = menuItem->GetExtraData();
	// update menu item in source menu: remove all objects in extradata which are not in
	// container anymore
	var c = 0;
	var i;
	for (i = 0; i < GetLength(objects); ++i)
	{
		var obj = objects[i];
		if (obj->Contained() != p_source.Object) 
		{
			objects[i] = nil;
			c++;
		}
	}
	
	// removed all? -> remove menu item
	if (c == GetLength(objects)) 
	{
		p_source.Menu->RemoveItem(menuItem);
	}
	else if(c > 0)
	{
		// otherwise, update
		
		// repair "holes"
		var remaining_objects = objects[:];
		RemoveHoles(remaining_objects);
		//Log("%v",remaining_objects);
		
		menuItem->SetData(remaining_objects);
		menuItem->SetCount(GetLength(remaining_objects));
		menuItem->SetSymbol(remaining_objects[0]);
	}
}

private func MoveObjects(proplist p_source, proplist p_target, object menuItem, int amount)
{
	// move object to new menu
	TransferObjects(p_source, p_target, menuItem, amount);
	// Update menus
	OnContentChange(p_source.Menu, p_source.Object);
	OnContentChange(p_target.Menu, p_target.Object);
}

/* Interface to menu item as commander_object */

public func OnMouseClick(int x, int y, bool alt)
{
	// Close menu if not clicked on one of the menus.
	var menu = FindObject(Find_Distance(160, x, y), Find_ID(GUI_CircleMenu));
	if (!menu || menu->GetCommander() != this)
		Close();
	return;
}

public func OnItemSelection(object menu, object item)
{
	// Transfer item to previous menu.
	var index = FindMenuPos(menu);
	if(index < 0) return;
	
	// Find this and previous menu.
	var p_source_menu = circ_menus[index];
	var p_target_menu = GetNextMenu(index, false);

	var amount = 1;
	MoveObjects(p_source_menu, p_target_menu, item, amount);
	return true;
}

public func OnItemSelectionAlt(object menu, object item)
{
	// Transfer item to next menu.
	var index = FindMenuPos(menu);
	if(index < 0) return;
	
	// Find this and next menu.
	var p_source_menu = circ_menus[index];
	var p_target_menu = GetNextMenu(index, true);

	var amount = 1;
	MoveObjects(p_source_menu, p_target_menu, item, amount);
	return true;
}

private func GetNextMenu(int index, bool alt)
{
	var last = GetLength(circ_menus) - 1;
	
	// Only one menu: to nothing
	if (last <= 0)
		return nil;
	
	if(alt)
	{
		// the clonk itself or one of the crewmembers (except leftmost)
		if(index < crew_count-1)
			index++;
		// leftmost crewmember (could be the clonk itself too)
		else if(index == crew_count-1)
		{
			// go to rightmost container
			if(container_count == 0)
				index = 0;
			else
				index = last;
		}
		// leftmost container
		else if(index == crew_count)
			index = 0; // go to clonk itself
		// a container
		else
			index--;
	}
	else
	{
		// the clonk itself
		if(index == 0)
			index = crew_count; // go to first container
		// a crewmember
		else if(index < crew_count)
			index--;
		// a container
		else
			index++;
	}
	
	if (index > last)
		index = crew_count-1;
	
	return circ_menus[index];	
}

private func FindMenuPos(object menu)
{
	for(var i = 0; i < GetLength(circ_menus); ++i)
		if (circ_menus[i].Menu == menu)
			return i;
	return -1;
}

func OnItemDropped(object menu, object dropped, object on_item)
{
	var index = FindMenuPos(menu);
	if (index < 0) return false;
	
	var index2 = FindMenuPos(dropped->GetMenu());
	if (index2 < 0) return false;
	
	if (index == index2)
		return false;
	
	var p_source_menu = circ_menus[index2];
	var p_target_menu = circ_menus[index];

	var amount = 1;
	TransferObjects(p_source_menu, p_target_menu, dropped, amount);
	return true;
}

func OnItemDragDone(object menu, object dragged, object on_item)
{
	var index = FindMenuPos(menu);
	if(index < 0) return false;
	
	var p_source_menu = circ_menus[index];

	UpdateAfterTakenObjects(p_source_menu, dragged);
	return true;
}
