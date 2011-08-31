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
	CreateObject(Environment_Celestial);
	var time=CreateObject(Environment_Time);
	time->SetTime();
	time->SetCycleSpeed();
	FindObject(Find_ID(Moon))->SetPhase(3);
	FindObject(Find_ID(Moon))->SetCon(150);
	FindObject(Find_ID(Moon))->SetPosition(LandscapeWidth()/2,150);

	// Goal: Capture the flag, with bases in both hideouts.
	var goal = CreateObject(Goal_CaptureTheFlag, 0, 0, NO_OWNER);
	goal->SetFlagBase(1, 135, 260);
	goal->SetFlagBase(2, LandscapeWidth() - 135, 260);
	CreateObject(Rule_KillLogs);	
	
	var gate = CreateObject(StoneDoor, 345, 241, NO_OWNER);
	gate->SetClrModulation(RGB(130,190,255));
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 1);
	var gate = CreateObject(StoneDoor, LandscapeWidth()-345, 241, NO_OWNER);
	gate->SetClrModulation(RGB(130,190,255));
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 2);

	// Chests with weapons.
	var chest;
	chest = CreateObject(Chest, 60, 220, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 7 * 36,nil,nil,false);
	chest = CreateObject(Chest, 150, 370, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 7 * 36,nil,nil,true);
	chest = CreateObject(Chest, LandscapeWidth() - 60, 220, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 7 * 36,nil,nil,false);
	chest = CreateObject(Chest, LandscapeWidth() - 150, 370, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 7 * 36,nil,nil,true);

	
	chest = CreateObject(Chest, LandscapeWidth()/2, 320, NO_OWNER);
	AddEffect("FillOtherChest", chest, 100, 5 * 36);
	
	AddEffect("SnowyWinter", nil, 100, 1);
	Sound("WindLoop.ogg",true,20,nil,+1);
	AddEffect("GeysirExplosion", nil, 100, 1);
	// Brick edges, notice the symmetric landscape.
	PlaceEdges();
	return;
}

protected func CaptureFlagCount() { return (4 + GetPlayerCount()) / 2; }

global func FxGeysirExplosionTimer(object target, effect)
{
	effect.counter++;
	
	if(Random(2))
	{
		var x=600+Random(300);
		var y=250+Random(200);
		while(GetMaterial(x,y) != Material("Water"))
		{
			var x=600+Random(300);
			var y=250+Random(200);
		}
		Bubble(1,x,y);
	}
	if(effect.counter>1900 && effect.counter<2030)
	{

		for(var i=0; i<(-(1900-effect.counter)); i+=10 )
		{
			var x=600+Random(300);
			var y=250+Random(200);
			while(GetMaterial(x,y) != Material("Water"))
			{
				var x=600+Random(300);
				var y=250+Random(200);
			}
			Bubble(1,x,y);
		}
	}
	if(effect.counter>2000)
	{
		var x=LandscapeWidth()/2;
		var y=280;
		while(!GBackLiquid(x,y)) y++;
		y-=3;
		for(var i=0; i<(45); i++)InsertMaterial(Material("Water"),x+RandomX(-9,9),y-Random(5),RandomX(-10,10)+RandomX(-5,5)+RandomX(-10,10),-(10+Random(50)+Random(30)+Random(60)));
		for(var i=0; i<(25); i++)InsertMaterial(Material("Water"),x+RandomX(-16,16),y-Random(5),RandomX(-10,10)+RandomX(-15,15)+RandomX(-20,20),-(10+Random(50)));
		CreateParticle("Air",x+RandomX(-6,6),y-Random(3),-RandomX(-15,15),RandomX(-86,-2),100+Random(130));
		if(effect.counter>2072) effect.counter=0;
		for(var obj in FindObjects(Find_InRect(x-30,y-200,60,210)))
		{
			obj->SetYDir(Max(obj->GetYDir()-15,-50));
	
		}

	}
}

global func FxSnowyWinterTimer(object target, effect, int time)
{
	if(time%1200 == 100 ) 
	{
		var add=RandomX(-2,2);
		effect.snow_count=BoundBy(effect.snow_count+add,1,5);	
	}
	for(var i=0; i<(effect.snow_count); i++)
	{
		InsertMaterial(Material("Snow"),RandomX(300,LandscapeWidth()-300),1,RandomX(-10,10),10);
		ExtractLiquid(LandscapeWidth()/2,295);
	}
	ExtractLiquid(LandscapeWidth()/2,295);
	ExtractLiquid(LandscapeWidth()/2,285);
	ExtractLiquid(340,340);
	ExtractLiquid(400,340);
	ExtractLiquid(1100,340);
	ExtractLiquid(1160,340);
	if(!Random(3)) for(var obj in FindObjects(Find_Or(Find_InRect(0,-250,300,280),Find_InRect(LandscapeWidth()-300,-250,300,280))))
	{
		obj->~DoEnergy(-1); 
	}
	
	for(var dead in FindObjects(Find_ID(Clonk),Find_Not(Find_OCF(OCF_Alive))))
	{
		CastParticles("Air",100,50,dead->GetX(),dead->GetY(),50+Random(30));
		CastParticles("AirIntake",50,30,dead->GetX(),dead->GetY(),70+Random(60));
		dead->RemoveObject();		
	}
}

global func PlaceEdges()
{
	var x=[398, 356, 347, 317, 327, 339, 297, 307, 287, 277, 257, 267, 315, 325, 265, 275, 285, 305, 295, 430, 427, 437, 417, 387, 397, 407, 367, 377, 357, 347, 205, 158, 158, 69, 69, 358, 394, 419, 384, 374, 335, 335, 335, 205, 245, 255, 235, 225, 205, 197, 233, 223, 214, 204, 194, 184, 174, 164, 154, 144, 125, 135, 53, 53, 308, 308, 287, 297, 249, 278, 231, 240, 259, 268, 374, 384, 404, 394, 53, 53, 164, 174, 184, 194, 307, 337, 327, 307, 317, 45, 45, 116, 106, 126, 136, 176, 166, 146, 156, 207, 197, 217, 227, 186, 196, 216, 206, 398, 365, 390, 355, 355, 373, 187, 187, 215, 247, 237, 226, 236, 135, 125, 204, 214, 733, 677, 233, 140, 45, 45, 45, 45, 45, 45, 96, 86, 66, 76, 56, 46, 69, 88, 78, 97, 107, 202, 211, 192, 221, 164, 154, 173, 183, 145, 135, 116, 126, 437, 437, 733, 121, 121, 348, 356, 54, 45, 37, 725, 388, 335, 355, 355, 380, 364, 307, 223, 237, 237, 115, 115, 125];
	var y=[139, 164, 179, 188, 188, 187, 188, 188, 188, 188, 188, 188, 205, 205, 205, 205, 205, 205, 205, 317, 348, 348, 348, 348, 348, 348, 348, 348, 348, 348, 225, 277, 268, 367, 377, 277, 300, 300, 300, 300, 225, 235, 205, 215, 205, 205, 205, 205, 205, 238, 317, 317, 317, 317, 317, 317, 317, 317, 317, 317, 317, 317, 348, 338, 376, 366, 388, 388, 388, 388, 388, 388, 388, 388, 317, 317, 317, 317, 318, 328, 300, 300, 300, 300, 357, 348, 348, 347, 347, 235, 225, 157, 157, 157, 157, 157, 157, 157, 157, 188, 188, 188, 188, 157, 157, 157, 157, 149, 189, 157, 235, 205, 165, 228, 238, 205, 188, 188, 157, 157, 300, 300, 300, 300, 308, 300, 300, 277, 205, 215, 195, 185, 165, 175, 157, 157, 157, 157, 157, 157, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 388, 310, 299, 349, 314, 304, 302, 310, 261, 270, 301, 357, 139, 215, 215, 225, 147, 156, 388, 300, 304, 314, 269, 259, 259];
	var d=[0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 3, 2, 2, 3, 2, 2, 3, 2, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 2, 2, 0, 0, 2, 0, 0, 1, 1, 0, 3, 1, 3, 0, 2, 3, 3, 2, 2, 2, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 3, 2, 2, 0, 3, 1, 1, 0, 1, 0, 1, 0, 0, 1, 2, 3, 3, 2, 0, 2, 1, 0, 1, 0, 3, 0, 1, 1, 0, 2, 0, 2, 3, 3, 2, 2, 3, 3, 2, 0, 1, 1, 0, 3, 2, 2, 3, 2, 2, 2, 0, 2, 2, 1, 3, 3, 0, 1, 3, 2, 0, 1, 1, 0, 0, 0, 0, 3, 0, 2, 2, 0, 0, 2, 2, 3, 3, 2, 2, 3, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 2, 0, 2, 3, 1, 3, 3, 2, 2, 2, 2, 1, 1, 0, 2, 1, 1, 1, 1, nil, 2, 3, 1, nil];
	var o=[1,0,3,2];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i] + 5, NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]);
		edge->PermaEdge();
		
		edge=CreateObject(BrickEdge, x[i], y[i] + 5, NO_OWNER);
		edge->Initialize();
		edge->SetP(o[d[i]]);
		edge->SetPosition(LandscapeWidth()-x[i],y[i]);
		edge->PermaEdge();
	}
	return 1;
}

protected func InitializePlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 600, nil, PLRZOOM_Direct);
	return;
}

// Gamecall from CTF goal, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	var relaunch = CreateObject(RelaunchContainer, clonk->GetX(), clonk->GetY(), clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	relaunch->SetRelaunchTime(8, true);
	return;
}

func RelaunchWeaponList() { return [Musket, Sword, Javelin,  FrostboltScroll, Shovel]; }

/*-- Chest filler effects --*/

global func FxFillBaseChestStart(object target, effect, int temporary, bool supply)
{
	if (temporary) 
		return 1;
		
	effect.supply_type=supply;
	if(effect.supply_type) 
		var w_list = [Firestone, Dynamite, Shovel, Loam, Ropeladder];
	else
		var w_list = [Bow, Shield, Sword, Javelin, Musket, FrostboltScroll];
	for(var i=0; i<5; i++)
		target->CreateChestContents(w_list[i]);
	return 1;
}
global func FxFillBaseChestTimer(object target, effect)
{
	var maxcount = [];
	
	if(effect.supply_type) 
	{
		var w_list = [Firestone, Dynamite, Shovel, Loam, Ropeladder];
		var maxcount = [2,2,1,2,1];
	}
	else
	{
		var w_list = [Bow, Shield, Sword, Javelin, Musket, FrostboltScroll];
		var maxcount = [1,2,1,1,1,2];
	}
	
	var contents;
	for(var i=0; i<target->GetLength(w_list); i++)
		contents+=target->ContentsCount(w_list[i]);
	if(contents > 5) return 1;
	
	for(var i=0; i<2 ; i++)
	{
		var r = Random(GetLength(w_list));
		if (target->ContentsCount(w_list[r]) < maxcount[r])
		{
			target->CreateChestContents(w_list[r]);
			i=3;
		}
	}
	return 1;
}


global func FxFillOtherChestStart(object target, effect, int temporary)
{
	if (temporary) 
		return 1;
	var w_list = [Sword, Javelin, Club, Firestone, Dynamite, Firestone];
	if (target->ContentsCount() < 5)
		target->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxFillOtherChestTimer(object target)
{

	var w_list = [Sword, Javelin, Dynamite, WindScroll, FrostboltScroll, Loam, HardeningScroll, PowderKeg];
	var maxcount = [1,1,3,1,2,1,1,1];
	
	var contents;
	for(var i=0; i<target->GetLength(w_list); i++)
		contents+=target->ContentsCount(w_list[i]);
	if(contents > 5) return 1;
	
	for(var i=0; i<2 ; i++)
	{
		var r = Random(GetLength(w_list));
		if (target->ContentsCount(w_list[r]) < maxcount[r])
		{
			target->CreateChestContents(w_list[r]);
			i=3;
		}
	}
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
