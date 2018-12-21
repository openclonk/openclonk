/**
	ObjectInteractionMenu
	Handles the inventory exchange and general interaction between the player and buildings, vehicles etc.

	@author Zapper
*/

local Name = "$Name$";
local Description = "$Description$";

static const InteractionMenu_Contents = 2;
static const InteractionMenu_Custom = 4;

/*
	This contains an array with a proplist for every player number.
	The attributes are also always attached to every interaction menu on creation (menu.minimized = InteractionMenu_Attributes[plr].minimized;).
	The following properties are either used or nil:
		minimized (bool): whether the player minimized the menu.
			A minimized menu does not show some elements (like the description box).
*/
static InteractionMenu_Attributes;

local current_objects;

/*
	current_menus is an array with two fields
	each field contain a proplist with the attributes:
		target: the target object, needs to be in current_objects
		menu_object: target of the menu (usually a dummy object) (the ID is always 1; the menu's sidebar has the ID 2)
		menu_id
		forced: (boolean) Whether the menu was forced-open (f.e. by an extra-slot object) and is not necessarily attached to an outside-world object.
			Such an object might be removed from the list when the menu is closed.
		menus: array with more proplists with the following attributes:
			flag: bitwise flag (needed for content-menus f.e.)
			title
			decoration: ID of a menu decoration definition
			Priority: priority of the menu (Y position)
			BackgroundColor: background color of the menu
			callback: function called when an entry is selected, the function is passed the symbol of an entry and the user data
			callback_hover: function called when hovering over an entry, the function is passed everything "callback" gets plus the target of the description box menu
			callback_target: object to which the callback is made, ususally the target object (except for contents menus)
			menu_object: MenuStyle_Grid object, used to add/remove entries later
			entry_index_count: used to generate unique IDs for the entries
			entries_callback: (callback) function that can be used to retrieve a list of entries for that menu (at any point - it might also be called later).
				The function is called in the object that the menu was opened for and passes the player's Clonk as the first argument. 
				This callback should return an array of entries shown in the menu, the entries are proplists with the following attributes:
				symbol: icon of the item
				extra_data: custom user data (internal: in case of inventory menus this is a proplist containing some extra data (f.e. the one object for unstackable objects))
				text: text shown on the object (f.e. count in inventory)
				custom (optional): completely custom menu entry that is passed to the grid menu - allows for custom design
				unique_index: generated from entry_index_count (not set by user)
				fx: (optional) effect that gets a "OnMenuOpened(int menu_id, object menu_target, int subwindow_id)" callback once which can be used to update a specific entry only
			entries_callback_parameter (optional):
				Passed as second argument to entries_callback. Can be used for custom information.
			entries_callback_target (optional):
				By default the object for which the menu is opened, can be changed for more involved constructions.
			entries: last result of the callback function described above
				additional properties that are added are:
				ID: (menu) id of the entry as returned by the menu_object - can be used for updating
*/
local current_menus;
/*
	current_description_box contains information about the description box at the bottom of the menu:
		target: target object of the description box
		symbol_target: target object of the symbol sub-box
		desc_target: target object of the description sub-box
*/
local current_description_box;
// this is the ID of the root window that contains the other subwindows (i.e. the menus which contain the sidebars and the interaction-menu)
local current_main_menu_id;
// This holds the dummy object that is the target of the center column ("move all left/right").
local current_center_column_target;
// The Clonk who the menu was opened for.
local cursor;

public func Close() { return RemoveObject(); }
public func IsContentMenu() { return true; }
public func Show() { this.Visibility = VIS_Owner; return true; }
public func Hide() { this.Visibility = VIS_None; return true; }
// Called when the menu is open and the player clicks outside.
public func OnMouseClick() { return Close(); }

func Construction()
{
	current_objects = [];
	current_menus = [];
	current_description_box = {target=nil};
}

func Destruction()
{
	// we need only to remove the top-level menu target of the open menus, since the submenus close due to a clever use of OnClose!
	for (var menu in current_menus)
	{
		if (menu && menu.menu_object)
		{
			// Notify the object of the deleted menu.
			DoInteractionMenuClosedCallback(menu.target);
			menu.menu_object->RemoveObject();
		}
	}
	// remove all remaining contained dummy objects to prevent script warnings about objects in removed containers
	var i = ContentsCount(), obj = nil;
	while (obj = Contents(--i))
		obj->RemoveObject(false);
	// Remove check objects effect.
	if (cursor)
		RemoveEffect("IntCheckObjects", cursor);
}

// used as a static function
func CreateFor(object cursor, id style_def, array settings)
{
	var obj = CreateObject(GUI_ObjectInteractionMenu, AbsX(0), AbsY(0), cursor->GetOwner());
	obj.Visibility = VIS_Owner;
	
	if (InteractionMenu_Attributes == nil)
		InteractionMenu_Attributes = [];
	
	// Transfer some attributes from the player configuration.
	if (GetLength(InteractionMenu_Attributes) > cursor->GetOwner())
	{
		var config = InteractionMenu_Attributes[cursor->GetOwner()];
		obj.minimized = GetProperty("minimized", config) ?? false; 
	}
	else
	{
		obj.minimized = false;
	}
	if (settings != nil)
	{
		obj.Settings = settings;
		for (var setting in settings)
			obj[setting[0]] = setting[1];
	}
	obj->Init(cursor, style_def);
	cursor->SetMenu(obj);
	return obj;
}

// Reopen the menu with a certain settings value set (arbitrarily defined)
// All previous settings will be kept
public func ReopenWithSetting(string setting_name, value)
{
	var last_cursor = this.cursor;
	var settings = [];
	if (this.Settings != nil)
		var settings = this.Settings[:];
	var i;
	for (i = 0; i < GetLength(settings); i++)
		if (settings[i][0] == setting_name)
			break;
	settings[i] = [setting_name, value];
	var style_def = this.style_def;
	RemoveObject();
	GUI_ObjectInteractionMenu->CreateFor(last_cursor, style_def, settings);
}


public func Init(object cursor, id style_def)
{
	this.cursor = cursor;
	this.style_def = style_def;
	this.style_def->Init(this);
	//this.Uncollapsed = uncollapsed;
	var checking_effect = AddEffect("IntCheckObjects", cursor, 1, 10, this);
	// Notify the Clonk. This can be used to create custom entries in the objects list via helper objects. For example the "Your Environment" tab.
	// Note that the cursor is NOT informed when the menu is closed again. Helper objects can be attached to the livetime of this menu by, f.e., effects.
	cursor->~OnInteractionMenuOpen(this);
	// And then quickly refresh for the very first time. Successive refreshs will be only every 10 frames.
	EffectCall(cursor, checking_effect, "Timer");
}

func FxIntCheckObjectsStart(target, effect fx, temp)
{
	if (temp) return;
	EffectCall(target, fx, "Timer");
}

func FxIntCheckObjectsTimer(target, effect fx)
{
	// If contained, leave the search area intact, because otherwise we'd have to pass a "nil" parameter to FindObjects (which needs additional hacks).
	// This is a tiny bit slower (because the area AND the container have to be checked), but the usecase of contained Clonks is rare anyway.
	var container_restriction = Find_NoContainer();
	var container = target->Contained();
	if (container)
	{
		container_restriction = Find_Or(Find_Container(container), Find_InArray([container]));
	}
	
	var new_objects = FindObjects(Find_AtRect(target->GetX() - 5, target->GetY() - 10, 10, 21), container_restriction, Find_Layer(target->GetObjectLayer()),
		// Find all containers and objects with a custom menu.
		Find_Or(Find_Func("IsContainer"), Find_Func("HasInteractionMenu")),
		// Do not show objects with an extra slot though - even if they are containers. They count as items here and can be accessed via the surroundings tab.
		Find_Not(Find_And(Find_Property("Collectible"), Find_Func("HasExtraSlot"))),
		// Show only objects that the player can see.
		Find_Func("CheckVisibility", GetOwner()),
		// Normally sorted by z-order. But some objects may have a lower priority.
		Sort_Reverse(Sort_Func("GetInteractionPriority", target))
		);
	var equal = GetLength(new_objects) == GetLength(current_objects);
	
	if (equal)
	{
		for (var i = GetLength(new_objects) - 1; i >= 0; --i)
		{
			if (new_objects[i] == current_objects[i]) continue;
			equal = false;
			break;
		}
	}
	if (!equal)
		UpdateObjects(new_objects);
}

// updates the objects shown in the side bar
// if an object which is in the menu on the left or on the right is not in the side bar anymore, another object is selected
func UpdateObjects(array new_objects)
{
	// need to close a menu?
	for (var i = 0; i < GetLength(current_menus); ++i)
	{
		if (!current_menus[i]) continue; // todo: I don't actually know why this can happen.
		if (current_menus[i].forced) continue;
		
		var target = current_menus[i].target;
		// Still existant? Nothing to do!
		if (GetIndexOf(new_objects, target) != -1) continue;
		// not found? close!
		// sub menus close automatically (and remove their dummy) due to a clever usage of OnClose
		current_menus[i].menu_object->RemoveObject();
		current_menus[i] = nil;
		// Notify the target of the now closed menu.
		DoInteractionMenuClosedCallback(target);
	}
	
	current_objects = new_objects;
	
	// need to fill an empty menu slot?
	for (var i = 0; i < 2; ++i)
	{
		// If the menu already exists, just update the sidebar.
		if (current_menus[i] != nil)
		{
			RefreshSidebar(i);
			continue;
		}
		// look for next object to fill slot
		for (var obj in current_objects)
		{
			// but only if the object's menu is not already open
			var is_already_open = false;
			for (var menu in current_menus)
			{
				if (!menu) continue; // todo: I don't actually know why that can happen.
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

/*
	This is the entry point.
	Create a menu for an object (usually from current_objects) and also create everything around it if it's the first time a menu is opened.
*/
func OpenMenuForObject(object obj, int slot, bool forced)
{
	forced = forced ?? false;
	// clean up old menu
	var old_menu = current_menus[slot];
	var other_menu = current_menus[1 - slot];
	if (old_menu)
	{
		// Notify other object of the closed menu.
		DoInteractionMenuClosedCallback(old_menu.target);
		
		// Re-enable entry in (other!) sidebar.
		if (other_menu)
		{
			GuiUpdate({Symbol = nil}, current_main_menu_id, 1 + 1 - slot, old_menu.target);
		}
		// ..and close old menu.
		old_menu.menu_object->RemoveObject();
	}
	current_menus[slot] = nil;
	// before creating the sidebar, we have to create a new entry in current_menus, even if it contains not all information
	current_menus[slot] = {target = obj, forced = forced};
	// clean up old inventory-check effects that are not needed anymore
	var effect_index = 0, inv_effect = nil;
	while (inv_effect = GetEffect("IntRefreshContentsMenu", this, effect_index))
	{
		if (inv_effect.obj != current_menus[0].target && inv_effect.obj != current_menus[1].target)
			RemoveEffect(nil, nil, inv_effect);
		else
			++effect_index;
	}
	// Create a menu with all interaction possibilities for an object.
	// Always create the side bar AFTER the main menu, so that the target can be copied.
	var main = CreateMainMenu(obj, slot);
	// To close the part_menu automatically when the main menu is closed. The sidebar will use this target, too.
	current_menus[slot].menu_object = main.Target;
	// Now, the sidebar.
	var sidebar = CreateSideBar(slot);

	var part_menu = this.style_def->CreatePartMenu(this, slot, current_menus[slot].menu_object, sidebar, main);

	if (this.minimized)
	{
		part_menu.Bottom = nil; // maximum height
	}

	// need to open a completely new menu?
	if (!current_main_menu_id)
	{
		if (!current_description_box.target)
		{
			current_description_box.target = CreateDummy();
			current_description_box.symbol_target = CreateDummy();
			current_description_box.desc_target = CreateDummy();
		}
		if (!current_center_column_target)
			current_center_column_target = CreateDummy();
		
		var root_menu = this.style_def->CreateRootMenu(this, current_center_column_target, current_description_box);
		root_menu._one_part = part_menu;

		// Special setup for a minimized menu.
		if (this.minimized)
		{
			root_menu.Top = "75%";
			root_menu.minimize_button.Tooltip = "$Maximize$";
			root_menu.minimize_button.GraphicsName = "Up";
			root_menu.center_column.Bottom = nil; // full size
			root_menu.description_box = nil;
		}
		
		current_main_menu_id = GuiOpen(root_menu);
	}
	else // menu already exists and only one part has to be added
	{
		GuiUpdate({_update: part_menu}, current_main_menu_id, nil, nil);
	}
	
	// Show "put/take all items" buttons if applicable. Also update tooltip.
	var show_grab_all = current_menus[0] && current_menus[1];
	// Both objects have to be containers.
	show_grab_all = show_grab_all 
					&& (current_menus[0].target->~IsContainer())
					&& (current_menus[1].target->~IsContainer());
	// And neither must disallow interaction.
	show_grab_all = show_grab_all
					&& !current_menus[0].target->~RejectInteractionMenu(cursor)
					&& !current_menus[1].target->~RejectInteractionMenu(cursor);
	if (show_grab_all)
	{
		current_center_column_target.Visibility = VIS_Owner;
		for (var i = 0; i < 2; ++i)
			GuiUpdate({Tooltip: Format("$MoveAllTo$", current_menus[i].target->GetName())}, current_main_menu_id, 10 + i, current_center_column_target);
	}
	else
	{
		current_center_column_target.Visibility = VIS_None;
	}
	
	// Now tell all user-provided effects for the new menu that the menu is ready.
	// Those effects can be used to update only very specific menu entries without triggering a full refresh.
	for (var menu in current_menus[slot].menus)
	{
		if (!menu.entries) continue;
		
		for (var entry in menu.entries)
		{
			if (!entry.fx) continue;
			EffectCall(nil, entry.fx, "OnMenuOpened", current_main_menu_id, entry.ID, menu.menu_object);
		}
	}
	
	// Finally disable object for selection in other sidebar, if available.
	if (other_menu)
	{
		GuiUpdate({Symbol = Icon_Cancel}, current_main_menu_id, 1 + 1 - slot, obj);
	}
	
	// And notify the object of the fresh menu.
	DoInteractionMenuOpenedCallback(obj);
}

private func DoInteractionMenuOpenedCallback(object obj)
{
	if (!obj) return;
	if (obj._open_interaction_menus == nil)
		obj._open_interaction_menus = 0;
	
	obj._open_interaction_menus += 1;
	
	var is_first = obj._open_interaction_menus == 1;
	obj->~OnShownInInteractionMenuStart(is_first);
}

private func DoInteractionMenuClosedCallback(object obj)
{
	if (!obj) return;
	obj._open_interaction_menus = Max(0, obj._open_interaction_menus - 1);
	
	var is_last = obj._open_interaction_menus == 0;
	obj->~OnShownInInteractionMenuStop(is_last);
}

// Toggles the menu state between minimized and maximized.
public func OnToggleMinimizeClicked()
{
	var config = nil;
	if (GetLength(InteractionMenu_Attributes) <= GetOwner())
	{
		config = {minimized = false};
		InteractionMenu_Attributes[GetOwner()] = config;
	}
	else
	{
		config = InteractionMenu_Attributes[GetOwner()];
	}
	config.minimized = !(GetProperty("minimized", config) ?? false);
	
	// Reopen with new layout..
	var last_cursor = this.cursor;
	RemoveObject();
	GUI_ObjectInteractionMenu->CreateFor(last_cursor);
}

// Tries to put all items from the other menu's target into the target of menu menu_id. Returns nil.
public func OnMoveAllToClicked(int menu_id)
{
	// Sanity checks..
	for (var i = 0; i < 2; ++i)
	{
		if (!current_menus[i] || !current_menus[i].target)
			return;
		if (!current_menus[i].target->~IsContainer())
			return;
	}
	// Take all from the other object and try to put into the target.
	var other = current_menus[1 - menu_id].target;
	var target = current_menus[menu_id].target;
	
	// Get all contents in a separate step in case the object's inventory changes during the transfer.
	// Also do not use FindObject(Find_Container(...)), because this way an object can simply overload Contents to return an own collection of items.
	var contents = [];
	var index = 0, obj;
	while (obj = other->Contents(index++)) PushBack(contents, obj);
	
	var transfered = TransferObjectsFromToSimple(contents, other, target);
	
	if (transfered > 0)
	{
		PlaySoundTransfer();
		return;
	}
	else
	{
		PlaySoundError();
		return;
	}
}

// generates a proplist that defines a custom GUI that represents a side bar where objects 
// to interact with can be selected
func CreateSideBar(int slot)
{
	var other_menu = current_menus[1 - slot];

	var sidebar = this.style_def->CreateSideBar(this, slot, current_menus[slot].menu_object);

	// Now show the current_objects list as entries.
	// If there is a forced-open menu, also add it to bottom of sidebar..
	var sidebar_items = nil;
	if (current_menus[slot].forced)
	{
		sidebar_items = current_objects[:];
		PushBack(sidebar_items, current_menus[slot].target);
	}
	else
		sidebar_items = current_objects;

	for (var obj in sidebar_items)
	{
		var entry = this.style_def->CreateSideBarItem(this, slot, obj, current_menus[slot].target == obj, other_menu && other_menu.target == obj, obj == cursor);

		GuiAddSubwindow(entry, sidebar);
	}
	return sidebar;
}

// Updates the sidebar with the current objects (and closes the old one).
func RefreshSidebar(int slot)
{
	if (!current_menus[slot]) return;
	// Close old sidebar? This call will just do nothing if there is no sidebar present.
	GuiClose(current_main_menu_id, 2, current_menus[slot].menu_object);
	
	var sidebar = CreateSideBar(slot);
	GuiUpdate({sidebar = sidebar}, current_main_menu_id, 1, current_menus[slot].menu_object);
}

func OnSidebarEntrySelected(data, int player, int ID, int subwindowID, object target)
{
	if (!data.obj) return;
	
	// can not open object twice!
	for (var menu in current_menus)
		if (menu.target == data.obj) return;
	OpenMenuForObject(data.obj, data.slot);
}

/*
	Generates and creates one side of the menu.
	Returns the proplist that will be put into the main menu on the left or right side.
*/
func CreateMainMenu(object obj, int slot)
{
	var big_menu = this.style_def->CreateMainMenu(this, slot, CreateDummy(), obj);

	var container = big_menu.container;

	// Do virtually nothing if the building/object is not ready to be interacted with. This can be caused by several things.
	var error_message = obj->~RejectInteractionMenu(cursor);

	if (error_message)
	{
		if (GetType(error_message) != C4V_String)
			error_message = "$NoInteractionsPossible$";
		container.Style = GUI_TextVCenter | GUI_TextHCenter;
		container.Text = error_message;
		current_menus[slot].menus = [];
		return big_menu;
	}

	var menus = obj->~GetInteractionMenus(cursor) ?? [];
	// get all interaction info from the object and put it into a menu
	// contents first
	if (obj->~IsContainer() && !obj->~RejectContentsMenu())
	{
		var info =
		{
			flag = InteractionMenu_Contents,
			title = "$Contents$",
			entries = [],
			entries_callback = nil,
			callback = "OnContentsSelection",
			callback_target = this,
			BackgroundColor = RGB(0, 50, 0),
			Priority = 10
		};
		PushBack(menus, info);
	}

	current_menus[slot].menus = menus;

	// now generate the actual menus from the information-list
	for (var i = 0; i < GetLength(menus); ++i)
	{
		var menu = menus[i];

		// Everything not flagged is a custom entry
		if (!menu.flag)
		{
			menu.flag = InteractionMenu_Custom;
		}
		// If a callback is given, try to get entries
		if (menu.entries_callback)
		{
			var call_from = menu.entries_callback_target ?? obj;
			menu.entries = call_from->Call(menu.entries_callback, cursor, menu.entries_callback_parameter);
		}
		// Empty entries should not be there
		if (menu.entries == nil)
		{
			FatalError(Format("An interaction menu did not return valid entries. %s -> %v() (object %v)", obj->GetName(), menu.entries_callback, obj));
			continue;
		}

		var all = this.style_def->CreateMainMenuItem(this, slot, obj, menu, cursor, i);
		if (!all)
			continue;

		GuiAddSubwindow(all, container);
	}

	// add refreshing effects for all of the contents menus
	for (var i = 0; i < GetLength(menus); ++i)
	{
		if (!(menus[i].flag & InteractionMenu_Contents))
			continue;
		AddEffect("IntRefreshContentsMenu", this, 1, 1, this, nil, obj, slot, i);
	}

	return big_menu;
}

func GetEntryInformation(proplist menu_info, int entry_index)
{
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
	return {menu=menu, entry=entry};
}

func OnMenuEntryHover(proplist menu_info, int entry_index, int player)
{
	var info = GetEntryInformation(menu_info, entry_index);
	if (!info.entry) return;
	if (!info.entry.symbol) return;
	// update symbol of description box
	GuiUpdate({Symbol = info.entry.symbol}, current_main_menu_id, 1, current_description_box.symbol_target);
	// and update description itself
	// clean up existing description window in case it has been cluttered by sub-windows
	GuiClose(current_main_menu_id, 1, current_description_box.desc_target);
	// now add new subwindow to replace the recently removed one
	GuiUpdate({new_subwindow = {Target = current_description_box.desc_target, ID = 1}}, current_main_menu_id, 1, current_description_box.target);
	// default to description of object
	if (!info.menu.callback_target || !info.menu.callback_hover)
	{
		var text = Format("%s:|%s", info.entry.symbol->GetName(), info.entry.symbol.Description);

		// For contents menus, we can sometimes present additional information about objects.
		if (info.menu.flag == InteractionMenu_Contents)
		{
			// Get the first valid object of the clicked stack.
			var obj = nil;
			if (info.entry.extra_data && info.entry.extra_data.objects)
			{
				for (var possible in info.entry.extra_data.objects)
				{
					if (possible == nil) continue;
					obj = possible;
					break;
				}
			}
			// ..and use that object to fetch some more information.
			if (obj)
			{
				var additional = nil;
				if (obj->Contents())
				{
					additional = "$Contains$ ";
					var i = 0, count = obj->ContentsCount();
					// This currently justs lists contents one after the other.
					// Items are not stacked, which should be enough for everything we have ingame right now. If this is filed into the bugtracker at some point, fix here.
					for (;i < count;++i)
					{
						if (i > 0)
							additional = Format("%s, ", additional);
						additional = Format("%s%s", additional, obj->Contents(i)->GetName());
					}
				}
				if (additional != nil)
					text = Format("%s||%s", text, additional);
			}
		}

		GuiUpdateText(text, current_main_menu_id, 1, current_description_box.desc_target);
	}
	else
	{
		info.menu.callback_target->Call(info.menu.callback_hover, info.entry.symbol, info.entry.extra_data, current_description_box.desc_target, current_main_menu_id);
	}
}

func OnMenuEntrySelected(proplist menu_info, int entry_index, int player)
{
	var info = GetEntryInformation(menu_info, entry_index);
	if (!info.entry) return;

	var callback_target;
	if (!(callback_target = info.menu.callback_target)) return;
	if (!info.menu.callback) return; // The menu can actually decide to handle user interaction itself and not provide a callback.
	var result = callback_target->Call(info.menu.callback, info.entry.symbol, info.entry.extra_data, cursor);

	// todo: trigger refresh for special value of result?
}

private func OnContentsSelection(symbol, extra_data)
{
	if (!extra_data || !current_menus[extra_data.slot]) return;
	var target = current_menus[extra_data.slot].target;
	if (!target) return;
	// no target to swap to?
	if (!current_menus[1 - extra_data.slot]) return;
	var other_target = current_menus[1 - extra_data.slot].target;
	if (!other_target) return;

	// Only if the object wants to be interacted with (hostility etc.)
	if (other_target->~RejectInteractionMenu(cursor)) return;
	
	// Allow transfer only into containers.
	if (!other_target->~IsContainer())
	{
		cursor->~PlaySoundDoubt(true, nil, cursor->GetOwner());
		return;
	}

	var transfer_only_one = GetPlayerControlState(GetOwner(), CON_ModifierMenu1) == 0; // Transfer ONE object of the stack?
	var to_transfer = nil;

	if (transfer_only_one)
	{
		for (var possible in extra_data.objects)
		{
			if (possible == nil) continue;
			to_transfer = [possible];
			break;
		}
	}
	else
	{
		to_transfer = extra_data.objects;
	}

	var successful_transfers = TransferObjectsFromTo(to_transfer, target, other_target);
	
	// Did we at least transfer one item?
	if (successful_transfers > 0)
	{
		PlaySoundTransfer();
		return true;
	}
	else
	{
		PlaySoundTransferIncomplete();
		return false;
	}
}

func TransferObjectsFromToSimple(array to_transfer, object source, object destination)
{
	// Now try transferring each item once.
	var successful_transfers = 0;
	for (var obj in to_transfer)
	{
		// Sanity, can actually happen if an item merges with others during the transfer etc.
		if (!obj || !destination) continue;
		
		var handled = destination->Collect(obj, true);
		if (handled)
			++successful_transfers;
	}
	return successful_transfers;
}

func TransferObjectsFromTo(array to_transfer, object source, object destination)
{
	var successful_transfers = 0;
	
	// Try to transfer all the previously selected items.
	for (var obj in to_transfer)
	{
		if (!obj) continue;
		// Our target might have disappeared (e.g. a construction site completing after the first item).
		if (!destination) break;
		
		// Does the object not want to leave the other container anyway?
		if (!obj->Contained() || !obj->~QueryRejectDeparture(source))
		{
			var handled = false;

			// If stackable, always try to grab a full stack.
			// Imagine armory with 200 arrows, but not 10 stacks with 20 each but 200 stacks with 1 each.
			// TODO: 200 stacks of 1 arrow would each merge into the stacks that are already in the target
			//       when they enter the target. For this reason that special case is, imo, not needed here.    
			if (obj->~IsStackable())
			{
				var others = FindObjects(Find_Container(source), Find_ID(obj->GetID()), Find_Exclude(obj));
				for (var other in others) 
				{
					if (obj->IsFullStack()) break;
					other->TryAddToStack(obj);
				}
			}
			
			// More special handling for Stackable items..
			handled = obj->~MergeWithStacksIn(destination);
			// Try to normally collect the object otherwise.
			if (!handled && destination && obj)
				handled = destination->Collect(obj, true);

			if (handled)
				successful_transfers += 1;
		}
	}

	return successful_transfers;
}

func FxIntRefreshContentsMenuStart(object target, proplist effect, temp, object obj, int slot, int menu_index)
{
	if (temp) return;
	effect.obj = obj; // the property (with this name) is externally accessed!
	effect.slot = slot;
	effect.menu_index = menu_index;
	effect.last_inventory = [];
}

func FxIntRefreshContentsMenuTimer(object target, effect, int time)
{
	// Remove effect if menu is gone or menu at slot and index is not a contents menu.
	if (!effect.obj)
		return FX_Execute_Kill;
	if (!current_menus[effect.slot] || !current_menus[effect.slot].menus[effect.menu_index])
		return FX_Execute_Kill;
	if (!(current_menus[effect.slot].menus[effect.menu_index].flag & InteractionMenu_Contents))
		return FX_Execute_Kill;
	// Helper object used to track extra-slot objects. When this object is removed, the tracking stops.
	var extra_slot_keep_alive = current_menus[effect.slot].menu_object;
	// The fast interval is only used for the very first check to ensure a fast startup.
	// It can't be just called instantly though, because the menu might not have been opened yet.
	if (effect.Interval == 1) effect.Interval = 5;
	var inventory = [];
	var obj, i = 0;
	while (obj = effect.obj->Contents(i++))
	{
		// Ignore objects that do not want to be shown as contents
		if (obj->~RejectInteractionMenuContentEntry(cursor, effect.obj))
		{
			continue;
		}

		var symbol = obj;
		var extra_data = {slot = effect.slot, menu_index = effect.menu_index, objects = []};

		// check if already exists (and then stack!)
		var found = false;
		// Never stack containers with (different) contents, though.
		var is_container = obj->~IsContainer();
		// For extra-slot objects, we should attach a tracking effect to update the UI on changes.
		if (obj->~HasExtraSlot())
		{
			var j = 0, e = nil;
			var found_tracker = false;
			while (e = GetEffect("ExtraSlotTracker", obj, j++))
			{
				if (e.keep_alive != extra_slot_keep_alive) continue;
				found_tracker = true;
				break;
			}
			if (!found_tracker)
			{
				var e = AddEffect("ExtraSlotTracker", obj, 1, 30 + Random(60), nil, GetID());
				e.keep_alive = extra_slot_keep_alive;
				e.callback_effect = effect;
				e.obj = effect.obj;
			}
		}
		// How many objects are this object?!
		var object_amount = obj->~GetStackCount() ?? 1;
		// Infinite stacks work differently - showing an arbitrary amount would not make sense.
		if (object_amount > 1 && obj->~IsInfiniteStackCount())
			object_amount = 1;
		// Empty containers can be stacked.
		for (var inv in inventory)
		{
			if (!inv.extra_data.objects[0]->CanBeStackedWith(obj)) continue;
			if (!obj->CanBeStackedWith(inv.extra_data.objects[0])) continue;
			inv.count += object_amount;
			inv.text = Format("%dx", inv.count);
			PushBack(inv.extra_data.objects, obj);
			
			// This object has a custom symbol (because it's a container)? Then the normal text would not be displayed.
			if (inv.custom != nil)
			{
				if (inv.custom.top == nil)
					inv.custom.top = {};
				inv.custom.top.Text = inv.text;
				inv.custom.top.Style = inv.custom.top.Style | GUI_TextRight | GUI_TextBottom;
			}
			
			found = true;
			break;
		}

		// Add new!
		if (!found)
		{
			PushBack(extra_data.objects, obj);

			// Use a default grid-menu entry as the base.
			var custom = MenuStyle_Grid->MakeEntryProplist(symbol, nil);
			custom.BackgroundColor = {Std = 0, OnHover = RGB(0, 100, 0)};
			if (is_container)
			{
				// Pack the custom entry into a larger frame to allow for another button below.
				// The priority offset makes sure that double-height items are at the front.
				custom = {Right = custom.Right, Bottom = "4em", top = custom, Priority = -10000 + obj->GetValue()};
				// Then add a little container-symbol (that can be clicked).
				custom.bottom =
				{
					Top = "2em",
					BackgroundColor = {Std = 0, Selected = RGB(0, 100, 0)},
					OnMouseIn = GuiAction_SetTag("Selected"),
					OnMouseOut = GuiAction_SetTag("Std"),
					OnClick = GuiAction_Call(this, "OnExtraSlotClicked", {slot = effect.slot, objects = extra_data.objects, ID = obj->GetID()}),
					container = 
					{
						Symbol = Icon_ExtraSlot,
						Priority = 1
					}
				};

				// And if the object has contents, show the first one, too.
				if (obj->ContentsCount() != 0)
				{
					var first_contents = obj->Contents(0);
					// Add to GUI.
					custom.bottom.contents = 
					{
						Symbol = first_contents ,
						Margin = "0.125em",
						Priority = 2
					};
					// Possibly add text for stackable items - this is an special exception for the Library_Stackable.
					var count = first_contents->~GetStackCount();
					// Infinite stacks display an own overlay.
					if ((count > 1) && (first_contents->~IsInfiniteStackCount())) count = nil;
					
					count = count ?? obj->ContentsCount(first_contents->GetID());
					if (count > 1)
					{
						custom.bottom.contents.Text = Format("%dx", count);
						custom.bottom.contents.Style = GUI_TextBottom | GUI_TextRight;
					}
					var overlay = first_contents->~GetInventoryIconOverlay();
					if (overlay)
						custom.bottom.contents.overlay = overlay;
					// Also make the chest smaller, so that the contents symbol is not obstructed.
					custom.bottom.container.Bottom = "1em";
					custom.bottom.container.Left = "1em";
				}
			}
			// Enable objects to provide a custom overlay for the icon slot.
			// This could e.g. be used by special scenarios or third-party mods.
			var overlay = obj->~GetInventoryIconOverlay();
			if (overlay != nil)
			{
				custom.Priority = obj->GetValue();
				custom.top = { _overlay = overlay };
			}

			// Add to menu!
			var text = nil;
			if (object_amount > 1)
				text = Format("%dx", object_amount);
			PushBack(inventory,
				{
					symbol = symbol,
					extra_data = extra_data,
					custom = custom,
					count = object_amount,
					text = text
				});
		}
	}

	// Add a contents counter on top.
	var contents_count_bar = 
	{
		// Too low a value (-5000 doesn't work) and the bar will be at the bottom when
		// an extra slot item is in the container and the HUD breaks
		Priority = -10000,
		Bottom = "1em",
		Style = GUI_NoCrop,
		text = 
		{
			Priority = 3,
			Style = GUI_TextRight | GUI_TextVCenter,
			Bottom = "1em"
		}
	};

	if (effect.obj.MaxContentsCount)
	{
		var count = effect.obj->ContentsCount();
		var max = effect.obj.MaxContentsCount;
		contents_count_bar.text.Text = Format("<c eeeeee>%3d / %3d</c>", count, max);
		contents_count_bar.Bottom = "0.5em";
		contents_count_bar.bar =
		{
			Priority = 2,
			BackgroundColor = RGB(0, 100, 0),
			Right = ToPercentString(1000 * count / max, 10),
			Bottom = "50%"
		};
		contents_count_bar.background =
		{
			Priority = 1,
			BackgroundColor = RGBa(0, 0, 0, 100),
			Bottom = "50%"
		};
	}
	else
	{
		contents_count_bar.text.Text = Format("<c eeeeee>%3d</c>", effect.obj->ContentsCount());
	}

	PushBack(inventory, {symbol = nil, text = nil, custom = contents_count_bar});

	// Check if nothing changed. If so, we don't need to update.
	if (GetLength(inventory) == GetLength(effect.last_inventory))
	{
		var same = true;
		for (var i = GetLength(inventory) - 1; i >= 0; --i)
		{
			if (inventory[i].symbol == effect.last_inventory[i].symbol
				&& inventory[i].text == effect.last_inventory[i].text) continue;
			same = false;
			break;
		}
		if (same)
			return FX_OK;
	}

	effect.last_inventory = inventory[:];
	DoMenuRefresh(effect.slot, effect.menu_index, inventory);
	return FX_OK;
}

func FxExtraSlotTrackerTimer(object target, proplist effect, int time)
{
	if (!effect.keep_alive)
		return -1;
}

// This is called by the extra-slot library.
func FxExtraSlotTrackerUpdate(object target, proplist effect)
{
	// Simply overwrite the inventory cache of the IntRefreshContentsMenu effect.
	// This will lead to the inventory being upated asap.
	if (effect.callback_effect)
		effect.callback_effect.last_inventory = [];
}

func OnExtraSlotClicked(proplist extra_data)
{
	var menu = current_menus[extra_data.slot];
	if (!menu || !menu.target) return;
	var obj = nil;
	for (var possible in extra_data.objects)
	{
		if (possible == nil) continue;
		if (possible->Contained() != menu.target && !menu.target->~IsObjectContained(possible)) continue;
		obj = possible;
		break;
	}
	if (!obj) return;	
	OpenMenuForObject(obj, extra_data.slot, true);
}

// This function is supposed to be called when the menu already exists (is open) and some sub-menu needs an update.
// Note that the parameter "new_entries" is optional. If not supplied, the /entries_callback/ for the specified menu will be used to fill the menu.
func DoMenuRefresh(int slot, int menu_index, array new_entries)
{
	// go through new_entries and look for differences to currently open menu
	// then try to only adjust the existing menu when possible
	// the assumption is that ususally only few entries change
	var menu = current_menus[slot].menus[menu_index];
	var current_entries = menu.entries;
	if (!new_entries && menu.entries_callback)
	{
		var call_from = menu.entries_callback_target ?? current_menus[slot].target;
		new_entries = call_from->Call(menu.entries_callback, this.cursor, menu.entries_callback_parameter);
	}
	
	// step 0.1: update all items where the symbol and extra_data did not change but other things (f.e. the text)
	// this is done to maintain a consistent order that would be shuffled constantly if the entry was removed and re-added at the end
	for (var c = 0; c < GetLength(current_entries); ++c)
	{
		var old_entry = current_entries[c];
		if (!old_entry) continue;
		
		var found = false;
		var symbol_equal_index = -1;
		for (var ni = 0; ni < GetLength(new_entries); ++ni)
		{
			var new_entry = new_entries[ni];
			if (!new_entry) continue;
			
			if (!EntriesEqual(new_entry, old_entry))
			{
				// Exception for the inventory menus.. extra_data includes all the found objects, but they are allowed to differ here.
				// So we check for equality excluding the objects.
				var extra1 = new_entry.extra_data;
				if (GetType(extra1) == C4V_PropList) extra1 = new extra1 {objects = nil};
				var extra2 = old_entry.extra_data;
				if (GetType(extra2) == C4V_PropList) extra2 = new extra2 {objects = nil};
				// We also allow the symbols to change as long as the actual ID stays intact.
				var symbol1 = new_entry.symbol;
				var symbol2 = old_entry.symbol;
				var symbols_equal = symbol1 == symbol2;
				if (!symbols_equal && symbol1 && symbol2 && GetType(symbol1) == C4V_C4Object && GetType(symbol2) == C4V_C4Object)
					symbols_equal = symbol1->~GetID() == symbol2->~GetID();

				if (symbols_equal && DeepEqual(extra1, extra2) && DeepEqual(new_entry.custom, old_entry.custom) && (new_entry.fx == old_entry.fx))
					symbol_equal_index = ni;
				continue;
			}
			found = true;
			break;
		}
		// if the entry exist just like that, we do not need to do anything
		// same, if we don't have anything to replace it with, anyway
		if (found || symbol_equal_index == -1) continue;
		// now we can just update the symbol with the new data
		var new_entry = new_entries[symbol_equal_index];
		menu.menu_object->UpdateItem(new_entry.symbol, new_entry.text, old_entry.unique_index, this, "OnMenuEntrySelected", { slot = slot, index = menu_index }, new_entry["custom"], current_main_menu_id);
		new_entry.unique_index = old_entry.unique_index;
		// make sure it's not manipulated later on
		current_entries[c] = nil;
	}
	// step 1: remove (close) all current entries that have been removed
	for (var c = 0; c < GetLength(current_entries); ++c)
	{
		var old_entry = current_entries[c];
		if (!old_entry) continue;
		
		// check for removal
		var removed = true;
		for (var new_entry in new_entries)
		{
			if (!EntriesEqual(new_entry, old_entry)) continue;
			removed = false;
			break;
		}
		if (removed)
		{
			if (old_entry.fx)
				RemoveEffect(nil, nil, old_entry.fx);
			menu.menu_object->RemoveItem(old_entry.unique_index, current_main_menu_id);
			current_entries[c] = nil;
		}
	}
	
	// step 2: add new entries
	for (var c = 0; c < GetLength(new_entries); ++c)
	{
		var new_entry = new_entries[c];
		// the entry was already updated before?
		if (new_entry.unique_index != nil) continue;
		
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
		var added_entry = menu.menu_object->AddItem(new_entry.symbol, new_entry.text, new_entry.unique_index, this, "OnMenuEntrySelected", { slot = slot, index = menu_index }, new_entry["custom"], current_main_menu_id);
		new_entry.ID = added_entry.ID;
		
		if (new_entry.fx)
		{
			EffectCall(nil, new_entry.fx, "OnMenuOpened", current_main_menu_id, new_entry.ID, menu.menu_object);
		}
	}
	menu.entries = new_entries;
}

func EntriesEqual(proplist entry_a, proplist entry_b)
{
	return entry_a.symbol == entry_b.symbol
	&& entry_a.text == entry_b.text
	&& DeepEqual(entry_a.extra_data, entry_b.extra_data)
	&& DeepEqual(entry_a.custom, entry_b.custom);
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
	if (dummy)
		dummy->RemoveObject();
}

// updates the interaction menu for an object iff it is currently shown
func UpdateInteractionMenuFor(object target, callbacks)
{
	for (var slot = 0; slot < GetLength(current_menus); ++slot)
	{
		var current_menu = current_menus[slot];
		if (!current_menu || current_menu.target != target) continue;
		if (!callbacks) // do a full refresh
			OpenMenuForObject(target, slot);
		else // otherwise selectively update the menus for the callbacks
		{
			for (var callback in callbacks)
			{
				for (var menu_index = 0; menu_index < GetLength(current_menu.menus); ++menu_index)
				{
					var menu = current_menu.menus[menu_index];
					if (menu.entries_callback != callback)
					{
						continue;
					}
					DoMenuRefresh(slot, menu_index);
				}
			}
		}
	}
}

/*
	Updates all interaction menus that are currently attached to an object.
	This function can be called at all times, not only when a menu is open, making it more convenient for users, because there is no need to track open menus.
	If the /callbacks/ parameter is supplied, only menus that use those callbacks are updated. That way, a producer can f.e. only update its "queue" menu.
*/
global func UpdateInteractionMenus(callbacks)
{
	if (!this) return;
	if (callbacks && GetType(callbacks) != C4V_Array) callbacks = [callbacks];
	for (var interaction_menu in FindObjects(Find_ID(GUI_ObjectInteractionMenu)))
		interaction_menu->UpdateInteractionMenuFor(this, callbacks);
}

// Sounds

func PlaySoundTransfer()
{
	Sound("Hits::SoftTouch*", true, nil, GetOwner());
}

func PlaySoundTransferIncomplete()
{
	Sound("Hits::Materials::Wood::DullWoodHit*", true, nil, GetOwner());
}

func PlaySoundError()
{
	Sound("Objects::Balloon::Pop", true, nil, GetOwner());
}

// Overloadable functions for customization

func SidebarIconStandard()
{
	return Icon_Menu_RectangleRounded;
}

func SidebarIconOnHover()
{
	return Icon_Menu_RectangleBrightRounded;
}

func SidebarIconSelected()
{
	return Icon_Menu_RectangleBrightRounded;
}
