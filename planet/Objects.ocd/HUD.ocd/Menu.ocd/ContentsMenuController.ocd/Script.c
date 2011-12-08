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
	
	var objs = FindObjects(	Find_Or(
								Find_Not(Find_Exclude(this)),
								Find_And(Find_AtPoint(0,0), Find_NoContainer(), Find_OCF(OCF_Container))),
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

func Destruction() {
	// remove all menu objects
	for(var prop in circ_menus)
		prop.Menu->RemoveObject();
}

func Show() {
	for(var prop in circ_menus)
		prop.Menu->Show();
}

func Hide() {
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

private func MoveObjects(object p_source, object p_target, object menuItem, int amount)
{

	var objects = menuItem->GetExtraData();
	amount = BoundBy(amount, 0, GetLength(objects));
	
	// move to target container.
	// memorize which objects have been put where in order to know which menu items to
	// update how (Because the collection can be rejected by RejectCollect/RejectEntrance)
	var moved_to_target = [];
	var removed_from_source = [];
	for(var obj in objects) 
	{
		obj->Exit();
		if(!(obj->Collect(p_target.Object)))
		{
			// putting into other container failed. Put back.
			if(!(obj->Collect(p_source.Object)))
			{
				removed_from_source[GetLength(removed_from_source)] = obj;
				// (if it fails too, it just stays outside)
			}
		} else {
			moved_to_target[GetLength(moved_to_target)] = obj;
			removed_from_source[GetLength(removed_from_source)] = obj;
		}
	}
	
	// decimate which objects are displayed in source container
	var newobjects = [];
	if (GetLength(removed_from_source) < GetLength(objects))
		newobjects = Subtract(objects, removed_from_source);
	
	var items_moved_to_target = GetLength(moved_to_target);
	var items_removed_from_source = GetLength(removed_from_source);
	
	// update target menu
	if (items_moved_to_target > 0)
	{
		// if menu item with same objects already exists: update menu item
		var targetItem = CanStackObjIntoMenuItem(p_target.Menu, moved_to_target[0]);
		if(targetItem)
		{
			var new_extra_data = Concatenate(targetItem->GetExtraData(), moved_to_target);
			targetItem->SetExtraData(new_extra_data);
			targetItem->SetCount(GetLength(new_extra_data));
		// otherwise, add a new menu item to containing menu
		} else {
			p_target.Menu->AddItem(moved_to_target[0],GetLength(moved_to_target),moved_to_target);
		}
	}
	// update source menu
	if (items_removed_from_source > 0)
	{
		// not all items in the menu item were transferred: only update menu item
		if(items_removed_from_source < GetLength(objects))
		{
			menuItem->SetSymbol(newobjects[0]);
			menuItem->SetCount(GetLength(newobjects));
			menuItem->SetExtraData(newobjects);
		// otherwise, delete menu item from containing menu
		} else {
			p_source.Menu->RemoveItem(menuItem);
		}
	}
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
