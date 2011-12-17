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
		item->SetSymbol(stack[0]);
		item->SetCount(GetLength(stack));
		item->SetData(stack);
		if (!menu->AddItem(item))
			item->RemoveObject();
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

private func PutObjects(proplist p_source, proplist p_target, object menuItem, int amount)
{
	var objects = menuItem->GetExtraData();
	amount = BoundBy(amount, 0, GetLength(objects));
	
	// move to target container.
	// memorize which objects have been put where in order to know which menu items to
	// update how (Because the collection can be rejected by RejectCollect/RejectEntrance)
	var moved_to_target = [];
	for (var i=0; i<amount; ++i) 
	{
		var obj = objects[i];
		obj->Exit();
		if (p_target)
		{
			if (!(p_target.Object->Collect(obj)))
			{
				// putting into other container failed. Put back.
				if (!(p_source.Object->Collect(obj)))
				{
					// (if it fails too, it just stays outside)
				}
			} else {
				moved_to_target[GetLength(moved_to_target)] = obj;
			}
		}
	}
	
	var items_moved_to_target = GetLength(moved_to_target);
	
	// update target menu
	if (items_moved_to_target > 0)
	{
		// if menu item with same objects already exists: update menu item
		var targetItem = CanStackObjIntoMenuItem(p_target.Menu, moved_to_target[0]);
		if (targetItem)
		{
			var new_extra_data = Concatenate(targetItem->GetExtraData(), moved_to_target);
			targetItem->SetData(new_extra_data);
			targetItem->SetCount(GetLength(new_extra_data));
		// otherwise, add a new menu item to containing menu
		} else {
			var item = CreateObject(GUI_MenuItem);
			item->SetSymbol(moved_to_target[0]);
			item->SetCount(GetLength(moved_to_target));
			item->SetData(moved_to_target);
			if (!p_target.Menu->AddItem(item))
				item->RemoveObject();
		}
	}
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
		Log("%v",remaining_objects);
		
		menuItem->SetData(remaining_objects);
		menuItem->SetCount(GetLength(remaining_objects));
		menuItem->SetSymbol(remaining_objects[0]);
	}
}

private func MoveObjects(proplist p_source, proplist p_target, object menuItem, int amount)
{
	PutObjects(p_source, p_target, menuItem, amount);
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
	if(index < 0) return false;
	
	var index2 = FindMenuPos(dropped->GetMenu());
	if(index2 < 0) return false;
	
	var p_source_menu = circ_menus[index2];
	var p_target_menu = circ_menus[index];

	var amount = 1;
	PutObjects(p_source_menu, p_target_menu, dropped, amount);
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
