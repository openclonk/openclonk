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
	
	
	var gate = CreateObject(StoneDoor, 345, 230, NO_OWNER);
	gate->SetClrModulation(RGB(130,190,255));
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 1);
	//wheel = CreateObject(SpinWheel, 320, 460, NO_OWNER);
	//wheel->SetStoneDoor(gate);
	var gate = CreateObject(StoneDoor, LandscapeWidth()-345, 230, NO_OWNER);
	gate->SetClrModulation(RGB(130,190,255));
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 2);
	//wheel = CreateObject(SpinWheel, 280, 580, NO_OWNER);
	//wheel->SetStoneDoor(gate);

	
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
	// No Seaweed in snowy regions.
	RemoveAll(Find_ID(Seaweed));
	return;
}

protected func CaptureFlagCount() { return (4 + GetPlayerCount()) / 2; }

global func FxGeysirExplosionTimer(object target, int num)
{
	EffectVar(0, target, num)++;
	
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
	if(EffectVar(0, target, num)>1900 && EffectVar(0, target, num)<2030)
	{

		for(var i=0; i<(-(1900-EffectVar(0, target, num))); i+=10 )
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
	if(EffectVar(0, target, num)>2000)
	{
		var x=LandscapeWidth()/2;
		var y=280;
		while(!GBackLiquid(x,y)) y++;
		y-=3;
		for(var i=0; i<(45); i++)InsertMaterial(Material("Water"),x+RandomX(-9,9),y-Random(5),RandomX(-10,10)+RandomX(-5,5)+RandomX(-10,10),-(10+Random(50)+Random(30)+Random(60))-Sin(num*10,60));
		for(var i=0; i<(25); i++)InsertMaterial(Material("Water"),x+RandomX(-16,16),y-Random(5),RandomX(-10,10)+RandomX(-15,15)+RandomX(-20,20),-(10+Random(50))-Sin(num*10,60));
		CreateParticle("Air",x+RandomX(-6,6),y-Random(3),-RandomX(-15,15),RandomX(-86,-2),100+Random(130));
		if(EffectVar(0, target, num)>2072) EffectVar(0, target, num)=0;
		for(var obj in FindObjects(Find_InRect(x-30,y-200,60,210)))
		{
			obj->SetYDir(Max(obj->GetYDir()-15,-50));
	
		}

	}
}

global func FxSnowyWinterTimer(object target, int num, int time)
{
	if(time%1200 == 100 ) 
	{
		var add=RandomX(-2,2);
		EffectVar(0, target, num)=BoundBy(EffectVar(0, target, num)+add,1,5);	
	}
	for(var i=0; i<(EffectVar(0, target, num)); i++)
	{
		InsertMaterial(Material("Snow"),RandomX(300,LandscapeWidth()-300),1,RandomX(-10,10),10);
		ExtractLiquid(LandscapeWidth()/2,295);
	}
	ExtractLiquid(LandscapeWidth()/2,295);
	ExtractLiquid(LandscapeWidth()/2,285);
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
	var x=[335, 335, 315, 325, 335, 295, 305, 285, 275, 255, 265, 315, 325, 265, 275, 285, 305, 295, 425, 425, 435, 415, 385, 395, 405, 365, 375, 355, 345, 205, 155, 155, 115, 115, 125, 65, 65, 355, 365, 395, 415, 385, 375, 335, 335, 335, 205, 245, 255, 235, 225, 205, 195, 235, 225, 215, 205, 195, 185, 175, 165, 155, 145, 125, 135, 55, 55, 305, 305, 295, 305, 255, 285, 235, 245, 265, 275, 375, 385, 405, 395, 55, 55, 165, 155, 175, 185, 195, 205, 305, 335, 325, 305, 315, 45, 45, 115, 105, 125, 135, 175, 165, 145, 155, 205, 195, 215, 225, 185, 195, 215, 205, 355, 365, 365, 355, 355, 355, 355, 355, 355, 355, 1065, 1065, 185, 185, 215, 245, 235, 225, 235, 135, 125, 215, 225, 735, 765, 675, 825, 345, 235, 135, 45, 45, 45, 45, 45, 45, 95, 85, 65, 75, 55, 45, 65, 85, 75, 95, 105, 205, 215, 195, 225, 165, 155, 175, 185, 145, 135, 115, 125, 1165, 1165, 1205, 1195, 1165, 1225, 1215, 1235, 1245, 1265, 1255, 1185, 1175, 1225, 1215, 1205, 1195, 1075, 1065, 1185, 1075, 1105, 1095, 1085, 1125, 1115, 1135, 1145, 1295, 1345, 1345, 1385, 1385, 1375, 1435, 1435, 1145, 1135, 1105, 1085, 1115, 1125, 1165, 1165, 1295, 1165, 1165, 1295, 1245, 1235, 1255, 1265, 1285, 1305, 1265, 1275, 1285, 1295, 1305, 1315, 1325, 1335, 1345, 1355, 1375, 1365, 1445, 1445, 1195, 1195, 1195, 1435, 1235, 1205, 1255, 1245, 1225, 1215, 1125, 1115, 1095, 1105, 1445, 1445, 1335, 1345, 1325, 1315, 1305, 1295, 1195, 1155, 1165, 1195, 1175, 1455, 1455, 1455, 1385, 1355, 1345, 1315, 1375, 1335, 1325, 1305, 1175, 1295, 1285, 1305, 1295, 1365, 1285, 1145, 1135, 1135, 1145, 1145, 1145, 1145, 1145, 1145, 1145, 1145, 1065, 435, 1065, 435, 1315, 1315, 1275, 1185, 1275, 1275, 1265, 1365, 1375, 1285, 1275, 775, 765, 735, 1155, 1265, 1365, 1455, 1455, 1455, 1455, 1455, 1455, 1395, 1405, 1425, 1415, 1435, 1445, 1425, 1405, 1415, 1395, 1385, 1285, 1275, 1295, 1265, 1325, 1335, 1315, 1305, 1345, 1355, 1375, 1365, 120, 120, 355, 345, 355, 355, 55, 45, 35, 725, 1380, 1380, 1465, 1455, 1445, 1155, 1145, 1145, 1145, 355, 335];
	var y=[165, 175, 185, 185, 185, 185, 185, 185, 185, 185, 185, 205, 205, 205, 205, 205, 205, 205, 315, 345, 345, 345, 345, 345, 345, 345, 345, 345, 345, 225, 275, 265, 265, 275, 275, 365, 375, 275, 285, 295, 295, 295, 295, 225, 235, 205, 215, 205, 205, 205, 205, 205, 235, 315, 315, 315, 315, 315, 315, 315, 315, 315, 315, 315, 315, 345, 335, 375, 365, 385, 385, 385, 385, 385, 385, 385, 385, 315, 315, 315, 315, 315, 325, 295, 295, 295, 295, 295, 295, 355, 345, 345, 345, 345, 235, 225, 155, 155, 155, 155, 155, 155, 155, 155, 185, 185, 185, 185, 155, 155, 155, 155, 145, 155, 145, 235, 165, 175, 185, 195, 205, 225, 305, 295, 225, 235, 205, 185, 185, 155, 155, 295, 295, 295, 295, 305, 305, 295, 295, 155, 305, 275, 205, 215, 195, 185, 165, 175, 155, 155, 155, 155, 155, 155, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 165, 175, 185, 185, 185, 185, 185, 185, 185, 185, 185, 205, 205, 205, 205, 205, 205, 315, 345, 345, 345, 345, 345, 345, 345, 345, 345, 345, 225, 275, 265, 265, 275, 275, 365, 375, 275, 285, 295, 295, 295, 295, 215, 235, 205, 225, 205, 215, 205, 205, 205, 205, 205, 235, 315, 315, 315, 315, 315, 315, 315, 315, 315, 315, 315, 315, 345, 335, 375, 365, 385, 385, 385, 385, 385, 385, 385, 385, 315, 315, 315, 315, 315, 325, 295, 295, 295, 295, 295, 295, 355, 345, 345, 345, 345, 235, 225, 155, 155, 155, 155, 155, 155, 155, 155, 185, 185, 185, 185, 155, 155, 155, 155, 145, 155, 145, 235, 165, 175, 185, 195, 215, 205, 225, 305, 305, 295, 295, 225, 235, 205, 185, 185, 155, 155, 295, 295, 295, 295, 355, 345, 345, 155, 305, 275, 205, 215, 195, 185, 165, 175, 155, 155, 155, 155, 155, 155, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 385, 310, 300, 305, 295, 315, 305, 255, 265, 295, 355, 300, 310, 295, 265, 255, 295, 305, 315, 305, 215, 215];
	var d=[1, 3, 1, 0, 1, 1, 0, 0, 1, 1, 0, 3, 2, 2, 3, 2, 2, 3, 2, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 2, 2, 0, 1, 3, 2, 0, 2, 0, 0, 0, 1, 1, 0, 3, 1, 3, 0, 2, 3, 3, 2, 2, 2, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 3, 2, 2, 0, 3, 1, 1, 0, 1, 0, 1, 0, 0, 1, 2, 3, 3, 2, 0, 2, 0, 1, 1, 0, 1, 0, 3, 0, 1, 1, 0, 2, 0, 2, 3, 3, 2, 2, 3, 3, 2, 0, 1, 1, 0, 3, 2, 2, 3, 1, 2, 0, 0, 2, 0, 2, 0, 2, 2, 3, 1, 1, 3, 3, 0, 1, 3, 2, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 3, 0, 2, 2, 0, 0, 2, 2, 3, 3, 2, 2, 3, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 2, 0, 1, 0, 0, 1, 1, 0, 0, 1, 2, 3, 2, 3, 2, 3, 3, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 3, 3, 1, 0, 2, 3, 1, 3, 1, 1, 1, 0, 0, 1, 0, 0, 3, 2, 2, 1, 2, 3, 3, 2, 2, 3, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 2, 3, 3, 1, 2, 0, 1, 1, 1, 0, 1, 0, 0, 1, 3, 2, 2, 3, 1, 3, 1, 0, 0, 1, 0, 1, 2, 0, 1, 0, 0, 3, 1, 2, 3, 2, 3, 2, 2, 2, 3, 0, 1, 1, 0, 3, 2, 3, 3, 0, 3, 1, 1, 3, 1, 3, 1, 1, 3, 3, 3, 2, 1, 0, 0, 2, 3, 0, 1, 2, 3, 1, 0, 0, 1, 3, 3, 2, 0, 1, 2, 1, 3, 3, 1, 1, 3, 2, 3, 3, 2, 2, 3, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 3, 1, 1, 3, 3, 3, 2, 2, 2, 2, 0, 2, 3, 3, 3, 2, 2, 2, 0, 0, 1];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i] + 5, NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]);
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

global func FxFillBaseChestStart(object target, int num, int temporary, bool supply)
{
	if (temporary) 
		return 1;
		
	EffectVar(0, target, num)=supply;
	if(EffectVar(0, target, num)) 
		var w_list = [Firestone, Dynamite, Shovel, Loam, Ropeladder];
	else
		var w_list = [Bow, Shield, Sword, Javelin, Musket, FrostboltScroll];
	for(var i=0; i<5; i++)
		target->CreateChestContents(w_list[i]);
	return 1;
}
global func FxFillBaseChestTimer(object target, int num)
{
	var maxcount = [];
	
	if(EffectVar(0, target, num)) 
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


global func FxFillOtherChestStart(object target, int num, int temporary)
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
	if (obj_id == GrappleBow)
		AddEffect("NotTooLong",obj,100,36);
	obj->Enter(this);
	
	return;
}


global func FxNotTooLongTimer(object target, int num)
{	if (!(target->Contained())) return 1;
	if (target->Contained()->GetID() == Clonk) EffectVar(0, target, num)++;
	if (EffectVar(0, target, num) > 40) return target->RemoveObject();
	else if (EffectVar(0, target, num) > 35) target->Message("@<c ff%x%x>%d",(41-EffectVar(0, target, num))*50,(41-EffectVar(0, target, num))*50,41-EffectVar(0, target, num));
}