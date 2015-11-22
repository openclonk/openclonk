/**
	Homebase

	Manage buyable stuff and technology upgrades in Gidl

	@authors Sven2
*/

local buy_menu;
local base_material; // array of base material entries
local last_buy_idx;
local techs;

local is_selling; // temp to prevent recursion from object removal

static g_quickbuy_items;

// Types for purchasable stuff
local ITEMTYPE_Weapon = {
	free_rebuy = true,
	stack_count = -1
};
local ITEMTYPE_Consumable = {
	auto_rebuy = true
};
local ITEMTYPE_Technology = {
	remove_after_buy = true,
	callback = "GainTechnology",
	extra_width = 2,
};

/* Creation / Destruction */

public func Construction(...)
{
	base_material = [];
	techs = {};
	last_buy_idx = -1;
	// Buy menu
	buy_menu = CreateObject(GUI_BuyMenu, 0,0, GetOwner());
	buy_menu->SetHomebase(this);
	// Initial availability of items
	AddCaption("$Weapons$");
	AddHomebaseItem(new ITEMTYPE_Weapon     { item = Bow,                  ammo = Arrow });
	AddHomebaseItem(new ITEMTYPE_Weapon     { item = Sword,     cost = 25 });
	AddHomebaseItem(new ITEMTYPE_Consumable { item = Firestone, cost = 5  });
	AddHomebaseItem(new ITEMTYPE_Weapon     { item = Musket,    cost = 50, ammo = LeadShot, requirements = ["AdvancedWeapons"] });
	AddHomebaseItem(new ITEMTYPE_Consumable { item = IronBomb,  cost = 15,  requirements = ["AdvancedWeapons"] });
	AddHomebaseItem(new ITEMTYPE_Consumable { item = DynamiteBox,cost = 15,  requirements = ["AdvancedWeapons"] });
	AddHomebaseItem(new ITEMTYPE_Weapon     { item = GrenadeLauncher, ammo = IronBomb, requirements = ["MasterWeapons"] });
	
	AddCaption("$Items$");
	AddHomebaseItem(new ITEMTYPE_Consumable { item = Bread,     cost = 5  });
	AddHomebaseItem(new ITEMTYPE_Weapon { item = Hammer,    cost = 1000, extra_width = 1 });
	
	AddCaption("$Technology$");
	AddHomebaseItem(new ITEMTYPE_Technology { item = Icon_World,cost = 100, tech = "AdvancedWeapons" });
	AddHomebaseItem(new ITEMTYPE_Technology { item = Icon_World,cost = 1000, tech = "MasterWeapons", requirements = ["AdvancedWeapons"] });
	
	AddCaption("$Upgrades$");
	AddCaption("$Artifacts$");

	// Buy menu always open (but hidden at start)
	buy_menu->Open();
	return true;
}

public func AddCaption(string title, array requirements)
{
	return AddHomebaseItem({is_caption = true, title=title, requirements=requirements});
}

public func AddHomebaseItem(proplist entry)
{
	var idx = GetLength(base_material);
	base_material[idx] = entry;
	var quickbuy_idx = GetIndexOf(g_quickbuy_items, entry.item);
	if (quickbuy_idx >= 0) entry.hotkey = GetPlayerControlAssignment(GetOwner(), CON_QuickBuy0+quickbuy_idx, true);
	UpdateIndexedItem(idx);
	return entry;
}

public func UpdateIndexedItem(int index)
{
	if (index >= 0 && buy_menu)
	{
		var entry = base_material[index];
		if (entry.hidden) return true;
		var available = true;
		if (entry.requirements)
			for (var req in entry.requirements)
				if (!techs[req])
					available = false;
		if (entry.is_caption)
			return buy_menu->UpdateCaption(entry.title, available, index);
		else
			return buy_menu->UpdateBuyEntry(entry.item, available, entry.cost, index, index == last_buy_idx, entry.extra_width, entry.hotkey);
	}
	return false;
}

public func OnBuySelection(int callback_idx)
{
	// Buy directly into cursor
	var plr = GetOwner();
	var cursor = GetCursor(plr);
	if (!cursor) return false;
	// Safety
	var entry = base_material[callback_idx];
	if (!entry) return false;
	// Ignore if already have
	var last_item = cursor->Contents(), item;
	if (!last_item || (last_item->GetID() != entry.item))
	{
		// Requirements
		if (entry.requirements)
			for (var req in entry.requirements)
				if (!techs[req])
					return false;
		// Cost
		if (entry.cost)
		{
			if (GetWealth(plr) < entry.cost) return false;
			DoWealth(plr, -entry.cost);
			Sound("Cash", true, nil, plr);
			// Some items cost only once
			if (entry.free_rebuy) entry.cost = nil;
		}
		else
		{
			// Still some feedback even on free selections
			Sound("Waterdrop1", true, nil, plr);
		}
		// Technology?
		if (entry.callback)
		{
			// Teach technology
			if (!Call(entry.callback, entry)) return false;
		}
		else
		{
			// Regular item: Buy into inventory
			// Get rid of current item (unless it's the same we already want)
			var item;
			if (last_item) SellItem(last_item);
			// Create item
			if (!item) item = cursor->CreateContents(entry.item);
			if (!item) return false;
			// for later sale
			item.GidlValue = entry.cost;
			// ammo up!
			if (entry.ammo)
			{
				var ammo = item->CreateContents(entry.ammo);
				if (!ammo) return false;
				var stack_count = entry.stack_count;
				if (stack_count < 0)
					ammo->~SetInfiniteStackCount();
				else if (stack_count > 0)
					ammo->SetStackCount(stack_count);
			}
		}
	}
	// Buy only once? (all technologies)
	if (entry.remove_after_buy)
	{
		entry.hidden = true;
		if (buy_menu) buy_menu->RemoveItem(callback_idx);
	}
	else
	{
		// Remember what has been bought
		if (last_buy_idx != callback_idx)
		{
			var last_last_buy_idx = last_buy_idx;
			last_buy_idx = callback_idx;
			UpdateIndexedItem(last_last_buy_idx);
			UpdateIndexedItem(last_buy_idx);
		}
	}
	return true;
}

public func SellItem(item)
{
	// Cash!
	// Use custom value assigned by buy menu only
	if (item.GidlValue)
	{
		DoWealth(GetOwner(), item.GidlValue);
		Sound("Cash", true, nil, GetOwner());
	}
	is_selling = true; // no item re-buy during sale
	var success = item->RemoveObject();
	is_selling = false;
	return success;
}

public func OnOwnerChanged(new_owner)
{
	if (buy_menu) buy_menu->SetOwner(new_owner);
	return true;
}

// Callback from clonk or weapon: Ammo has been used up.
// Recharge from selected home base item if that option has been enabled
public func OnNoAmmo(object clonk)
{
	if (is_selling) return false;
	if (last_buy_idx < 0) return false;
	var entry = base_material[last_buy_idx];
	if (entry && entry.auto_rebuy)
	{
		ScheduleCall(this, this.OnBuySelection, 1, 1, last_buy_idx);
	}
	return false;
}

public func QuickBuyItem(id item)
{
	// Find item in buy list
	var entry, i=0;
	for (entry in base_material)
		if (entry.item == item)
			break;
		else
			++i;
	// If found, try to buy it
	if (!OnBuySelection(i))
	{
		// TODO: Error sound
		return false;
	}
	return true;
}

private func GainTechnology(proplist entry)
{
	techs[entry.tech] = true;
	Call(Format("~Gain%s", entry.tech));
	// Update any related techs that may become available
	var n = GetLength(base_material), req;
	for (var i=0; i<n; ++i)
		if (req = base_material[i].requirements)
			if (GetIndexOf(req, entry.tech) >= 0)
				UpdateIndexedItem(i);
	return true;
}

private func GainAdvancedWeapons(proplist entry)
{
	// All done by requirements
}

public func Definition(def)
{
	// Arrays in static const are broken
	// So init them here
	g_quickbuy_items = [Hammer, Bow, Sword, Musket, GrenadeLauncher, nil, Firestone, IronBomb, nil, nil];
}
