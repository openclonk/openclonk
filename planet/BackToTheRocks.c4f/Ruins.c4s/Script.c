/*-- 
	Ruins
	Author: Mimmo_O
	
	An arena like last man standing round for up to 12 players.
--*/

static const RUINS_RAIN_PERIOD_TIME=3200;

protected func Initialize()
{
	// Goal.
	CreateObject(Goal_LastManStanding, 0, 0, NO_OWNER);
	
	// Mood.
	SetSkyAdjust(RGBa(255, 255, 255, 127), RGB(255, 200, 150));
	SetGamma(RGB(40, 35, 30), RGB(140, 135, 130), RGB(255, 250, 245));
	
	// Chests with weapons.
	CreateObject(Chest, 230, 220, NO_OWNER);
	CreateObject(Chest, 500, 30, NO_OWNER);
	CreateObject(Chest, 120, 110, NO_OWNER);
	CreateObject(Chest, 340, 400, NO_OWNER);
	AddEffect("IntFillChests", nil, 100, 2 * 36, this);
	
	// Ropeladders to get to the upper part.

	CreateObject(Ropeladder, 382, 112, NO_OWNER)->Unroll(-1,0,19);
	CreateObject(Ropeladder, 135, 135, NO_OWNER)->Unroll(1,0,16);
	
	// Objects fade after 5 seconds.
	CreateObject(Rule_ObjectFade)->DoFadeTime(5 * 36);

	// Smooth brick edges.
	PlaceEdges();
	AddEffect("DryTime",0,100,2);
	return;
}

global func PlaceEdges()
{
	var x=[45, 215, 285, 295, 375, 545, 515, 485, 505, 535, 545, 625, 545, 525, 385, 495, 475, 455, 575, 555, 525, 475, 435, 75, 95, 145, 155, 205, 475, 455, 445, 475, 405, 245, 225, 155, 165, 125, 155, 175, 205, 255, 395, 405, 435, 415, 385, 425, 595, 485, 505, 595, 585, 575, 175, 135, 125, 105, 65, 55, 45, 45, 45, 115, 115, 125, 65, 75, 85, 85, 595, 605, 615, 605, 625, 555, 495, 465, 365, 385, 265];
	var y=[185, 415, 415, 425, 425, 365, 305, 345, 335, 315, 295, 195, -5, 295, 285, 315, 325, 335, 345, 355, 375, 405, 415, 325, 345, 385, 395, 355, 285, 235, 65, 245, 235, 345, 295, 295, 405, 255, 245, 235, 225, 225, 105, 85, 85, 95, 95, 75, 335, 395, 385, 285, 265, 255, 415, 375, 365, 355, 295, 285, 265, 225, 245, 265, 285, 295, 95, 65, 55, 35, 115, 235, 225, 35, 75, 15, 75, 415, 435, 415, 295];
	
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i] + 5, NO_OWNER);
		edge->Initialize();
		edge->PermaEdge();
	}

	var x=[105, 135, 115, 185, 205, 265, 245, 305, 325];
	var y=[135, 145, 145, 125, 125, 135, 135, 105, 105];
	var d=[3, 2, 3, 3, 2, 2, 3, 3, 2];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i] + 5, NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->PermaEdge();
	}

	var a;
	a=CreateObject(BrickEdge, 355, 250, NO_OWNER);
	a->SetP(3); a->PermaEdge();
	a=CreateObject(BrickEdge, 305, 240, NO_OWNER);
	a->SetP(2); a->PermaEdge();
	return;
}

// Gamecall from LastManStanding goal, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	return;
}


global func FxRainTimer(object pTarget, int noum, int timer)
{
	if(timer<400)
	{
		InsertMaterial(Material("Water"),Random(LandscapeWidth()-60)+30,1,Random(7)-3,100+Random(100));
		return 1;
	} 
		for(var i=0; i<(6+Random(3)); i++)
	{
		InsertMaterial(Material("Water"),Random(LandscapeWidth()-60)+30,1,Random(7)-3,100+Random(100));
	}
	if(timer>(RUINS_RAIN_PERIOD_TIME+Random(800))) 
	{
	AddEffect("DryTime",0,100,2);
	return -1;	
	}
	
	return 1;
}
global func FxDryTimeTimer(object pTarget, int noum, int timer)
{
	if(timer<(380+Random(300))){
	InsertMaterial(Material("Water"),Random(LandscapeWidth()-60)+30,1,Random(7)-3,100+Random(100));
		return 1;
	}
	for(var i=0; i<6+Random(4);i++)
		ExtractLiquid(310+Random(50),430+Random(10));
	if(!GBackLiquid(335,430))
	{
		AddEffect("Rain",0,100,2);
		return -1;
	}	
}



// Refill/fill chests.
global func FxIntFillChestsStart(object target, int num, int temporary)
{
	if(temporary) return 1;
	var chests = FindObjects(Find_ID(Chest));
	var w_list = [Bow, Musket, Shield, Sword, Club, Javelin, Bow, Musket, Shield, Sword, Club, Javelin, DynamiteBox];
	
	for(var chest in chests)
		for(var i=0; i<4; ++i)
			chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxIntFillChestsTimer()
{
	SetTemperature(100);
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	var w_list = [Boompack, Dynamite, Loam, Firestone, Bow, Musket, Sword, Javelin];
	
	if (chest->ContentsCount() < 5)
		chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
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

// GameCall from RelaunchContainer.
func OnClonkLeftRelaunch(object clonk)
{
	clonk->SetPosition(RandomX(200, LandscapeWidth() - 200), -20);
}

func KillsToRelaunch() { return 0; }
func RelaunchWeaponList() { return [Bow, Shield, Sword, Club, Javelin, Musket]; }
