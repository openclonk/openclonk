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
	var w_list = [Shovel,Bow,Musket,Shield,Sword,Club,Javelin,DynamiteBox];
	
	for(var chest in chests)
		for(var i=0; i<4; ++i)
			chest->CreateContents(w_list[Random(GetLength(w_list))]);
}

global func FxIntFillChestsTimer()
{
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	var w_list = [LeadShot,Boompack,Dynamite,Loam,Firestone,Arrow];
	
	if (chest->ContentsCount() < 5)
		chest->CreateContents(w_list[Random(GetLength(w_list))]);
}

// GameCall from MicroMelee_Relaunch
func OnClonkLeftRelaunchObject(clonk)
{
	clonk->SetPosition(RandomX(30, LandscapeWidth() - 30), -20);
}

func KillsToRelaunch() { return 0; }
