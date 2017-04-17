/*--
	Relaunch container
	Author: Maikel

	This container holds the clonk after relaunches.
	* The time the clonk is held can be specified by calling GetRelaunchRule()->SetRespawnDelay(int time).
	* After that time the clonk is released and OnClonkLeftRelaunch(object clonk) is called in the scenario script.
	* Optionally the clonk can choose a weapon if GetRelaunchWeaponList in the scenario script returns a valid id-array.
--*/

local menu;
local has_selected;

local crew;

// Sets the GetRelaunchRule().RelaunchTime, in seconds, the clonk is held in the container.

// Returns the GetRelaunchRule().RelaunchTime, in seconds the clonk is held.
public func GetRelaunchTime() { return GetRelaunchRule().relaunch_time / 36; }

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
			menu->SetTitle(Format("$MsgWeapon$", GetRelaunchRule().relaunch_time / 36));
			clonk->SetMenu(menu, true);
			
			if(GetType(GetRelaunchRule().last_used_player_weapons) != C4V_Array) GetRelaunchRule().last_used_player_weapons = [];
			for (var weapon in weapons)
			{
				if(GetRelaunchRule().last_used_player_weapons[clonk->GetOwner()] != weapon)
				{
					menu->AddItem(weapon, weapon->GetName(), nil, this, "OnWeaponSelected", weapon);
				}
			}
				
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
	if (fxtime >= GetRelaunchRule().relaunch_time)
	{
		if (!has_selected && WeaponList())
			GiveWeapon(WeaponList()[Random(GetLength(WeaponList()))]);
		RelaunchClonk();
		return -1;
	}
	if (menu)
		menu->SetTitle(Format("$MsgWeapon$", (GetRelaunchRule().relaunch_time - fxtime) / 36));
	else
		PlayerMessage(clonk->GetOwner(), Format("$MsgRelaunch$", (GetRelaunchRule().relaunch_time - fxtime) / 36));
	return 1;
}

public func OnWeaponSelected(id weapon)
{
	if(!crew) return;
	GiveWeapon(weapon);
	if(GetRelaunchRule().disable_last_weapon) GetRelaunchRule().last_used_player_weapons[crew->GetOwner()] = weapon;
	has_selected = true;
	// Close menu manually, to prevent selecting more weapons.
	if (menu)
		menu->Close();

	if (!GetRelaunchRule().hold)
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
	GameCall("OnClonkLeftRelaunch", clonk, clonk->GetOwner());
	if (menu)
		menu->Close();
	PlayerMessage(clonk->GetOwner(), "");
	RemoveObject();
	return;
}

private func GiveWeapon(id weapon_id)
{
	var newobj = CreateObjectAbove(weapon_id);
	newobj->~OnRelaunchCreation(crew);
	crew->Collect(newobj);
	return true;
}

public func SaveScenarioObject() { return false; }

local Name = "$Name$";
