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
protected func OnPlrRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	clonk->Contents()->RemoveObject();
	var relaunch = CreateObject(Goal_Relaunch, LandscapeWidth() / 2, LandscapeHeight() / 2, clonk->GetOwner());
	clonk->Enter(relaunch);
	relaunch->~WeaponMenu(clonk);
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
	if (obj_id == Bow)
	{
		var bow = CreateObject(Bow, 0, 0, NO_OWNER);
		bow->CreateContents(Arrow);
		bow->Enter(this);
	}
	else if (obj_id == Musket)
	{
		var bow = CreateObject(Musket, 0, 0, NO_OWNER);
		bow->CreateContents(LeadShot);
		bow->Enter(this);
	}
	else
		this->CreateContents(obj_id);
	return;	
}

// GameCall from MicroMelee_Relaunch
func OnClonkLeftRelaunchObject(clonk)
{
	clonk->SetPosition(RandomX(30, LandscapeWidth() - 30), -20);
}

func KillsToRelaunch() { return 0; }
