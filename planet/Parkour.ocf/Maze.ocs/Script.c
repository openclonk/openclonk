/*-- 
	Maze
	Author: Sven2
	
	Dynamic maze
--*/

local goal_cave;

func InitializePlayer(int plr)
{
	// Harsher zoom range
	for (var flag in [PLRZOOM_LimitMax, PLRZOOM_Direct])
		SetPlayerZoomByViewRange(plr,240,160,flag);
		//SetPlayerZoomByViewRange(plr,LandscapeWidth(),LandscapeHeight(),flag);
	SetPlayerViewLock(plr, true);
	// Position and materials
	LaunchPlayer(plr);
	return true;
}

func LaunchPlayer(int plr)
{
	// Position and materials
	var starting_cave = g_caves[0];
	var i, crew;
	for (i=0; crew=GetCrew(plr,i); ++i)
	{
		crew->SetPosition(starting_cave.X/2, starting_cave.Y-18);
		for (var tool in [Pickaxe, GrappleBow, SprayCan])
		{
			var obj = FindObject(Find_ID(tool), Find_Owner(plr), Find_NoContainer());
			if (obj) obj->RemoveObject();
			crew->CreateContents(tool);
		}
		crew->CreateContents(Dynamite,2);
	}
	return true;
}

func RelaunchPlayer(int plr)
{
	var clonk = CreateObjectAbove(Clonk,0,0,plr);
	if (!clonk) return false;
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	return LaunchPlayer(plr);
}

func CreateBonus(int x, int y, int value, bool is_cooperative)
{
	var obj;
	if (Random(value) > 50)
	{
		obj = CreateObjectAbove(Signpost, x,y);
		if (obj)
		{
			if (Random(value) > 5)
			{
				var dx = goal_cave.X - x, dy = goal_cave.Y - y;
				if (Abs(dx) > Abs(dy))
					if (dx>0) obj->SetText("$MsgGoalRight$"); else obj->SetText("$MsgGoalLeft$");
				else
					if (dy>0) obj->SetText("$MsgGoalBelow$"); else obj->SetText("$MsgGoalAbove$");
			}
		}
	}
	else
	{
		obj = CreateObjectAbove(Chest, x,y);
		if (obj)
		{
			if (Random(value) > 90) obj->CreateContents(Shovel);
			if (Random(value) > 90) obj->CreateContents(WindBag);
			if (Random(value) > 90) obj->CreateContents(TeleGlove);
			if (Random(value) > 90) obj->CreateContents(Club);
			if (Random(value) > 5) obj->CreateContents(Loam, 1+Random(2));
			if (Random(value) > 5) obj->CreateContents(Dynamite, 1+Random(2));
			if (Random(value) > 25) obj->CreateContents(DynamiteBox, 1+Random(2));
			if (Random(value) > 10) obj->CreateContents(Bread, 1+Random(2));
		}
	}
	return obj;
}

protected func Initialize()
{
	var zoom = 10, cave, n_caves = GetLength(g_caves);
	for (cave in g_caves) { cave.X *= zoom; cave.Y *= zoom; }
	// Light at end cave
	var light = CreateLight(g_end_cave_x * zoom, g_end_cave_y * zoom, 100, Fx_Light.LGT_Constant, NO_OWNER, 100);
	if (light) light->SetLightColor(0xff8080);
	// Goal
	var is_cooperative = (SCENPAR_Goal == 1);
	var starting_cave = g_caves[0];
	var goal = FindObject(Find_ID(Goal_RubyHunt));
	if (!goal) goal = CreateObject(Goal_RubyHunt);
	goal->SetPosition();
	goal->SetGoalRect(Rectangle(0, starting_cave.Y-40, starting_cave.X-20, 40));
	goal->SetCooperative(is_cooperative);
	goal_cave = g_caves[n_caves-1];
	// Place extra elements in caves (except at start/end)
	for (cave in g_caves)
	{
		if (cave == g_caves[0] || cave == g_caves[n_caves-1]) continue;
		var x=cave.X, y=cave.Y;
		while (!GBackSolid(x,y)) ++y;
		if (cave.n_links <= 1)
		{
			// This is a dead end.
			if (cave.dirs == 8)
			{
				// Facing downwards. Hard to reach, but cannot place a chest here :(
				CreateObjectAbove(Trunk, cave.X, cave.Y)->SetR(160+Random(41));
			}
			else
			{
				CreateBonus(x, y, 100);
			}
		}
		else if (!(cave.dirs & 8))
		{
			// Connecting cave without bottom
			CreateBonus(x, y, 25 + 25 * !cave.is_main_path, is_cooperative);
		}
	}
	return true;
}
