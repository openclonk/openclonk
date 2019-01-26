/* Flint Brawl */

local team_spawn_positions = nil;
local spawn_item_particles;
func Initialize()
{
	spawn_item_particles =
	{
		Size = PV_Linear(2, 0),
		Alpha = PV_KeyFrames(0, 0, 0, 300, 255, 1000, 0),
		Rotation = PV_Random(0, 360),
		R = 255, G = 200, B = 50,
		Stretch = PV_Random(10000, 15000),
		BlitMode = GFX_BLIT_Additive
	};

}

local SpawnItem = new Effect
{
	Timer = func(int time)
	{
		if (Random(time / 38))
		{
			CreateParticle("StarSpark", this.x + RandomX(-2, 2), this.y + RandomX(-2, 2), 0, 0, PV_Random(1, 7), Scenario.spawn_item_particles, 10);
		}

		if (time > 38 * 4)
		{
			var obj = CreateObject(this.ID, this.x, this.y, NO_OWNER);
			obj->SetR(Random(360));
			return FX_Execute_Kill;
		}
		return FX_OK;
	}
};

local SpawnEffect = new Effect
{
	Timer = func(int time)
	{
		var location = FindLocation(Loc_Material("Earth"));
		if (!location) return FX_OK;
		var fx = Scenario->CreateEffect(Scenario.SpawnItem, 1, 2);
		fx.x = location.x;
		fx.y = location.y;

		var items = [Dynamite, Firestone, Bouncy, QuickSandBomb, IronBomb];

		if (!Random(4) && ((FrameCounter() - this.round_start_time) > 38 * 60))
			items = [Lantern, ChippyFlint, Boompack, SmokeBomb, GrenadeLauncher];
		var item = items[Random(GetLength(items))];
		fx.ID = item;
	}
};

func GetTeamSpawnPosition(int team_id)
{
	if ((team_id >= GetLength(team_spawn_positions)) || (team_spawn_positions[team_id] == nil))
	{
		var location = FindLocation(Loc_Wall(CNAT_Bottom), Loc_Space(25, CNAT_Top));
		team_spawn_positions[team_id] = location;
		if (location)
			Fireworks(GetTeamColor(team_id), location.x, location.y - 20);
	}

	return team_spawn_positions[team_id];
}

func InitializeRound()
{
	team_spawn_positions = []; // reset team spawns.

	var i, pos;
	var ls_wdt = LandscapeWidth(), ls_hgt = LandscapeHeight();
	// Materials: Firestones
	for (i=0; i<30; ++i)
	{
		var pos = FindLocation(Loc_InRect(0,ls_hgt/2,ls_wdt,ls_hgt/3), Loc_Solid());
		if (pos && IsFirestoneSpot(pos.x,pos.y))
			CreateObjectAbove(Firestone,pos.x,pos.y-1);
	}
	// Some firestones and bombs in lower half. For ap type 1, more firestones in lower than upper half.
	var items = [Firestone,IronBomb, Dynamite, Loam, PowderKeg];
	var item_count = GetLength(items);
	for (i=0; i<30; ++i)
	{
		var pos = FindLocation(Loc_InRect(0,0,ls_wdt,ls_hgt), Loc_Solid());
		if (pos && IsFirestoneSpot(pos.x,pos.y))
		{
			var item_idx = Random(item_count);
			if (Random(3)) item_idx = Random(item_idx);
			CreateObjectAbove(items[item_idx],pos.x,pos.y-1);
		}
	}

	var trees = [];
	for (var ID in [Tree_Coniferous, Tree_Coniferous2, Tree_Coniferous3, Tree_Coniferous4])
		for (var tree in ID->Place(Random(3))) PushBack(trees, tree);


	for (var tree in trees)
	{
		tree->CreateObjectInTreetop(Zaphive);
	}
	Mushroom->Place(10);
	Stalactite->Place(5, Shape->Rectangle(0, 0, ls_wdt, ls_hgt/2));
	LargeCaveMushroom->Place(2);
	Bat->Place(2);

	var location = FindLocation(Loc_Tunnel(), Loc_Wall(CNAT_Bottom), Loc_Space(20, CNAT_Top));
	if (location)
	{
		var obj = CreateObjectAbove(Cannon, location.x, location.y, NO_OWNER);
		obj->CreateContents(PowderKeg);
	}

	if (!FindObject(Find_ID(Time)))
		CreateObject(Time);
	Time->SetTime(24 * 60);
	Meteor->SetChance(10, Chippie_Egg, 10);

	var spawn_fx = CreateEffect(this.SpawnEffect, 1, (38 * 3) / SCENPAR_ItemAmount);
	spawn_fx.round_start_time = FrameCounter();
	return true;
}

func InitPlayerRound(int plr, object crew)
{
	// everything visible
	SetFoW(false, plr);
	// Player positioning.
	var ls_wdt = LandscapeWidth(), ls_hgt = LandscapeHeight();
	var start_pos = nil;
	if (SCENPAR_TeamsTogether == 1)
	{
		start_pos = GetTeamSpawnPosition(GetPlayerTeam(plr));
	}
	if (start_pos == nil)
	{
		// Start positions not defined or exhausted: Spawn in lower area for both maps becuase starting high is an an advantage.
		start_pos = FindLocation(Loc_InRect(ls_wdt/5,ls_hgt/2,ls_wdt*3/5,ls_hgt/3), Loc_Wall(CNAT_Bottom), Loc_Func(Scenario.IsStartSpot));
		if (!start_pos) start_pos = FindLocation(Loc_InRect(ls_wdt/10,0,ls_wdt*8/10,ls_hgt*4/5), Loc_Wall(CNAT_Bottom), Loc_Func(Scenario.IsStartSpot));
		if (!start_pos) start_pos = {x=Random(ls_wdt*6/10)+ls_wdt*2/10, y=ls_hgt*58/100};
	}

	crew->SetPosition(start_pos.x, start_pos.y-10);

	// initial material
	crew->CreateContents(WindBag);
	crew->CreateContents(Shovel);

	crew.MaxEnergy = 50000;
	crew->DoEnergy(1000);
}

// Horizontal Loc_Space doesn't work with Loc_Wall because it checks inside the ground.
func IsStartSpot(int x, int y)
{
	// Don't spawn just at the border of an island.
	if (!GBackSolid(x-3,y+2)) return false;
	if (!GBackSolid(x+3,y+2)) return false;
	if (GBackSemiSolid(x, y)) return false;
	// Spawn with some space.
	return PathFree(x-5, y, x+5, y) && PathFree(x, y-21, x, y-1);
}

func IsFirestoneSpot(int x, int y)
{
	// Very thorough material surrounding check so they don't explode right away or when the first layer of ice melts
	return GBackSolid(x,y-1) && GBackSolid(x,y+4) && GBackSolid(x-2,y) && GBackSolid(x+2,y);
}
