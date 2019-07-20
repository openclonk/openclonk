/*-- 
	Hideout
	Author: Mimmo
	
	A capture the flag scenario for two teams, both teams occupy a hideout and must steal the flag from
	the opposing team.
--*/

protected func Initialize()
{
	// Environment 
	CreateObject(Rule_ObjectFade)->DoFadeTime(10 * 36);
	
	GetRelaunchRule()->SetLastWeaponUse(false);
	
	var time = CreateObject(Time);
	time->SetTime();
	time->SetCycleSpeed();
	FindObject(Find_ID(Moon))->SetMoonPhase(3);
	FindObject(Find_ID(Moon))->SetCon(150);
	FindObject(Find_ID(Moon))->SetPosition(LandscapeWidth()/2, 150);

	// Goal: Capture the flag, with bases in both hideouts.
	var goal = CreateObject(Goal_CaptureTheFlag, 0, 0, NO_OWNER);
	goal->SetFlagBase(1, 135, 266);
	goal->SetFlagBase(2, LandscapeWidth() - 135, 266);
	CreateObject(Rule_KillLogs);	
	
	var gate = CreateObjectAbove(StoneDoor, 345, 272, NO_OWNER);
	gate->SetClrModulation(RGB(140, 185, 255));
	gate->SetAutoControl(1);
	var gate = CreateObjectAbove(StoneDoor, LandscapeWidth()-344, 272, NO_OWNER);
	gate->SetClrModulation(RGB(140, 185, 255));
	gate->SetAutoControl(2);

	// Chests with weapons.
	var chest;
	chest = CreateObjectAbove(Chest, 60, 220, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillBaseChest", chest, 100, 7 * 36, nil, nil, false);
	chest = CreateObjectAbove(Chest, 150, 370, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillBaseChest", chest, 100, 7 * 36, nil, nil, true);
	chest = CreateObjectAbove(Chest, LandscapeWidth() - 60, 220, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillBaseChest", chest, 100, 7 * 36, nil, nil, false);
	chest = CreateObjectAbove(Chest, LandscapeWidth() - 150, 370, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillBaseChest", chest, 100, 7 * 36, nil, nil, true);

	
	chest = CreateObjectAbove(Chest, LandscapeWidth()/2, 320, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillOtherChest", chest, 100, 5 * 36);
	
	AddEffect("SnowyWinter", nil, 100, 1);
	Sound("Environment::WindLoop",true, 20, nil,+1);
	AddEffect("GeysirExplosion", nil, 100, 1);
	// Brick edges, notice the symmetric landscape.
	PlaceEdges();
	return;
}

protected func CaptureFlagCount() { return 2;} //4 + GetPlayerCount()) / 2; }

global func FxGeysirExplosionTimer(object target, effect)
{
	effect.counter++;
	
	if (Random(2))
	{
		var x = 600 + Random(300);
		var y = 250 + Random(200);
		while (GetMaterial(x, y) != Material("Water"))
		{
			var x = 600 + Random(300);
			var y = 250 + Random(200);
		}
		Bubble(1, x, y);
	}
	if (effect.counter>1900 && effect.counter<2030)
	{

		for (var i = 0; i<(-(1900-effect.counter)); i += 10 )
		{
			var x = 600 + Random(300);
			var y = 250 + Random(200);
			while (GetMaterial(x, y) != Material("Water"))
			{
				var x = 600 + Random(300);
				var y = 250 + Random(200);
			}
			Bubble(1, x, y);
		}
	}
	if (effect.counter>2000)
	{
		var x = LandscapeWidth()/2;
		var y = 280;
		while (!GBackLiquid(x, y)) y++;
		y -= 3;
		for (var i = 0; i<(45); i++)InsertMaterial(Material("Water"),x + RandomX(-9, 9),y-Random(5),RandomX(-10, 10)+RandomX(-5, 5)+RandomX(-10, 10),-(10 + Random(50)+Random(30)+Random(60)));
		for (var i = 0; i<(25); i++)InsertMaterial(Material("Water"),x + RandomX(-16, 16),y-Random(5),RandomX(-10, 10)+RandomX(-15, 15)+RandomX(-20, 20),-(10 + Random(50)));
		CreateParticle("Air", PV_Random(x-6, x + 6), PV_Random(y-3, y), PV_Random(-15, 15), PV_Random(-90, -5), PV_Random(20, 100), Particles_Air());
		if (effect.counter>2072) effect.counter = 0;
		for (var obj in FindObjects(Find_InRect(x-30, y-200, 60, 210)))
		{
			obj->SetYDir(Max(obj->GetYDir()-15,-50));
		}

	}
}

global func FxSnowyWinterTimer(object target, effect, int time)
{
	if (time%1200 == 100 ) 
	{
		var add = RandomX(-2, 2);
		effect.snow_count = BoundBy(effect.snow_count + add, 1, 5);	
	}
	for (var i = 0; i<(effect.snow_count); i++)
	{
		InsertMaterial(Material("Snow"),RandomX(300, LandscapeWidth()-300),1, RandomX(-10, 10),10);
		ExtractLiquid(LandscapeWidth()/2, 295);
	}
	ExtractLiquid(LandscapeWidth()/2, 295);
	ExtractLiquid(LandscapeWidth()/2, 285);
	ExtractLiquid(340, 340);
	ExtractLiquid(400, 340);
	ExtractLiquid(1100, 340);
	ExtractLiquid(1160, 340);
	if (!Random(3)) for (var obj in FindObjects(Find_Or(Find_InRect(0,-250, 300, 280),Find_InRect(LandscapeWidth()-300,-250, 300, 280))))
	{
		obj->~DoEnergy(-1); 
	}
}

global func PlaceEdges()
{
	var x=[124, 116, 116, 236, 236, 223, 308, 364, 380, 353, 353, 336, 388, 724, 37, 45, 53, 356, 348, 118, 118, 732, 437, 437, 126, 118, 137, 145, 183, 175, 156, 164, 221, 194, 213, 202, 107, 99, 80, 88, 69, 46, 54, 74, 66, 86, 94, 45, 45, 45, 45, 45, 45, 140, 231, 677, 733, 212, 204, 122, 130, 234, 226, 239, 247, 217, 188, 188, 373, 353, 353, 389, 365, 397, 206, 214, 194, 186, 227, 219, 196, 204, 154, 146, 166, 174, 134, 126, 106, 114, 45, 45, 316, 308, 327, 335, 308, 194, 187, 174, 166, 53, 53, 394, 404, 386, 373, 270, 259, 240, 232, 278, 251, 297, 289, 308, 308, 53, 53, 130, 122, 146, 154, 167, 174, 186, 194, 205, 213, 224, 232, 196, 205, 225, 237, 257, 245, 205, 336, 336, 336, 373, 386, 420, 394, 357, 69, 69, 157, 157, 205, 347, 355, 375, 367, 407, 395, 387, 415, 435, 427, 429, 297, 305, 285, 277, 265, 325, 317, 267, 259, 279, 287, 307, 299, 340, 327, 319, 348, 356, 396, 353, 336, 353, 336];
	var y=[261, 261, 269, 313, 305, 300, 388, 156, 148, 218, 210, 210, 140, 357, 301, 269, 261, 309, 301, 305, 313, 349, 301, 309, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 157, 157, 157, 157, 157, 157, 173, 165, 185, 193, 213, 205, 277, 300, 300, 308, 300, 300, 301, 301, 157, 157, 188, 188, 205, 237, 229, 165, 205, 223, 157, 189, 148, 157, 157, 157, 157, 188, 188, 188, 188, 157, 157, 157, 157, 157, 157, 157, 157, 225, 232, 348, 349, 348, 348, 356, 300, 300, 300, 300, 328, 321, 317, 317, 317, 317, 388, 388, 388, 388, 388, 388, 388, 388, 366, 374, 338, 346, 317, 317, 317, 317, 317, 317, 317, 317, 317, 317, 317, 317, 237, 205, 205, 205, 205, 205, 215, 205, 223, 218, 300, 300, 300, 300, 276, 377, 369, 268, 276, 223, 348, 348, 348, 348, 348, 348, 348, 348, 348, 348, 317, 205, 205, 205, 205, 205, 205, 205, 188, 188, 188, 188, 188, 188, 188, 188, 188, 180, 164, 140, 231, 231, 237, 237];
	var d=[0, 1, 3, 2, 0, 1, 1, 1, 1, 2, 0, 1, 1, 2, 2, 2, 2, 3, 3, 1, 3, 2, 0, 2, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 3, 2, 2, 3, 3, 2, 2, 0, 0, 2, 2, 0, 3, 0, 0, 0, 0, 1, 1, 0, 2, 3, 1, 0, 3, 3, 1, 2, 2, 0, 2, 2, 2, 3, 2, 2, 3, 0, 1, 1, 0, 2, 3, 3, 2, 2, 3, 3, 2, 0, 2, 0, 1, 1, 0, 3, 0, 1, 0, 1, 2, 0, 2, 3, 3, 2, 1, 0, 0, 1, 0, 1, 0, 1, 1, 3, 0, 2, 2, 3, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 2, 2, 2, 3, 3, 2, 0, 3, 1, 3, 0, 1, 1, 0, 0, 2, 0, 0, 2, 2, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 2, 3, 2, 2, 3, 2, 2, 3, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 2, 3, 0, 1];
	for (var i = 0; i < GetLength(x); i++)
	{
		DrawMaterialTriangle("Brick-brick", x[i], y[i], [0, 3, 1, 2][d[i]]);
		DrawMaterialTriangle("Brick-brick", LandscapeWidth() + 1 - x[i], y[i], [3, 0, 2, 1][d[i]]);
	}
	return 1;
}

protected func InitializePlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 600, nil, PLRZOOM_Direct);
	return;
}

func RelaunchWeaponList() { return [Blunderbuss, Sword, Javelin,  FrostboltScroll, Shovel]; }

/*-- Chest filler effects --*/

global func FxFillBaseChestStart(object target, effect, int temporary, bool supply)
{
	if (temporary) 
		return 1;
		
	effect.supply_type = supply;
	var w_list;
	if (effect.supply_type) 
		w_list = [Firestone, Dynamite, IronBomb, Shovel, Loam, Ropeladder];
	else
		w_list = [Bow, Shield, Sword, Javelin, Blunderbuss, FrostboltScroll];
	for (var i = 0; i<5; i++)
		target->CreateChestContents(w_list[i]);
	return 1;
}
global func FxFillBaseChestTimer(object target, effect)
{
	var maxcount = [], w_list = [];
	
	if (effect.supply_type) 
	{
		w_list = [Firestone, Dynamite, IronBomb, Shovel, Loam, Ropeladder];
		maxcount = [2, 2, 1, 2, 1];
	}
	else
	{
		w_list = [Bow, Shield, Sword, Javelin, Blunderbuss, FrostboltScroll];
		maxcount = [1, 2, 1, 1, 1, 2];
	}
	
	var contents;
	for (var i = 0; i<target->GetLength(w_list); i++)
		contents += target->ContentsCount(w_list[i]);
	if (contents > 5) return 1;
	
	for (var i = 0; i<2 ; i++)
	{
		var r = Random(GetLength(w_list));
		if (target->ContentsCount(w_list[r]) < maxcount[r])
		{
			target->CreateChestContents(w_list[r]);
			i = 3;
		}
	}
	return 1;
}


global func FxFillOtherChestStart(object target, effect, int temporary)
{
	if (temporary) 
		return 1;
	var w_list = [Sword, Javelin, Club, Firestone, Dynamite, IronBomb, Firestone];
	if (target->ContentsCount() < 5)
		target->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxFillOtherChestTimer(object target)
{

	var w_list = [Sword, Javelin, Dynamite, IronBomb, WindScroll, FrostboltScroll, Loam, HardeningScroll, PowderKeg];
	var maxcount = [1, 1, 3, 1, 2, 1, 1, 1];
	
	var contents;
	for (var i = 0; i<target->GetLength(w_list); i++)
		contents += target->ContentsCount(w_list[i]);
	if (contents > 5) return 1;
	
	for (var i = 0; i<2 ; i++)
	{
		var r = Random(GetLength(w_list));
		if (target->ContentsCount(w_list[r]) < maxcount[r])
		{
			target->CreateChestContents(w_list[r]);
			i = 3;
		}
	}
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
