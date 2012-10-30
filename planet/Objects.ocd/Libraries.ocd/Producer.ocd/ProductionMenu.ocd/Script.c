/**
	Production Menu, it consists of two parts: a circle with possible products
	and a rectangle with the current queue.
	
	@author Maikel
*/

#include GUI_CircleMenu

local menu_queue;
local productinfo_shown;

/** Creates a production menu for the calling object. This is supposed to be a crew member, 
	controlled by a player.
	@param producer the producer for which to create the production menu.
	@return a pointer to the created menu, or \c nil if failed.
*/
global func CreateProductionMenu(object producer)
{
	// Safety checks.
	if (!this) return;
	if (!(GetOCF() & OCF_CrewMember)) return;
	if (!(this->~HasMenuControl())) return;

	// Create the menu controller.
	var controller = CreateObject(Library_ProductionMenu);
	controller->SetMenuObject(this);
	this->SetMenu(controller);
	controller->SetCommander(producer);
	
	// Add all products to the menu (circle).
	controller->AddMenuProducts(producer);
	
	// Add currennt queue to the menu (rectangle).
	controller->AddMenuQueue(producer);
	
	// Show and return production menu.
	controller->Show();
	return controller;
}

protected func Construction()
{
	_inherited(...);
	menu_queue = [];
	SetPosition(600, 400);
	// Production menus are no drag targets.
	this.MouseDrag = MD_NoClick;
	return;
}

public func AddMenuProducts(object producer)
{
	for (var product in producer->GetProducts(GetMenuObject()))
	{
		var item = CreateObject(GUI_MenuItem);
		if (!AddItem(item))
			return item->RemoveObject();
		item->SetSymbol(product);
		item.MouseDrag = MD_DragSource;
	}
	return;
}

public func AddMenuQueue(object producer)
{
	for (var qitem in producer->GetQueue())
	{
		var item = CreateObject(GUI_MenuItem);
		if (!AddQueueItem(item))
			return item->RemoveObject();
		item->SetSymbol(qitem.Product);
		item->SetCount(qitem.Amount);
	}
	UpdateQueueMenu();
	return;
}

public func UpdateMenuQueue(object producer)
{
	for (var qitem in menu_queue)
		qitem->RemoveObject();
	SetLength(menu_queue, 0);
	
	AddMenuQueue(producer);	
	return;	
}

public func ShowProductInfo(object item)
{
	var product_id = item->GetSymbol();
	var costs = menu_commander->ProductionCosts(product_id);
	var cost_msg = "@";
	var liquid, material;
	for (var comp in costs)
		cost_msg = Format("%s %dx {{%i}}", cost_msg, comp[1], comp[0]);
	if (menu_commander->FuelNeed(product_id))
		cost_msg = Format("%s 1x {{Icon_Producer_Fuel}}", cost_msg);
	if (liquid = menu_commander->LiquidNeed(product_id))
		cost_msg = Format("%s %dx {{Icon_Producer_%s}}", cost_msg, liquid[1], liquid[0]);
	if (material = menu_commander->MaterialNeed(product_id))
		cost_msg = Format("%s %dx {{%i}}", cost_msg, material[1], product_id->~GetMaterialIcon(material[0]));
	if (menu_commander->PowerNeed(product_id))
		cost_msg = Format("%s + {{Library_PowerConsumer}}", cost_msg);

	CustomMessage(cost_msg, this, GetOwner(), 250, 270, nil, nil, nil, 1);
	productinfo_shown = item;
	return;
}

public func HideProductInfo()
{
	CustomMessage("", this, GetOwner());
	productinfo_shown = false;
}

/* Menu properties */

public func IsProductionMenu() { return true; }

public func AddQueueItem(object item)
{
	var queue_cnt = GetLength(menu_queue);
	// Create new menu item.
	menu_queue[queue_cnt] = item;	
	item->SetMenu(this);
	item->SetOwner(GetOwner());
	// Set item visibility.
	item.Visibility = VIS_None;
	if (menu_shown)
		item.Visibility = VIS_Owner;
	return true;
}

public func UpdateQueueMenu()
{
	// Safety: check for queue items.
	var queue_cnt = GetLength(menu_queue);
	var x = GetX();
	var y = GetY();
	for (var i = 0; i < queue_cnt; i++)
	{
		var pos = 2 * i * MENU_Radius / 3 + 200;
		var qitem = menu_queue[i];
		if (qitem)
		{
			qitem->SetSize(200 * MENU_Radius / 3 / 96);
			qitem->SetPosition(x + pos, y - 80);
		}
	}	
	return;
}

public func Close() 
{
	if(menu_object)
		menu_object->~MenuClosed(this);
	RemoveObject();
}

public func Show()
{
	// Change visibility.
	for (var qitem in menu_queue)
		if (qitem)
			qitem.Visibility = VIS_Owner;
	return _inherited(...);
}

public func Hide()
{
	// Change visibility.
	for (var qitem in menu_queue)
		if (qitem)
			qitem.Visibility = VIS_None;
	return _inherited(...);
}

// Engine callback: if the menu is destroyed, the items must follow.
protected func Destruction()
{
	for (var i = 0; i < GetLength(menu_queue); i++)
		if (menu_queue[i])
			menu_queue[i]->RemoveObject();
	return _inherited(...);
}

private func IsProductItem(object item)
{
	for (var test in menu_items)
		if (test == item)
			return true;
	return false;
}

private func IsQueueItem(object item)
{
	for (var test in menu_queue)
		if (test == item)
			return true;
	return false;
}

private func GetQueueIndex(object item)
{
	var index =  0;
	for (index = 0; index < GetLength(menu_queue); index++)
		if (menu_queue[index] == item)
			break;
	return index;
}

public func HasCommander(object producer)
{
	if (menu_commander == producer)
		return true;
	return false;
}

/* Callbacks from the menu items, to be translated into commands for the producer. */

// Called when a click outside the menu has been made.
public func OnMouseClick(int x, int y, bool alt)
{
	// Close menu if not clicked on one of the menus.
	if (!(Distance(x, y, 0, 0) < 160 || (x > 160 && x < 480 && Abs(y) < 160)))
		Close();
	return;
}

// Called when an item has been selected (left mouse button).
public func OnItemSelection(object item)
{
	// If item is from products, add one to queue.
	if (IsProductItem(item))
	{
		menu_commander->AddToQueue(item->GetSymbol(), 1);
	}
	// If item is from queue, remove one from queue.
	if (IsQueueItem(item))
	{
		menu_commander->RemoveFromQueue(GetQueueIndex(item), 1);
	}	
	return _inherited(item, ...);
}

// Called when an item has been selected (right mouse button).
public func OnItemSelectionAlt(object item)
{
	// Show the production cost of the selected production item.
	if (IsProductItem(item))
	{
		// Nothing to do here?
	}
	// Show production status of the selected queue item.
	if (IsQueueItem(item))
	{
		// TODO: Implement if considered necessary.	
	}
	return _inherited(item, ...);
}

// Called when an object is dragged onto the menu
public func OnMouseDrop(int plr, obj)
{
	return _inherited(plr, obj, ...);
}

// Called when another item has been dropped on an item in this menu.
public func OnItemDropped(object drop_item, object on_item)
{
	// This will only be called from queue items, since product items are only drag sources.
	// If the dropped item is a product: insert product into queue.
	if (IsProductItem(drop_item))
	{
		menu_commander->InsertIntoQueue(drop_item->GetSymbol(), 1, GetQueueIndex(on_item));
	}
	// If the dropped item is in the queue: shift item around in the queue.
	if (IsQueueItem(drop_item))
	{
		// TODO: Fix me, then uncomment.
		//var index = GetQueueIndex(on_item);
		//var amount = drop_item->GetCount();
		//menu_commander->InsertIntoQueue(drop_item->GetSymbol(), amount, index);
		//menu_commander->RemoveFromQueue(index - 1, amount);
	}
	
	return _inherited(drop_item, on_item, ...);
}

// Called after an item from this menu has been dragged onto an item
public func OnItemDragDone(object drag_item, object on_item)
{
	return _inherited(drag_item, on_item, ...);
}

// Called if the mouse cursor enters hovering over an item.
public func OnMouseOverItem(object over_item, object dragged_item)
{
	ShowProductInfo(over_item);
	
	return _inherited(over_item, dragged_item, ...);
}

// Called if the mouse cursor exits hovering over an item.
public func OnMouseOutItem(object out_item, object dragged_item)
{
	HideProductInfo();
	
	return _inherited(out_item, dragged_item, ...);
}

