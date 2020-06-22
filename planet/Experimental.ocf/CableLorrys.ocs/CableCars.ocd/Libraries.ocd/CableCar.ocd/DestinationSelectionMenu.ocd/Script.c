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
	if (!cursor || !car || !station)
		return;

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

public func Destruction()
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
	
	// Create dummy for destination highlighting.
	this.dummy = CreateDummy(station);

	var dest_list = station->GetDestinationList();
	var dest_menu =
	{
		Target = this,
		Decoration = GUI_MenuDeco,
		BackgroundColor = RGB(0, 0, 0),
		Top = "0%",
		Bottom = "0%+14em",
		Left = "0%",
		Right = "0%+12em",
		Priority = 1,
		Player = cursor->GetOwner(),
		caption = 
		{
			Text = "$SelectDestination$",
			Bottom = "1em",
			Priority = 2,
		},
		buttons = 
		{
			Top = "1em",
			Bottom = "13em",
			Style = GUI_GridLayout,
			Priority = 999
		},
		info = 
		{
			Top = "13em",
			Text = nil,		
			Priority = 9999	
		}
	};

	FillDestinationButtons(dest_menu, dest_list);
	GuiOpen(dest_menu);
}

public func FillDestinationButtons(proplist menu, array dest_list)
{
	var index = 0;
	for (var dest in dest_list)
	{
		if (!dest)
			continue;
		var connected = dest.connected_building;
		var connected_info = {};
		if (connected)
		{
			connected_info =
			{
				Left = "1em",
				Top = "1.5em",
				conn = 
				{
					Left = "0.5em",
					Symbol = connected
				},
				symbol_conn =
				{
					Right = "1em",
					Symbol = ConstructionPreviewer_IconCombine
				}
			};
		}		
		menu.buttons[Format("dest%d", index)] = 
		{
			Right = "3em",
			Bottom = "3em",
			BackgroundColor = { Std = RGB(0, 0, 0), Hover = RGB(100, 30, 30) },
			OnMouseOut = GuiAction_SetTag("Std"),
			Symbol = dest,
			Tooltip = Format("$SendTo$", dest->GetName()),
			OnMouseIn = [GuiAction_SetTag("Hover"), GuiAction_Call(this, "PreviewDestination", dest)],
			OnClick = GuiAction_Call(this, "SelectDestination", dest),
			Priority = 1000 + index,
			connected_to = connected_info
		};
		index++;
	}
	return;
}

public func PreviewDestination(object to_preview, int plr, int menu_id, int submenu_id, object target)
{
	if (!to_preview)
		return;
		
	if (!dummy)
		dummy = CreateDummy(to_preview);

	// Set view, light and draw the item's graphics in front of it again to achieve a highlighting effect.
	SetPlrView(plr, to_preview, true);
	dummy.Plane = to_preview.Plane + 1;
	dummy->SetPosition(to_preview->GetX(), to_preview->GetY());
	dummy->SetVertexXY(0, to_preview->GetVertex(0, VTX_X), to_preview->GetVertex(0, VTX_Y));
	dummy->SetAction("Attach", to_preview);
	dummy->SetGraphics(nil, nil, 1, GFXOV_MODE_Object, nil, GFX_BLIT_Additive, to_preview);
	
	// Update info on this destination.
	var eta = this.cable_station->GetLengthToTarget(to_preview) / this.cable_car->GetCableSpeed() / 36;
	var update = {info = {Text = Format("$InfoETA$", eta)}};
	GuiUpdate(update, menu_id);
}

public func SelectDestination(object target, int plr, int menu_id, int submenu_id, object target)
{
	if (target == nil || !cable_car)
		return;
	cable_car->SetDestination(target);
	Sound("UI::Click", true, nil, plr);
	RemoveObject();
}

private func CreateDummy(object for_obj)
{
	var new_dummy = CreateObject(Dummy, AbsX(for_obj->GetX()), AbsY(for_obj->GetY()), GetOwner());
	new_dummy.Visibility = VIS_Owner;
	new_dummy->SetLightRange(5, 20);
	new_dummy.ActMap = 
	{
		Attach = 
		{
			Name = "Attach",
			Procedure = DFA_ATTACH,
			FacetBase = 1
		}
	};
	return new_dummy;
}
