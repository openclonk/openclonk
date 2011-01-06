/* larger dynamite explosion */

#appendto RelaunchContainer

private func OpenWeaponMenu(object clonk)
{
	if (!menu)
	{
		var weapons = WeaponList();
		if(weapons)
		{
			menu = clonk->CreateRingMenu(Clonk, this);
			for (var weapon in weapons)
			{
				if(weapon == Firestone) menu->AddItem(weapon,2);
				else if(weapon == Dynamite) menu->AddItem(weapon,2);
				else menu->AddItem(weapon);
			}
			menu->Show();
		}
	}
}


public func Selected(object menu, object selector, bool alt)
{
	if (!selector)
		return false;
	
	for (var i = 0; i < selector->GetAmount(); i++)
	{
		var newobj = CreateObject(selector->GetSymbol());
		if (newobj->GetID() == Bow)
			newobj->CreateContents(Arrow);
		if (newobj->GetID() == Musket)
			newobj->CreateContents(LeadShot);
		Contents()->Collect(newobj);
	}
	menu->Show();
	RelaunchClonk();
	return true;
}