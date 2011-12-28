/**
	Control object for content menus.
	
	@author Newton, Maikel
*/

local circ_menus;
local menu_object;

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
	
	// Add content menus for all containers at the position of the caller.
	var containers = FindObjects(Find_Or(Find_Not(Find_Exclude(this)), Find_And(Find_AtPoint(), Find_NoContainer(), Find_Func("IsContainer"))), Sort_Func("SortInventoryObjs"));
	for(var index = 0; index  < GetLength(containers); index++)
		controller->AddContentMenu(containers[index], index, GetLength(containers));
	
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

func SetMenuObject(object menu_object)
{
	menu_object = menu_object;
}

func Construction()
{
	circ_menus = [];
}

func Close() {
	if(menu_object)
		menu_object->~MenuClosed(this);
	RemoveObject();
}

func Destruction() 
{
	// remove all menu objects
	for(var prop in circ_menus)
		prop.Menu->RemoveObject();
}

func Show() 
{
	for(var prop in circ_menus)
		prop.Menu->Show();
}

func Hide() 
{
	for(var prop in circ_menus)
		prop.Menu->Hide();
}

func AddContentMenu(object container, int pos, int length)
{
	var menu = CreateObject(GUI_Menu, 0, 0, GetOwner());

	menu->SetSymbol(container);
	menu->SetMenuObject(menu_object);
	menu->SetCommander(this);
	menu->SetDragDropMenu(true);

	PutContentsIntoMenu(menu, container);
	circ_menus[GetLength(circ_menus)] = {Object = container, Menu = menu};
	
	UpdateContentMenus();	
	return;
}

// Draws the contents menus to the right positions.
private func UpdateContentMenus()
{
	var menu_count = GetLength(circ_menus);
	for (var index = 0; index < menu_count; index++)
	{
		var menu = circ_menus[index].Menu;
		var r = 160;
		var d = 2*r/3;
		var dx = Sqrt(r**2 - d**2);
		
		// Determine x-position.
		var x = 800 + dx * (2*index - menu_count + 1);
		// Alternate y-position.
		var y = 320 + (-1)**index * d;
		// TODO: reduce size of menus to fit more into screen.
		menu->SetPosition(x, y);
	}
	return;
}

private func PutContentsIntoMenu(object menu, object obj)
{
	var stacked_contents = obj->GetStackedContents();
	for(var stack in stacked_contents)
	{
		// into the menu item, all the objects of the stack are saved
		// as an array into it's extradata
		var item = CreateObject(GUI_MenuItem);
		if (!menu->AddItem(item))
			return item->RemoveObject();
		item->SetSymbol(stack[0]);
		item->SetCount(GetLength(stack));
		item->SetData(stack);
		// Track removal of contents.
		for (var stack_obj in stack)
			AddEffect("ContentTracker", stack_obj, 100, nil, this, nil, menu, item, stack);
	}
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

public func FxContentTrackerStart(object target, proplist effect, int temporary, object menu, object item, object stack)
{
	if (temporary == 0)
	{
		effect.Menu = menu;
		effect.Item = item;
		effect.Stack = stack;
	}
	return 1;
}

public func FxContentTrackerTimer()
{

}

public func FxContentTrackerStop(object target, proplist effect, int reason)
{
	// Notify content menu if object has been removed.
	if (reason == 3)
	{
		//Log("Object removed");
		effect.CommandTarget->~OnExternalContentRemoval(effect.Menu, effect.Item, effect.Stack, target);
	}
	return 1;
}

public func OnExternalContentRemoval(object menu, object item, array stack, object content)
{
	//Log("%v", stack);
	
	// If stack will be empty, remove menu item.
	if (GetLength(stack) <= 1)
		return item->RemoveObject();
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
	var objects = menu_item->GetExtraData();
	amount = BoundBy(amount, 0, GetLength(objects));
		
	// Move to object from source container to target container.
	// Memorize which objects have been put where in order to know which menu items to update
	// how (Because the collection can be rejected by RejectCollect/RejectEntrance).
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
			RemoveEffect("ContentTracker", obj);
		}
	}
	
	// Update target menu, source menu is updated elsewhere.
	if (moved_length > 0)
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
			var item = CreateObject(GUI_MenuItem);
			if (!p_target.Menu->AddItem(item))
				item->RemoveObject();
			else
			{
				item->SetSymbol(moved_to_target[0]);
				item->SetCount(GetLength(moved_to_target));
				item->SetData(moved_to_target);
				for (var stack_obj in moved_to_target)
					AddEffect("ContentTracker", stack_obj, 100, nil, this, nil, p_target.Menu, item, moved_to_target);
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
	for (i=0; i<GetLength(objects); ++i)
	{
		var obj = objects[i];
		if(obj->Contained() != p_source.Object) {
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
		var remaining_objects = RemoveHoles(objects);
		//Log("%v",remaining_objects);
		
		menuItem->SetData(remaining_objects);
		menuItem->SetCount(GetLength(remaining_objects));
		menuItem->SetSymbol(remaining_objects[0]);
	}
}

private func MoveObjects(proplist p_source, proplist p_target, object menuItem, int amount)
{
	TransferObjects(p_source, p_target, menuItem, amount);
	UpdateAfterTakenObjects(p_source, menuItem);
}

/* Interface to menu item as commander_object */

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
	// Determine next menu.
	if (alt)
		index++;
	else
		index--;
	if (index > last)
		index = 0;
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
