/**
	Default
	This is a simple stand-alone menu that can be used to present arbitrary items to the player.
	
	Internally, a list-style menu is used.
*/


local Name = "Default Menu";
local menu_id = nil;
local on_mouse_over_callback = nil;
local on_mouse_out_callback = nil;


func Construction()
{
	_inherited(...);
	this.topic = 
	{
		Bottom = "1.5em",
		BackgroundColor = RGB(50, 50, 50),
		Style = GUI_TextHCenter | GUI_TextVCenter,
		ID = 2,
		Target = this
		// Text = nil, <- will be set with SetTitle
	};
	this.container = 
	{
		Top = "3em",
		list_submenu = CreateObject(MenuStyle_List, nil, nil, NO_OWNER),
		description_submenu = 
		{
			ID = 5,
			Target = this,
			Left = "50%",
			textbox = {Top = "6em"},
			headline = {Top = "4.5em", Bottom = "5.5em", BackgroundColor = RGB(50, 50, 50), Style = GUI_TextVCenter | GUI_TextHCenter},
			icon = {Bottom = "4em"}
		}
	};
	this.Target = this;
	this.Visibility = VIS_Owner;
	this.BackgroundColor = RGB(1, 1, 1);
	this.Decoration = GUI_MenuDeco;
	this.container.list_submenu.Right = "50%";
	
	// Hook into MouseIn/Out of the list menu to be able to show a description.
	this.container.list_submenu->SetMouseOverCallback(this, "OnMouseOverListMenu");
	this.container.list_submenu->SetMouseOutCallback(this, "OnMouseOutListMenu");
}

/* Updates the description box on the right side on mouse-over. */
func UpdateDescription(id obj)
{
	if (!menu_id) return;
	
	var symbol = nil, text = nil, headline = nil;
	if (obj)
	{
		headline = obj->GetName();
		text = obj.Description;
		symbol = obj;
	}
	
	var update =
	{
		headline = {Text = headline},
		textbox = {Text = text},
		icon = {Symbol = symbol}
	};
	GuiUpdate(update, menu_id, 5, this);
}

/* Sets the headline of the menu. This can also be called after the menu had been opened. */
public func SetTitle(string text)
{
	this.topic.Text = text;
	
	if (menu_id) // already opened?
		GuiUpdateText(text, menu_id, 2, this);
}

/* Opens the menu as a stand-alone menu. Don't forget to call clonk->SetMenu(menu) first, if it's for a clonk. */
public func Open()
{
	menu_id = GuiOpen(this);
	return menu_id;
}

/* Closes the menu. */
public func Close()
{
	// The list-style submenu cares about removing its menu and executing potential callbacks.
	RemoveObject();
}

/* We need to have a wrapper around the MouseIn/Out callbacks, as we want to show a description. */
func SetMouseOverCallback(proplist target, callback)
{
	on_mouse_over_callback = [target, callback];
}

func SetMouseOutCallback(proplist target, callback)
{
	on_mouse_out_callback = [target, callback];
}

func OnMouseOverListMenu(parameter, int user_ID, int player)
{
	if (on_mouse_over_callback)
		on_mouse_over_callback[0]->Call(on_mouse_over_callback[1], parameter, user_ID, player);
	UpdateDescription(parameter);
}

func OnMouseOutListMenu(parameter, int user_ID, int player)
{
	if (on_mouse_out_callback)
		on_mouse_out_callback[0]->Call(on_mouse_out_callback[1], parameter, user_ID, player);
}

/* This mimics the interface of the list-style menu. */
public func SetPermanent(...) { return this.container.list_submenu->SetPermanent(...); }
public func SetFitChildren(...) { return this.container.list_submenu->SetFitChildren(...); }
public func SetCloseCallback(...) { return this.container.list_submenu->SetCloseCallback(...); }
public func AddItem(...) { return this.container.list_submenu->AddItem(...); }
public func UpdateItem(...) { return this.container.list_submenu->UpdateItem(...); }
public func RemoveItem(...) { return this.container.list_submenu->RemoveItem(...); }

