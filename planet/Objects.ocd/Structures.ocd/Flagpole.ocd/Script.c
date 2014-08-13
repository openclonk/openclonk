/*-- Flagpole --*/

#include Library_Structure
#include Library_Flag
#include Library_GoldSeller
#include Library_Base // Needed for DuBuy...

protected func Initialize()
{
	// SetCategory(C4D_StaticBack);
	return _inherited(...);
}

protected func Construction()
{
	SetProperty("MeshTransformation", Trans_Translate(0,4000,0));
	return _inherited(...);
}

public func NoConstructionFlip() { return true; }

/*-- Interaction --*/

public func IsInteractable(object clonk)
	{
	if (!ObjectCount(Find_ID(Rule_BuyAtFlagpole))) return false;
	if (GetCon() < 100) return false;
	return !Hostile(GetOwner(), clonk->GetOwner());
	}

public func GetInteractionMetaInfo(object clonk)
{
	return { Description = "$MsgBuy$", IconName = nil, IconID = Library_Base };
}

public func Interact(object clonk)
{
	var menu;
	var i = 0, item, amount;
	while (item = GetBaseMaterial(GetOwner(), nil, i++))
	{
		amount = GetBaseMaterial(GetOwner(), item);
		// Add even if amount==0
		if (!menu) menu = clonk->CreateRingMenu(Flagpole, this);
		if (!menu) return false;
		menu->AddItem(item, amount, nil);
	}
	if (!menu) return false;
	menu->Show();
	return true;
}

public func Selected(object menu, proplist menu_item, bool alt)
{
	// Safety
	var clonk = menu->GetMenuObject();
	if (!clonk || !IsInteractable(clonk)) return;
	var def = menu_item->GetSymbol();
	if (!def) return;
	// Buy
	DoBuy(def, clonk->GetController(), GetOwner(), clonk, alt);
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
	// Update available count
	menu_item->SetAmount(GetBaseMaterial(GetOwner(), def));
	menu->Show();
	return true;
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


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local HitPoints = 60;
