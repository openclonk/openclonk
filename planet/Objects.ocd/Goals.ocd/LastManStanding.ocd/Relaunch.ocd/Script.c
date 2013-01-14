/*--
	Relaunch container
	Author: Maikel

	This container holds the clonk after relaunches.
	* The time the clonk is held can be specified with SetRelaunchTime(int time);
	* After that time the clonk is released and OnClonkLeftRelaunch(object clonk) is called in the scenario script.
	* Optionally the clonk can choose a weapon if GetRelaunchWeaponList in the scenario script returns a valid id-array.
--*/

local time;
local menu;
local hold;
local has_selected;

local crew;

protected func Initialize()
{
	time = 36 * 10;
	return;
}

// Sets the time, in seconds, the clonk is held in the container.
public func SetRelaunchTime(int to_time, bool to_hold)
{
	time = to_time * 36;
	hold = to_hold;
	return;
}

// Returns the time, in seconds the clonk is held.
public func GetRelaunchTime() { return time / 36; }

// Retrieve weapon list from scenario.
private func WeaponList() { return GameCall("RelaunchWeaponList"); }

public func StartRelaunch(object clonk)
{
	if (!clonk)
		return;
	// only 1 clonk can be inside
	if(crew)
		return;
	// save clonk for later use
	crew = clonk;
	clonk->Enter(this);
	ScheduleCall(this, "OpenWeaponMenu", 36, 0, clonk);
	AddEffect("IntTimeLimit", this, 100, 36, this);
	
	return true;
}

private func OpenWeaponMenu(object clonk)
{
	if (!clonk)
		return;	
	if (!menu)
	{
		var weapons = WeaponList();
		if (weapons)
		{
			menu = clonk->CreateRingMenu(this, this);
			for (var weapon in weapons)
				menu->AddItem(weapon, 1);
			menu->Show();
			menu->SetUncloseable();
		}
	}
}

func FxIntTimeLimitTimer(object target, effect, int fxtime)
{
	var clonk = crew;
	if (!clonk)
	{
		RemoveObject();
		return -1;
	}
	if (fxtime >= time)
	{
		if (!has_selected && WeaponList())
			GiveWeapon(WeaponList()[Random(GetLength(WeaponList()))]);
		RelaunchClonk();
		return -1;
	}
	if (menu)
		PlayerMessage(clonk->GetOwner(), Format("$MsgWeapon$", (time - fxtime) / 36));
	else
		PlayerMessage(clonk->GetOwner(), Format("$MsgRelaunch$", (time - fxtime) / 36));
	return 1;
}

public func Selected(object menu, object selector, bool alt)
{
	if (!selector)
		return false;
	
	var amount = selector->GetAmount();
	if (amount > 1)
		alt = nil;
	
	for (var i = 0; i < amount; i++)
		GiveWeapon(selector->GetSymbol(), alt);
	
	has_selected = true;
	// Close menu manually, to prevent selecting more weapons.
	if (menu)
		menu->Close(true);

	if (!hold)
		RelaunchClonk();
	return true;
}

private func RelaunchClonk()
{
	var clonk = crew;
	clonk->Exit();
	GameCall("OnClonkLeftRelaunch", clonk);
	if (menu)
		menu->Close(true);
	PlayerMessage(clonk->GetOwner(), "");
	RemoveObject();
	return;
}

private func GiveWeapon(id weapon_id, bool alt)
{
	var newobj = CreateObject(weapon_id);
	if (weapon_id == Bow)
		newobj->CreateContents(Arrow);
	if (weapon_id == Musket)
		newobj->CreateContents(LeadShot);
	crew->Collect(newobj, nil, alt);
	return;
}

local Name = "$Name$";
