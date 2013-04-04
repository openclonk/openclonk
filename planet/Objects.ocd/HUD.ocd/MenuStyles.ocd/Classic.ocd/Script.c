/**
	Classic
	Mimics the interface to a classic menu.
*/

local Name = "Classic Menu";

// set on creation
local menu_def, permanent;
local menu_layout;
local target;

// set later
local menu_id;
local entries;

func Construction()
{
	entries = [];
}

global func CreateClassicMenu(id symbol, object command_object, int extra, string caption, int extra_data, int style, bool permanent, id menu_id)
{
	if (!this) return;
	var menu = CreateObject(HUD_MenuStyle_Classic, 0, 0, GetOwner());
	menu.Visibility = VIS_Owner;
	menu.menu_def = menu_id;
	menu.permanent = permanent;
	menu.target = this;
	
	menu.menu_layout =
	{
		BackgroundColor = 0x50553300,
		Decoration = GUI_MenuDeco,
		Target = menu,
		inner =
		{
			header =
			{
				Hgt = [0, 32],
				icon = {Symbol = symbol, Wdt = [0, 32], Hgt = [0, 32]}, 
				caption = {X = [0, 32], Text = caption, Style = MENU_TextVCenter}
			},
			body =
			{
				Y = [0, 32],
				items =
				{
					Wdt = 500,
					Style = MENU_GridLayout
				},
				description =
				{
					ID = 1,
					Target = 0,
					X = 500,
					Text = "Empty"
				},
			}
		}
	};
	Menu_AddMargin(menu.menu_layout.inner, 15, 15);
	return menu;
}

public func AddMenuItem(string caption, string command, symbol, int count, parameter, string info_caption, int extra, XPar1, XPar2)
{
	var ID = GetLength(entries) + 1;
	var entry =
	{
		Target = target, // needed for the call
		ID = ID,
		BackgroundColor = {Std = 0, Hover = 0x50ff0000},
		Symbol = symbol,
		Wdt = [0, 64],
		Hgt = [0, 64],
		Text = Format("%dx", count),
		Priority = ID,
		OnClick = MenuAction_Call(this, "OnClick", [symbol, ID, command, parameter]),
		OnMouseIn = [MenuAction_SetTag(nil, 0, "Hover"), MenuAction_Call(this, "UpdateDesc")],
		OnMouseOut = MenuAction_SetTag(nil, 0, "Std"),
	};
	entries[ID] = [info_caption ?? symbol.Description];
	
	menu_layout.inner.body.items[Format("child%d", ID)] = entry;
	return entry;
}

func Open()
{
	return CustomMenuOpen(menu_layout);
}

func UpdateDesc(int player, int ID, int subwindowID, object target, data)
{
	var update = { Text = entries[subwindowID][0] };
	CustomMenuUpdate(update, ID, 1, 0);
}

func OnClick(int player, int ID, int subwindowID, object target, data)
{
	target->Call(data[2], data[0], data[3]);
	if (!permanent)
		CustomMenuClose(ID);
}