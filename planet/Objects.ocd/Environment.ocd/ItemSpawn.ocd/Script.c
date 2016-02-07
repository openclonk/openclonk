/**
	Item Spawn
	If the clonk moves through the spawn it automatically gets the item.
	
	@author Maikel, Sven2
*/ 

local spawn_id; // Item to be spawned
local team;     // If assigned, spawner can only be used by specific team
local spawn_list; // List keeping track of collected objects to block spawning until used up
local Visibility; // Custom visibility
local spawn_particle; // prepared gfx proplist

/* Interface */

// Definition call: Create an item spawner and set its spawn def
public func Create(id def, int x, int y)
{
	if (this != ItemSpawn)
		return;
	var spawn = CreateObject(ItemSpawn, x, y);
	if (spawn) spawn->SetSpawnObject(def);
	return spawn;
}

// Change object to be spawned by this
public func SetSpawnObject(id def)
{
	spawn_id = def;
	SetGraphics(nil, spawn_id, GFX_Overlay, GFXOV_MODE_Base);
	return true;
}

// If called, items can only be collected by members of this team
public func SetTeam(int new_team)
{
	team = new_team;
	for (var plr in GetPlayers()) UpdateVisibility(plr);
	return true;
}



/*-- Spawn Effect --*/

public func Construction()
{
	spawn_list = [];
	spawn_particle =
		{
			Size = PV_Linear(24, 36),
			Alpha = PV_Linear(0, 255),
			R = PV_Random(200, 255),
			G = PV_Random(200, 255),
			B = PV_Random(200, 255)
		};
	// Initial visibility for all players
	Visibility = [VIS_Select,false,false,false,false];
	for (var plr in GetPlayers()) UpdateVisibility(plr);
	// Timer effect
	AddEffect("Spawn", this, 1, 2, this);
	return true;
}

private func UpdateVisibility(int plr)
{
	// Spawn is visible if the item is currently not collected by the player
	// In case a team is set, it also needs to match
	Visibility[plr+1] = !spawn_list[plr] && (!team || team == GetPlayerTeam(plr));
	return true;
}

private func FxSpawnTimer(object target, proplist effect, int time)
{
	// Produce some particles for the spawn ring.
	if (time % 30 == 0)
		CreateParticle("MagicRing", 0, 0, 0, 0, 60, spawn_particle, 1);
	
	// Update item availability for all active players.
	if (time % 10 == 0)
		for (var plr in GetPlayers()) UpdateVisibility(plr);
	
	// Check for crew members near the item spawn and give the item.
	for (var crew in FindObjects(Find_OCF(OCF_CrewMember), Find_Distance(20)))
	{
		var plr = crew->GetOwner();
		if (!spawn_list[plr] && Visibility[plr+1] && spawn_id)
		{
			if (crew->ContentsCount() < crew.MaxContentsCount || (spawn_id->~IsCarryHeavy() && !crew->IsCarryingHeavy()))
			{
				spawn_list[plr] = crew->CreateContents(spawn_id);
				UpdateVisibility(plr);
				crew->~Get(spawn_list[plr]); // for sound
			}
		}
	}
	return FX_OK;
}



/* Player/team changes */

public func InitializePlayer(int plr)
{
	// Update visibility and clear spawned item for new player (so we don't need to handle RemovePlayer)
	spawn_list[plr] = nil;
	return UpdateVisibility(plr);
}

public func OnTeamSwitch(int plr, int new_team, int old_team)
{
	// Broadcast on player team switch: Update visibility
	return UpdateVisibility(plr);
}


/* Scenario saving */

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	var fx_spawn = GetEffect("ProcessSpawn", this);
	if (!fx_spawn) return true; // effects lost? Just save the unused object then; maybe someone wants to assign it an item later.
	// Item spawner has its own creation procedure
	props->RemoveCreation();
	props->Add(SAVEOBJ_Creation, "%i->Create(%i,%d,%d,%i)", GetID() /* to allow overloads */, fx_spawn.spawn_id, GetX(), GetY());
	return true;
}
