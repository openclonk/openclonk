/*-- 
	Ruins
	Author: Mimmo_O
	
	An arena like last man standing round for up to 12 players.
--*/

static const RUINS_RAIN_PERIOD_TIME = 3200;

protected func Initialize()
{
	// Goal.
	CreateObject(Goal_LastManStanding, 0, 0, NO_OWNER);
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_Gravestones);
	GetRelaunchRule()->SetLastWeaponUse(false);
	
	// Mood.
	SetSkyAdjust(RGBa(255, 255, 255, 127), RGB(255, 200, 150));
	SetGamma(109, 105, 101);
	
	// Chests with weapons.
	CreateObjectAbove(Chest, 230, 224, NO_OWNER)->MakeInvincible();
	CreateObjectAbove(Chest, 500, 64, NO_OWNER)->MakeInvincible();
	CreateObjectAbove(Chest, 124, 128, NO_OWNER)->MakeInvincible();
	CreateObjectAbove(Chest, 340, 440, NO_OWNER)->MakeInvincible();
	AddEffect("IntFillChests", nil, 100, 2 * 36);
	
	// Ropeladders to get to the upper part.

	CreateObjectAbove(Ropeladder, 380, 112, NO_OWNER)->Unroll(-1, 0, 19);
	CreateObjectAbove(Ropeladder, 135, 135, NO_OWNER)->Unroll(1, 0, 16);
	
	// Objects fade after 5 seconds.
	CreateObject(Rule_ObjectFade)->DoFadeTime(5 * 36);

	AddEffect("DryTime",nil, 100, 2);
	return;
}


global func FxRainTimer(object pTarget, effect, int timer)
{
	if (timer<400)
	{
		InsertMaterial(Material("Water"),Random(LandscapeWidth()-60)+30, 1, Random(7)-3, 100 + Random(100));
		return 1;
	} 
		for (var i = 0; i<(6 + Random(3)); i++)
	{
		InsertMaterial(Material("Water"),Random(LandscapeWidth()-60)+30, 1, Random(7)-3, 100 + Random(100));
	}
	if (timer>(RUINS_RAIN_PERIOD_TIME + Random(800))) 
	{
	AddEffect("DryTime",nil, 100, 2);
	return -1;	
	}
	
	return 1;
}
global func FxDryTimeTimer(object pTarget, effect, int timer)
{
	if (timer<(380 + Random(300))){
	InsertMaterial(Material("Water"),Random(LandscapeWidth()-60)+30, 1, Random(7)-3, 100 + Random(100));
		return 1;
	}
	ExtractLiquidAmount(310 + Random(50),430 + Random(10),6 + Random(4));
	
	if (!GBackLiquid(335, 430))
	{
		AddEffect("Rain",nil, 100, 2);
		return -1;
	}	
}



// Refill/fill chests.
global func FxIntFillChestsStart(object target, effect, int temporary)
{
	if (temporary) return 1;
	var chests = FindObjects(Find_ID(Chest));
	var w_list = [Bow, Blunderbuss, Shield, Sword, Club, GrenadeLauncher, Bow, Blunderbuss, Shield, Sword, Club, GrenadeLauncher, DynamiteBox];
	
	for (var chest in chests)
		for (var i = 0; i<4; ++i)
			chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxIntFillChestsTimer()
{
	SetTemperature(100);
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	var w_list = [Boompack, IronBomb, IronBomb, Firestone, Bow, Blunderbuss, Sword, Javelin];
	
	if (chest->ContentsCount() < 5)
		chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func CreateChestContents(id obj_id)
{
	if (!this)
		return;
	var obj = CreateObjectAbove(obj_id);
	if (obj_id == Bow)
		obj->CreateContents(Arrow);
	if (obj_id == Blunderbuss)
		obj->CreateContents(LeadBullet);
	obj->Enter(this);
	return;
}

// GameCall from RelaunchContainer.
func OnClonkLeftRelaunch(object clonk)
{
	clonk->SetPosition(RandomX(200, LandscapeWidth() - 200), -20);
}

func KillsToRelaunch() { return 0; }
func RelaunchWeaponList() { return [Bow, Shield, Sword, Club, Javelin, Blunderbuss]; }
