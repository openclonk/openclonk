/* Ruby cave */

func Initialize()
{
	// Goal
	var goal = FindObject(Find_ID(Goal_SellGems));
	if (!goal) goal = CreateObject(Goal_SellGems);
	goal->SetTargetAmount(10);
	// Rules
	if (!ObjectCount(Find_ID(Rule_TeamAccount))) CreateObject(Rule_TeamAccount);
	if (!ObjectCount(Find_ID(Rule_BuyAtFlagpole))) CreateObject(Rule_BuyAtFlagpole);
	// Mushrooms before any earth materials, because they create their own caves
	LargeCaveMushroom->Place(15, Rectangle(400,0,800,200));
	// Create earth materials
	// Create them in big clusters so the whole object arrangement looks a bit less uniform and more interesting
	PlaceBatches([Firestone], 3, 100, 5);
	PlaceBatches([Rock, Loam, Loam], 10, 200, 10);
	// Misc vegetation
	SproutBerryBush->Place(5, Rectangle(350,0,850,250));
	Mushroom->Place(5, Rectangle(350,0,850,250));
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
		crew->SetPosition(100+Random(80), 192-10);
		crew->CreateContents(Shovel);
		if (!i)
		{
			crew->CreateContents(Hammer);
		}
		else if (i==1)
		{
			crew->CreateContents(Axe);
		}
	}
	return true;
}

private func InitBase(int owner)
{
	// Create standard base owned by player
	var y=192;
	var workshop = CreateObject(ToolsWorkshop, 70, y, owner);
	if (workshop)
	{
		workshop->CreateContents(Wood, 3);
		workshop->CreateContents(Metal, 1);
		workshop->CreateContents(Pipe, 2);
	}
	var windgenerator = CreateObject(WindGenerator, 150,y, owner);
	var flag = CreateObject(Flagpole, 180,y, owner);
	var foundry = CreateObject(Foundry, 220,y, owner);
	if (foundry)
	{
		foundry->CreateContents(Coal, 3);
		foundry->CreateContents(Metal, 2);
	}
	var chest = CreateObject(Chest, 260,y, owner);
	if (chest)
	{
		chest->CreateContents(DynamiteBox, 1);
		chest->CreateContents(Dynamite, 2);
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
					if (obj=CreateObject(item_ids[Random(n_item_ids)],loc2.x,loc2.y))
					{
						obj->SetPosition(loc2.x,loc2.y);
						++n_created;
					}
	return n_created;
}
