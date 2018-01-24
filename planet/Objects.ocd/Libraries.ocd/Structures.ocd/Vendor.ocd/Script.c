/**
	Vendor Library
	Basic vendor library for structures, handles:
	 * Selling objects in the interaction menu.
	 * Buying objects in the interaction menu.
	
	@author Randrian, Marky (buy and sell logic), Maikel (menu)
*/

 // Proplist that avoids clashes in variables.
local lib_vendor = {};


/*-- Overloadable: Buying & Selling --*/

// These functions can be overloaded by more specific vendors, if needed.

public func AllowBuyMenuEntries() { return true; }

// Returns the value of the object if bought in this base.
public func GetBuyValue(id item)
{
	// By default call the engine function.
	return item->GetValue();
}

public func GetBuyableItems(int for_player)
{
	// By default get the base material.
	var i, item;
	var items = [];
	while (item = GetBaseMaterial(for_player, nil, i++))
	{
		PushBack(items, item);
	}
	return items;
}

public func GetBuyableAmount(int for_player, id item)
{
	// by default use the base material.
	return GetBaseMaterial(for_player, item);
}

public func ChangeBuyableAmount(int for_player, id item, int amount)
{
	// By default use base engine function.
	DoBaseMaterial(for_player, item, amount);
}

// Returns the value of the object if sold in this base.
public func GetSellValue(object item)
{
	// By default call the engine function.
	return item->GetValue();
}


/*-- Internal functions --*/

// Tries to buy an object or all available objects for buy_all_available == true.
// Returns the last bought object.
public func DoBuy(id item, int for_player, int wealth_player, object buyer, bool buy_all_available, bool error_sound)
{
	var num_available = this->GetBuyableAmount(wealth_player, item);
	if (!num_available)
		return;
	var num_buy = 1;
	var purchased = nil;
	if (buy_all_available)
		num_buy = num_available;
	while (num_buy--)
	{
		var price = this->GetBuyValue(item);
		// Does the player have enough money?
		if (price > GetWealth(wealth_player))
		{
			if (error_sound)
				Sound("UI::Error", {player = for_player});
			break;
		}
		// Take the cash.
		DoWealth(wealth_player, -price);
		Sound("UI::UnCash?", {player = for_player});
		// Decrease the base material, allow runtime overload.
		this->ChangeBuyableAmount(wealth_player, item, -1);
		// Deliver the object.
		purchased = CreateContents(item);
		purchased->SetOwner(for_player);
		if (purchased->GetOCF() & OCF_CrewMember)
			purchased->MakeCrewMember(for_player);
		if (purchased->GetOCF() & OCF_Collectible)
			buyer->Collect(purchased);
	}
	return purchased;
}

// Sells the object and transfers the wealth to the selling player.
public func DoSell(object obj, int wealth_player)
{
	if (obj->~QueryOnSell(wealth_player, this))
		return false;

	// Sell contents first.
	for (var contents in FindObjects(Find_Container(obj)))
	{
		DoSell(contents, wealth_player);
	}

	// Give the player the cash.
	DoWealth(wealth_player, this->GetSellValue(obj));
	Sound("UI::Cash", {player = wealth_player});
	
	// Add the item to the homebase material.
	if (!obj->~QueryRebuy(wealth_player, this))
	{
		this->ChangeBuyableAmount(wealth_player, obj->GetID(), +1);
	}

	// OnSale callback to object e.g. for goal updates.
	obj->~OnSale(wealth_player, this);

	// Remove object, but eject contents
	// ejecting contents may be important, because those contents
	// that return true in QueryOnSell are still in the object
	// and they should not be removed (why else would they have QueryOnSell)?
	if (obj)
		obj->RemoveObject(true);
	return true;
}


public func IsVendor()
{
	return lib_vendor.is_vendor;
}

// Makes this building a vendor or removes the base functionallity.
public func MakeVendor(bool should_be_vendor)
{
	if (should_be_vendor)
	{
		if (!lib_vendor.is_vendor)
			lib_vendor.is_vendor = AddEffect("IntVendor", this, 1, 10, this);
	}
	else
	{
		if (lib_vendor.is_vendor)
			RemoveEffect(nil, nil, lib_vendor.vendor);

		lib_vendor.is_vendor = nil;
	}
}

public func FxIntVendorTimer(object target, proplist effect, int time)
{
	// Search all objects for objects that want to be sold automatically.
	for (var item in FindObjects(Find_Container(this), Find_Func("AutoSell")))
		Sell(item->GetOwner(), item, this);
}


/*-- Menus --*/

// Provides an interaction menu for buying things.
public func HasInteractionMenu() { return true; }

public func GetInteractionMenus(object clonk)
{
	var menus = _inherited(clonk, ...) ?? [];
	// only open the menus if ready
	if (this->AllowBuyMenuEntries())
	{
		var buy_menu =
		{
			title = "$MsgBuy$",
			entries_callback = this.GetBuyMenuEntries,
			callback = "OnBuyMenuSelection",
			callback_target = this,
			BackgroundColor = RGB(50, 50, 0),
			Priority = 20
		};
		PushBack(menus, buy_menu);
	}
	
	return menus;
}

public func GetBuyMenuEntry(int index, id item, int amount, int value)
{
	var entry = 
	{
		Right = "2em", Bottom = "3em",
		BackgroundColor = {Std = 0, OnHover = 0x50ff0000},
		image = {Bottom = "2em", Style = GUI_TextBottom | GUI_TextRight},
		price = {Style = GUI_TextBottom | GUI_TextRight, Priority = 3}
	};
	entry.image.Symbol = item;
	entry.image.Text = Format("%dx", amount);
	entry.price.Text = Format("<c ffff00>%d{{Icon_Wealth}}</c>", value);
	// Order by value and then by BaseMaterial index.
	entry.Priority = 1000 * value + index;
	return entry;
}

public func GetBuyMenuEntries(object clonk)
{
	// We need to know when exactly we should refresh the menu to prevent unecessary refreshs.
	var lowest_greyed_out_price = nil;

	// Distinguish owners here. at the moment they are the same, but this may change.
	var wealth_player = GetOwner();
	var for_player = GetOwner();

	var wealth = GetWealth(wealth_player); 
	var menu_entries = [];
	var index = 0, item, amount;
	
	for (item in this->GetBuyableItems(for_player))
	{
		amount = this->GetBuyableAmount(for_player, item);
		var value = this->GetBuyValue(item);
		var entry = GetBuyMenuEntry(index++, item, amount, value);
		// If the player can't afford it, the item (except for the price) is overlayed by a greyish color.
		if (value > wealth)
		{
			entry.overlay = {Priority = 2, BackgroundColor = RGBa(50, 50, 50, 150)};
			if (lowest_greyed_out_price == nil || value < lowest_greyed_out_price)
				lowest_greyed_out_price = value;
		}
		PushBack(menu_entries, {symbol = item, extra_data = nil, custom = entry});
	}
	
	// At the top of the menu, we add the player's wealth.
	var entry = 
	{
		Bottom = "1.1em",
		BackgroundColor = RGBa(100, 100, 50, 100),
		Priority = -1,
		left_text =
		{
			Style = GUI_TextVCenter | GUI_TextLeft,
			Text = "<c 888888>$YourWealth$:</c>"
		},
		right_text = 
		{
			Style = GUI_TextVCenter | GUI_TextRight, 
			Text = Format("<c ffff00>%d{{Icon_Wealth}}</c>", wealth)
		}
	};
	var fx = AddEffect("UpdateWealthDisplay", this, 1, 5, nil, GetID());
	fx.lowest_greyed_out_price = lowest_greyed_out_price;
	fx.last_wealth = wealth;
	fx.wealth_player = wealth_player;
	PushBack(menu_entries, {symbol = nil, extra_data = nil, custom = entry, fx = fx});
	return menu_entries;
}

public func OnBuyMenuSelection(id def, extra_data, object clonk)
{
	if (!def)
		return;
	// Distinguish owners.
	var wealth_player = GetOwner();
	var for_player = clonk->GetController();
	// TODO: uncomment the next line for buy all on Shift + MouseClick. Needs testing so after release.
	var buy_all_available = false; //GetPlayerControlState(for_player, CON_ModifierMenu1) != 0;
	// Buy.
	DoBuy(def, for_player, wealth_player, clonk, buy_all_available, true);
	// Excess objects exit flag (can't get them out...).
	EjectAllContents();
	UpdateInteractionMenus(this.GetBuyMenuEntries);
}

private func EjectAllContents()
{
	var i = ContentsCount();
	var obj;
	while (i--) 
		if (obj = Contents(i))
			EjectContents(obj);
}

private func EjectContents(object contents)
{
	contents->Exit(0, GetDefHeight() / 2);
	// newly bought items do not fade out until they've been collected once
	if (contents && ObjectCount(Find_ID(Rule_ObjectFade)) && !contents.HasNoFadeOut)
	{
		contents.HasNoFadeOut = this.BuyItem_HasNoFadeout;
		contents.BuyOverload_Entrance = contents.Entrance;
		contents.Entrance = this.BuyItem_Entrance;
	}
}


/*-- Menu Updates --*/

private func FxUpdateWealthDisplayTimer(object target, effect fx, int time)
{
	if (!fx.menu_target) return FX_Execute_Kill;
	if (fx.last_wealth == GetWealth(fx.wealth_player)) return FX_OK;
	fx.last_wealth = GetWealth(fx.wealth_player);
	// Do we need a full refresh? New objects might have become available.
	if (fx.lowest_greyed_out_price && fx.lowest_greyed_out_price <= fx.last_wealth)
	{
		target->UpdateInteractionMenus(target.GetBuyMenuEntries);
		return FX_OK;
	}
	// Just update the money display otherwise.
	GuiUpdate({right_text = {Text = Format("<c ffff00>%d{{Icon_Wealth}}</c>", fx.last_wealth)}}, fx.main_ID, fx.ID, fx.menu_target);
	return FX_OK;
}

public func FxUpdateWealthDisplayOnMenuOpened(object target, effect fx, int main_ID, int ID, object subwindow_target)
{
	fx.main_ID = main_ID;
	fx.menu_target = subwindow_target;
	fx.ID = ID;
}

// Newly bought items do not fade out unless collected.
public func BuyItem_HasNoFadeout() { return true; }

public func BuyItem_Entrance()
{
	// After first collection, fade out rule should be effective again.
	var overloaded_fn = this.BuyOverload_Entrance;
	this.HasNoFadeOut = nil;
	this.BuyOverload_Entrance = nil;
	this.Entrance = overloaded_fn;
	if (overloaded_fn)
		return Call(overloaded_fn, ...);
}
