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
			menu = CreateObject(MenuStyle_Default, nil, nil, clonk->GetOwner());
			menu->SetPermanent();
			menu->SetTitle(Format("$MsgWeapon$", time / 36));
			clonk->SetMenu(menu, true); 
			
			for (var weapon in weapons)
				menu->AddItem(weapon, weapon->GetName(), nil, this, "OnWeaponSelected", weapon);
				
			menu->Open();
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
		menu->SetTitle(Format("$MsgWeapon$", (time - fxtime) / 36));
	else
		PlayerMessage(clonk->GetOwner(), Format("$MsgRelaunch$", (time - fxtime) / 36));
	return 1;
}

public func OnWeaponSelected(id weapon)
{
	GiveWeapon(weapon);
	
	has_selected = true;
	// Close menu manually, to prevent selecting more weapons.
	if (menu)
		menu->Close();

	if (!hold)
		RelaunchClonk();
	return true;
}

private func RelaunchClonk()
{
	var clonk = crew;
	// When relaunching from disabled state (i.e base respawn), reset view to clonk
	if (!clonk->GetCrewEnabled())
	{
		clonk->SetCrewEnabled(true);
		SetCursor(clonk->GetOwner(), clonk);
		SetPlrView(clonk->GetOwner(), clonk);
	}
	clonk->Exit();
	GameCall("OnClonkLeftRelaunch", clonk);
	if (menu)
		menu->Close();
	PlayerMessage(clonk->GetOwner(), "");
	RemoveObject();
	return;
}

private func GiveWeapon(id weapon_id)
{
	var newobj = CreateObjectAbove(weapon_id);
	if (weapon_id == Bow)
		newobj->CreateContents(Arrow);
	if (weapon_id == Musket)
		newobj->CreateContents(LeadShot);
	crew->Collect(newobj);
	return;
}

public func SaveScenarioObject() { return false; }

local Name = "$Name$";
