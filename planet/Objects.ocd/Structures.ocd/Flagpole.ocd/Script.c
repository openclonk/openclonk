/*-- Flagpole --*/

#include Library_Structure
#include Library_Flag
#include Library_GoldSeller
#include Library_Base // Needed for DuBuy...

local Name = "$Name$";
local Description = "$Description$";
 
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
	while (item = GetHomebaseMaterial(GetOwner(), nil, i++))
	{
		amount = GetHomebaseMaterial(GetOwner(), item);
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
			Contents(i)->Exit(0, GetDefHeight() / 2);
	// Update available count
	menu_item->SetAmount(GetHomebaseMaterial(GetOwner(), def));
	menu->Show();
	return true;
}

func OnOwnerRemoved(int new_owner)
{
	// Our owner is dead :(
	// Flag is passed on to the next best owner
	SetOwner(new_owner);
	return true;
}
