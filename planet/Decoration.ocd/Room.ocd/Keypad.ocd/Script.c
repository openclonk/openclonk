/**
	Keypad
	allows to enter digit codes to trigger an event.
	
	@author Maikel	
*/

#include Library_Switch

// Background colors for hovering and bars and description.
static const KEYPADMENU_BackgroundColor = 0x77000000;
static const KEYPADMENU_HoverColor = 0x99888888;
static const KEYPADMENU_BarColor = 0x99888888;

local code, correct_code;
local correct_code_action, wrong_code_action;
local replacement_images;
local menu, menu_id, menu_target, menu_controller;


/*-- Script Interface --*/

public func SetKeypadCode(string to_code)
{
	// Check if code contains any non-digits.
	var non_digits = RegexMatch(to_code, "[^0-9]+");
	if (!DeepEqual(non_digits, []))
	{
		Log("$WarningKeypadCodeNonDigit$", to_code);
		return;
	}
	// Check if code is not too long.
	if (GetLength(to_code) > this.CodeMaxLength)
	{
		to_code = TakeString(to_code, 0, this.CodeMaxLength);
		Log("$WarningKeypadCodeLong$", to_code);
	}
	// If code is valid, set it.	
	correct_code = to_code;
	return;
}

public func GiveAccess(object clonk)
{
	SetSwitchState(true, clonk);
	return;
}

public func OnCorrectCodeEntered(object clonk)
{
	// Open door/switch on if specified.
	GiveAccess(clonk);
	// Perform user action last; it may delete the door/clonk/etc.
	UserAction->EvaluateAction(correct_code_action, this, clonk);
	return;
}

public func OnWrongCodeEntered(object clonk)
{
	// Perform user action last; it may delete the door/clonk/etc.
	UserAction->EvaluateAction(wrong_code_action, this, clonk);
	return;
}

public func SetCodeActions(new_correct_action, new_wrong_action)
{
	correct_code_action = new_correct_action;
	wrong_code_action = new_wrong_action;
	return;
}

// A list of id's which replace the default images of the buttons.
// The nth entry in the list replaces the nth button.
public func SetReplacementImages(array id_list)
{
	// Only store at most 10 images.
	SetLength(id_list, Min(GetLength(id_list), 10));
	replacement_images = id_list[:];
	// Update the keypad graphics.
	//UpdateKeypadGraphics();
	return;
}

public func GetReplacementImages()
{
	return replacement_images;
}

private func UpdateKeypadGraphics()
{
	if (replacement_images == nil)
		return;
	
	for (var index = 0; index < GetLength(replacement_images); index++)
	{
		var image = replacement_images[index];
		if (image)
		{
			var pos_x = (index - 1) % 3;
			var pos_y = (index - 1) / 3;
			if (index == 0)
			{
				pos_x = 1;
				pos_y = 3;			
			}
			SetGraphics(nil, image, index + 1, GFXOV_MODE_IngamePicture);
			SetObjDrawTransform(320, 0, -4000 + 4000 * pos_x, 0, 240, -6000 + 4000 * pos_y, index + 1);
		}		
	}
	return;
}


/*-- Saving --*/

public func SaveScenarioObject(proplist props)
{
	if (!_inherited(props, ...)) return false;
	if (correct_code) props->AddCall("Code", this, "SetKeypadCode", Format("%v", correct_code));
	if (correct_code_action || wrong_code_action) props->AddCall("Action", this, "SetCodeActions", correct_code_action, wrong_code_action);
	if (replacement_images) props->AddCall("Replacements", this, "SetReplacementImages", replacement_images);
	return true;
}


/*-- Editor --*/

public func Definition(proplist def)
{
	if (!def.EditorProps)
		def.EditorProps = {};
	def.EditorProps.correct_code = { Name = "$KeypadCode$", Type = "string", Set="SetKeypadCode", EditorHelp = "$HelpKeypadCode$" };	
	def.EditorProps.correct_code_action = new UserAction.Prop { Name = "$OnCorrectCodeAction$", EditorHelp = "$HelpOnCorrectCodeAction$" };
	def.EditorProps.wrong_code_action = new UserAction.Prop { Name = "$OnWrongCodeAction$", EditorHelp = "$HelpOnWrongCodeAction$" };
	return;
}


/*-- Interaction --*/

public func IsInteractable(object clonk)
{
	return clonk->GetProcedure() == "WALK" && (!clonk->GetMenu() || clonk->GetMenu().ID != menu_id);
}

public func Interact(object clonk)
{
	code = "";
	OpenKeypadMenu(clonk);
	return true;
}


/*-- Menu --*/

public func OpenKeypadMenu(object clonk)
{
	// Only one clonk at a time can handle the keypad.
	if (menu_controller)
		return;
		
	// If the menu is already open, don't open another instance.
	if (clonk->GetMenu() && clonk->GetMenu().ID == menu_id)
		return;
		
	// This object functions as menu target and for visibility.
	menu_target = CreateContents(Dummy);
	menu_target.Visibility = VIS_Owner;
	menu_target->SetOwner(clonk->GetOwner());
	menu_controller = clonk;
	
	// Make the room/credits menu.
	menu =
	{
		Target = menu_target,
		Left = "50%-6em",
		Right = "50%+6em",
		Top = "50%-10em",
		Bottom = "50%+8em",
		Decoration = GUI_MenuDeco,
		BackgroundColor = {Std = KEYPADMENU_BackgroundColor},
	};
	menu.code =
	{
		Target = this,
		ID = 2,
		Bottom = "1.5em",
		text =
		{
			Target = this,
			ID = 21,
			Right = "4em",
			Style = GUI_TextVCenter,
			Text = "$MsgEnterCode$"		
		},
		value =
		{
			Target = this,
			ID = 22,
			Left = "4em",
			Right = "100%-0.05em",
			Style = GUI_TextVCenter | GUI_TextRight,
			Text = nil
		}
	};
	menu.bar =
	{
		Target = this,
		ID = 3,
		Top = "1.5em",
		Bottom = "2em",
		BackgroundColor = {Std = KEYPADMENU_BarColor},
	};
	menu.keys =
	{
		Target = this,
		ID = 4,
		Top = "2em",
	};
	
	// Create code buttons.
	var positions = [[1, 3], [0, 0], [1, 0], [2, 0], [0, 1], [1, 1], [2, 1], [0, 2], [1, 2], [2, 2]];
	for (var index = 0; index < 10; index++)
	{
		var pos_x = positions[index][0];
		var pos_y = positions[index][1];
		var icon = Icon_Number;
		var name = Format("%d", index);
		if (replacement_images)
		{
			if (GetType(replacement_images[index]) == C4V_Def)
			{
				// Draw replacement image on button.
				var pos_x = positions[index][0];
				var pos_y = positions[index][1];
				icon = replacement_images[index];
				name = nil;
			}
			else
			{
				// Don't create button if replacement image is not defined.
				continue;
			}
			
		}	
		menu.keys[Format("key%d", index)] = MakePadButton(pos_x, pos_y, icon, name, "$TooltipDigit$", GuiAction_Call(this, "UpdateMenuCode", Format("%d", index)));
	}
	
	// Create other buttons.
	menu.keys.enter = MakePadButton(2, 3, Icon_Ok, nil, "$TooltipCheck$", GuiAction_Call(this, "EnterKeypadCode", nil));
	
	menu.keys.clearlast = MakePadButton(0, 0, Icon_Number, "Hash", "$TooltipClearLast$", GuiAction_Call(this, "UpdateMenuCode", nil));
	menu.keys.clearlast.Left = "0em"; menu.keys.clearlast.Right = "2em"; menu.keys.clearlast.Top = "12em"; menu.keys.clearlast.Bottom = "14em";
	menu.keys.clearlast.image = {Left = "50%", Top = "50%", Symbol = Icon_Arrow, GraphicsName = "Left"};
		
	menu.keys.clearcode = MakePadButton(0, 0, Icon_Number, "Hash", "$TooltipClearCode$", GuiAction_Call(this, "ClearMenuCode", nil));
	menu.keys.clearcode.Left = "2em"; menu.keys.clearcode.Right = "4em"; menu.keys.clearcode.Top = "12em"; menu.keys.clearcode.Bottom = "14em";
	menu.keys.clearcode.image = {Left = "50%", Top = "50%", Symbol = Icon_Cancel};
	
	menu.keys.resetcode = MakePadButton(0, 0, Icon_Number, "Hash", "$TooltipResetCode$", GuiAction_Call(this, "ResetKeypadCode", nil));
	menu.keys.resetcode.Left = "2em"; menu.keys.resetcode.Right = "4em"; menu.keys.resetcode.Top = "14em"; menu.keys.resetcode.Bottom = "16em";
	menu.keys.resetcode.image = {Left = "50%", Top = "50%", Symbol = Icon_Swap};
	
	menu.keys.close = MakePadButton(0, 0, Icon_Cancel, nil, "$TooltipClose$", GuiAction_Call(this, "CloseKeypadMenu", nil));
	menu.keys.close.Left = "0em"; menu.keys.close.Right = "2em"; menu.keys.close.Top = "14em"; menu.keys.close.Bottom = "16em";
		
	// Open the menu and store the menu ID.
	menu_id = GuiOpen(menu);
	// Notify the clonk and set the menu.
	clonk->SetMenu(this);
	return;
}

public func MakePadButton(int x, int y, id symbol, string graphics_name, string tooltip, on_click)
{
	return
	{
		Left = Format("%dem", 4 * x),
		Right = Format("%dem", 4 * (x + 1)),
		Top = Format("%dem", 4 * y),
		Bottom = Format("%dem", 4 * (y + 1)),
		Symbol = symbol,
		GraphicsName = graphics_name,
		BackgroundColor = {Std = 0, Hover = KEYPADMENU_HoverColor},
		OnMouseIn = GuiAction_SetTag("Hover"),
		OnMouseOut = GuiAction_SetTag("Std"),
		OnClick = on_click,
		Tooltip = tooltip
	};
}

public func UpdateMenuCode(string digit_pressed)
{
	if (digit_pressed == nil)
	{
		// Remove last digit or symbol.
		if (GetType(code) == C4V_String)
			code = TakeString(code, 0, GetLength(code) - 1);
		Sound("UI::Click?", {player = menu_controller->GetOwner()});
	}
	else
	{
		if (code == nil || GetLength(code) < this.CodeMaxLength)
		{
			if (code == nil)
				code = Format("%s", digit_pressed);
			else
				code = Format("%s%s", code, digit_pressed);
			Sound("UI::Tick", {player = menu_controller->GetOwner()});
		}
		else
		{
			Sound("UI::Click?", {player = menu_controller->GetOwner()});
		}		
	}
	menu.code.value.Text = CodeToDisplay(code);
	GuiUpdate(menu.code.value, menu_id, menu.code.value.ID, this);
	return;
}

// Turns a string of digits into a string of digits and images.
private func CodeToDisplay(string code)
{
	if (GetType(code) != C4V_String)
		return "";
	var display = "";
	for (var index = 0; index < GetLength(code); index++)
	{
		var digit = GetChar(code, index) - 48;
		if (replacement_images && replacement_images[digit])
			display = Format("%s{{%i}}", display, replacement_images[digit]);
		else
			display = Format("%s%d", display, digit);
	}
	return display;
}

public func ClearMenuCode()
{
	code = nil;
	Sound("UI::Click?", {player = menu_controller->GetOwner()});
	menu.code.value.Text = code;
	GuiUpdate(menu.code.value, menu_id, menu.code.value.ID, this);
	return;
}

public func EnterKeypadCode()
{
	if (menu_controller.code_reset_state == "reset")
	{
		correct_code = code;
		code = nil;
		menu.code.value.Text = "$MsgCodeReset$";
		menu_controller.code_reset_state = nil;
		Sound("UI::Confirmed", {player = menu_controller->GetOwner()});
		menu.code.text.Text = "$MsgEnterCode$";
		GuiUpdate(menu.code.text, menu_id, menu.code.text.ID, this);
	}
	else
	{
		if (correct_code == nil)
		{
			code = nil;
			menu.code.value.Text = "$MsgNoCode$";
			Sound("UI::Error", {player = menu_controller->GetOwner()});
		}
		else if (correct_code == code)
		{
			code = nil;
			Sound("UI::Confirmed", {player = menu_controller->GetOwner()});
			if (menu_controller.code_reset_state == "confirm")
			{
				menu.code.value.Text = "$MsgCodeConfirmed$";
				menu_controller.code_reset_state = "reset";
				menu.code.text.Text = "$MsgEnterNewCode$";
				GuiUpdate(menu.code.text, menu_id, menu.code.text.ID, this);
			}
			else
			{
				menu.code.value.Text = "$MsgCorrectCode$";
				// Execute the correct code trigger.
				OnCorrectCodeEntered(menu_controller);
			}
		}
		else
		{
			code = nil;
			menu.code.value.Text = "$MsgWrongCode$";
			Sound("UI::Error", {player = menu_controller->GetOwner()});
			// Execute the wrong code trigger.
			OnWrongCodeEntered(menu_controller);
		}
	}
	GuiUpdate(menu.code.value, menu_id, menu.code.value.ID, this);
	return;
}

public func ResetKeypadCode()
{
	// Only allow resetting the code if the clonk (menu_target) has entered the correct code before.
	if (correct_code == nil)
	{
		menu_controller.code_reset_state = "reset";
		menu.code.text.Text = "$MsgEnterNewCode$";
		GuiUpdate(menu.code.text, menu_id, menu.code.text.ID, this);
	}
	else if (menu_controller.code_reset_state != "reset" && menu_controller.code_reset_state != "confirm")
	{
		menu_controller.code_reset_state = "confirm";
		menu.code.text.Text = "$MsgConfirmCode$";
		GuiUpdate(menu.code.text, menu_id, menu.code.text.ID, this);
	}
	else
	{
		menu_controller.code_reset_state = nil;
		menu.code.text.Text = "$MsgEnterCode$";
		GuiUpdate(menu.code.text, menu_id, menu.code.text.ID, this);
	}
	Sound("UI::Tick", {player = menu_controller->GetOwner()});
	return;
}

public func CloseKeypadMenu()
{
	// Close the menu and inform the controller.
	Sound("UI::Close", {player = menu_controller->GetOwner()});
	GuiClose(menu_id, nil, this);
	menu_target->RemoveObject();
	menu_target = nil;
	menu_id = nil;
	if (menu_controller)
	{
		menu_controller.code_reset_state = nil;
		menu_controller->MenuClosed();
	}
	menu_controller = nil;
	return;
}

public func Close() 
{
	CloseKeypadMenu();
	return;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local CodeMaxLength = 12;
