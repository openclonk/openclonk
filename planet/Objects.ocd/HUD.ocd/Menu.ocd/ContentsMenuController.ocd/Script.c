/**
	Circular menu controller for contents
	
	Authors: Newton
*/

local circ_menus;
local menu_object;

global func CreateContentsMenus()
{
	if (!this) return;
	if (!(this->GetOCF() & OCF_CrewMember))	return;
	if (!(this->~HasMenuControl())) return;

	var controller = CreateObject(GUI_Contents_Controller);
	controller->SetMenuObject(this);
	this->SetMenu(controller);
	
	var objs = FindObjects(	Find_Or(
								Find_Not(Find_Exclude(this)),
								Find_And(Find_AtPoint(0,0), Find_NoContainer(), Find_Func("IsContainer"))),
							Sort_Func("SortInventoryObjs"));

	var i = 0;
	// for all objects with accessible inventory...
	for(var obj in objs)
	{
		// add menu
		controller->AddMenuFor(obj,i++,GetLength(objs));
	}
	controller->Show();
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
	this.menu_object = menu_object;
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

func AddMenuFor(object obj, int pos, int length)
{
	var menu = CreateObject(GUI_Menu, 0, 0, GetOwner());

	var r = 350;
	
	menu->SetPosition(r+(1280-2*r)*pos/length , r+100);
	menu->SetSymbol(obj);
	menu->SetMenuObject(menu_object);
	menu->SetCommander(this);
	menu->SetDragDropMenu(true);

	PutContentsIntoMenu(menu, obj);
	circ_menus[GetLength(circ_menus)] = {Object = obj, Menu = menu};
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
		item->SetExtraData(stack[1]);
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
			if (!(content->DoesStackWith(stackcontent[0]))) continue;
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

global func DoesStackWith(object other)
{
	if(this->GetID() != other->GetID()) return false;
	if(this->~RejectStack(other)) return false;
	return true;
}

private func CanStackObjIntoMenuItem(object menu, object obj) {
	for(var item in menu->GetItems()) {
		var stack = item->GetExtraData();
		if(obj->DoesStackWith(stack[0])) {
			return item;
		}
	}
}

private func PutObjects(object p_target, object menuItem, int amount)
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
		if (!(obj->Collect(p_target.Object)))
		{
			// putting into other container failed. Put back.
			if (!(obj->Collect(p_source.Object)))
			{
				// (if it fails too, it just stays outside)
			}
		} else {
			moved_to_target[GetLength(moved_to_target)] = obj;
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
			targetItem->SetExtraData(new_extra_data);
			targetItem->SetCount(GetLength(new_extra_data));
		// otherwise, add a new menu item to containing menu
		} else {
			p_target.Menu->AddItem(moved_to_target[0],GetLength(moved_to_target),moved_to_target);
		}
	}
}

private func UpdateAfterTakenObjects(object p_source, object menuItem)
{
	var objects = menuItem->GetExtraData();
	// update menu item in source menu: remove all objects in extradata which are not in
	// container anymore
	var i;
	for (i=0; i<GetLength(objects); ++i)
	{
		if(obj->Contained() != p_source.Object) {
			objects[i] = nil;
		}
	}
	// removed all? -> remove menu item
	if (i == GetLength(objects)) {
		p_source.Menu->RemoveItem(menuItem);
	} else {
		// otherwise, update
		
		// repair "holes"
		var remaining_objects = RemoveHoles(object);
		
		menuItem->SetExtraData(remaining_objects);
		menuItem->SetCount(GetLength(remaining_objects));
		menuItem->SetSymbol(remaining_objects[0]);
	}
}

private func MoveObjects(object p_source, object p_target, object menuItem, int amount)
{
	PutObjects(p_target, menuItem, amount);
	UpdateAfterTakenObjects(p_source, menuItem);
}

/* Interface to menu item as commander_object */

func OnItemSelection(object menu, object item)
{
	var index = FindMenuPos(menu);
	if(index < 0) return;
	
	var p_source_menu = circ_menus[index];
	var p_target_menu = DetermineClickTarget(index);

	var amount = 1;
	MoveObjects(p_source_menu, p_target_menu, item, amount);
	return true;
}

func OnItemSelectionAlt(object menu, object item)
{
	// nothing
}

private func DetermineClickTarget(int index)
{
	var last = GetLength(circ_menus)-1;
	// only one menu: to nothing
	if (last == 0)
		return nil;
	// last: click to second-last
	else if (index == last)
		return circ_menus[last-1];
	// not last: click to last
	else
		return circ_menus[last];
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
	
	var p_target_menu = circ_menus[index];

	var amount = 1;
	PutObjects(p_target_menu, dropped, amount);
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