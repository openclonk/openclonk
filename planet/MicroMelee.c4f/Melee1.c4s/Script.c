/*-- Melee --*/

protected func Initialize()
{
	CreateObject(Rule_MicroMelee, 0, 0, NO_OWNER);
	SetSkyAdjust (RGB(230,210,150), RGB(150,100,0));
	// Chests.
	CreateObject(Chest, 500, 470, NO_OWNER);
	CreateObject(Chest, 70, 508, NO_OWNER);
	CreateObject(Chest, 218, 337, NO_OWNER);
	CreateObject(Chest, 805, 327, NO_OWNER);
	CreateObject(Chest, 431, 137, NO_OWNER);
	CreateObject(Chest, 958, 466, NO_OWNER);
	CreateObject(Chest, 919, 219, NO_OWNER);
	AddEffect("IntFillChests", nil, 100, 70, this);
	return;
}

// Gamecall from Mircomelee rule, on respawning.
protected func OnPlrRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	clonk->Contents()->RemoveObject();
	var relaunch = CreateObject(MicroMelee_Relaunch, LandscapeWidth() / 2, LandscapeHeight() / 2, clonk->GetOwner());
	clonk->Enter(relaunch);
	relaunch->~WeaponMenu(clonk);
	return;
}

// Refill chests.
global func FxIntFillChestsTimer()
{
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	if (ObjectCount(Find_Container(chest)) > 5)
		chest->Contents(Random(6))->RemoveObject();
	var w_list = [Shovel,Bow,Arrow,Musket,LeadShot,Shield,Sword,Club,Javelin,Boompack,DynamiteBox,Dynamite,Loam,Firestone,Fireglobe];
	if (chest)
		chest->CreateContents(w_list[Random(GetLength(w_list))]);
	return;
}

// The weapons available to the players. Needed by MicroMelee_Relaunch
func GetMicroMeleeWeaponList()
{
	return[Shovel,Bow,Arrow,Musket,LeadShot,Shield,Sword,Club,Javelin,Boompack,DynamiteBox,Dynamite,Loam,Firestone,Fireglobe];
}

// GameCall from MicroMelee_Relaunch
func OnClonkLeftRelaunchObject(clonk)
{
	clonk->SetPosition(RandomX(30, LandscapeWidth() - 30), -20);
}

func KillsToRelaunch() { return 0; }
