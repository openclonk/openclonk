/**
	List
	Shows a simple list menu.
*/

local Name = "List Menu";

local entries;
local on_mouse_over_callback, on_mouse_out_callback;
local on_close_callback;
local permanent;

local menu_id;

func Construction()
{
	entries = [];
	this.Style = MENU_VerticalLayout;
	this.Target = this;
}

func Close()
{
	if (menu_id)
		CustomMenuClose(menu_id);
	if (on_close_callback && on_close_callback[0])
		on_close_callback[0]->Call(on_close_callback[1], on_close_callback[2]);
	RemoveObject();
}

func SetPermanent(bool perm) { permanent = perm ?? true; }

func SetCloseCallback(proplist target, callback, parameter)
{
	on_close_callback = [target, callback, parameter];
}

func SetMouseOverCallback(proplist target, callback)
{
	on_mouse_over_callback = [target, callback];
}

func SetMouseOutCallback(proplist target, callback)
{
	on_mouse_out_callback = [target, callback];
}

func AddItem(symbol, string text, user_ID, proplist target, command, parameter, custom_entry)
{
	var on_hover = MenuAction_SetTag(nil, 0, "OnHover");
	if (on_mouse_over_callback)
		on_hover = [on_hover, MenuAction_Call(this, "DoCallback", on_mouse_over_callback)];
	var on_hover_stop = MenuAction_SetTag(nil, 0, "Std");
	if (on_mouse_out_callback)
		on_hover_stop = [on_hover_stop, MenuAction_Call(this, "DoCallback", on_mouse_out_callback)];
	
	var ID = GetLength(entries) + 1;
	if (!custom_entry)
	{
		custom_entry = {Hgt = [0, 64], sym = {Wdt = [0, 64], Hgt = [0, 64]}, desc = {X = [0, 64]}};
		custom_entry.sym.Symbol = symbol;
		custom_entry.desc.Text = text;
		custom_entry.desc.Style = MENU_TextVCenter;
		custom_entry.Style = MENU_FitChildren;
		custom_entry.ID = ID;
		custom_entry.Target = this;
		custom_entry.Priority = ID;
		custom_entry.BackgroundColor = {Std = 0, OnHover = 0x50ff0000};
		custom_entry.OnClick = MenuAction_Call(this, "OnClick");
		custom_entry.OnMouseIn = on_hover;
		custom_entry.OnMouseOut = on_hover_stop;
	}
	entries[ID - 1] = [target, command, parameter, user_ID];
	this[Format("menuChild%d", ID)] = custom_entry;
	
	return custom_entry;
}

func DoCall(int ID, command, proplist target, bool noclose, int player)
{
	var self = this; // safety
	var entry = entries[ID - 1];
	target = target ?? entry[0];
	// target removed? safety first!
	if (target)
	{
		if (target->Call(command ?? entry[1], entry[2], entry[3], player) == -1) return;
	}
	Log("self: %v, noclose: %d, permanent: %d", self, noclose, permanent);
	if (self)
	if (!noclose && !permanent)
		Close();
}

func OnClick(int player, int ID, int subwindowID, object target, data)
{
	DoCall(subwindowID, nil, nil, nil, player);	
}

func DoCallback(int player, int ID, int subwindowID, object target, data)
{
	DoCall(subwindowID, data[1], data[0], true, player);
}

func Open()
{
	menu_id = CustomMenuOpen(this);
	return menu_id;
}