/*-- 
	Hideout
	Author: Maikel
	
	A capture the flag scenario for two teams, both teams occupy a hideout and must steal the flag from
	the opposing team. The hideout is protected by doors and various weapons are there to fight with.
--*/


protected func Initialize()
{
	// Environment 
	PlaceGrass(85);
	
	// Goal: Capture the flag, with bases in both hideouts.
	var goal = CreateObject(Goal_CaptureTheFlag, 0, 0, NO_OWNER);
	goal->SetFlagBase(1, 120, 500);
	goal->SetFlagBase(2, LandscapeWidth() - 120, 500);
	
	// Rules
	CreateObject(Rule_Restart);
	CreateObject(Rule_ObjectFade)->DoFadeTime(5 * 36);
	
	// Doors and spinwheels.
	var gate, wheel;
	gate = CreateObject(CastleDoor, 366, 420, NO_OWNER);
	wheel = CreateObject(SpinWheel, 320, 460, NO_OWNER);
	wheel->SetCastleDoor(gate);
	gate = CreateObject(CastleDoor, 346, 550, NO_OWNER);
	wheel = CreateObject(SpinWheel, 280, 580, NO_OWNER);
	wheel->SetCastleDoor(gate);
	gate = CreateObject(CastleDoor, 846, 450, NO_OWNER);
	wheel = CreateObject(SpinWheel, 780, 480, NO_OWNER);
	wheel->SetCastleDoor(gate);
	gate = CreateObject(CastleDoor, LandscapeWidth() - 364, 420, NO_OWNER);
	wheel = CreateObject(SpinWheel, LandscapeWidth() - 320, 460, NO_OWNER);
	wheel->SetCastleDoor(gate);
	gate = CreateObject(CastleDoor, LandscapeWidth() - 344, 550, NO_OWNER);
	wheel = CreateObject(SpinWheel, LandscapeWidth() - 280, 580, NO_OWNER);
	wheel->SetCastleDoor(gate);
	gate = CreateObject(CastleDoor, LandscapeWidth() - 844, 450, NO_OWNER);
	wheel = CreateObject(SpinWheel, LandscapeWidth() - 780, 480, NO_OWNER);
	wheel->SetCastleDoor(gate);
	
	// Chests with weapons.
	var chest;
	chest = CreateObject(Chest, 110, 590, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 2 * 36);
	chest = CreateObject(Chest, 25, 460, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 2 * 36);
	chest = CreateObject(Chest, 810, 600, NO_OWNER);
	AddEffect("FillOtherChest", chest, 100, 2 * 36);
	chest = CreateObject(Chest, LandscapeWidth() - 110, 590, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 2 * 36);
	chest = CreateObject(Chest, LandscapeWidth() - 25, 460, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 2 * 36);
	chest = CreateObject(Chest, LandscapeWidth() - 810, 600, NO_OWNER);
	AddEffect("FillOtherChest", chest, 100, 2 * 36);
	chest = CreateObject(Chest, 1400, 500, NO_OWNER);
	AddEffect("FillOtherChest", chest, 100, 2 * 36);
	
	// Cannons loaded with 12 shots.
	var cannon;
	cannon = CreateObject(Cannon, 429, 444, NO_OWNER);
	cannon->SetDir(DIR_Right);
	cannon->SetR(15);
	cannon->CreateContents(PowderKeg);
	cannon = CreateObject(Cannon, 2372, 444, NO_OWNER);
	cannon->SetDir(DIR_Left);
	cannon->SetR(-15);
	cannon->CreateContents(PowderKeg);
	
	// Brick edges, notice the symmetric landscape.
	var edges = [[80,480],[100,490],[110,500],[220,550],[230,560],[250,570],[270,580],[340,460],[400,450],[620,570],[630,560],[1320,590],[1330,580],[1340,570],[1370,530],[1380,520],[1390,510],[1340,500]];
	for(var i = 0; i < GetLength(edges); i++)
	{
		CreateObject(BrickEdge, edges[i][0], edges[i][1], NO_OWNER)->PermaEdge();
		CreateObject(BrickEdge, LandscapeWidth() - edges[i][0] + 10 , edges[i][1], NO_OWNER)->PermaEdge();
	}
	
	return;
}

protected func InitializePlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 600, nil, PLRZOOM_Direct);
	return;
}

// Gamecall from CTF goal, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	var relaunch = CreateObject(RelaunchContainer, clonk->GetX(), clonk->GetY(), clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	relaunch->SetRelaunchTime(8, true);
	return;
}

func RelaunchWeaponList() { return [Firestone, Loam, Shovel, Dynamite]; }

/*-- Chest filler effects --*/

global func FxFillBaseChestStart(object target, int num, int temporary)
{
	if(temporary) 
		return 1;
	var w_list = [Bow, Shield, Sword, Javelin, Shovel, Firestone, Dynamite, Loam];
	if (target->ContentsCount() < 5)
		target->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxFillBaseChestTimer(object target)
{
	var w_list = [Bow, Shield, Sword, Javelin, Shovel, Firestone, Dynamite, Loam];
	
	if (target->ContentsCount() < 5)
		target->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxFillOtherChestStart(object target, int num, int temporary)
{
	if(temporary) 
		return 1;
	var w_list = [Bow, Shield, Sword, Javelin, Club, Musket, DynamiteBox, GrappleBow, Balloon];
	if (target->ContentsCount() < 5)
		target->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxFillOtherChestTimer(object target)
{
	var w_list = [Bow, Shield, Sword, Javelin, Club, Musket, DynamiteBox, GrappleBow, Balloon];
	if (target->ContentsCount() < 5)
		target->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func CreateChestContents(id obj_id)
{
	if (!this)
		return;
	var obj = CreateObject(obj_id);
	if (obj_id == Bow)
		obj->CreateContents(Arrow);
	if (obj_id == Musket)
		obj->CreateContents(LeadShot);
	obj->Enter(this);
	return;
}