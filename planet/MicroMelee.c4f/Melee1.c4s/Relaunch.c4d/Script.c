/*
	Relaunch container
	Author: Maikel

	This container holds the clonk after relaunches, so the player will be able to choose some weapons.
*/

local menu;
local choses;

private func WeaponList()
{
	return[Shovel,Bow,Arrow,Musket,LeadShot,Shield,Sword,Club,Javelin,Boompack,DynamiteBox,Dynamite,Loam,Firestone,Fireglobe];
}

public func WeaponMenu(object clonk)
{
	if (!menu)
	{
		menu = clonk->CreateRingMenu(Clonk, 0, 0, this);
		for (var weapon in WeaponList())
			menu->AddItem(weapon);
		menu->Show();    		
    }
	AddEffect("IntTimeLimit", this, 100, 10, this);
	return true;
}

func FxIntTimeLimitTimer(target, num, time)
{
	var clonk = Contents();
	if (time > 350)
	{    
		RelaunchClonk();
		menu->Close();
		PlayerMessage(clonk->GetOwner(), "", this);
		this->RemoveObject();
		return -1;
	}
	PlayerMessage(clonk->GetOwner(), Format("%d seconds remaining.", (350 - time) / 35), this);
	return 1;
}

public func Selected(object menu, object selector)
{
	if(!selector) return 0;
	
	for(var i = 0; i<(selector->GetAmount()); i++)
	{
		var newobj = CreateObject(selector->GetSymbol());
		newobj->Enter(Contents());
	}
	selector->RemoveObject();
	menu->Show();
	if (choses > 0)
	{
		RelaunchClonk();
      	this->RemoveObject();
		return 1;
	}
	choses++;
	return 0;
}

private func RelaunchClonk()
{
	var clonk = Contents();
	clonk->Exit();
	clonk->SetPosition(RandomX(30,LandscapeWidth()-30),-20);
	return;
}


func Definition(def) {
        SetProperty("Name", "relaunch", def);
}