/**
	Item Spawn
	If the clonk moves through the spawn it automatically get the item.
	
	@author Maikel
*/ 


public func Create(id def, int x, int y)
{
	if (this != ItemSpawn)
		return;
	var spawn = CreateObject(ItemSpawn, x, y);
	AddEffect("ProcessSpawn", spawn, 100, 1, spawn, nil, def);
	return spawn;
}

public func InitializePlayer(int plr)
{
	var spawn_effect = GetEffect("ProcessSpawn", this);
	if (spawn_effect)
		EffectCall(this, spawn_effect, "AddPlayer", plr);
	return;
}

public func RemovePlayer(int plr)
{
	var spawn_effect = GetEffect("ProcessSpawn", this);
	if (spawn_effect)
		EffectCall(this, spawn_effect, "RemovePlayer", plr);
	return;
}


/*-- Spawn Effect --*/

public func FxProcessSpawnStart(object target, proplist effect, int temp, id def)
{
	if (temp)
		return FX_OK;
	effect.spawn_id = def;
	effect.spawn_list = [];
	effect.graphic_list = [];
	for (var plr in GetPlayers())
	{
		var plrid = GetPlayerID();
		effect.graphic_list[plrid] = CreateObject(Dummy, 0, 0, plr);
		effect.graphic_list[plrid].Visibility = VIS_Owner;
		effect.graphic_list[plrid]->SetGraphics(nil, effect.spawn_id, GFX_Overlay, GFXOV_MODE_Base);
	}
	return FX_OK;
}

public func FxProcessSpawnAddPlayer(object target, proplist effect, int add_plr)
{
	var plrid = GetPlayerID(add_plr);
	effect.graphic_list[plrid] = CreateObject(Dummy, 0, 0, add_plr);
	effect.graphic_list[plrid].Visibility = VIS_Owner;
	effect.graphic_list[plrid]->SetGraphics(nil, effect.spawn_id, GFX_Overlay, GFXOV_MODE_Base);
	return FX_OK;
}

public func FxProcessSpawnRemovePlayer(object target, proplist effect, int remove_plr)
{
	var plrid = GetPlayerID(remove_plr);
	if (effect.graphic_list[plrid])
		effect.graphic_list[plrid]->RemoveObject();
	if (effect.spawn_list[plrid])
		effect.spawn_list[plrid].item = nil;
	return FX_OK;
}

public func FxProcessSpawnTimer(object target, proplist effect, int time)
{
	// Produce some particles for the spawn ring.
	if (time % 30 == 0)
	{
		var spawn_particle =
		{
			Size = PV_Linear(24, 36),
			Alpha = PV_Linear(0, 255),
			R = PV_Random(200, 255),
			G = PV_Random(200, 255),
			B = PV_Random(200, 255)
		};
		CreateParticle("MagicRing", 0, 0, 0, 0, 60, spawn_particle, 1);
	}
	
	// Update item availability for all active players.
	if (time % 10 == 0)
	{
		for (var plr in GetPlayers())
		{
			var plrid = GetPlayerID();
			if (effect.spawn_list[plrid] && !effect.spawn_list[plrid].item && effect.graphic_list[plrid].Visibility == VIS_None)
				effect.graphic_list[plrid].Visibility = VIS_Owner;
		}
	}
	
	// Check for crew members near the item spawn and give the item.
	for (var crew in FindObjects(Find_OCF(OCF_CrewMember), Find_Distance(20)))
	{
		var plrid = GetPlayerID(crew->GetOwner());
		if (effect.spawn_list[plrid] == nil)
			effect.spawn_list[plrid] = {};
		if (!effect.spawn_list[plrid].item)
		{
			if (crew->ContentsCount() < crew.MaxContentsCount || (effect.spawn_id->~IsCarryHeavy() && !crew->IsCarryingHeavy()))	
			{
				var spawned = crew->CreateContents(effect.spawn_id);
				effect.spawn_list[plrid].item = spawned;
				effect.graphic_list[plrid].Visibility = VIS_None;
			}
		}
	}
	return FX_OK;
}

public func FxProcessSpawnStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return FX_OK;

	return FX_OK;
}