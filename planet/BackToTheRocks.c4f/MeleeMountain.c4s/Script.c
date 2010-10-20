/*-- Mountain Melee --*/

protected func Initialize()
{
	CreateObject(Goal_LastManStanding, 0, 0, NO_OWNER);
	SetSkyAdjust (RGB(230,210,150), RGB(150,100,0));
	//Environment
	PlaceGrass(80);
	// Chests.
	CreateObject(Chest, 600, 495, NO_OWNER);
	CreateObject(Chest, 1169, 454, NO_OWNER);
	CreateObject(Chest, 1123, 124, NO_OWNER);
	CreateObject(Chest, 180, 404, NO_OWNER);
	CreateObject(Chest, 261, 163, NO_OWNER);
	CreateObject(Rule_ObjectFade)->DoFadeTime(5*37);
	
	AddEffect("IntFillChests", nil, 100, 70, this);
	return;
}

// Gamecall from lastmanstanding rule, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	return;
}

// Refill chests.
global func FxIntFillChestsTimer()
{
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	var w_list = [Shovel,Bow,Musket,Club,Javelin,Boompack,Loam,Firestone,JarOfWinds,GrappleBow];
	
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

// GameCall from lastmanstanding
func OnClonkLeftRelaunch(object clonk)
{
	clonk->SetPosition(RandomX(30, LandscapeWidth() - 30), -20);
}

func KillsToRelaunch() { return 0; }
func RelaunchWeaponList(){ return [Boompack, JarOfWinds, GrappleBow, Shovel]; }
