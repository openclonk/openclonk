/**
	This defines a menu style for the interaction menu.

	The style definition is given to the interaction menu object upon creation when
	CreateFor()
	is called. See Library_ClonkControl, ObjectControl() if you want to provide a
	different, custom style.

	The object interaction menu will need the following functions in the style definition
	to work properly:

	Init(object menu)
		menu: The interaction menu object (GUI_ObjectInteractionMenu)
	Does not expect any return value but can be used to manipulate the menu object in any
	way desired before the GUI is actually opened. In here, it is used to apply the
	settings.

	CreateRootMenu(object menu, object center_column_menu, proplist description_box)
		menu: The interaction menu object (GUI_ObjectInteractionMenu)
		center_column_menu: A dummy object, target for the center column
		description_box: A proplist, containing initial information for the description box
			Properties are:
			.target: A dummy object, target for the surrounding box
			.symbol_target: A dummy object, target for the symbol box
			.desc_target: A dummy object, target for the description box
	The function is called when the menu is first opened, but also during it being open
	but whenever a total reset of the menu is required.
	The return value has to be a proplist containing the overarching structure of the
	interaction menu. As is obvious from the parameters, the menu is expected to have two
	submenus: one called .center_column and one called .description_box (again with two submenus:
	.symbol_part and .desc_part). Though the actual placement of the elements is arbitrary (i.e.
	center_column does not have to be centered). The center column holds the 'move all' buttons.

	CreateMainMenu(object menu, int slot, object target, object obj_for)
		menu: The interaction menu object (GUI_ObjectInteractionMenu)
		slot: Either 0 or 1. 0: the left-hand menu, 1: the right-hand menu
		target: A dummy object, target for the main menu
		obj_for: The object this menu is opened for (which infos are displayed)
	This is to assemble the menu for a currently selected object. There are always two 'main' menus
	open, one at the left, one at the right. The displayed object is changed with the sidebar. The
	menu must not necessarily contain visual elements (but either the main menu, the part menu or the
	root menu should). The menu will be filled with entries generated from CreateMainMenuItem(). The
	assembled proplist must be returned.

	CreateMainMenuItem(object menu, int slot, object target, proplist menu_entry, object cursor, int i)
		menu: The interaction menu object (GUI_ObjectInteractionMenu)
		slot: Either 0 or 1. 0: the left-hand menu, 1: the right-hand menu
		target: A dummy object, target for the main menu
		menu_entry: A proplist with various attributes, as defined in the object interaction menu
		cursor: The player's current cursor (clonk)
		i: An ascending number
	The function should assemble and return a unified subelement for the main menu. The content is often
	up to the specific object whose entries are displayed. Some properties of menu_entry are irrelevant
	for the visuals, some are not (see Object Interaction Menu for a full list of possible entries).
	Subentries (like inventory objects) can be included in menu_entry.entries or added later or change
	dynamically (like inventory!).

	CreateSideBar(object menu, int slot, object target)
		menu: The interaction menu object (GUI_ObjectInteractionMenu)
		slot: Either 0 or 1. 0: the left-hand menu, 1: the right-hand menu
		target: A dummy object, target for the menu
	Assemble a sidebar element which is shown next to the main item and contains a list of all other
	object that could be displayed in the main menu (because they are available to the clonk). The
	menu will be filled with entries generated from CreateSideBarItem(). The assembled proplist
	must be returned.

	CreateSideBarItem(object menu, int slot, object target, bool selected, bool crossed_out, bool cursor)
		menu: The interaction menu object (GUI_ObjectInteractionMenu)
		slot: Either 0 or 1. 0: the left-hand menu, 1: the right-hand menu
		target: Object shown in this entry
		selected: True if the object is currently selected for the main menu
		crossed_out: True if the object should be unavailable for selection (usually because it is shown
		             in the other main menu)
		cursor: The player's current cursor (clonk)
	The function should assemble and return a unified subelement for the sidebar. The visuals can be
	freely defined. Tabs are a good choice but not the only one.

	CreatePartMenu(object menu, int slot, object target, proplist sidebar, proplist main)
		menu: The interaction menu object (GUI_ObjectInteractionMenu)
		slot: Either 0 or 1. 0: the left-hand menu, 1: the right-hand menu
		target: The object displayed in the 'main' menu
		sidebar: The complete proplist for the sidebar
		main: The complete proplist for the main menu
	The function is mainly expected to assemble to sidebar and the main object into one
	larger menu that does not need to have any visual elements. The finished proplist must
	be returned.


	The object interaction menu offers this callback in return:

	ReopenWithSetting(string setting, any value)
		Closes the menu and reopens it wit a specific property (setting) set to a specific
		value. The property is even available in the Init() function.
*/

// Coordinates in em

local SideBarSize = 40;
local CenterColumnWidth = 50;

local ExtraData_Settings_Transparency = "InteractionMenu_NewStyle_Transp";
local ExtraData_Settings_HideText = "InteractionMenu_NewStyle_THidden";
local ExtraData_Settings_Saturation = "InteractionMenu_NewStyle_Saturat";

/* Callbacks from the menu that should be handled by the style */

public func ToggleSettings(object menu) // Open/Close additional settings
{
	var value = true;
	if (menu.SettingsOpen != nil)
		value = !menu.SettingsOpen;

	menu->ReopenWithSetting("SettingsOpen", value);
}

public func ToggleBackground(object menu) // Change transparency of background
{
	var owner = menu->GetOwner();
	if (owner == NO_OWNER)
		return; // NO_OWNER opened a menu?

	// Settings are:
	// 0 = Semi-transparent background, opaque entries (Default)
	// 1 = Very transparent background, semi-transparent entries (maximum transparency)
	// 2 = Non-transparent backgrounds (minimum transparency)
	var value = 1;
	if (menu.BackgroundTransparency != nil)
	{
		value = menu.BackgroundTransparency + 1;
		if (value > 2)
			value = 0;
	}

	SetPlrExtraData(owner, ExtraData_Settings_Transparency, value);
	menu->ReopenWithSetting("BackgroundTransparency", value);
}

public func ToggleText(object menu) // Hide text captions
{
	var owner = menu->GetOwner();
	if (owner == NO_OWNER)
		return; // NO_OWNER opened a menu?

	// Settings are:
	// 0 = Captions are there (Default)
	// 1 = Captions are hidden
	var value = 1;
	if (menu.TextHidden != nil)
	{
		value = menu.TextHidden + 1;
		if (value > 1)
			value = 0;
	}

	SetPlrExtraData(owner, ExtraData_Settings_HideText, value);
	menu->ReopenWithSetting("TextHidden", value);
}

public func ToggleSaturation(object menu) // Desaturate the menu
{
	var owner = menu->GetOwner();
	if (owner == NO_OWNER)
		return; // NO_OWNER opened a menu?

	// Settings are:
	// 0 = With color (Default)
	// 1 = Without color
	var value = 1;
	if (menu.Saturation != nil)
	{
		value = menu.Saturation + 1;
		if (value > 1)
			value = 0;
	}

	SetPlrExtraData(owner, ExtraData_Settings_Saturation, value);
	menu->ReopenWithSetting("Saturation", value);
}

/* Everything called by Object Interation Menu */

// Initialization of the menu
public func Init(object menu)
{
	var owner = menu->GetOwner();
	if (owner == NO_OWNER)
		return; // NO_OWNER opened a menu?

	// Set background transparency setting
	if (menu.BackgroundTransparency == nil)
	{
		if (GetPlrExtraData(owner, ExtraData_Settings_Transparency) != nil)
			menu.BackgroundTransparency = GetPlrExtraData(owner, ExtraData_Settings_Transparency);
		else
			menu.BackgroundTransparency = 0;
	}

	// Set caption display setting
	if (menu.TextHidden == nil)
	{
		if (GetPlrExtraData(owner, ExtraData_Settings_HideText) != nil)
			menu.TextHidden = GetPlrExtraData(owner, ExtraData_Settings_HideText);
		else
			menu.TextHidden = 0;
	}

	// Set saturation setting
	if (menu.Saturation == nil)
	{
		if (GetPlrExtraData(owner, ExtraData_Settings_Saturation) != nil)
			menu.Saturation = GetPlrExtraData(owner, ExtraData_Settings_Saturation);
		else
			menu.Saturation = 0;
	}
}

// Root menu is the overall surrounding structure of the menu
// Parameters are: Interaction Menu Object, Dummy Object for center column, Dummy Object for description box
public func CreateRootMenu(object menu, object center_column_menu, proplist description_box)
{
	var owner = menu->GetOwner();
	if (owner == NO_OWNER)
		return; // NO_OWNER opened a menu?

	// Assemble button row (Close, Settings, Background change, Hide text)
	var buttons_height = 40;
	if (menu.SettingsOpen)
		buttons_height = 100;

	var close_button =
	{
		Tooltip = "$TooltipClose$",
		Priority = 0x0fffff,
		Left = "0%", Top = "0%+0em",
		Right = "100%", Bottom = "0%+2em",
		Symbol = Icon_Cancel,
		BackgroundColor = {Std = nil, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(menu, "Close")
	};

	var settings_button =
	{
		Tooltip = "$TooltipSettings$",
		Priority = 0x0ffffe,
		Left = "0%", Top = "0%+2em",
		Right = "100%", Bottom = "0%+4em",
		Symbol = Icon_Settings,
		BackgroundColor = {Std = nil, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(this, "ToggleSettings", menu)
	};
	if (menu.SettingsOpen)
		settings_button.Tooltip = "$TooltipSettingsClose$";

	var background_change =
	{
		Tooltip = "$TooltipBackground$",
		Priority = 0x0ffffd,
		Left = "0%", Top = "0%+4em",
		Right = "100%", Bottom = "0%+6em",
		Symbol = Icon_ChangeBackground,
		BackgroundColor = {Std = nil, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(this, "ToggleBackground", menu)
	};

	var hide_text =
	{
		Tooltip = "$TooltipHideText$",
		Priority = 0x0ffffd,
		Left = "0%", Top = "0%+6em",
		Right = "100%", Bottom = "0%+8em",
		Symbol = Icon_HideText,
		BackgroundColor = {Std = nil, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(this, "ToggleText", menu)
	};
	if (menu.TextHidden == 1)
	{
		hide_text.Tooltip = "$TooltipShowText$";
		hide_text.GraphicsName = "Show";
	}

	var saturation =
	{
		Tooltip = "$TooltipWithoutSaturation$",
		Priority = 0x0ffffc,
		Left = "0%", Top = "0%+8em",
		Right = "100%", Bottom = "0%+10em",
		Symbol = Icon_ChangeColor,
		GraphicsName = "Grey",
		BackgroundColor = {Std = nil, Hover = 0x50ffff00},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(this, "ToggleSaturation", menu)
	};
	if (menu.Saturation == 1)
	{
		saturation.Tooltip = "$TooltipWithSaturation$";
		saturation.GraphicsName = nil;
	}

	// For the description box
	var background_color = RGBa(25,25,25, 200);
	if (menu.BackgroundTransparency == 1)
		background_color = RGBa(25,25,25, 100);
	if (menu.BackgroundTransparency == 2)
		background_color = RGBa(25,25,25, 255);

	// Assemble the menu
	var root_menu =
	{
		Target = menu,
		// Open just below the inventory bar, -20 to counter the standard margin
		Top = ToEmString(GUI_Controller_InventoryBar_IconMarginScreenTop * 2 + GUI_Controller_InventoryBar_IconSize + GUI_Controller_InventoryBar_IconSize/2 - 20),
		Style = GUI_NoCrop,
		// Mostly transparent but shows the 'transfer all' buttons
		center_column =
		{
			// -10 because the button bar takes up 2em on the right side
			Left = Format("50%% %s", ToEmString(-CenterColumnWidth / 2 - 10)),
			Right = Format("50%% %s", ToEmString(CenterColumnWidth / 2 - 10)),
			Top = "1.5em",
			Bottom = "4.5em",
			Style = GUI_VerticalLayout,
			move_all_left =
			{
				Target = center_column_menu,
				ID = 10 + 0,
				Right = ToEmString(CenterColumnWidth), Bottom = "1.5em",
				Margin = ["0.1em"],
				Style = GUI_TextHCenter | GUI_TextVCenter,
				Symbol = Icon_MoveItems, GraphicsName = "Left",
				Tooltip = "$TooltipTransferAllLeft$",
				BackgroundColor ={Std = nil, Hover = 0x50ffff00},
				OnMouseIn = GuiAction_SetTag("Hover"),
				OnMouseOut = GuiAction_SetTag("Std"),
				OnClick = GuiAction_Call(menu, "OnMoveAllToClicked", 0)
			},
			move_all_right =
			{
				Target = center_column_menu,
				ID = 10 + 1,
				Right = ToEmString(CenterColumnWidth), Bottom = "1.5em",
				Margin = ["0.1em"],
				Style = GUI_TextHCenter | GUI_TextVCenter,
				Symbol = Icon_MoveItems,
				Tooltip = "$TooltipTransferAllRight$",
				BackgroundColor ={Std = nil, Hover = 0x50ffff00},
				OnMouseIn = GuiAction_SetTag("Hover"),
				OnMouseOut = GuiAction_SetTag("Std"),
				OnClick = GuiAction_Call(menu, "OnMoveAllToClicked", 1)
			}
		},
		// Show descriptive texts when hovering stuff
		description_box =
		{
			Top = "100%-5em",
			Right = "100% - 2em",
			Margin = [ToEmString(SideBarSize), "0em"],
			Decoration = GUI_MenuDeco2,
			BackgroundColor = background_color,
			symbol_part =
			{
				Right = "5em",
				Symbol = nil,
				Margin = "0.2em",
				ID = 1,
				Target = description_box.symbol_target
			},
			desc_part =
			{
				Left = "5em",
				Margin = "0.2em",
				ID = 1,
				Target = description_box.target,
				real_contents = // nested one more time so it can dynamically be replaced without messing up the layout
				{
					ID = 1,
					Target = description_box.desc_target
				}
			}
		},
		// Buttons bar at the top right
		buttons =
		{
			ID = 10 + 2,
			Left = "100%-2em",
			Top = "0%+0em",
			Right = "100%",
			Bottom = ToEmString(buttons_height),
			Decoration = GUI_MenuDecoInventoryHeader
		}
	};

	GuiAddSubwindow(close_button, root_menu.buttons);
	GuiAddSubwindow(settings_button, root_menu.buttons);
	if (menu.SettingsOpen)
	{
		GuiAddSubwindow(background_change, root_menu.buttons);
		GuiAddSubwindow(hide_text, root_menu.buttons);
		GuiAddSubwindow(saturation, root_menu.buttons);
	}

	return root_menu;
}

// Assemble the main menu and the sidebar into a surrounding structure
// There are always two part menus open
// Parameters are: Interaction Menu Object, Slot of the current sidebar (either 0 or 1, left/right), Menu target (probably the same as the first parameter but can change), the sidebar menu, the main menu
public func CreatePartMenu(object menu, int slot, object target, proplist sidebar, proplist main)
{
	// Stretch from left border to center column
	var part_menu =
	{
		Left = "0%", Right = Format("50%% %s", ToEmString(-CenterColumnWidth / 2 - 10)),
		sidebar = sidebar, main = main,
		Target = target,
		ID = 1,
		Style = GUI_NoCrop
	};

	// Right part menu will stretch from center column to the button bar
	if (slot == 1)
	{
		part_menu.Left = Format("50%% %s", ToEmString(CenterColumnWidth / 2 - 10));
		part_menu.Right = "100%-2em";
	}

	return part_menu;
}

// Main menu is the big information menu for a single inspected object
// So there are always two 'main' menus open
// Parameters are: Interaction Menu Object, Slot of the current main menu (either 0 or 1, left/right), Dummy menu target, Object whose settings are displayed
public func CreateMainMenu(object menu, int slot, object target, object obj_for)
{
	var background_color = RGBa(25,25,25, 200);
	if (menu.BackgroundTransparency == 1)
		background_color = RGBa(25,25,25, 100);
	if (menu.BackgroundTransparency == 2)
		background_color = RGBa(25,25,25, 255);

	var big_menu =
	{
		Target = target,
		Priority = 5,
		Right = Format("100%% %s", ToEmString(-SideBarSize)),
		Bottom = "100% - 5.5em",
		Decoration = GUI_MenuDeco2,
		BackgroundColor = background_color,
		container =
		{
			Priority = 7,
			Top = "1em",
			Style = GUI_VerticalLayout,
			Margin = ["0.1em"],
		},
		headline =
		{
			Priority = 7,
			Bottom = "1em",
			Text = obj_for->GetName(),
			Style = GUI_TextHCenter | GUI_TextVCenter,
		},
	};

	if (slot == 0)
	{
		big_menu.Left = ToEmString(SideBarSize);
		big_menu.Right = "100%";
	}

	return big_menu;
}

// Create a single information menu (row) in the main menu
// Parameters are: Interaction Menu Object, Slot of the current main menu (either 0 or 1, left/right), Object whose settings are displayed, the menu information, the player's cursor, an ascending integer
// This function should create a GUI target (like MenuStyle_Grid)
// menu can have the following properties:
// flag: Marking special entries (currently only contents), if not set should be set to InteractionMenu_Custom
// title: The entry's caption
// entries: An array of proplists with subentries ({ symbol, text, unique_index, custom })
// entries_callback: A callback the interaction menu will ask for subentries (subentries is most likely empty if given)
// callback: A callback if the entry is clicked
// callback_hover: A callback if the entry is hovered over
// callback_target: Target object for the callback
// Anything that goes into scripted GUIs anyway (see documentation)
// Most of these entries are of no concern to this function and can be ignored
public func CreateMainMenuItem(object menu, int slot, object target, proplist menu_entry, object cursor, int i)
{
	// Create a basic layout object
	menu_entry.menu_object = CreateObject(MenuStyle_Grid);

	// Special display settings for contents entries
	if (menu_entry.flag == InteractionMenu_Contents)
	{
		menu_entry.menu_object->SetTightGridLayout();
	}

	// Settings
	menu_entry.menu_object.Top = "+1em";
	menu_entry.menu_object.Priority = 7;
	menu_entry.menu_object->SetPermanent();
	menu_entry.menu_object->SetFitChildren();
	menu_entry.menu_object->SetMouseOverCallback(menu, "OnMenuEntryHover");

	// Handle subentries
	for (var e = 0; e < GetLength(menu_entry.entries); ++e)
	{
		var entry = menu_entry.entries[e];
		entry.unique_index = ++menu_entry.entry_index_count;
		// This also allows the interaction-menu user to supply a custom entry with custom layout f.e.
		var added_entry = menu_entry.menu_object->AddItem(entry.symbol, entry.text, entry.unique_index, menu, "OnMenuEntrySelected", { slot = slot, index = i }, entry["custom"]);
	}

	// Assemble the entry
	var all =
	{
		Priority = menu_entry.Priority ?? i,
		Style = GUI_FitChildren,
		// Caption
		title_bar =
		{
			Priority = (menu_entry.Priority ?? i) - 2,
			Style = GUI_TextVCenter | GUI_TextHCenter,
			Bottom = "+1em",
			Text = menu_entry.title,
			// Visual separation at the top
			hline = {Bottom = "0.05em", BackgroundColor = RGB(100, 100, 100)},
		},
		hline = {Top = "1em", Bottom = "1.05em", BackgroundColor = RGB(100, 100, 100), Priority = (menu_entry.Priority ?? i) - 1},
		Margin = [nil, nil, nil, "0.25em"],
		// The content of this entry
		real_menu = menu_entry.menu_object,
		spacer = {Left = "0em", Right = "0em", Bottom = "3em"} // guarantees a minimum height
	};

	// Background color according to transparency setting
	var background_color = nil;

	// Background color is defined
	if (menu_entry.BackgroundColor)
	{
		if (menu.BackgroundTransparency == 0)
			background_color = SetRGBaValue(menu_entry.BackgroundColor, Min(200, GetRGBaValue(menu_entry.BackgroundColor, RGBA_ALPHA)), RGBA_ALPHA);
		if (menu.BackgroundTransparency == 1)
			background_color = SetRGBaValue(menu_entry.BackgroundColor, Min(100, GetRGBaValue(menu_entry.BackgroundColor, RGBA_ALPHA)), RGBA_ALPHA);
		if (menu.BackgroundTransparency == 2)
			background_color = menu_entry.BackgroundColor;
	}
	// Menu decoration is defined
	else if (menu_entry.Decoration)
	{
		background_color = menu_entry.Decoration->FrameDecorationBackClr();
		if (menu.BackgroundTransparency == 0)
			background_color = SetRGBaValue(menu_entry.Decoration->FrameDecorationBackClr(), Min(200, GetRGBaValue(menu_entry.Decoration->FrameDecorationBackClr(), RGBA_ALPHA)), RGBA_ALPHA);
		if (menu.BackgroundTransparency == 1)
			background_color = SetRGBaValue(menu_entry.Decoration->FrameDecorationBackClr(), Min(100, GetRGBaValue(menu_entry.Decoration->FrameDecorationBackClr(), RGBA_ALPHA)), RGBA_ALPHA);
		//if (menu.BackgroundTransparency == 2)
			// No change required
	}

	// Menu color saturation
	if (background_color)
	{
		if (menu.Saturation == 1)
		{
			var alpha = GetRGBaValue(background_color, RGBA_ALPHA);
			var val = RGB2HSL(background_color);
			// Desaturate
			if (GetRGBaValue(val, RGBA_GREEN) > 10)
				val = SetRGBaValue(val, 10, RGBA_GREEN);
			// Increase light
			if (GetRGBaValue(val, RGBA_BLUE) < 50)
				val = SetRGBaValue(val, 50, RGBA_BLUE);
			val = HSL2RGB(val);
			// Restore alpha
			val = SetRGBaValue(val, alpha, RGBA_ALPHA);
			background_color = val;
		}
	}

	//menu_entry.menu_object.BackgroundColor = background_color;
	all.BackgroundColor = background_color;

	// Hide captions?
	if (menu.TextHidden)
	{
		all.title_bar.Text = nil;
		all.title_bar.Bottom = "0.05em";
		all.title_bar.hline.Bottom = nil;
		all.real_menu.Top = "0.05em";
		all.spacer.Bottom = "2.05em";
		all.hline = nil;
	}

	return all;
}

// The sidebar shows all available objects to display as tabs
// There are always two sidebars open (one left, one right)
// Parameters are: Interaction Menu Object, Slot of the current sidebar (either 0 or 1, left/right), Menu target (probably the same as the first parameter but can change)
public func CreateSideBar(object menu, int slot, object target)
{
	// Entries will be inserted in the next step
	var sidebar =
	{
		Priority = 10,
		Right = ToEmString(SideBarSize),
		Style = GUI_VerticalLayout,
		Target = target,
		ID = 2
	};

	if (slot == 1)
	{
		sidebar.Left = Format("100%% %s", ToEmString(-SideBarSize));
		sidebar.Right = "100%";
	}

	return sidebar;
}

// Create a single sidebar item (/entry)
// Parameters are: Interaction Menu Object, Slot of the current sidebar (either 0 or 1, left/right), Object shown in the entry, is the entry selected, is the entry selected on the other side, is the entry the player's cursor
public func CreateSideBarItem(object menu, int slot, object target, bool selected, bool crossed_out, bool cursor)
{
	// For sorting: the vertical layout will sort by priority
	var priority = 10000 - target.Plane;
	// The cursor should always be at the top
	if (cursor)
		priority = 1;

	var background_symbol = "";
	var highlight_symbol = "Highlight";
	if (selected)
	{
		background_symbol = "Selected";
		highlight_symbol = "Selected";
	}

	if (slot == 1)
	{
		background_symbol = "Right";
		highlight_symbol = "RightHighlight";
		if (selected)
		{
			background_symbol = "RightSelected";
			highlight_symbol = "RightSelected";
		}
	}

	var deactivation_symbol = nil;
	if (crossed_out)
		deactivation_symbol = Icon_Cancel;

	// Coordinates for the little picture & name
	var left = "2.25em";
	var right = "4em";
	var top = "0%";
	var bottom = "1.75em";
	var text_margin = [nil, nil, "0.25em", nil];
	var text_style = GUI_TextRight | GUI_TextTop | GUI_FitChildren;

	if (slot == 1)
	{
		left = "0%";
		right = "1.5em";
		var text_margin = ["0.25em", nil, nil, nil];
		var text_style = GUI_TextLeft | GUI_TextTop | GUI_FitChildren;
	}

	var entry =
	{
		// The object is added as the target of the entry, so it can easily be identified later.
		// For example, to apply show the grey haze to indicate that it cannot be clicked.
		Target = target,
		Right = ToEmString(SideBarSize), Bottom = "2.75em",
		Priority = priority,
		Style = GUI_FitChildren,
		OnMouseIn = GuiAction_SetTag("OnHover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = GuiAction_Call(menu, "OnSidebarEntrySelected", {slot = slot, obj = target}),
		Priority = 1,
		obj_background = {
			Top = top,
			Left = left,
			Right = right,
			Bottom = bottom,
			Symbol = Icon_Menu_Tab,
			GraphicsName = { Std = background_symbol, OnHover = highlight_symbol },
			Priority = 2
		},
		obj_symbol = {
			Top = top,
			Left = left,
			Right = right,
			Bottom = bottom,
			Symbol = target,
			Priority = 3
		},
		obj_symbol_deactivated = {
			Target = target,
			ID = 1 + slot,
			Top = top,
			Left = left,
			Right = right,
			Bottom = bottom,
			Symbol = deactivation_symbol,
			Priority = 4
		},
		obj_name = {
			Style = text_style,
			Top = "1.75em",
			Bottom = "2.75em",
			Margin = text_margin,
			Text = target->GetName(),
			Priority = 5
		}
	};

	// Hide object's name if the option is selected
	if (menu.TextHidden)
	{
		entry.obj_name = nil;
		entry.Bottom = "1.75em";
	}

	return entry;
}