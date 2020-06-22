/**
	ControllerActionBar

	Shows a small preview next to the crew portraits of what happens on CON_Interact.

	@authors Zapper, Clonkonaut
*/

// HUD margin and size in tenths of em.
// !!! Does also use the following constants from ControllerCrewBar.ocd:
// GUI_Controller_CrewBar_IconSize
// GUI_Controller_CrewBar_IconMargin
// GUI_Controller_CrewBar_CursorSize
// GUI_Controller_CrewBar_CursorMargin

// !!! Does also use all the ACTIONTYPE_* constants from ClonkControl.ocd

local current_interaction;

local actionbar_gui_menu;
local actionbar_gui_id;

/* GUI creation */

// For custom HUD graphics overload the following function as deemed fit.

func AssembleActionBar()
{
	// Calculate margin + width of crew portraits
	var icon_margin = GUI_Controller_CrewBar_CursorMargin +
	                  GUI_Controller_CrewBar_CursorSize +
	                  GUI_Controller_CrewBar_IconMargin +
	                  GUI_Controller_CrewBar_IconSize +
	                  GUI_Controller_CrewBar_IconMargin;
	var icon_size = icon_margin + GUI_Controller_CrewBar_IconSize;

	return
	{
		Target = this,
		Player = NO_OWNER, // will be shown only if there is an interaction possible, e.g. standing in front of a vehicle
		Style = GUI_Multiple | GUI_NoCrop | GUI_IgnoreMouse,
		Left = ToEmString(icon_margin),
		Right = ToEmString(icon_size),
		Top = ToEmString(GUI_Controller_CrewBar_IconMargin),
		Bottom = ToEmString(GUI_Controller_CrewBar_IconMargin + GUI_Controller_CrewBar_IconSize),
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
			Top = ToEmString(GUI_Controller_CrewBar_IconSize - 5),
			Bottom = ToEmString(GUI_Controller_CrewBar_IconMargin + GUI_Controller_CrewBar_IconSize + 15),
			Text = "",
			Priority = 5
		},
		plus =// shown if there are multiple interactions
		{
			Target = this,
			Player = NO_OWNER,
			ID = 1,
			Style = GUI_NoCrop,
			Left = ToEmString(GUI_Controller_CrewBar_IconSize / 2),
			Right = ToEmString(GUI_Controller_CrewBar_IconSize * 3 / 2),
			Bottom = ToEmString(GUI_Controller_CrewBar_IconSize / 2),
			Symbol = Icon_Number,
			GraphicsName = "PlusGreen",
			Priority = 6
		}
	};
}

/* Creation / Destruction */

private func Construction()
{

	actionbar_gui_menu = AssembleActionBar();

	actionbar_gui_id = GuiOpen(actionbar_gui_menu);

	EnableInteractionUpdating(true);
	return _inherited(...);
}

private func Destruction()
{
	GuiClose(actionbar_gui_id);
	EnableInteractionUpdating(false);
	current_interaction = nil;
	_inherited(...);
}

public func SetCurrentInteraction(proplist interaction)
{
	// Already showing?
	if (DeepEqual(interaction, current_interaction)) return;
	
	if (interaction == nil)
	{
		HideInteraction();
		return;
	}
	
	// Display that information
	current_interaction = interaction;
	actionbar_gui_menu.Player = GetOwner();
	// We can assert that we have a cursor here.
	var cursor = GetCursor(GetOwner());
	var help = GetInteractionHelp(current_interaction, cursor);
	
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
	
	return help;
}

public func GetCurrentInteraction()
{
	return current_interaction;
}

/* Timer */

public func EnableInteractionUpdating(bool enable)
{
	var fx = GetEffect("IntUpdateInteraction", this);
	if (fx && !enable)
		RemoveEffect(nil, nil, fx);
	else if (!fx && enable)
		AddEffect("IntUpdateInteraction", this, 1, 10, this);
}

/*
	Searched for interactable objects and updates the display.
*/
public func UpdateInteractionObject()
{
	var cursor = GetCursor(GetOwner());
	if (!cursor || !cursor->GetCrewEnabled())
	{
		HideInteraction();
		return;
	}

	var interactables = cursor->~GetInteractableObjects();
	if (!interactables || !GetLength(interactables))
	{
		HideInteraction();
		return;
	}

	// Get the interaction with the highest priority
	var high_prio = nil;
	for (var interactable in interactables)
		if (high_prio == nil || (interactable.priority < high_prio.priority)) high_prio = interactable;

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
	
	SetCurrentInteraction(high_prio);
}

// Runs every 10 frames to look for possible interactable objects and shows the interaction icon if applicable
private func FxIntUpdateInteractionTimer()
{
	UpdateInteractionObject();
	return FX_OK;
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

/* Returns a proplist with the following properties to display:
help_text: A text describing the interaction or ""
help_icon: A pictographic icon definition symbolising the interaction or nil
help_icon_graphics: The graphics of the icon definition to use or ""
*/
private func GetInteractionHelp(proplist interaction, object clonk)
{
	var actiontype = interaction.actiontype;
	var to_interact = interaction.interaction_object;
	var interaction_index = interaction.interaction_index;
	
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