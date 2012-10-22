/*-- 
	Molten Monarch
	Author: Mimmo_O
	
	A king of the hill in a lava cave.
--*/



protected func Initialize()
{
	// Goal.
	var goal = CreateObject(Goal_KingOfTheHill, 555, 250, NO_OWNER);
	goal->SetRadius(80);
	goal->SetPointLimit(6);
	AddEffect("BlessTheKing",goal,100,1,nil);
	// Objects fade after 7 seconds.
	CreateObject(Rule_ObjectFade)->DoFadeTime(7 * 36);
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_Gravestones);
	
	//make lava collapse
	CreateObject(Firestone,625,480);

	// Chests with weapons.
	CreateObject(Chest, 320, 80, NO_OWNER)->MakeInvincible();
	CreateObject(Chest, 720, 192, NO_OWNER)->MakeInvincible();
	CreateObject(Chest, 668, 336, NO_OWNER)->MakeInvincible();
	CreateObject(Chest, 320, 440, NO_OWNER)->MakeInvincible();
	CreateObject(Chest, 48, 256, NO_OWNER)->MakeInvincible();
	AddEffect("IntFillChests", nil, 100, 5 * 36);
	
	// Moving bricks.
	var brick;
	brick = CreateObject(MovingBrick, 542, 176);
	brick->SetSize(3);
	brick->MoveVertical(168, 472, 8);
	brick = CreateObject(MovingBrick, 588, 192);
	brick->SetSize(3);
	brick->MoveVertical(160, 464, 8);
	
	brick = CreateObject(MovingBrick, 77, 0);
	brick->MoveVertical(0, LandscapeHeight(), 6);
	AddEffect("LavaBrickReset", brick, 100, 10);
	brick = CreateObject(MovingBrick, 77, LandscapeHeight() / 3);
	brick->MoveVertical(0, LandscapeHeight(), 6);
	AddEffect("LavaBrickReset", brick, 100, 10);
	brick = CreateObject(MovingBrick, 77, 2 * LandscapeHeight() / 3);
	brick->MoveVertical(0, LandscapeHeight(), 6);
	AddEffect("LavaBrickReset", brick, 100, 10);
	
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

global func FxBlessTheKingTimer(object target, effect, int timer)
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

global func FxDeathByFireTimer(object target, effect, int timer)
{
	for(var obj in FindObjects(Find_InRect(55,0,50,70),Find_OCF(OCF_Alive)))
		obj->~Kill();

	for(var obj in FindObjects(Find_InRect(55,0,50,30),Find_OCF(OCF_Alive),Find_Not(Find_ID(MovingBrick))))
		obj->RemoveObject();	
		
	for(var i=0; i<20; i++)
	{
		CreateParticle("MagicFire",50+Random(60),Random(60),RandomX(-3,3),RandomX(-1,10)+ Random(3)*10,150+Random(50),HSLa(-30+Random(60),200+Random(50),255,128+Random(100)));
		CreateParticle("MagicFire",50+Random(60),Random(30),RandomX(-3,3),Random(60),150+Random(50),HSLa(-30+Random(60),200+Random(50),255,128+Random(100)));
	}
}

global func FxLavaBrickResetTimer(object target, effect, int timer)
{
	if(target->GetY() < 10)
		target->SetPosition(target->GetX(),LandscapeHeight()-10);
	return 1;
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
	var chests = FindObjects(Find_ID(Chest));
	var w_list = [IronBomb, Rock, IronBomb, Firestone, Firestone, Bow, Musket, Sword, Javelin];
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

protected func InitializePlayer(int plr)
{
	return JoinPlayer(plr);
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
	var position = [[420,200],[300,440],[130,176],[140,368],[700,192],[670,336],[750,440],[440,392],[45,256]];
	var r=Random(GetLength(position));
	var x = position[r][0], y = position[r][1];
	var relaunch = CreateObject(RelaunchContainer, x, y, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	return;
}

func RelaunchWeaponList() { return [Bow, Shield, Sword, Javelin, Musket, Club]; }
