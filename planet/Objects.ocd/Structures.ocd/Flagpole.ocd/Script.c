/*-- Flagpole --*/

#include Library_Structure
#include Library_Flag
#include Library_GoldSeller
#include Library_Base // Needed for DoBuy...

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLOAT,
		NextAction = "Hold",
	},
};

protected func Initialize()
{
	// The flag is not supposed to be moved ever, ever.
	// But setting the category to StaticBack would mess with other things that rely on recognizing buildings by category.
	// no: SetCategory(C4D_StaticBack);
	// Thus the flag can now fly.
	SetAction("Fly");
	SetComDir(COMD_Stop);
	return _inherited(...);
}

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Translate(0,4000,0));
	return _inherited(...);
}

public func NoConstructionFlip() { return true; }

/*-- Interaction --*/

public func GetInteractionMenus(object clonk)
{
	var menus = _inherited() ?? [];
	// only open the menus if ready
	if (ObjectCount(Find_ID(Rule_BuyAtFlagpole)))
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

public func GetBuyMenuEntries(object clonk)
{
	// default design of a control menu item
	var custom_entry = 
	{
		Right = "8em", Bottom = "4em",
		BackgroundColor = {Std = 0, OnHover = 0x50ff0000},
		image = {Right = "4em", Style = GUI_TextBottom | GUI_TextRight},
		price = {Left = "4em", Priority = 3}
	};
	
	var wealth = GetWealth(GetOwner()); // Note that the flag owner pays for everything atm. 
	var menu_entries = [];
	var i = 0, item, amount;
	while (item = GetBaseMaterial(GetOwner(), nil, i++))
	{
		amount = GetBaseMaterial(GetOwner(), item);
		var entry = 
		{
			Prototype = custom_entry,
			image = {Prototype = custom_entry.image},
			price = {Prototype = custom_entry.price}
		};
		entry.image.Symbol = item;
		entry.image.Text = Format("%dx", amount);
		var value = GetBuyValue(item);
		entry.price.Text = Format("<c ffff00>%d{{Icon_Wealth}}</c>", value);
		entry.Priority = 1000 * value + i; // Order by value and then by BaseMaterial index.
		if (value > wealth) // If the player can't afford it, the item (except for the price) is overlayed by a greyish color.
		{
			entry.overlay = {Priority = 2, BackgroundColor = RGBa(50, 50, 50, 150)};
		}
		PushBack(menu_entries, {symbol = item, extra_data = nil, custom = entry});
	}
	
	return menu_entries;
}

public func OnBuyMenuSelection(id def, extra_data, object clonk)
{
	// Buy
	DoBuy(def, clonk->GetController(), GetOwner(), clonk);
	// Excess objects exit flag (can't get them out...)
	var i = ContentsCount();
	var obj;
	while (i--) 
		if (obj = Contents(i))
		{
			obj->Exit(0, GetDefHeight() / 2);
			// newly bought items do not fade out until they've been collected once
			if (obj && ObjectCount(Find_ID(Rule_ObjectFade)) && !obj.HasNoFadeOut)
			{
				obj.HasNoFadeOut = this.BuyItem_HasNoFadeout;
				obj.BuyOverload_Entrance = obj.Entrance;
				obj.Entrance = this.BuyItem_Entrance;
			}
		}
	UpdateInteractionMenus(this.GetBuyMenuEntries);
}

// newly bought items do not fade out unless collected
func BuyItem_HasNoFadeout() { return true; }

func BuyItem_Entrance()
{
	// after first collection, fade out rule should be effective again
	var overloaded_fn = this.BuyOverload_Entrance;
	this.HasNoFadeOut = nil;
	this.BuyOverload_Entranc = nil;
	this.Entrance = overloaded_fn;
	if (overloaded_fn) return Call(overloaded_fn, ...);
}

func OnOwnerRemoved(int new_owner)
{
	// Our owner is dead :(
	// Flag is passed on to the next best owner
	SetOwner(new_owner);
	return true;
}


/* Neutral banners - used as respawn points only */

func IsNeutral() { return neutral; }

func SetNeutral(bool to_val)
{
	// Neutral flagpoles: A bit smaller and different texture. No marker Radius.
	if (neutral = to_val)
	{
		SetMeshMaterial("NeutralFlagBanner",0);
		//SetMeshMaterial("NeutralFlagPole",1);
		SetFlagRadius(0);
		this.MeshTransformation = Trans_Mul(Trans_Scale(700,700,700), Trans_Translate(0,6000,0));
	}
	else
	{
		SetMeshMaterial("FlagBanner",0);
		//SetMeshMaterial("SettlementFlagPole",1);
		SetFlagRadius(this.DefaultFlagRadius);
		this.MeshTransformation = Trans_Translate(0,4000,0);
	}
	return true;
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (neutral)
	{
		props->Remove("Radius");
		props->Remove("Color");
		props->Remove("MeshMaterial");
		props->AddCall("Neutral", this, "SetNeutral", true);
	}
	return true;
}



/*-- Properties --*/
// Provides an interaction menu for buying things.
public func HasInteractionMenu() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local HitPoints = 60;
local neutral = false;
