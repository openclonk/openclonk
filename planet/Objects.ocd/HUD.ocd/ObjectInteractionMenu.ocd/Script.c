/**
	ObjectInteractionMenu
	Handles the inventory exchange and general interaction between the player and buildings, vehicles etc.

	@author Zapper
*/

local Name = "$Name$";
local Description = "$Description$";

static const InteractionMenu_Contents = 2;
static const InteractionMenu_Custom = 4;

local current_objects;

/*
	current_menus is an array with two fields
	each field contain a proplist with the attributes:
		target: the target object, needs to be in current_objects
		menu_object: target of the menu (usually a dummy object)
		menu_id
		sidebar: proplist with the following attributes
			menu_object
		menus: array with more proplists with the following attributes:
			type: flag (needed for content-menus f.e.)
			title
			decoration: ID of a menu decoration definition
			priority: priority of the menu (Y position)
			callback: function called when an entry is selected, the function is passed the symbol of an entry and the user data
			callback_target: object to which the callback is made, ususally the target object (except for contents menus)
			menu_object: MenuStyle_Grid object, used to add/remove entries later
			entry_index_count: used to generate unique IDs for the entries
			entries: array of entries shown in the menu, the entries are proplists with the following attributes
				symbol:
				extra_data: custom user data
				text: text shown on the object
				unique_index: generated from entry_index_count (not set by user)
*/
local current_menus;

// this is the ID of the root window that contains the other subwindows (i.e. the menus which contain the sidebars and the interaction-menu)
local current_main_menu_id;

local cursor;

public func Close() { return RemoveObject(); }
public func IsContentMenu() { return true; }
public func Show() { this.Visibility = VIS_Owner; return true; }
public func Hide() { this.Visibility = VIS_None; return true; }

func Construction()
{
	current_objects = [];
	current_menus = [];
}

func Destruction()
{
	// we need only to remove the top-level menu target of the open menus, since the submenus close due to a clever use of OnClose!
	for (var menu in current_menus)
	{
		if (menu.menu_object)
			menu.menu_object->RemoveObject();
	}
}

// used as a static function
func CreateFor(object cursor)
{
	var obj = CreateObject(GUI_ObjectInteractionMenu, AbsX(0), AbsY(0), cursor->GetOwner());
	obj.Visibility = VIS_Owner;
	obj->Init(cursor);
	cursor->SetMenu(obj);
	return obj;
}


func Init(object cursor)
{
	AddEffect("IntCheckObjects", cursor, 1, 10, this);
}

func FxIntCheckObjectsStart(target, effect, temp)
{
	if (temp) return;
	EffectCall(target, effect, "Timer");
}

func FxIntCheckObjectsTimer(target, effect, timer)
{
	var new_objects = FindObjects(Find_AtPoint(target->GetX(), target->GetY()), Find_Or(Find_Category(C4D_Vehicle), Find_Category(C4D_Structure), Find_Func("IsContainer"), Find_Func("IsClonk")));
	
	if (new_objects == current_objects) return;
	
	UpdateObjects(new_objects);
}

// updates the objects shown in the side bar
// if an object which is in the menu on the left or on the right is not in the side bar anymore, another object is selected
func UpdateObjects(array new_objects)
{
	// need to close a menu?
	for (var i = 0; i < GetLength(current_menus); ++i)
	{
		var target = current_menus[i].target;
		var found = false;
		for (var obj in new_objects)
		{
			if (target != obj) continue;
			found = true;
			break;
		}
		if (found) continue;
		// not found? close!
		// sub menus close automatically (and remove their dummy) due to a clever usage of OnClose
		current_menus[i].menu_object->RemoveObject();
		current_menus[i] = nil;
	}
	
	current_objects = new_objects;
	
	// need to fill an empty menu slot?
	for (var i = 0; i < 2; ++i)
	{
		if (current_menus[i] != nil) continue;
		// look for next object to fill slot
		for (var obj in current_objects)
		{
			// but only if the object's menu is not already open
			var is_already_open = false;
			for (var menu in current_menus)
			{
				if (menu.target != obj) continue;
				is_already_open = true;
				break;
			}
			if (is_already_open) continue;
			// use object to create a new menu at that slot
			OpenMenuForObject(obj, i);
			break;
		}
	}
}

func FxIntCheckObjectsStop(target, effect, reason, temp)
{
	if (temp) return;
	if (this)
		this->RemoveObject();
}

// obj has to be in current_objects
func OpenMenuForObject(object obj, int slot)
{
	// clean up old menu
	var old_menu = current_menus[slot];
	if (old_menu)
		old_menu.menu_object->RemoveObject();
	current_menus[slot] = nil;
	// before creating the sidebar, we have to create a new entry in current_menus, even if it contains not all information
	current_menus[slot] = {target = obj};
	// create a menu with all interaction possibilities for an object
	// always recreate the side bar
	var sidebar = CreateSideBar(slot);
	var main = CreateMainMenu(obj, slot);
	// to close the part_menu automatically when the main menu is closed
	current_menus[slot].menu_object = main.Target;
	
	if(false)
	for (var key in GetProperties(sidebar))
	{
		var item = sidebar[key];
		var obj;
		if ((GetType(item) == C4V_PropList) && (obj = item.obj_symbol.Symbol))
			Log("sidebar %s entry [%s] %s", sidebar.Target->GetName(), key, obj->GetName());
	}
	Log("sidebar for %s: %v", obj->GetName(), sidebar);
	var part_menu = 
	{
		Style = GUI_NoCrop,
		Left = "0%", Right = "50%-1em",
		sidebar = sidebar, main = main,
		Target = current_menus[slot].menu_object
	};
	
	if (slot == 1)
	{
		part_menu.Left = "50%+1.5em";
		part_menu.Right = "100%";
	}
	
	
	// need to open a completely new menu?
	if (!current_main_menu_id)
	{
		var root_menu = 
		{
			one_part = part_menu,
			Target = this
		};
		current_main_menu_id = CustomGuiOpen(root_menu);
	}
	else // menu already exists and only one part has to be added
	{
		CustomGuiUpdate({update = part_menu}, current_main_menu_id, nil, nil);
	}
	
	// todo: create main menu
	// description menu?
}

// generates a proplist that defines a custom GUI that represents a side bar where objects 
// to interact with can be selected
func CreateSideBar(int slot)
{
	var s = "";
	for (var obj in current_objects)
		s = Format("%s, %s", s, obj->GetName());
	Log("sidebar for: %s", s);
	var dummy = CreateDummy();
	var sidebar =
	{
		Priority = 10,
		Right = "+4em",
		Style = GUI_VerticalLayout,
		Target = dummy,
		OnClose = GuiAction_Call(this, "RemoveDummy", dummy),
	};
	if (slot == 1)
	{
		sidebar.Left = "100%-4em";
		sidebar.Right = "100%";
	}
	
	// now show the current_objects list as entries
	for (var obj in current_objects)
	{
		var background_color = nil;
		var symbol = {Std = Icon_Menu_RectangleRounded, OnHover = Icon_Menu_RectangleBrightRounded};
		// figure out whether the object is already selected
		// if so, highlight the entry
		if (current_menus[slot].target == obj)
		{
			background_color = RGBa(255, 255, 0, 10);
			symbol = Icon_Menu_RectangleBrightRounded;
		}
		var entry = 
		{
			Right = "+4em", Bottom = "+4em",
			Symbol = symbol,
			Style = GUI_TextBottom | GUI_TextHCenter,
			BackgroundColor = background_color,
			OnMouseIn = GuiAction_SetTag("OnHover"),
			OnMouseOut = GuiAction_SetTag("Std"),
			OnClick = GuiAction_Call(this, "OnSidebarEntrySelected", {slot = slot, obj = obj}),
			Text = obj->GetName(),
			obj_symbol = {Symbol = obj, Margin = "0.5em"}
		};
		
		Gui_AddSubwindow(entry, sidebar);
	}
	return sidebar;
}

func OnSidebarEntrySelected(data, int player, int ID, int subwindowID, object target)
{
	Log("OnSideBarEntrySelected obj: %s, slot: %d", data.obj->GetName(), data.slot); 
	if (!data.obj) return;
	
	// can not open object twice!
	for (var menu in current_menus)
		if (menu.target == data.obj) return;
	OpenMenuForObject(data.obj, data.slot);
}

func CreateMainMenu(object obj, int slot)
{
	var container = 
	{
		Target = CreateDummy(),
		Priority = 5,
		Decoration = GUI_MenuDeco,
		Right = "100%-4em",
		Style = GUI_VerticalLayout
	};
	if (slot == 0)
	{
		container.Left = "+4em";
		container.Right = "100%";
	}
	var menus = [];
	// get all interaction info from the object and put it into a menu
	// contents first
	if (obj->~IsContainer() || obj->~IsClonk())
	{
		var info = 
		{
			flag = InteractionMenu_Contents,
			title = "$Contents$",
			entries = [],
			callback = "OnContentsSelection",
			callback_target = this,
			decoration = GUI_MenuDecoInventoryHeader,
			priority = 10
		};
		PushBack(menus, info);
	}
	
	current_menus[slot].menus = menus;
	
	// now generate the actual menus from the information-list
	for (var i = 0; i < GetLength(menus); ++i)
	{
		var menu = menus[i];
		menu.menu_object = CreateObject(MenuStyle_Grid);
		if (menu.decoration)
			menu.menu_object.BackgroundColor = menu.decoration->FrameDecorationBackClr();
		menu.menu_object.Top = "+2em";
		menu.menu_object.Priority = 2;
		menu.menu_object->SetPermanent();

		for (var e = 0; e < GetLength(menu.entries); ++e)
		{
			var entry = menu.entries[e];
			entry.unique_index = ++menu.entry_index_count;
			menu.menu_object->AddItem(entry.symbol, entry.text, entry.unique_index, this, "OnMenuEntrySelected", { slot = slot, index = i });
		}
		
		var all = // menu with title bar
		{
			Priority = menu.priority ?? i,
			Style = GUI_NoCrop,
			title_bar = 
			{
				Priority = 1,
				Bottom = "+1em",
				Text = menu.title,
				Decoration = menu.decoration
			},
			real_menu = menu.menu_object
		};
		Gui_AddSubwindow(all, container);
	}
	
	// add refreshing effects for all of the contents menus
	for (var i = 0; i < GetLength(menus); ++i)
	{
		if (!(menus[i].flag & InteractionMenu_Contents)) continue;
		AddEffect("IntRefreshContentsMenu", this, 1, 5, this, nil, obj, slot, i);
	}
	
	return container;
}

func OnMenuEntrySelected(proplist menu_info, int entry_index, int player)
{
	Log("OnMenuEntrySelected slot: %d, index: %d, entry: %d", menu_info.slot, menu_info.index, entry_index);
	if (!current_menus[menu_info.slot]) return;
	var menu;
	if (!(menu = current_menus[menu_info.slot].menus[menu_info.index])) return;
	var entry;
	for (var possible in menu.entries)
	{
		if (possible.unique_index != entry_index) continue;
		entry = possible;
		break;
	}
	if (!entry) return;
	
	var callback_target;
	if (!(callback_target = menu.callback_target)) return;
	var result = callback_target->Call(menu.callback, entry.symbol, entry.extra_data);
	
	// todo: trigger refresh for special value of result
}

func OnContentsSelection(symbol, extra_data)
{
	Log("OnContentsSelection symbol: %s", symbol->GetName());
	var target = current_menus[extra_data.slot].target;
	if (!target) return;
	// no target to swap to?
	if (!current_menus[1 - extra_data.slot]) return;
	var other_target = current_menus[1 - extra_data.slot].target;
	if (!other_target) return;
	
	var obj = FindObject(Find_Container(target), Find_ID(symbol));
	if (!obj) return;
	
	if (other_target->Collect(obj, true))
	{
		Sound("SoftTouch*", true, nil, GetOwner());
		return true;
	}
	else
	{
		Sound("BalloonPop", true, nil, GetOwner());
		return false;
	}
}

func FxIntRefreshContentsMenuStart(object target, proplist effect, temp, object obj, int slot, int menu_index)
{
	if (temp) return;
	effect.obj = obj;
	effect.slot = slot;
	effect.menu_index = menu_index;
	effect.last_inventory = [];
}

func FxIntRefreshContentsMenuTimer(target, effect, time)
{
	if (!effect.obj) return -1;
	
	var inventory = [];
	var obj, i = 0;
	while (obj = effect.obj->Contents(i++))
	{
		var symbol = obj->GetID();
		var extra_data = {slot = effect.slot, menu_index = effect.menu_index};
		
		// check if already exists (and then stack!)
		var found = false;
		for (var inv in inventory)
		{
			if (inv.symbol != symbol) continue;
			if (inv.count) ++inv.count;
			else inv.count = 2;
			inv.text = Format("%dx", inv.count);
			found = true;
			break;
		}
		if (!found)
		{
			PushBack(inventory, {symbol = symbol, extra_data = extra_data});
		}
	}
	
	/*for(var i = 0; i < Max(GetLength(effect.last_inventory), GetLength(inventory)); ++i)
	{
		var ls = "left: /"; if (i < GetLength(inventory))
			ls = Format("left: %s", inventory[i].symbol->GetName());
		var rs = "right: /"; if (i < GetLength(effect.last_inventory))
			rs = Format("right: %s", effect.last_inventory[i].symbol->GetName());
		Log("%s   %s", ls, rs);
	}*/
	
	if (GetLength(inventory) == GetLength(effect.last_inventory))
	{
		var same = true;
		for (var i = GetLength(inventory)-1; i >= 0; --i)
		{
			if (inventory[i].symbol == effect.last_inventory[i].symbol
				&& inventory[i].text == effect.last_inventory[i].text) continue;
			same = false;
			break;
		}
		if (same) return 1;
	}
	

	effect.last_inventory = inventory;
	DoMenuRefresh(effect.slot, effect.menu_index, inventory);
	return 1;
}

// this function is supposed to be called when the menu already exists (is open) and some sub-menu needs an update
func DoMenuRefresh(int slot, int menu_index, array new_entries)
{
	// go through new_entries and look for differences to currently open menu
	// then try to only adjust the existing menu when possible
	// the assumption is that ususally only few entries change
	var menu = current_menus[slot].menus[menu_index];
	var current_entries = menu.entries;
	
	// step 1: remove (close) all current entries that have been removed
	for (var c = 0; c < GetLength(current_entries); ++c)
	{
		// check for removal
		var removed = true;
		for (var new_entry in new_entries)
		{
			if (!EntriesEqual(new_entry, current_entries[c])) continue;
			removed = false;
			break;
		}
		if (removed)
		{
			menu.menu_object->RemoveItem(current_entries[c].unique_index, current_main_menu_id);
			current_entries[c] = nil;
		}
	}
	
	// step 2: add new entries
	var debug = "new entries: ";
	for (var c = 0; c < GetLength(new_entries); ++c)
	{
		var new_entry = new_entries[c];
		debug = Format("%s, %s", debug, new_entry.symbol->GetName());
		var existing = false;
		for (var old_entry in current_entries)
		{
			if (old_entry == nil) // might be nil as a result of step 1
				continue;
			if (!EntriesEqual(new_entry, old_entry)) continue;
			existing = true;
			
			// fix unique indices for the new array
			new_entry.unique_index = old_entry.unique_index;
			break;
		}
		if (existing) continue;
		
		new_entry.unique_index = ++menu.entry_index_count;
		menu.menu_object->AddItem(new_entry.symbol, new_entry.text, new_entry.unique_index, this, "OnMenuEntrySelected", { slot = slot, index = menu_index }, nil, current_main_menu_id);
		
	}
	Log(debug);
	menu.entries = new_entries;
}

func EntriesEqual(proplist entry_a, proplist entry_b)
{
	return entry_a.symbol == entry_b.symbol
	&& entry_a.text == entry_b.text
	&& entry_a.extra_data == entry_b.extra_data;
}

func CreateDummy()
{
	var dummy = CreateContents(Dummy);
	dummy.Visibility = VIS_Owner;
	dummy->SetOwner(GetOwner());
	return dummy;
}

func RemoveDummy(object dummy, int player, int ID, int subwindowID, object target)
{
	Log("removing dummy!");
	if (dummy)
		dummy->RemoveObject();
}
