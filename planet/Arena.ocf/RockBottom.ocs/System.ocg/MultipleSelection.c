#appendto RelaunchContainer

private func OpenWeaponMenu(object clonk)
{
	if (!menu)
	{
		var weapons = WeaponList();
		if(weapons)
		{
			menu = clonk->CreateRingMenu(this, this);
			for (var weapon in weapons)
			{
				if(weapon == Firestone) menu->AddItem(weapon,2);
				else if(weapon == Dynamite) menu->AddItem(weapon,2);
				else menu->AddItem(weapon, 1);
			}
			menu->Show();
			menu->SetUncloseable();
		}
	}
}
