/*-- 
	Escape the volcano EXTREME
	Author: Sven2
	
	Difficult upwards parkour. Now with extra volcano coming from bottom!
--*/

static g_volcano;

protected func Initialize()
{
	var w = LandscapeWidth(), h = LandscapeHeight();
	// Create the parkour goal.
	var goal = FindObject(Find_ID(Goal_Parkour));
	if (!goal) goal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	goal->DisableRespawnHandling();
	// Set start and finish point.
	goal->SetStartpoint(w*2/5, h*93/100);
	goal->SetFinishpoint(w/2, h*5/100);
	// Create earth materials
	// Create them in big clusters so the whole object arrangement looks a bit less uniform and more interesting
	PlaceBatches([Firestone], 5, 100, 15);
	PlaceBatches([Dynamite, Dynamite, Dynamite, DynamiteBox], 3, 50, 6);
	PlaceBatches([Rock, Loam, Loam], 10, 200, 10);
	// Starting chest
	var start_chest = CreateObjectAbove(Chest, w*2/5, h*94/100);
	if (start_chest)
	{
		start_chest->CreateContents(Loam,4);
		start_chest->CreateContents(Bread,3);
		start_chest->CreateContents(Firestone,3);
		start_chest->CreateContents(DynamiteBox,2);
	}
	// Create big volcano
	g_volcano=CreateObjectAbove(BigVolcano,0,0,NO_OWNER);
	var h0 = h-10;
	g_volcano->Activate(h0, h*10/100);
	// Schedule script to update volcano speed multiplier
	
	var fx_volcano = new Effect {
		Timer = Scenario.VolcanoTimer
	};
	
	CreateEffect(fx_volcano, 1, 40);
	ScheduleCall(nil, Global.RemoveAll, 3, nil, Find_ID(Rule_Restart));
	// Bottom is open, so put some stable lava here to prevent remaining lava from just flowing out of the map
	DrawMaterialQuad("StableLava",0,h0,w,h0,w,h,0,h);
	return;
}

// Timer callback: Update volcano speed (rubberband effect)
func VolcanoTimer()
{
	// Safety
	if (!g_volcano) return;
	// Get volcano height
	var y_volcano = g_volcano->GetLavaPeak();
	// Get player progress
	var y_plr, crew, n_crew;
	for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
		if (crew = GetCursor(GetPlayerByIndex(i, C4PT_User)))
		{
			y_plr += crew->GetY();
			++n_crew;
		}
	if (n_crew) y_plr = y_plr / n_crew;
	// Calc rubber band
	var rubber_length = 85 * y_plr / LandscapeHeight() + 65;
	var new_multiplier;
	if (n_crew)
		new_multiplier = Max(1, (y_volcano - y_plr) / rubber_length);
	else
		new_multiplier = 1;
	g_volcano->SetSpeedMultiplier(new_multiplier);
	//Log("speed %v", new_multiplier);
	return true;
}

func InitializePlayer(int plr)
{
	// Players only
	if (GetPlayerType(plr)!=C4PT_User) return;
	// Harsh zoom range
	for (var flag in [PLRZOOM_LimitMax, PLRZOOM_Direct])
		SetPlayerZoomByViewRange(plr,400,250,flag);
	SetPlayerViewLock(plr, false); // no view lock so you can see the volcano!
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
