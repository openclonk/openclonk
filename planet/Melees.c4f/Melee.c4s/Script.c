/*-- Melee --*/

protected func Initialize()
{
	CreateObject(Goal_LastManStanding, 0, 0, NO_OWNER);
	SetSkyAdjust (RGB(230,210,150), RGB(150,100,0));
	// Chests.
	CreateObject(Chest, 500, 470, NO_OWNER);
	CreateObject(Chest, 70, 508, NO_OWNER);
	CreateObject(Chest, 218, 337, NO_OWNER);
	CreateObject(Chest, 805, 327, NO_OWNER);
	CreateObject(Chest, 431, 137, NO_OWNER);
	CreateObject(Chest, 958, 466, NO_OWNER);
	CreateObject(Chest, 919, 219, NO_OWNER);
	AddEffect("IntFillChests", nil, 200, 70, this);
	return;
}

// Gamecall from Mircomelee rule, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	return;
}

// Refill/fill chests.
global func FxIntFillChestsStart()
{
	var chests = FindObjects(Find_ID(Chest));
	var w_list = [Bow,Musket,Shield,Sword,Club,Javelin,Bow,Musket,Shield,Sword,Club,Javelin,DynamiteBox];
	
	for(var chest in chests)
		for(var i=0; i<4; ++i)
			chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
}

global func FxIntFillChestsTimer()
{
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	var w_list = [Boompack,DynamiteBox,Loam,Firestone,Bow,Musket,Sword,Javelin,JarOfWinds];
	
	if (chest->ContentsCount() < 5)
		chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
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

// GameCall from MicroMelee_Relaunch
func OnClonkLeftRelaunch(clonk)
{
	clonk->SetPosition(RandomX(30, LandscapeWidth() - 30), -20);
}

func KillsToRelaunch() { return 0; }
