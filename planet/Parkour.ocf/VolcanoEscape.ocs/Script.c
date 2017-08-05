/*-- 
	Escape the volcano
	Author: Sven2
	
	Difficult upwards parkour.
--*/

// set to true to allow (endless) respawn
static const LavaParkour_AllowRespawn = false;

protected func Initialize()
{
	// Create the parkour goal.
	var goal = FindObject(Find_ID(Goal_Parkour));
	if (!goal) goal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	if (!LavaParkour_AllowRespawn) goal->DisableRespawnHandling();
	// Set start point.
	goal->SetStartpoint(LandscapeWidth()*2/5, LandscapeHeight()*97/100);
	// Create check points
	var checkpoint_locs = [], n_checkpoint_locs = 0, cave_loc;
	for (var i=0; i<10; ++i)
	{
		// Find a good cave
		cave_loc = FindLocation(Loc_InRect(50,LandscapeHeight()/6,LandscapeWidth()-100,LandscapeHeight()*17/25), Loc_Material("Tunnel"), Loc_Space(10));
		if (cave_loc)
		{
			// No other check point nearby?
			for (var check_loc in checkpoint_locs)
				if (Distance(cave_loc.x, cave_loc.y, check_loc.x, check_loc.y) < 50)
				{
					cave_loc=nil; break;
				}
			// This spot is OK.
			if (cave_loc)
			{
				checkpoint_locs[n_checkpoint_locs++] = cave_loc;
				// max four extra checkpoints
				if (n_checkpoint_locs >= 4) break;
			}
		}
	}
	SortArrayByProperty(checkpoint_locs, "y", true);
	var mode = PARKOUR_CP_Check | PARKOUR_CP_Ordered;
	if (LavaParkour_AllowRespawn) mode |= PARKOUR_CP_Respawn;
	for (cave_loc in checkpoint_locs)
		goal->AddCheckpoint(cave_loc.x, cave_loc.y, mode);
	goal->SetFinishpoint(LandscapeWidth()/2, LandscapeHeight()*5/100);
	// Create earth materials
	// Create them in big clusters so the whole object arrangement looks a bit less uniform and more interesting
	PlaceBatches([Firestone], 5, 100, 15);
	PlaceBatches([Dynamite, Dynamite, Dynamite, DynamiteBox], 3, 50, 6);
	PlaceBatches([Rock, Loam, Loam], 10, 200, 10);
	// Some dead trees.
	Tree_Coniferous_Burned->Place(4);	
	Tree_Coniferous2_Burned->Place(2);	
	Tree_Coniferous3_Burned->Place(2);	
	Tree_Coniferous4_Burned->Place(2);
	// Starting chest
	var start_chest = CreateObjectAbove(Chest, LandscapeWidth()*2/5, LandscapeHeight()*97/100);
	if (start_chest)
	{
		start_chest->CreateContents(Loam,4);
		start_chest->CreateContents(Bread,3);
		start_chest->CreateContents(Firestone,3);
		start_chest->CreateContents(DynamiteBox,2);
	}
	return;
}

func InitializePlayer(int plr)
{
	// Players only
	if (GetPlayerType(plr)!=C4PT_User) return;
	// Harsh zoom range
	for (var flag in [PLRZOOM_LimitMax, PLRZOOM_Direct])
		SetPlayerZoomByViewRange(plr,400,250,flag);
	SetPlayerViewLock(plr, true);
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

// Gamecall from parkour goal, on respawning.
protected func OnPlayerRespawn(int plr, object cp)
{
	var clonk = GetCrew(plr);
	RecoverItem(clonk, Shovel);
	RecoverItem(clonk, Pickaxe);
	RecoverItem(clonk, Loam);
	return;
}

private func RecoverItem(object clonk, id item_id)
{
	// Try to recover the player's item. if it can't be found, recreate one
	// Don't fetch item from allied Clonks though
	var item = FindObject(Find_ID(item_id), Find_Owner(clonk->GetOwner()));
	if (!item || item->Contained() && item->Contained()->GetAlive())
		item = clonk->CreateContents(item_id);
	else
		item->Enter(clonk);
	return item;
}

// Gamecall from parkour goal, on reaching a bonus cp.
protected func GivePlrBonus(int plr, object cp)
{
	// No bonus.
	return;
}

// Lava is deadly.
global func OnInIncendiaryMaterial()
{
	return this->Incinerate(100, NO_OWNER);
}
