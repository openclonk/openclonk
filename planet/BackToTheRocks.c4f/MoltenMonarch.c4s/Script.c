/*-- 
	Down the Fountain
	Author: Mimmo_O
	
	An arena like last man standing round for up two to three players.
--*/



protected func Initialize()
{
	// Goal.
	var goal = CreateObject(Goal_KingOfTheHill, 555, 250, NO_OWNER);
	goal->SetRadius(80);
	goal->SetPointLimit(10);
	AddEffect("BlessTheKing",goal,100,1,nil);
	// Objects fade after 7 seconds.
	CreateObject(Rule_ObjectFade)->DoFadeTime(7 * 36);
	CreateObject(Rule_KillLogs);
	
	//make lava collapse
	CreateObject(Firestone,625,480);

	// Chests with weapons.
	CreateObject(Chest, 320, 60, NO_OWNER);
	CreateObject(Chest, 730, 100, NO_OWNER);
	CreateObject(Chest, 665, 300, NO_OWNER);
	CreateObject(Chest, 310, 400, NO_OWNER);
	CreateObject(Chest, 48, 250, NO_OWNER);
	AddEffect("IntFillChests", nil, 100, 5 * 36, this);
	
	CreateObject(MovingBrick,590,170)->Room(0,300,0,0,8);
	var brck=CreateObject(MovingBrick,540,180);
	brck->Room(0,300,3,0,8);
	brck->SetPosition(540,215);
	
	var brck=CreateObject(MovingBrick,80,0);
	brck->Room(0,LandscapeHeight()+60,4,0,6);
	AddEffect("LavaBrickReset",brck,100,10,nil);
	
	var brck=CreateObject(MovingBrick,80,0);
	brck->Room(0,LandscapeHeight()+60,4,0,6);
	AddEffect("LavaBrickReset",brck,100,10,nil);
	brck->SetPosition(80,LandscapeHeight()/3 *2);
	brck->SetYDir(-15);
	
		var brck=CreateObject(MovingBrick,80,0);
	brck->Room(0,LandscapeHeight()+60,4,0,6);
	AddEffect("LavaBrickReset",brck,100,10,nil);
	brck->SetPosition(80,LandscapeHeight()/3);
	brck->SetYDir(-15);
	
		PlaceEdges();
	AddEffect("DeathByFire",nil,100,2,nil);
	AddEffect("RemoveCorpses",nil,100,2,nil);
	return;
}


global func FxRemoveCorpsesTimer()
{
	//uber effect abuse
	
	for(var dead in FindObjects(Find_ID(Clonk),Find_Not(Find_OCF(OCF_Alive))))
	{
		CastParticles("MagicFire",100,50,dead->GetX(),dead->GetY(),50+Random(30));
		CastParticles("MagicFire",50,30,dead->GetX(),dead->GetY(),70+Random(60));
		dead->RemoveObject();		
	}
	for(var burning in FindObjects(Find_ID(Clonk),Find_OCF(OCF_OnFire)))
	{
		burning->DoEnergy(-3); //lava hurts a lot
	}

}

global func FxBlessTheKingTimer(object target, int num, int timer)
{


	if(!FindObject(Find_ID(KingOfTheHill_Location))) return 1;
	if(FindObject(Find_ID(KingOfTheHill_Location))->GetKing() == nil) return 1;
	
	var king=FindObject(Find_ID(KingOfTheHill_Location))->GetKing();
	//if(king->GetOCF() & OCF_OnFire) king->Extinguish();
	
	if(king->Contents(0)) king->Contents(0)->~MakeKingSize();
	if(king->Contents(1)) king->Contents(1)->~MakeKingSize();
	for(var i=0; i<25; i++)
	CreateParticle("MagicFire",king->GetX()+RandomX(-3,3),king->GetY()+RandomX(-11,8),RandomX(-6,6),RandomX(-10,3),Random(30),RGBa(255,255-Random(50),255-Random(160),20+Random(160)));
	for(; i<45; i++)
	CreateParticle("MagicFire2",king->GetX()+RandomX(-4,4),king->GetY()+RandomX(-7,8),RandomX(-6,6),RandomX(-10,3),30+Random(30),RGBa(255,255-Random(100),255-Random(100),10+Random(20)));
	return 1;
}
global func FxDeathByFireTimer(object target, int noum, int timer)
{
	for(var obj in FindObjects(Find_InRect(55,0,50,50,70),Find_OCF(OCF_Alive)))
		obj->~Kill();

	for(var obj in FindObjects(Find_InRect(55,0,50,50,30),Find_OCF(OCF_Alive),Find_Not(Find_ID(MovingBrick))))
		obj->RemoveObject();	
		
	for(var i=0; i<20; i++)
	{
		CreateParticle("MagicFire",50+Random(60),Random(60),RandomX(-3,3),RandomX(-1,10)+ Random(3)*10,150+Random(50),HSLa(-30+Random(60),200+Random(50),255,128+Random(100)));
		CreateParticle("MagicFire",50+Random(60),Random(30),RandomX(-3,3),Random(60),150+Random(50),HSLa(-30+Random(60),200+Random(50),255,128+Random(100)));
	}
}

global func FxLavaBrickResetTimer(object target, int noum, int timer)
{
	if(target->GetY() < 10)
	{
		target->SetPosition(target->GetX(),LandscapeHeight()-10);
		target->SetYDir(-15);
	}
}


global func PlaceEdges()
{
	var x=[675, 635, 375, 365, 305, 265, 265, 695, 375, 695, 485, 695, 645, 725, 715, 705, 645, 465, 345, 365, 395, 395, 375, 375, 385, 385, 365, 365, 355, 355, 415, 415, 105, 105, 105, 105, 195, 195, 185, 205, 195, 235, 245, 225, 215, 265, 255, 175, 185, 265, 275, 225, 235, 255, 245, 205, 215, 195];
	var y=[195, 185, 415, 415, 255, 315, 385, 465, 455, 185, 205, 345, 345, 445, 455, 425, 425, 395, 445, 405, 215, 205, 235, 225, 215, 225, 245, 235, 245, 255, 405, 395, 375, 325, 135, 185, 325, 375, 125, 105, 115, 75, 65, 85, 95, 45, 55, 135, 175, 95, 85, 135, 125, 105, 115, 155, 145, 165];
	var d=[3, 3, 2, 3, 3, 2, 0, 1, 0, 0, 2, 2, 3, 1, 1, 2, 3, 2, 0, 1, 2, 1, 2, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1, 3, 3, 1, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1];
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

// Gamecall from LastManStanding goal, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	SetFoW(false,plr);
	return;
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
	var chests = FindObjects(Find_ID(Chest));
	var w_list = [Dynamite, Rock, Dynamite, Firestone, Firestone, Bow, Musket, Sword, Javelin];
	for(var chest in chests)
		if (chest->ContentsCount() < 5 )
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
protected func RelaunchPlayer(int plr)
{
	var clonk = CreateObject(Clonk, 0, 0, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	JoinPlayer(plr);
	return;
}

protected func JoinPlayer(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
	var position = [[420,150],[300,370],[130,160],[140,350],[700,150],[670,300],[750,410],[440,350],[40,240]];
	var r=Random(GetLength(position));
	var x = position[r][0], y = position[r][1];
	var relaunch = CreateObject(RelaunchContainer, x,y, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);

	return;
}


func RelaunchWeaponList() { return [Bow, Shield, Sword, Javelin, Musket, Club]; }
