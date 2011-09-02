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
	CreateObject(Rule_KillLogs);
	
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
	var x=[620, 397, 405, 326, 308, 243, 262, 206, 187, 116, 133, 493, 388, 364, 477, 493, 628, 604, 612, 604, 597, 116, 124, 132, 174, 580, 588, 596, 524, 508, 596, 428, 396, 413, 437, 404, 388, 253, 172, 164, 156, 148, 165, 404, 469, 444, 461, 477, 204, 156, 140, 108, 100, 437, 484, 540, 572, 588, 452, 468, 500, 532, 540, 628, 484, 541, 525, 509, 516, 556, 372, 293, 285, 211, 261, 468, 476, 148, 92, 84, 76, 68, 125];
	var y=[220, 109, 101, 109, 109, 133, 133, 125, 125, 149, 149, 341, 420, 436, 349, 77, 77, 37, 228, 236, 116, 356, 364, 372, 421, 261, 269, 277, 380, 388, 340, 76, 92, 93, 85, 84, 100, 228, 228, 236, 244, 252, 412, 236, 244, 68, 236, 252, 357, 396, 380, 348, 340, 420, 396, 372, 356, 348, 340, 332, 316, 300, -4, 212, 324, 317, 325, 333, 308, 364, 428, 428, 420, 421, 236, 420, 412, 388, 332, 324, 316, 308, 260];
	var d=[1, 2, 2, 2, 3, 3, 2, 2, 3, 3, 2, 2, 1, 1, 2, 2, 3, 3, 1, 1, 0, 0, nil, nil, nil, 3, 3, 3, 1, 1, 1, 1, 1, 2, 2, 1, 1, nil, 1, 1, 1, 1, nil, 1, 0, 1, 0, 0, 3, 0, nil, nil, 0, nil, 1, 1, 1, 1, 1, 1, 1, 1, nil, 1, 1, 2, 2, 2, 1, 1, 1, nil, nil, 1, nil, 1, 1, nil, nil, nil, nil, nil, 1];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i] + 4, NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]);
		edge->PermaEdge();
	}
	return 1;
}

// Gamecall from LastManStanding goal, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	return;
}


global func FxRainTimer(object pTarget, effect, int timer)
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
global func FxDryTimeTimer(object pTarget, effect, int timer)
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
global func FxIntFillChestsStart(object target, effect, int temporary)
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
