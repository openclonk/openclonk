/**
	ControllerActionBar
	Controlls the action bar interaction and layout.

	@author Zapper
*/


static const GUI_MAX_ACTIONBAR = 10; // maximum amount of actionbar-slots

static const GUI_Controller_ActionBar_Delay = 20; // delay before the action bar is shown when pressing Interact
// all values given in 10 em (-> 10 = 1.0em)
static const GUI_Controller_ActionBar_IconSize = 40;
static const GUI_Controller_ActionBar_Margin = 10;

local interaction_check_active;
local actionbar_gui_target;
local actionbar_gui_id;

local actionbar_items;	// Array, action-buttons at the bottom

func GetActionBarGuiID()
{
	if (actionbar_gui_id) return actionbar_gui_id;
	FatalError("GUI_Controller_ActionBar::GetActionBarGuiID called when no menu was open");
	return nil;
}

func CloseActionBarGui()
{
	if (actionbar_gui_id)
		GuiClose(actionbar_gui_id);
	
	actionbar_gui_id = nil;
	actionbar_items = [];
}

// this function returns a dummy object that is used as the custom GUI target by the inventory menu
func GetActionBarGuiTarget()
{
	if (actionbar_gui_target)
		return actionbar_gui_target;
	actionbar_gui_target = CreateObject(Dummy, AbsX(0), AbsY(0), GetOwner());
	actionbar_gui_target.Visibility = VIS_Owner;
	return actionbar_gui_target;
}

func Construction()
{
	actionbar_items = [];
	interaction_check_active = false;
	return _inherited();
}

func Destruction()
{
	if (actionbar_gui_target)
		actionbar_gui_target->RemoveObject();
	return _inherited();
}


// executes the mouseclick onto an actionbutton through hotkeys
public func ControlHotkey(int hotindex)
{
	return OnActionBarButtonSelected(hotindex);
}


func StartInteractionCheck(object clonk)
{
	interaction_check_active = true;
	if (!GetEffect("IntSearchInteractionObjects", clonk))
		AddEffect("IntSearchInteractionObjects", clonk, 1, 10, this, nil);
}

func StopInteractionCheck(object clonk)
{
	clonk = clonk ?? GetCursor(GetOwner());
	if (!clonk) return false;
	var canceled = GetLength(actionbar_items) > 0;
	interaction_check_active = false;
	RemoveEffect("IntSearchInteractionObjects", clonk);
	CloseActionBarGui();
	return canceled;
}

// call from HUDAdapter (Clonk)
public func OnCrewSelection(object clonk, bool deselect)
{
	// selected
	if (!deselect)
	{
		// and start effect to monitor vehicles and structures...
		if (interaction_check_active)
			AddEffect("IntSearchInteractionObjects", clonk, 1, 10, this, nil);
	}
	else
	{
		var active = interaction_check_active;
		StopInteractionCheck(clonk);
		// stop the effect, yet start again when new clonk is selected!
		interaction_check_active = active;
	}
		
	return _inherited(clonk, deselect, ...);
}

public func FxIntSearchInteractionObjectsEffect(string newname, object target)
{
	if(newname == "IntSearchInteractionObjects")
		return -1;
}

public func FxIntSearchInteractionObjectsStart(object target, effect, int temp)
{
	if(temp != 0) return;
}

// takes care of displaying the interactions
public func FxIntSearchInteractionObjectsTimer(object target, effect, int time)
{
	if (time < GUI_Controller_ActionBar_Delay)
		return 1;
	if (!interaction_check_active)
		return -1;
		
	// only update if we see any changes
	var old_actionbar_items = actionbar_items;
	actionbar_items = target->GetInteractableObjects();
	PrepareAndSortNewActionBarItems(GetCursor(GetOwner()));
	
	// determine if update is necessary
	var do_update = GetLength(actionbar_items) != GetLength(old_actionbar_items);
	if (!do_update)
	{
		// check if every new object was also an old object
		for (var new_entry in actionbar_items)
		{
			var found = false;
			for (var old_entry in old_actionbar_items)
			{
				if (new_entry.interaction_object != old_entry.interaction_object
				||  new_entry.interaction_index != old_entry.interaction_index)
					continue;
				found = true;
				break;
			}
			if (!found)
			{
				do_update = true;
				break;
			}
		} 
	}
	
	if (do_update)
		UpdateActionBarDisplay();
	else
		actionbar_items = old_actionbar_items;
	return 1;
}

func IsActionInfoSelected(action_info, crew)
{
	return (action_info.actiontype == ACTIONTYPE_VEHICLE && crew->GetProcedure() == "PUSH" && crew->GetActionTarget() == action_info.interaction_object)
					|| (action_info.actiontype == ACTIONTYPE_STRUCTURE && crew->Contained() == action_info.interaction_object)
					|| (action_info.actiontype == ACTIONTYPE_INVENTORY);
}

func UpdateActionBarDisplay()
{
	var crew = GetCursor(GetOwner());
	// clean up old first
	// the action bar is always completely re-created when an update is detected
	var current_bar = actionbar_items;
	CloseActionBarGui();
	actionbar_items = current_bar;

	var menu =
	{
		Target = GetActionBarGuiTarget(),
		Style = 0,//GUI_GridLayout,
		Top = "50%", Bottom = Format("50%%%s", ToEmString(GUI_Controller_ActionBar_IconSize))
	};
	
	var button_number = 0;
	var all_buttons_count = GetLength(actionbar_items);
	for (var action_info in actionbar_items)
	{
		var pos_offset_x = - (GUI_Controller_ActionBar_Margin + GUI_Controller_ActionBar_IconSize) * all_buttons_count / 2 + (GUI_Controller_ActionBar_Margin / 2);
		var pos_x = pos_offset_x + (GUI_Controller_ActionBar_Margin + GUI_Controller_ActionBar_IconSize) * button_number;
		
		var hotkey = button_number + 1;
		var entry =
		{
			ID = hotkey,
			Priority = action_info.priority,
			Style = GUI_NoCrop | GUI_TextTop | GUI_TextRight,
			Left = Format("50%%%s", ToEmString(pos_x)),
			Right = Format("50%%%s", ToEmString(pos_x + GUI_Controller_ActionBar_IconSize)),
			Bottom = Format("0%%%s", ToEmString(GUI_Controller_ActionBar_IconSize)),
			Symbol = Icon_Menu_RectangleBrightRounded,
			Text = Format("%2d", hotkey),
			icon =
			{
				Priority = 1
			},
			info_icon =
			{
				Priority = 2,
				Left = "+0.5em", Right = "0%+2.2em",
				Top = "+0.5em", Bottom = "0%+2.2em"
			},
			OnClick = GuiAction_Call(this, "OnActionBarSelected", button_number),
			OnMouseIn = GuiAction_SetTag("OnHover"), 
			OnMouseOut = GuiAction_SetTag("Std"), 
		};
		
		var selected = IsActionInfoSelected(action_info, crew);
		
		var graphics_name = nil;
		var graphics_id = nil, tooltip = nil;
		
		if (action_info.actiontype == ACTIONTYPE_EXTRA && action_info.extra_data)
		{
			graphics_name = action_info.extra_data.IconName;
			graphics_id = action_info.extra_data.IconID;
			tooltip = action_info.extra_data.Description;
		}
		else if (action_info.actiontype == ACTIONTYPE_SCRIPT)
		{
			var metainfo = action_info.interaction_object->~GetInteractionMetaInfo(crew, action_info.interaction_index);
			if(metainfo)
			{
				graphics_name = metainfo.IconName;
				graphics_id = metainfo.IconID;
				tooltip = metainfo.Description;
				selected = metainfo.Selected;
			}
		}
		else if (action_info.actiontype == ACTIONTYPE_VEHICLE)
		{
			if (selected)
			{
				graphics_id = Icon_LetGo;
				tooltip = Format("$TxtUnGrab$",action_info.interaction_object->GetName());
			}
			else
			{
				if(!action_info.interaction_object->Contained())
				{
					graphics_id = Icon_Grab;
					tooltip = Format("$TxtGrab$",action_info.interaction_object->GetName());
				}
				else
				{
					graphics_id = Icon_Exit;
					tooltip = Format("$TxtPushOut$",action_info.interaction_object->GetName());
				}
			}
		}
		else if (action_info.actiontype == ACTIONTYPE_STRUCTURE)
		{
			if (selected)
			{
				graphics_id = Icon_Exit;
				tooltip = Format("$TxtExit$",action_info.interaction_object->GetName());
			}
			else
			{
				graphics_id = Icon_Enter;
				tooltip = Format("$TxtEnter$",action_info.interaction_object->GetName());
			}
		}
		
		entry.icon.Symbol = action_info.interaction_object;
		
		if (graphics_id)
		{
			entry.info_icon.Symbol = Icon_Menu_RectangleRounded;
			entry.info_icon.background = { Symbol = graphics_id };
		}
		
		GuiAddSubwindow(entry, menu);
		
		++button_number;
	}
	
	actionbar_gui_id = GuiOpen(menu);
}

// insert a button into the actionbar
// it will be sorted to the correct position via /priority/ before drawing
func CreateNewActionButton(int priority, object interaction_object, int actiontype, int interaction_index /* if an object has multiple interactions, this indicates which one is used */, proplist extra_data)
{
	// the actionbar has a maximum size
	if (GetLength(actionbar_items) > GUI_MAX_ACTIONBAR)
		return;
		
	var button_info = 
	{
		interaction_object = interaction_object,
		priority = priority,
		interaction_index = interaction_index,
		extra_data = extra_data,
		actiontype = actiontype
	};
	
	PushBack(actionbar_items, button_info);
}

func PrepareAndSortNewActionBarItems(object crew)
{
	RemoveHoles(actionbar_items);
	
	// sort correctly for priority
	var len = GetLength(actionbar_items);
	for (var c = 0; c < len; ++c)
	{
		var min = nil;
		for (var i = c; i < len; ++i)
		{
			if (min == nil || actionbar_items[min].priority > actionbar_items[i].priority)
				min = i;
		}
		var temp = actionbar_items[min];
		actionbar_items[min] = actionbar_items[c];
		actionbar_items[c] = temp;
	}
}

func OnActionBarSelected(int button_index, int player, int guiID, int subwindowID, target)
{
	OnActionBarButtonSelected(button_index);
}

func OnActionBarButtonSelected(int button_index)
{
	if (button_index < 0 || button_index >= GetLength(actionbar_items))
		return false;
	var action_info = actionbar_items[button_index];
	var crew = GetCursor(GetOwner());
	if (!crew) return false;
	crew->ExecuteInteraction(action_info);
	StopInteractionCheck(crew);
	crew.control.hotkeypressed = true; // for Library_ClonkControl
	return true;
}
