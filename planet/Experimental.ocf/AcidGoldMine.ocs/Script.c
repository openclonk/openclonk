/* Acid gold mine */

func Initialize()
{
	// Goal
	var goal = FindObject(Find_ID(Goal_Wealth));
	if (!goal) goal = CreateObject(Goal_Wealth);
	goal->SetWealthGoal(225);
	// Acid rain!
	Cloud->Place(10);
	Cloud->SetPrecipitation("Acid", 100);
	// Earthquacks and lava!
	Earthquake->SetChance(100);
	Volcano->SetChance(3);
	Volcano->SetMaterial("DuroLava");
	Meteor->SetChance(15);
	// We aren't doing much outside anyway; celestials are a bit of a waste
	/*CreateObject(Environment_Celestial);
	var time = CreateObject(Environment_Time);
	time->SetTime(60*12);
	time->SetCycleSpeed(20);*/
	// Starting materials in lorry
	var pos = FindTopSpot();
	var lorry = CreateObject(Lorry, pos.x, pos.y);
	if (lorry)
	{
		lorry->CreateContents(WallKit,3);
		lorry->CreateContents(Wood,12);
		lorry->CreateContents(Metal,5);
		lorry->CreateContents(Rock,5);
		lorry->CreateContents(Bread,3);
	}
	// Mushrooms before any earth materials, because they create their own caves
	LargeCaveMushroom->Place(15, Rectangle(LandscapeWidth()/4,LandscapeHeight()*72/100,LandscapeWidth()/2,LandscapeHeight()*7/100));
	// Create earth materials
	// Create them in big clusters so the whole object arrangement looks a bit less uniform and more interesting
	PlaceBatches([Firestone], 5, 100, 15);
	PlaceBatches([Dynamite], 3, 50, 6);
	PlaceBatches([Rock, Loam, Loam], 10, 200, 10);
	// Create some chests in caves
	var chest_pos, chest;
	var chest_sets = [[[DynamiteBox,2], [Dynamite,5]], [[Loam,5], [WallKit,3], [Wood,8]], [[Bread,5],[Firestone,5],[Wood,8]]];
	for (var i=0; i<3; ++i)
		if (chest_pos = FindLocation(Loc_Material("Tunnel"), Loc_Wall(CNAT_Bottom)))
			if (chest = CreateObject(Chest, chest_pos.x, chest_pos.y))
				for (var chest_fill in chest_sets[i])
					chest->CreateContents(chest_fill[0],chest_fill[1]);
	// A barrel
	if (chest_pos = FindLocation(Loc_Material("Tunnel"), Loc_Wall(CNAT_Bottom)))
		CreateObject(Barrel, chest_pos.x, chest_pos.y);
	// Misc vegetation
	SproutBerryBush->Place(5, Rectangle(0,LandscapeHeight()/4,LandscapeWidth(),LandscapeHeight()*3/4));
	Mushroom->Place(5, Rectangle(0,LandscapeHeight()/4,LandscapeWidth(),LandscapeHeight()*3/4));
	Tree_Coniferous_Burned->Place(2, Rectangle(0,0,LandscapeWidth(),LandscapeHeight()/4));
	// Bottom item killer
	var fx = AddEffect("KeepAreaClear", nil, 1, 5);
	fx.search_criterion=Find_And(Find_AtRect(0,LandscapeHeight()-10,LandscapeWidth(),10), Find_Not(Find_Category(C4D_StaticBack)));
	return true;
}

func InitializePlayer(int plr)
{
	// Harsh zoom range
	for (var flag in [PLRZOOM_LimitMax, PLRZOOM_Direct])
		SetPlayerZoomByViewRange(plr,400,250,flag);
	SetPlayerViewLock(plr, true);
	// Position and materials
	var i, crew;
	for (i=0; crew=GetCrew(plr,i); ++i)
	{
		var pos = FindTopSpot();
		crew->SetPosition(pos.x, pos.y-10);
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

func FindTopSpot()
{
	return FindLocation(Loc_InRect(LandscapeWidth()/4,0,LandscapeWidth()/2,LandscapeHeight()/9), Loc_Wall(CNAT_Bottom), Loc_Space(10)) ?? {x=LandscapeWidth()/3+Random(30), y=LandscapeHeight()/12  };
}


global func FxKeepAreaClearTimer(object q, proplist fx, int time)
{
	for (var obj in FindObjects(fx.search_criterion))
		if (obj) obj->RemoveObject();
	return FX_OK;
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
