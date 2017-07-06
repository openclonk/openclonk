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

local hold_crew = false;
local relaunch_time = 36 * 10;

// Sets the time, in seconds, the clonk is held in the container.
public func SetRelaunchTime(int to_time, bool to_hold)
{
	relaunch_time = to_time * 36;
	hold_crew = to_hold;
	return;
}

// Returns the time, in seconds the clonk is held.
public func GetRelaunchTime() { return relaunch_time / 36; }

// Retrieve weapon list from scenario.
private func WeaponList() { return GameCall("RelaunchWeaponList"); }

public func StartRelaunch(object clonk)
{
	if (!clonk)
		return;
	// Only 1 clonk can be inside.
	if (crew)
		return;
	// Save clonk for later use.
	crew = clonk;
	clonk->Enter(this);
	AddEffect("IntTimeLimit", this, 100, 36, this);
	ScheduleCall(this, "OpenWeaponMenu", 36, 0, clonk);
	GameCall("OnClonkEnteredRelaunch", clonk, clonk->GetOwner());
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
			menu->SetTitle(Format("$MsgWeapon$", relaunch_time / 36));
			clonk->SetMenu(menu, true);
			
			if (GetType(GetRelaunchRule().last_used_player_weapons) != C4V_Array)
				GetRelaunchRule().last_used_player_weapons = [];
			for (var weapon in weapons)
			{
				if (GetRelaunchRule().last_used_player_weapons[clonk->GetOwner()] != weapon)
				{
					menu->AddItem(weapon, weapon->GetName(), nil, this, "OnWeaponSelected", weapon);
				}
			}
				
			menu->Open();
		}
	}
}

public func FxIntTimeLimitTimer(object target, effect fx, int fxtime)
{
	var clonk = crew;
	if (!clonk)
	{
		RemoveObject();
		return FX_Execute_Kill;
	}
	if (fxtime >= relaunch_time)
	{
		if (!has_selected && WeaponList())
			GiveWeapon(RandomElement(WeaponList()));
		RelaunchClonk();
		return FX_Execute_Kill;
	}
	if (menu)
		menu->SetTitle(Format("$MsgWeapon$", (relaunch_time - fxtime) / 36));
	else
		PlayerMessage(clonk->GetOwner(), Format("$MsgRelaunch$", (relaunch_time - fxtime) / 36));
	return FX_OK;
}

public func OnWeaponSelected(id weapon)
{
	if (!crew)
		return;
	GiveWeapon(weapon);
	if (GetRelaunchRule()->GetLastWeaponUse())
		GetRelaunchRule().last_used_player_weapons[crew->GetOwner()] = weapon;
	has_selected = true;
	// Close menu manually, to prevent selecting more weapons.
	if (menu)
		menu->Close();

	if (!hold_crew)
		RelaunchClonk();
	return true;
}

private func RelaunchClonk()
{
	var clonk = crew;
	// When relaunching from disabled state (i.e base respawn), reset view to clonk.
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


/*-- Saving --*/

public func SaveScenarioObject() { return false; }
