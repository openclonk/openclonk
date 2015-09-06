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
local entries;

func Construction()
{
	entries = [];
	SetPosition(0, 0);
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
			Margin = "0.5em",
			header =
			{
				Bottom = "1em",
				icon = {Symbol = symbol, Right = "1em", Bottom = "1em"}, 
				caption = {Left = "1em", Text = caption, Style = GUI_TextVCenter}
			},
			body =
			{
				Top = "1em",
				items =
				{
					Right = "50%",
					Style = GUI_GridLayout
				},
				description =
				{
					ID = 1,
					Target = 0,
					Left = "50%",
					Text = "Empty"
				},
			}
		}
	};
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
		Right = "+2em",
		Bottom = "+2em",
		Text = Format("%dx", count),
		Priority = ID,
		OnClick = GuiAction_Call(this, "OnClick", [symbol, ID, command, parameter]),
		OnMouseIn = [GuiAction_SetTag("Hover", 0, nil), GuiAction_Call(this, "UpdateDesc")],
		OnMouseOut = GuiAction_SetTag("Std", 0, nil),
	};
	entries[ID] = [info_caption ?? symbol.Description];
	
	menu_layout.inner.body.items[Format("child%d", ID)] = entry;
	return entry;
}

func Open()
{
	return GuiOpen(menu_layout);
}

func UpdateDesc(data, int player, int ID, int subwindowID, object target)
{
	var update = { Text = entries[subwindowID][0] };
	GuiUpdate(update, ID, 1, 0);
}

func OnClick(data, int player, int ID, int subwindowID, object target)
{
	target->Call(data[2], data[0], data[3]);
	if (!permanent)
		GuiClose(ID);
}