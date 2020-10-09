/** 
	Tutorial Guide
	The tutorial guide can be clicked by the player, it supplies the player with information and hints.
	The following callbacks are made to the scenario script:
	 * OnGuideMessageShown(int plr, int index) when a message is shown
	 * OnGuideMessageRemoved(int plr, int index) when a message is removed, all events
	
	@author Maikel
*/


local messages; // A container to hold all messages.
local message_open; // Currently open message.
local close_on_last; // Close guide on after last message.

protected func Initialize()
{
	// Visibility
	this.Visibility = VIS_Owner;
	messages = [];
	message_open = nil;
	close_on_last = false;
	
	// Initialize menu properties.
	InitializeMenu();
	
	// create the main menu
	UpdateGuideMenu("");	
	return;
}

/* Adds a message to the guide. The internal index is set to this message meaning that this message will
* be shown if the player clicks the guide.
* @param msg Message that should be added to the message stack.
*/
public func AddGuideMessage(string msg)
{
	// Add message to list.
	messages[GetLength(messages)] = msg;
	// Update the menu, because of the next button.
	if (message_open != nil)
		ShowGuideMenu(message_open);
	return;
}

/* Shows a guide message to the player, also resets the internal index to that point.
*	@param show_index The message corresponding to this index will be shown, if nil the last message will be shown.
*/
public func ShowGuideMessage(int show_index)
{
	// If the index is not specified show last message, index must be between first and last message.
	if (show_index == nil)
		show_index = GetLength(messages) - 1;
	show_index = BoundBy(show_index, 0, GetLength(messages) - 1);
	// Show the guide message.
	ShowGuideMenu(show_index);
	return;
}

// Hides the guide and its menu to the player.
public func HideGuide()
{
	if (this.Visibility == VIS_Owner)
	{
		// Change visibility and do script callback.
		this.Visibility = VIS_None;
		GameCall("OnGuideMessageRemoved", GetOwner(), message_open);
	}
	return;
}

// Shows the guide and its menu to the player, if it was hidden before.
public func ShowGuide()
{
	if (this.Visibility == VIS_None)
	{
		// Change visibility and do script callback.
		this.Visibility = VIS_Owner;
		GameCall("OnGuideMessageShown", GetOwner(), message_open);
	}
	return;
}

// Returns whether the guide is currently hidden.
public func IsHidden()
{
	return this.Visibility == VIS_None;
}

public func EnableCloseOnLastMessage(bool disable)
{
	close_on_last = !disable;
	return;
}

public func GetMessageCount()
{
	return GetLength(messages);
}

protected func Destruction()
{
	CloseGuideMenu();
	return;
}


/*-- Menu implementation --*/

// Menu IDs.
local id_menu;

// Menu proplists.
local prop_menu;
local prop_next;
local prop_prev;

private func InitializeMenu()
{
	var menu_width = 25; // in percentage of whole screen
	var meny_offset = 4; // in em
	var menu_height = 6; // in em, should correspond to six lines of text
	var text_margin = 1; // margins in 1/10 em
	
	// Menu proplist.
	prop_menu =
	{
		Target = this,
		Style = GUI_Multiple,
		Decoration = GUI_MenuDeco,
		Left = Format("%d%%", 50 - menu_width),
		Right = Format("%d%%", 50 + menu_width),
		Top = Format("0%%+%dem", meny_offset),
		Bottom = Format("0%%+%dem", meny_offset + menu_height),
		BackgroundColor = {Std = 0},
	};

	// Submenu proplists.
	var prop_guide = 
	{
		Target = this,
		ID = 1,
		Right = Format("0%%+%dem", menu_height),
		Symbol = GetID(),
	};
	var prop_text = 
	{
		Left = Format("0%%%s", ToEmString(10 * menu_height + text_margin)),
		// 'Right' will be set on update
		Target = this,
		ID = 2,
		Text = nil,
	};	
	prop_next =
	{
		Target = this,
		ID = 3,
		Left = Format("100%%-%dem", menu_height / 2),
		Top = Format("100%%-%dem", menu_height / 2),
		Symbol = Icon_Arrow,
		GraphicsName = "Down",
		Text = "$MsgNext$",
		Style = GUI_TextHCenter | GUI_TextVCenter,
		BackgroundColor = {Std = 0, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(this, "ShowNextMessage"),
	};
	prop_prev =
	{
		Target = this,
		ID = 4,
		Left = Format("100%%-%dem", menu_height / 2),
		Bottom = Format("0%%+%dem", menu_height / 2),
		Symbol = Icon_Arrow,
		GraphicsName = "Up",
		Text = "$MsgPrevious$",
		Style = GUI_TextHCenter | GUI_TextVCenter,
		BackgroundColor = {Std = 0, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(this, "ShowPreviousMessage"),
	};
	
	// Add menu elements.
	prop_menu.guide = prop_guide;
	prop_menu.text = prop_text;
	prop_menu.next = prop_next;
	prop_menu.prev = prop_prev;
	
	// Menu ID.
	id_menu = GuiOpen(prop_menu);
	return;
}

private func ShowGuideMenu(int index)
{
	// There will always be the removal of the previous message.
	if (message_open != nil && !IsHidden())
		GameCall("OnGuideMessageRemoved", GetOwner(), message_open);

	// Show the new message.
	var message = messages[index];
	if (!message)
		return;
	var has_next = index < GetLength(messages) - 1;
	var has_prev = index > 0;
	var has_close = close_on_last && !has_next;
	UpdateGuideMenu(message, has_next, has_prev, has_close);
	message_open = index;

	// Notify the scenario script.
	if (message_open != nil && !IsHidden())	
		GameCall("OnGuideMessageShown", GetOwner(), message_open);
	return;
}

private func UpdateGuideMenu(string guide_message, bool has_next, bool has_prev, bool has_close)
{
	// Update the text message entry.
	prop_menu.text.Text = guide_message;

	// Don't usually leave a margin for the text - just when actually showing buttons.
	var is_showing_buttons = has_next || has_close || has_prev;
	
	var text_right_side = "100%";
	if (is_showing_buttons)
	{
		text_right_side = Format("100%-2.9em");
	}
	GuiUpdate({Right = text_right_side, Text = guide_message}, id_menu, prop_menu.text.ID, this);
	
	// Update the next/close button.
	if (has_next || has_close)
	{
		prop_menu.next = prop_next;
		if (has_next)
		{
			prop_menu.next.Symbol = Icon_Arrow;
			prop_menu.next.GraphicsName = "Down";
			prop_menu.next.Text = "$MsgNext$";
		}
		else if (has_close)
		{
			prop_menu.next.Symbol = Icon_Cancel;
			prop_menu.next.Text = "$MsgClose$";
		}
		GuiUpdate(prop_menu, id_menu, prop_menu.ID, this);
	}
	else if (prop_menu.next != nil)
	{
		GuiClose(id_menu, prop_menu.next.ID, this);
		prop_menu.next = nil;
	}
		
	// Update the previous button.	
	if (has_prev)
	{
		prop_menu.prev = prop_prev;
		GuiUpdate(prop_menu, id_menu, prop_menu.ID, this);
	}
	else if (prop_menu.prev != nil)
	{
		GuiClose(id_menu, prop_menu.prev.ID, this);
		prop_menu.prev = nil;
	}
	return;
}

private func CloseGuideMenu()
{
	// Gamecall on closing of the open message.
	if (message_open != nil)
		GameCall("OnGuideMessageRemoved", GetOwner(), message_open);
	message_open = nil;
	GuiClose(id_menu, nil, this);
	return;
}

// Menu callback: the player has clicked on next message.
private func ShowNextMessage()
{
	if (message_open >= GetLength(messages) - 1)
	{
		if (close_on_last)
			CloseGuideMenu();
		return;
	}
	ShowGuideMenu(message_open + 1);
	return;
}

// Menu callback: the player has clicked on previous message.
private func ShowPreviousMessage()
{
	if (message_open == 0)
		return;
	ShowGuideMenu(message_open - 1);
	return;
}


/*-- Properties --*/

local Name = "$Name$";
