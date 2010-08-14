/*-- Melee --*/

protected func Initialize()
{
	CreateObject(Goal_LastManStanding, 0, 0, NO_OWNER);
	SetSkyAdjust(RGBa(255,255,255,127),RGB(255,200,150));
	SetGamma(RGB(40,35,30), RGB(140,135,130), RGB(255,250,245));
	// Chests.
	CreateObject(Chest, 420, 320, NO_OWNER);
	CreateObject(Chest, 230, 170, NO_OWNER);
	CreateObject(Chest, 570, 220, NO_OWNER);
	CreateObject(Chest, 70, 80, NO_OWNER);
	CreateObject(Chest, 340, 200, NO_OWNER);
	CreateObject(Rule_ObjectFade); 
	CreateObject(Rule_ObjectFade); 
	CreateObject(Rule_ObjectFade); 
	CreateObject(Rule_ObjectFade); 
	CreateObject(Rule_ObjectFade); 

	AddEffect("IntFillChests", nil, 200, 70, this);
	Edges();
	return;
}


protected func Edges()
{
var x=[545, 365, 545, 525, 385, 495, 475, 455, 565, 555, 525, 475, 425, 245, 75, 95, 125, 135, 205, 475, 455, 435, 475, 405, 245, 225, 155, 145, 125, 155, 175, 205, 255, 315, 455, 425, 405, 385, 375, 285, 265, 245, 225];
var y=[295, 165, -5, 295, 285, 315, 325, 335, 345, 355, 365, 375, 385, 385, 325, 345, 355, 365, 355, 285, 235, 155, 245, 235, 345, 295, 295, 375, 255, 245, 235, 225, 225, 165, 135, 115, 105, 95, 85, 85, 95, 105, 115];
for(var i=0; i<GetLength(x); i++)
{
	CreateObject(BrickEdge,x[i], y[i]+5);
}
CreateObject(BrickEdge,355,250)->SetP(3);
CreateObject(BrickEdge,305,240)->SetP(2);

}

// Gamecall from LastManStanding goal, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	clonk->Contents()->RemoveObject();
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	return;
}

// Refill/fill chests.
global func FxIntFillChestsStart()
{
	var chests = FindObjects(Find_ID(Chest));
	var w_list = [Bow,Musket,Shield,Sword,Club,Javelin,Bow,Musket,Shield,Sword,Club,Javelin,DynamiteBox,JarOfWinds];
	
	for(var chest in chests)
		for(var i=0; i<4; ++i)
			chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
}

global func FxIntFillChestsTimer()
{
	SetTemperature(100);
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	var w_list = [Boompack,Dynamite,Loam,Firestone,Bow,Musket,Sword,Javelin,JarOfWinds];
	
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
func OnClonkLeftRelaunch(object clonk)
{
	clonk->SetPosition(RandomX(30, LandscapeWidth() - 30), -20);
}

func KillsToRelaunch() { return 0; }
func RelaunchWeaponList(){ return [Bow,Shield,Sword,Club,Javelin,Musket]; }
