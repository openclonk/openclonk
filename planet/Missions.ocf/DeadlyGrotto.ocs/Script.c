/* Deadly grotto */

func Initialize()
{
	// Goal
	var goal = FindObject(Find_ID(Goal_RepairStatue));
	if (!goal) goal = CreateObject(Goal_RepairStatue);
	var statue = CreateObjectAbove(MinersStatue, 600,736);
	statue->SetBroken();
	var statue_head = CreateObjectAbove(MinersStatue_Head, 2200,560);
	goal->SetStatue(statue);
	// Rules
	if (!ObjectCount(Find_ID(Rule_TeamAccount))) CreateObject(Rule_TeamAccount);
	if (!ObjectCount(Find_ID(Rule_BuyAtFlagpole))) CreateObject(Rule_BuyAtFlagpole);
	// Mushrooms before any earth materials, because they create their own caves
	LargeCaveMushroom->Place(15, Shape->Rectangle(100, 0, 600, 300));
	// Create earth materials
	// Create them in big clusters so the whole object arrangement looks a bit less uniform and more interesting
	PlaceBatches([Firestone], 5, 100, 10);
	PlaceBatches([Rock, Loam, Loam], 10, 200, 10);
	// Misc vegetation
	SproutBerryBush->Place(5, Shape->Rectangle(100, 0, 600, 300));
	Mushroom->Place(5, Shape->Rectangle(100,0,600,300));
	// Sky
	SetSkyParallax(1, 20,20, 0,0, nil, nil);
	return true;
}

static g_was_player_init;

func InitializePlayer(int plr)
{
	// Harsh zoom range
	for (var flag in [PLRZOOM_LimitMax, PLRZOOM_Direct])
		SetPlayerZoomByViewRange(plr,500,350,flag);
	SetPlayerViewLock(plr, true);
	// First player init base
	if (!g_was_player_init)
	{
		InitBase(plr);
		g_was_player_init = true;
	}
	// Position and materials
	var i, crew;
	for (i=0; crew=GetCrew(plr,i); ++i)
	{
		crew->SetPosition(600+Random(40), 736-10);
		crew->CreateContents(Shovel);
	}
	return true;
}

private func InitBase(int owner)
{
	// Create standard base owned by player
	var y=736;
	var flag = CreateObjectAbove(Flagpole, 670,y, owner);
	var lorry = CreateObjectAbove(Lorry, 650,y-2, owner);
	if (lorry)
	{
		lorry->CreateContents(Loam, 6);
		lorry->CreateContents(Wood, 15);
		lorry->CreateContents(Metal, 4);
		lorry->CreateContents(WallKit, 2);
		lorry->CreateContents(Axe, 1);
		lorry->CreateContents(Pickaxe, 1);
		lorry->CreateContents(Hammer, 1);
		lorry->CreateContents(DynamiteBox, 2);
		lorry->CreateContents(Dynamite, 5);
	}
	return true;
}

private func PlaceBatches(array item_ids, int n_per_batch, int batch_radius, int n_batches)
{
	// place a number (n_batches) of batches of objects of types item_ids. Each batch has n_per_batch objects.
	// fewer batches and/or objects may be placed if no space is found
	var loc,loc2,n_item_ids=GetLength(item_ids), n_created=0, obj;
	for (var i=0; i<n_batches; ++i)
		if (loc = FindLocation(Loc_Material("Earth")))
			for (var j=0; j<n_per_batch; ++j)
				if (loc2 = FindLocation(Loc_InRect(loc.x-batch_radius,loc.y-batch_radius,batch_radius*2,batch_radius*2), Loc_Material("Earth")))
					if (obj=CreateObjectAbove(item_ids[Random(n_item_ids)],loc2.x,loc2.y))
					{
						obj->SetPosition(loc2.x,loc2.y);
						++n_created;
					}
	return n_created;
}

func OnGoalsFulfilled()
{
	GainScenarioAchievement("Done");
	return false;
}
