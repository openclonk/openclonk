/**
	Homebase

	Manage buyable stuff and technology upgrades in Gidl

	@authors Sven2
*/

local buy_menu;
local base_material; // array of base material entries
local last_buy_idx;
local techs, requirement_names;

local is_selling; // temp to prevent recursion from object removal

// Technology fields - queried by objects using them
local tech_load_speed_multiplier = 100;

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
	requirement_names = {};
	last_buy_idx = -1;
	// Buy menu
	buy_menu = CreateObject(GUI_BuyMenu, 0,0, GetOwner());
	buy_menu->SetHomebase(this);
	// Initial availability of items
	AddCaption("$Weapons$");
	AddHomebaseItem(new ITEMTYPE_Weapon     { item = Bow,                  ammo = Arrow, desc = "$DescBow$" });
	AddHomebaseItem(new ITEMTYPE_Weapon     { item = Sword,     cost = 25 });
	AddHomebaseItem(new ITEMTYPE_Consumable { item = Firestone, cost = 5});
	AddHomebaseItem(new ITEMTYPE_Weapon     { item = Musket,    cost = 50, ammo = LeadShot, desc = "$DescMusket$",     requirements = ["AdvancedWeapons"] });
	AddHomebaseItem(new ITEMTYPE_Consumable { item = IronBomb,  cost = 15,                                             requirements = ["AdvancedWeapons"] });
	AddHomebaseItem(new ITEMTYPE_Consumable { item = DynamiteBox,cost = 15,                                            requirements = ["AdvancedWeapons"] });
	AddHomebaseItem(new ITEMTYPE_Weapon     { item = GrenadeLauncher, ammo = IronBomb, desc = "$DescGrenadeLauncher$", requirements = ["MasterWeapons"] });
	
	AddCaption("$Items$");
	AddHomebaseItem(new ITEMTYPE_Consumable { item = Bread,     cost = 5  });
	AddHomebaseItem(new ITEMTYPE_Weapon { item = Hammer,    cost = 1000, desc = "$DescHammer$", extra_width = 1 });
	
	AddCaption("$Technology$");
	AddHomebaseItem(new ITEMTYPE_Technology { name="$AdvancedWeapons$", item = Icon_World,cost = 100, desc="$DescAdvancedWeapons$", tech = "AdvancedWeapons" });
	AddHomebaseItem(new ITEMTYPE_Technology { name="$MasterWeapons$", item = Icon_World,cost = 1000, desc = "$DescMasterWeapons$", tech = "MasterWeapons", requirements = ["AdvancedWeapons"] });
	
	AddCaption("$Upgrades$");
	AddHomebaseItem(new ITEMTYPE_Technology { name="$LoadSpeed$", item = Homebase_Icon, graphics="LoadSpeed%d", costs = [100, 500, 1000], desc = "$DescLoadSpeed$", tech = "LoadSpeed", tiers=3 });
	AddCaption("$Artifacts$");

	// Buy menu always open (but hidden at start)
	buy_menu->Open();
	return true;
}

public func Destruction()
{
	if (buy_menu) buy_menu->RemoveObject();
	return true;
}

public func AddCaption(string title, array requirements)
{
	return AddHomebaseItem({is_caption = true, title=title, requirements=requirements});
}

public func AddHomebaseItem(proplist entry)
{
	// Default name and description
	if (entry.item)
	{
		if (!entry.name) entry.name = entry.item.Name;
		if (!entry.desc) entry.desc = entry.item.Description;
	}
	// Remember tech name mapping
	if (entry.tech) requirement_names[entry.tech] = entry.name;
	// Add to end of list
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
		if (entry.tiers)
		{
			var tier = techs[entry.tech];
			entry.graphic = Format(entry.graphics, tier+1);
			entry.cost = entry.costs[tier];
		}
		if (entry.is_caption)
			return buy_menu->UpdateCaption(entry.title, available, entry, index);
		else
			return buy_menu->UpdateBuyEntry(entry.item, available, entry, index, index == last_buy_idx, tier);
	}
	return false;
}

public func GetEntryInformation(int entry_idx)
{
	// Fill with current information for this entry
	var entry = base_material[entry_idx];
	var msg = "";
	if (entry.requirements)
	{
		var req_str = "";
		var req_sep = "";
		for (var req in entry.requirements)
		{
			var clr = 0xff00;
			if (!techs[req]) clr = 0xff0000;
			req_str = Format("%s%s<c %x>%s</c>", req_str, req_sep, clr, requirement_names[req]);
			req_sep = ", ";
		}
		msg = Format("$Requirements$: %s|", req_str);
	}
	else
	{
		msg = "";
	}
	entry.message = msg;
	return entry;
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
	// Buy only once? (all technologies without further upgrade tiers)
	if (entry.remove_after_buy)
	{
		entry.hidden = true;
		if (buy_menu) buy_menu->RemoveItem(callback_idx);
	}
	else if (!entry.tech)
	{
		// Remember what has been bought (except for multi-tier tech)
		if (last_buy_idx != callback_idx)
		{
			var last_last_buy_idx = last_buy_idx;
			last_buy_idx = callback_idx;
			if (last_last_buy_idx >= 0) UpdateIndexedItem(last_last_buy_idx);
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
	// Reach next tier
	var tier = techs[entry.tech] + 1;
	techs[entry.tech] = tier;
	// For multi-tier-technologies, remove entry when last tier has been reached
	if (entry.tiers) entry.remove_after_buy = (tier == entry.tiers);
	// Technology gain callback.
	Call(Format("~Gain%s", entry.tech), entry, tier);
	// Update any related techs that may become available
	var n = GetLength(base_material), req;
	for (var i=0; i<n; ++i)
		if (req = base_material[i].requirements)
			if (GetIndexOf(req, entry.tech) >= 0)
				UpdateIndexedItem(i);
	return true;
}

private func GainAdvancedWeapons(proplist entry, int tier)
{
	// All done by requirements
	return true;
}

private func GainLoadSpeed(proplist entry, int tier)
{
	// Increase player's load speed
	tech_load_speed_multiplier = [100, 40, 20, 1][tier];
	// Update all current weapons
	for (var weapon in FindObjects(Find_Owner(GetOwner()), Find_Func("Gidl_IsRangedWeapon")))
		weapon->Gidl_UpdateLoadTimes();
	return true;
}


public func Definition(def)
{
	// Arrays in static const are broken
	// So init them here
	g_quickbuy_items = [Hammer, Bow, Sword, Musket, GrenadeLauncher, nil, Firestone, IronBomb, nil, nil];
}
