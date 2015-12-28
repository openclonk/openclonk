#appendto WindBag

func Entrance(object container)
{
	if (this == g_windbag && container && container->IsClonk())
	{
		var plr = container->GetOwner();
		if (GetPlayerType(plr) != C4PT_User) return _inherited(container, ...);

		var homebase = FindObject(Find_ID(Homebase), Find_Owner(plr));
		if (!homebase) return _inherited(container, ...); // ???

		homebase->SetItemAvailable(homebase->GetEntryByID(GetID()));
		g_windbag = nil;
		Sound("UI::Cleared", false, nil, plr);
		CustomMessage("$MsgGotWindbag$", container, plr);
		var menu = FindObject(Find_ID(GUI_ObjectInteractionMenu), Find_Owner(plr));
		if (menu) Schedule(menu, "RemoveObject()", 1);
		return RemoveObject();
	}
	return _inherited(container, ...);
}