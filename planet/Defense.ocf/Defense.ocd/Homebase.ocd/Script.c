/**
	Homebase

	Manage buyable stuff and technology upgrades in Gidl

	@authors Sven2
*/

static g_homebases;

local buy_menu;
local base_material; // array of base material entries
local last_buy_idx;
local techs, requirement_names;

local is_selling; // temp to prevent recursion from object removal

// Technology fields - queried by objects using them
local tech_load_speed_multiplier = 100;
local tech_shooting_strength_multiplier = 0;
local tech_life = 1;

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
	// Manage pointers
	if (GetOwner() < 0) FatalError("Invalid Homebase owner");
	if (!g_homebases) g_homebases = [];
	if (g_homebases[GetOwner()]) g_homebases[GetOwner()]->RemoveObject(); // remove old (shouldn't be here)
	g_homebases[GetOwner()] = this;
	// Init
	base_material = [];
	techs = {};
	requirement_names = {};
	last_buy_idx = -1;
	// Buy menu
	buy_menu = CreateObject(GUI_BuyMenu, 0, 0, GetOwner());
	buy_menu->SetHomebase(this);
	// Get available items
	GameCall("FillHomebase", this);

	// Buy menu always open (but hidden at start)
	buy_menu->Open();
	return true;
}

public func Destruction()
{
	if (buy_menu) buy_menu->RemoveObject();
	return true;
}

public func SetQuickbuyItems(array list)
{
	g_quickbuy_items = list;
}

public func AddCaption(string title, array requirements)
{
	return AddHomebaseItem({is_caption = true, title = title, requirements = requirements});
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
	if (quickbuy_idx >= 0) entry.hotkey = GetPlayerControlAssignment(GetOwner(), CON_QuickBuy0 + quickbuy_idx, true);
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
		var tier;
		if (entry.tiers)
		{
			tier = techs[entry.tech];
			entry.graphic = Format(entry.graphics, tier + 1);
			entry.cost = entry.costs[tier];
		}
		if (entry.is_caption)
			return buy_menu->UpdateCaption(entry.title, available, entry, index);
		else
			return buy_menu->UpdateBuyEntry(entry.item, available, entry, index, index == last_buy_idx, tier);
	}
	return false;
}

public func GetEntryByID(def id)
{
	for (var i = 0; i < GetLength(base_material); i++)
		if (base_material[i].item == id)
			return i;
	return nil;
}

public func GetEntryInformation(int entry_idx)
{
	// Fill with current information for this entry
	var entry = base_material[entry_idx];
	// Append (Tier x/y) to name
	if (entry.tiers)
	{
		if (!entry.base_name) entry.base_name = entry.name;
		var tier = techs[entry.tech];
		entry.name = Format("%s ($Tier$ %d/%d)", entry.base_name, tier + 1, entry.tiers);
	}
	// Compose info message
	// Info message: Requirements
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
	// Info message: Cannot afford
	if (entry.cost > GetWealth(GetOwner()))
	{
		msg = Format("%s<c ff0000>$Cost$: %d</c>", msg, entry.cost);
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
			Sound("UI::Cash", true, nil, plr);
			// Some items cost only once
			if (entry.free_rebuy) entry.cost = nil;
		}
		else
		{
			// Still some feedback even on free selections
			Sound("Liquids::Waterdrop1", true, nil, plr);
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
			// Get rid of current item
			if (last_item)
				SellItem(last_item);
			var item;
			// Create item
			if (!item) item = cursor->CreateContents(entry.item);
			if (!item) return false; // ???
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
			// stack count
			if (entry.infinite && item)
			{
				item->SetInfiniteStackCount();
			}
		}
	}
	// Buy only once? (all technologies without further upgrade tiers)
	if (entry.remove_after_buy)
	{
		entry.hidden = true;
		if (buy_menu) buy_menu->RemoveItem(callback_idx);
	}
	else if (entry.tech)
	{
		// Multi-tier tech upgrade
		UpdateIndexedItem(callback_idx);
	}
	else
	{
		// Non-tech: Remember what has been bought
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
		Sound("UI::Cash", true, nil, GetOwner());
	}
	is_selling = true; // no item re-buy during sale
	var success = item->RemoveObject();
	is_selling = false;
	return success;
}

// Makes an item available even though the requirements aren't yet met
public func SetItemAvailable(int entry_idx)
{
	// Safety
	var entry = base_material[entry_idx];
	if (!entry) return false;

	entry.requirements = nil;
	entry.cost = nil;
	return true;
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
	var entry, i = 0;
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
	var n = GetLength(base_material);
	for (var i = 0; i<n; ++i)
	{
		var req = base_material[i].requirements;
		if (req && GetIndexOf(req, entry.tech) >= 0)
			UpdateIndexedItem(i);
	}
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

private func GainShootingStrength(proplist entry, int tier)
{
	// Increase player's shooting strength
	// Weapon get plus x percent shooting strength
	tech_shooting_strength_multiplier = [10, 20, 30, 40][tier];
	// Update all current weapons
	for (var weapon in FindObjects(Find_Owner(GetOwner()), Find_Func("Gidl_IsRangedWeapon")))
		weapon->Guardians_UpdateShootingStrength();
	return true;
}

private func GainLife(proplist entry, int tier)
{
	// Increase player's life
	tech_life = [1, 5, 10, 20][tier];
	// Full refresh and max increase on current value
	for (var clonk in FindObjects(Find_Owner(GetOwner()), Find_Func("IsClonk")))
	{
		clonk.MaxEnergy = clonk.Prototype.MaxEnergy * tech_life;
		clonk->DoEnergy(clonk.MaxEnergy, true);
	}
	return true;
}
