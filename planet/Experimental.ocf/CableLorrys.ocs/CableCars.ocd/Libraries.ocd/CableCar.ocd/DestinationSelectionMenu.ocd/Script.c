/**
	DestinationSelectionMenu
	Handles the destination selection for cable cars.

	@author Clonkonaut
*/

local Name = "$Name$";
local Description = "$Description$";

// used as a static function
public func CreateFor(object cursor, object car, object station)
{
	if (!cursor) return;
	if (!car) return;
	if (!station) return;

	var obj = CreateObject(GUI_DestinationSelectionMenu, AbsX(0), AbsY(0), cursor->GetOwner());
	obj.Visibility = VIS_Owner;

	obj->Init(cursor, car, station);
	cursor->SetMenu(obj);
	return obj;
}

// The Clonk whom the menu was opened for
local cursor;
// The menu id of the opened menu
local menu_id;
// The cable car which opened this menu
local cable_car;
// The station the cable car is hooked up to
local cable_station;
// A dummy object used to bring light to stations when previewed
local dummy;

public func Close() { return RemoveObject(); }
public func IsDestinationMenu() { return true; }
public func Show() { this.Visibility = VIS_Owner; return true; }
public func Hide() { this.Visibility = VIS_None; return true; }
// Called when the menu is open and the player clicks outside.
public func OnMouseClick() { return Close(); }

func Destruction()
{
	if (menu_id)
		GuiClose(menu_id);
	if (dummy)
		dummy->RemoveObject();
	if (cursor)
		SetPlrView(cursor->GetOwner(), cursor, true);
}

public func Init(object cursor, object car, object station)
{
	this.cursor = cursor;
	this.cable_car = car;
	this.cable_station = station;
	this.dummy = CreateObject(Dummy, AbsX(station->GetX()), AbsY(station->GetY()), GetOwner());
	dummy.Visibility = VIS_Owner;
	dummy->SetLightRange(5, 20);

	var dest_list = station->GetDestinationList(nil);

	var dest_menu = {
		Target = this,
		Decoration = GUI_MenuDeco,
		BackgroundColor = RGB(0, 0, 0),
		Bottom = "5em",
		Left = "50% - 7.5em",
		Right = "50% + 7.5em",
		Priority = 1,
		Player = cursor->GetOwner(),
		caption = {
			Text = "$SelectDestination$",
			Bottom = "1em",
			Priority = 2,
		},
		buttons = {
			Top = "1em",
			Bottom = "4em",
			Style = GUI_GridLayout,
			Priority = 999
		},
		info = {
			Top = "4em",
			Text = nil,		
			Priority = 9999	
		}
	};

	FillDestinationButtons(dest_menu, dest_list);

	GuiOpen(dest_menu);
}

func FillDestinationButtons(proplist menu, array list)
{
	var priority = 1000;
	// Left button
	var left = {
		Right = "3em",
		Bottom = "3em",
		Symbol = Icon_LibraryCableCar,
		GraphicsName = "LeftGrey",
		BackgroundColor = { Std = RGB(0, 0, 0), Hover = RGB(100, 30, 30) },
		Priority = ++priority
	};
	if (list[3] > 3)
	{
		left.OnMouseIn = GuiAction_SetTag("Hover");
		left.OnMouseOut = GuiAction_SetTag("Std");
		left.GraphicsName = "Left";
		left.OnClick = GuiAction_Call(this, "ShiftSelection", list[0]);
	}
	// List buttons
	var list_button = {
		Right = "3em",
		Bottom = "3em",
		BackgroundColor = { Std = RGB(0, 0, 0), Hover = RGB(100, 30, 30) },
		OnMouseOut = GuiAction_SetTag("Std")
	};
	var buttons = CreateArray(3);
	for (var i = 0; i < 3; i++)
	{
		if (list[i])
		{
			var connected = list[i].connected_building;
			var connected_info = {};
			if (connected)
			{
				connected_info = {
					Left = "1em",
					Top = "1.5em",
					conn = {
						Left = "0.5em",
						Symbol = connected
					},
					symbol_conn = {
						Right = "1em",
						Symbol = ConstructionPreviewer_IconCombine
					}
				};
			}
			buttons[i] = new list_button {
				Symbol = list[i],
				Tooltip = Format("$SendTo$", list[i]->GetName()),
				OnMouseIn = [GuiAction_SetTag("Hover"), GuiAction_Call(this, "PreviewDestination", list[i])],
				OnClick = GuiAction_Call(this, "SelectDestination", list[i]),
				Priority = ++priority,
				connected_to = connected_info
			};
		} else {
			buttons[i] = new list_button {};
		}
	}
	// Right button
	var right = {
		Right = "3em",
		Bottom = "3em",
		Symbol = Icon_LibraryCableCar,
		GraphicsName = "Grey",
		BackgroundColor = { Std = RGB(0, 0, 0), Hover = RGB(100, 30, 30) },
		Priority = ++priority
	};
	if (list[3] > 3)
	{
		right.OnMouseIn = GuiAction_SetTag("Hover");
		right.OnMouseOut = GuiAction_SetTag("Std");
		right.GraphicsName = nil;
		right.OnClick = GuiAction_Call(this, "ShiftSelection", list[2]);
	}
	// Assemble
	menu.buttons.left = left;
	menu.buttons.first = buttons[0];
	menu.buttons.second = buttons[1];
	menu.buttons.third = buttons[2];
	menu.buttons.right = right;
}

func PreviewDestination(object to_preview, int plr, int menu_id, int submenu_id, object target)
{
	if (to_preview == nil) return;
	if (dummy == nil)
	{
		dummy = CreateObject(Dummy, AbsX(to_preview->GetX()), AbsY(to_preview->GetY()), GetOwner());
		dummy.Visibility = VIS_Owner;
		dummy->SetLightRange(5, 20);
	}

	SetPlrView(plr, to_preview, true);
	dummy->SetPosition(to_preview->GetX(), to_preview->GetY());
	
	// Update info on this destination.
	var eta = this.cable_station->GetLengthToTarget(to_preview) / this.cable_car->GetCableSpeed() / 36;
	var update = {info = {Text = Format("$InfoETA$", eta)}};
	GuiUpdate(update, menu_id);
}

func ShiftSelection(object new_middle, int plr, int menu_id, int submenu_id, object target)
{
	if (!cable_station) return;
	if (!menu_id) return;

	var update = { buttons = { } };
	var list = cable_station->GetDestinationList(new_middle);

	FillDestinationButtons(update, list);
	GuiUpdate(update, menu_id);
	Sound("UI::Click", true, nil, plr);
}

func SelectDestination(object target, int plr, int menu_id, int submenu_id, object target)
{
	if (target == nil) return;
	if (!cable_car) return;

	cable_car->SetDestination(target);
	Sound("UI::Click", true, nil, plr);

	RemoveObject();
}