/*-- Mountain Melee --*/

protected func Initialize()
{
	CreateObject(Goal_LastManStanding, 0, 0, NO_OWNER);
	SetSkyAdjust (RGB(230,210,150), RGB(150,100,0));
	//Environment
	CreateObject(Environment_Grass);
	PlaceGrass(80);
	// Chests.
	CreateObject(Chest, 600, 495, NO_OWNER);
	CreateObject(Chest, 1169, 454, NO_OWNER);
	CreateObject(Chest, 1123, 124, NO_OWNER);
	CreateObject(Chest, 180, 404, NO_OWNER);
	CreateObject(Chest, 261, 163, NO_OWNER);
	AddEffect("IntFillChests", nil, 100, 70, this);
	return;
}

// Gamecall from Mircomelee rule, on respawning.
protected func OnPlrRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	clonk->SetPosition(Random(LandscapeWidth()),10);
	var shovel = clonk->Contents();
	shovel->Exit();
	shovel->RemoveObject();
	return;
}

// Refill chests.
global func FxIntFillChestsTimer()
{
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	if (ObjectCount(Find_Container(chest)) > 5)
		chest->Contents(Random(6))->RemoveObject();
	var w_list = [Shovel,Bow,Musket,Club,Javelin,Boompack,Dynamite,Loam,Firestone,Balloon,JarOfWinds,GrappleBow];
	if (chest)
		chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return;
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

// The weapons available to the players. Needed by MicroMelee_Relaunch
func GetMicroMeleeWeaponList()
{
	return 0;
}

// GameCall from MicroMelee_Relaunch
func OnClonkLeftRelaunchObject(clonk)
{
	clonk->SetPosition(RandomX(30, LandscapeWidth() - 30), -20);
}

func KillsToRelaunch() { return 0; }
