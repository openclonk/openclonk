/**
	ControllerActionBar

	Shows a small preview next to the crew portraits of what happens on CON_Interact.
	Also displays a menu ("action bar") when CON_Interact (space by default) is held with all possible interactions.

	@authors Zapper, Clonkonaut
*/

/*
	actionbar_items contains an array of proplists as returned by GetInteractableObjects(), see ClonkControl.ocd
*/

// HUD margin and size in tenths of em.
static const GUI_Controller_ActionBar_IconSize = 30;
static const GUI_Controller_ActionBar_MarginLeft = 6;
static const GUI_Controller_ActionBar_MarginTop = 6;
// !!! Does also use the following constants from ControllerCrewBar.ocd:
// GUI_Controller_CrewBar_IconSize
// GUI_Controller_CrewBar_IconMargin
// GUI_Controller_CrewBar_CursorSize
// GUI_Controller_CrewBar_CursorMargin

// !!! Does also use all the ACTIONTYPE_* constants from ClonkControl.ocd

static const GUI_MAX_ACTIONBAR = 10;
// delay before the action bar is shown when pressing Interact
static const GUI_Controller_ActionBar_Delay = 20;

local current_interaction;

local actionbar_gui_menu;
local actionbar_gui_id;

local actionbar_info_menu;
local actionbar_info_id;

local actionbar_items;

/* Creation / Destruction */

private func Construction()
{
	actionbar_items = [];

	// Calculate margin + width of crew portraits
	var icon_margin = GUI_Controller_CrewBar_CursorMargin +
	                  GUI_Controller_CrewBar_CursorSize +
	                  GUI_Controller_CrewBar_IconMargin +
	                  GUI_Controller_CrewBar_IconSize +
	                  GUI_Controller_ActionBar_MarginLeft;
	var icon_size = icon_margin + GUI_Controller_ActionBar_IconSize;

	actionbar_gui_menu =
	{
		Target = this,
		Player = NO_OWNER, // will be shown only if there is an interaction possible, e.g. standing in front of a vehicle
		Style = GUI_Multiple | GUI_NoCrop | GUI_IgnoreMouse,
		Left = ToEmString(icon_margin),
		Right = ToEmString(icon_size),
		Top = ToEmString(GUI_Controller_ActionBar_MarginTop),
		Bottom = ToEmString(GUI_Controller_ActionBar_MarginTop + GUI_Controller_ActionBar_IconSize),
		Symbol = Icon_Menu_RectangleRounded,
		Priority = 1,
		key =// the interaction key
		{
			Target = this,
			Style = GUI_NoCrop | GUI_TextHCenter | GUI_TextBottom,
			Left = "-50%",
			Right = "150%",
			Top = ToEmString(-6),
			Bottom = ToEmString(4),
			Text = Format("<c dddd00>[%s]</c>", GetPlayerControlAssignment(GetOwner(), CON_Interact, true)),
			Priority = 2
		},
		symbol =// the object to interact with
		{
			Target = this,
			Symbol = nil,
			Priority = 3
		},
		icon =// small icon visualising the action
		{
			Target = this,
			Style = GUI_NoCrop,
			Left = "50%",
			Top = "50%",
			Symbol = nil,
			Priority = 4
		},
		text =// interaction description
		{
			Target = this,
			Style = GUI_NoCrop | GUI_TextHCenter,
			Left = "-50%",
			Right = "150%",
			Top = ToEmString(GUI_Controller_ActionBar_IconSize - 5),
			Bottom = ToEmString(GUI_Controller_ActionBar_MarginTop + GUI_Controller_ActionBar_IconSize + 15),
			Text = "",
			Priority = 5
		},
		plus =// shown if there are multiple interactions
		{
			Target = this,
			Player = NO_OWNER,
			ID = 1,
			Style = GUI_NoCrop,
			Left = ToEmString(GUI_Controller_ActionBar_IconSize / 2),
			Right = ToEmString(GUI_Controller_ActionBar_IconSize * 3 / 2),
			Bottom = ToEmString(GUI_Controller_ActionBar_IconSize / 2),
			Symbol = Icon_Number,
			GraphicsName = "PlusGreen",
			Priority = 6
		}
	};
	actionbar_gui_id = GuiOpen(actionbar_gui_menu);

	AddTimer("ShowInteraction", 10);

	actionbar_info_menu =
	{
		Target = this,
		Style = GUI_Multiple,
		Top = "50%", Bottom = Format("50%%%s", ToEmString(GUI_Controller_ActionBar_IconSize))
	};

	return _inherited(...);
}

private func Destruction()
{
	GuiClose(actionbar_gui_id);
	RemoveTimer("ShowInteraction");
	current_interaction = nil;

	CloseActionBar();

	_inherited(...);
}

/* Callbacks */

private func StartInteractionCheck(object clonk)
{
	ScheduleCall(this, "OpenActionBar", GUI_Controller_ActionBar_Delay);
}

private func StopInteractionCheck(object clonk)
{
	return CloseActionBar();
}

private func ControlHotkey(int hotkey)
{
	// If action bar is opened, try to execute an interaction associated with the hotkey
	if (actionbar_info_id)
		OnActionBarSelected(hotkey);
}

/* Timer */

// Runs every 10 frames to look for possible interactable objects and shows the interaction icon if applicable
private func ShowInteraction()
{
	var cursor = GetCursor(GetOwner());
	if (!cursor || !cursor->GetCrewEnabled()) return HideInteraction();

	var interactables = cursor->~GetInteractableObjects();
	if (!interactables || !GetLength(interactables)) return HideInteraction();

	// Get the interaction with the highest priority
	var high_prio = nil;
	for (var interactable in interactables)
		if (high_prio == nil || (interactable.priority < high_prio.priority)) high_prio = interactable;

	// If the full action bar is displayed, update
	if (actionbar_info_id) UpdateActionBar(interactables, cursor);

	// If there are multiple interactions, show plus sign
	if (GetLength(interactables) > 1)
	{
		if (actionbar_gui_menu.plus.Player != GetOwner())
		{
			actionbar_gui_menu.plus.Player = GetOwner();
			GuiUpdate({ Player = GetOwner() }, actionbar_gui_id, 1, this);
		}
	}
	else
	{
		if (actionbar_gui_menu.plus.Player == GetOwner())
		{
			actionbar_gui_menu.plus.Player = NO_OWNER;
			GuiUpdate({ Player = NO_OWNER }, actionbar_gui_id, 1, this);
		}
	}

	// Already showing?
	if (DeepEqual(high_prio, current_interaction)) return;

	// Display that information

	current_interaction =high_prio;
	actionbar_gui_menu.Player = GetOwner();
	var help = GetInteractionHelp(current_interaction.actiontype, current_interaction.interaction_object, current_interaction.interaction_index, cursor, current_interaction);

	var update = {
		Player = GetOwner(),
		symbol =
		{
			Symbol = current_interaction.interaction_object
		},
		icon =
		{
			Symbol = help.help_icon,
			GraphicsName = help.help_icon_graphics
		},
		text =
		{
			Text = help.help_text
		}
	};

	GuiUpdate(update, actionbar_gui_id);
}

// Does hide the interaction icon
private func HideInteraction()
{
	// Already hidden, do nothing
	if (actionbar_gui_menu.Player == NO_OWNER) return;

	actionbar_gui_menu.Player = NO_OWNER;
	current_interaction = nil;
	GuiUpdate({ Player = NO_OWNER }, actionbar_gui_id);
}

/* Display */

// Opens the full interaction bar if CON_Interact was held down
private func OpenActionBar()
{
	var cursor = GetCursor(GetOwner());
	if (!cursor || !cursor->GetCrewEnabled()) return;

	actionbar_info_id = GuiOpen(actionbar_info_menu);
	// Immediately update the entries
	ShowInteraction();
}

private func CloseActionBar()
{
	ClearScheduleCall(this, "OpenActionBar");

	if (!actionbar_info_id) return false;

	GuiClose(actionbar_info_id);
	actionbar_info_id = nil;
	actionbar_items = [];
	return true;
}

private func UpdateActionBar(array interactables, object cursor)
{
	// Check whether an update is necessary
	var do_update = false;

	// Do update if the amount of interactions has changed
	if (GetLength(interactables) != GetLength(actionbar_items))
		do_update = true;
	// Do a more thourough check, comparing everything in detail
	if (!do_update)
	{
		// Check if every new object was also an old object
		for (var new_entry in interactables)
		{
			var found = false;
			for (var old_entry in actionbar_items)
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
	if (!do_update) return;

	// Clean up and sort actionbar_items
	actionbar_items = interactables;
	PrepareAndSortNewActionBarItems();

	// Update all buttons up to the maximum of possible buttons
	var button_count = Min(GetLength(actionbar_items), GUI_MAX_ACTIONBAR);
	var x_offset = -(GUI_Controller_ActionBar_MarginLeft + GUI_Controller_ActionBar_IconSize) * button_count / 2 + (GUI_Controller_ActionBar_MarginLeft / 2);

	for (var i = 0; i < GUI_MAX_ACTIONBAR; i++)
	{
		if (!actionbar_items[i])
		{
			actionbar_info_menu[Format("button%d", i)] =
			{
				Player = NO_OWNER
			};
		}
		else
		{
			var left = x_offset + (GUI_Controller_ActionBar_MarginLeft + GUI_Controller_ActionBar_IconSize) * i;
			var hotkey = i + 1;
			var help = GetInteractionHelp(actionbar_items[i].actiontype, actionbar_items[i].interaction_object, actionbar_items[i].interaction_index, cursor, actionbar_items[i]);

			actionbar_info_menu[Format("button%d", i)] =
			{
				Target = this,
				Player = GetOwner(),
				ID = hotkey,
				Style = GUI_NoCrop | GUI_TextTop | GUI_TextRight,
				Left = Format("50%%%s", ToEmString(left)),
				Right = Format("50%%%s", ToEmString(left + GUI_Controller_ActionBar_IconSize)),
				Bottom = Format("0%%%s", ToEmString(GUI_Controller_ActionBar_IconSize)),
				Symbol = Icon_Menu_RectangleBrightRounded,
				Priority = 1,
				Text = Format("%2d", hotkey),
				Tooltip = help.help_text,
				OnClick = GuiAction_Call(this, "OnActionBarSelected", i),
				symbol =
				{
					Target = this,
					Symbol = actionbar_items[i].interaction_object,
					Priority = 3
				},
				icon =
				{
					Target = this,
					Style = GUI_NoCrop,
					Left = "50%",
					Top = "50%",
					Symbol = help.help_icon,
					GraphicsName = help.help_icon_graphics,
					Priority = 4
				},
				text =
				{
					Target = this,
					Style = GUI_NoCrop | GUI_TextHCenter,
					Left = "-50%",
					Right = "150%",
					Top = ToEmString(GUI_Controller_ActionBar_IconSize - 5),
					Bottom = ToEmString(GUI_Controller_ActionBar_MarginTop + GUI_Controller_ActionBar_IconSize + 15),
					Text = "",
					Priority = 5
				}
			};
		}
	}

	GuiUpdate(actionbar_info_menu, actionbar_info_id);
}

private func PrepareAndSortNewActionBarItems()
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

// When a button from the interaction bar was clicked or a hotkey was pressed
private func OnActionBarSelected(int button_index)
{
	if (button_index < 0 || button_index >= GetLength(actionbar_items))
		return false;
	var action_info = actionbar_items[button_index];
	var crew = GetCursor(GetOwner());
	if (!crew || !crew->GetCrewEnabled()) return false;
	crew->ExecuteInteraction(action_info);
	CloseActionBar();
	crew.control.hotkeypressed = true; // for Library_ClonkControl
	return true;
}

/* Returns a proplist with the following properties to display:
help_text: A text describing the interaction or ""
help_icon: A pictographic icon definition symbolising the interaction or nil
help_icon_graphics: The graphics of the icon definition to use or ""
*/
private func GetInteractionHelp(int actiontype, object to_interact, int interaction_index, object clonk, proplist interaction)
{
	var ret =
	{
		help_text = "",
		help_icon = nil,
		help_icon_graphics = ""
	};

	// Help text: Grabbing / Ungrabbing / Pushing out
	if (actiontype == ACTIONTYPE_VEHICLE)
	{
		if (clonk->Contained() && to_interact->Contained() == clonk->Contained())
		{
			ret.help_text = Format("$TxtPushOut$", to_interact->GetName());
			ret.help_icon = Icon_Exit;
		}
		else if (clonk->GetProcedure() == "PUSH" && clonk->GetActionTarget() == to_interact)
		{
			ret.help_text = Format("$TxtUnGrab$", to_interact->GetName());
			ret.help_icon = Icon_LetGo;
		}
		else
		{
			ret.help_text = Format("$TxtGrab$", to_interact->GetName());
			ret.help_icon = Icon_Grab;
		}
	}

	// Help text: Enter / Exit
	if (actiontype == ACTIONTYPE_STRUCTURE)
	{
		if (clonk->Contained() && clonk->Contained() == to_interact)
		{
			ret.help_text = Format("$TxtExit$", to_interact->GetName());
			ret.help_icon = Icon_Exit;
		}
		else
		{
			ret.help_text = Format("$TxtEnter$", to_interact->GetName());
			ret.help_icon = Icon_Enter;
		}
	}

	// Help text: Script Interaction
	if (actiontype == ACTIONTYPE_SCRIPT)
	{
		var metainfo = to_interact->~GetInteractionMetaInfo(clonk, interaction_index);
		if (metainfo)
		{
			ret.help_text = metainfo.Description;
			ret.help_icon = metainfo.IconID;
			ret.help_icon_graphics = metainfo.IconName;
		}
	}

	// Help text: Extra Interaction (already in proplist)
	if (actiontype == ACTIONTYPE_EXTRA)
	{
		ret.help_text = interaction.extra_data.Description;
		ret.help_icon = interaction.extra_data.IconID;
		ret.help_icon_graphics = interaction.extra_data.IconName;
	}

	return ret;
}