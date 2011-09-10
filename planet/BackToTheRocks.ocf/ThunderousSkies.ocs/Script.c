/*-- 
	Thunderous Skies
	Author: Mimmo_O
	
	King of the hill high in the skies.
--*/



protected func Initialize()
{
	// Goal.
	CreateObject(Rule_ObjectFade)->DoFadeTime(8 * 36);
	var goal = CreateObject(Goal_KingOfTheHill, 450, 380, NO_OWNER);
	goal->SetRadius(90);
	goal->SetPointLimit(10);
	AddEffect("BlessTheKing",goal,100,1,nil);
	CreateObject(Rule_KillLogs);
	
	//Enviroment.
	//SetSkyAdjust(RGBa(250,250,255,128),RGB(200,200,220));
	Sound("BirdsLoop.ogg",true,100,nil,+1);
		
	CreateObject(Column,650,379);
	CreateObject(Column,350,409);
	CreateObject(Column,160,229);
	CreateObject(Column,448,269);
	CreateObject(Column,810,179);

	// Chests with weapons.
	CreateObject(Chest, 175, 200, NO_OWNER);
	CreateObject(Chest, 800, 150, NO_OWNER);
	CreateObject(Chest, 430, 240, NO_OWNER);
	CreateObject(Chest, 610, 340, NO_OWNER);
	CreateObject(Chest, 355, 390, NO_OWNER);
	
	AddEffect("IntFillChests", nil, 100, 2 * 36, this);
	// Smooth brick edges.
	AddEffect("ChanneledWind", nil, 100, 1, this);
	AddEffect("Balloons", nil, 100, 100, this);
	
	// Moving bricks.
	var brick;
	brick = CreateObject(MovingBrick,370,424);
	brick->MoveHorizontal(344, 544);
	brick = CreateObject(MovingBrick,550,250);
	brick->MoveVertical(240, 296);

	CreateObject(BrickEdge, 380, 416)->PermaEdge();
	
	PlaceGras();
	return;
}
global func FxLifestealDamage(object target, effect, int damage, int cause, int from)
{
	var goal = FindObject(Find_ID(KingOfTheHill_Location));
	if (!goal) return damage;
	var king = goal->GetKing();
	if (!king) return damage;
	if (from == -1) return damage;
	if (damage > 0) return damage;
	if (target->GetOwner() == from) return damage;
	if (king->GetOwner() == from)
		AddEffect("Lifedrain",king,100,1,nil,nil,damage/3);
	return damage;
}
global func FxLifedrainStart(object target, effect, int temporary,damage)
{
	if(temporary) return 1;
	effect.drain=damage/10;
}
global func FxLifedrainAdd(object target, effect, string new_name, int new_timer, damage)
{
	effect.drain+=damage/10;
}
global func FxLifedrainTimer(object target, effect, int timer)
{
	if(effect.drain>0) return -1;
	target->DoEnergy(+100,1,0,-1);
	effect.drain+=10;
}
global func FxBlessTheKingTimer(object target, effect, int timer)
{
	
	//evil effect abuse :O
	for(var dead in FindObjects(Find_ID(Clonk),Find_Not(Find_OCF(OCF_Alive))))
	{
		CastParticles("Air",50,50,dead->GetX(),dead->GetY(),50+Random(30));
		for(var i=0; i<200; i++)
		{
			var r=Random(360);
			CreateParticle("AirIntake",dead->GetX()+Sin(r,6-Random(5)),dead->GetY()-Cos(r,6-Random(5)),RandomX(-2,2),-(i/3),20+Random(20),dead->GetPlayerColor());
		}
		dead->RemoveObject();		
	}

	if(!FindObject(Find_ID(KingOfTheHill_Location))) return 1;
	if(FindObject(Find_ID(KingOfTheHill_Location))->GetKing() == nil) return 1;
	
	var king=FindObject(Find_ID(KingOfTheHill_Location))->GetKing();

	for(var i=0; i<5; i++)
	{
		var r=Random(360);
		var clr=RGBa(30,100-Random(50),255-Random(160),230+Random(20));
		var str=0;
		var lifedrain = GetEffect("Lifedrain",king);
		if(lifedrain)
		{
			clr=RGBa(BoundBy(-lifedrain.drain,30,225)+Random(20),160-(BoundBy(-lifedrain.drain/2,10,100))-Random(50),230-BoundBy(-lifedrain.drain,30,225)+Random(20), 230+Random(20));
			var str=Random(BoundBy(-lifedrain.drain/10,2,6));
		}
		CreateParticle("AirIntake",king->GetX()+Sin(r,6-Random(5)),king->GetY()-Cos(r,6-Random(5)),Sin(r + 90,8+Random(4)+str),-Cos(r +90,8+Random(4)+str),20+Random(30) +str*str,clr);
		CreateParticle("AirIntake",king->GetX()+Sin(r + 180,6-Random(5)),king->GetY()-Cos(r+180,6-Random(5)),Sin(r + 90,8+Random(4)+str),-Cos(r +90,8+Random(4)+str),20+Random(30)+str*str,clr);
		
	}
	CreateParticle("AirIntake",king->GetX()+Sin(r + 180,6-Random(5)),king->GetY()-Cos(r+180,6-Random(5)),0,-25,20+Random(20),king->GetPlayerColor());
	return 1;
}

global func FxChanneledWindTimer()
{
	for(var obj in FindObjects(Find_InRect(230,300,40,90)))
	{
		obj->SetYDir(Max(obj->GetYDir()-5,-50));
		obj->SetXDir(obj->GetXDir()+RandomX(-1,1));
	}
	for(var obj in FindObjects(Find_InRect(700,250,60,100)))
	{
		obj->SetYDir(Max(obj->GetYDir()-5,-50));
		obj->SetXDir(obj->GetXDir()+RandomX(-1,1));
	}

	CreateParticle("AirIntake",230+Random(40),398,RandomX(-1,1),-30,60+Random(10),RGB(100+Random(25),128+Random(20),255));
	CreateParticle("AirIntake",700+Random(60),348,RandomX(-1,1),-30,60+Random(10),RGB(100+Random(25),128+Random(20),255));
}

global func FxBalloonsTimer()
{
	if(ObjectCount(Find_ID(TargetBalloon)) > 2 )
	{
		return 1;
	}
	if(ObjectCount(Find_ID(TargetBalloon)) )
	{
		if(Random(6)) return 1;	
	}

	if(Random(2)) return 1;
	
	var x = Random(300)+50;
	if(Random(2)) x = LandscapeWidth() - x;
	var y = Random(50) + 100;
	var target;
	
	var r = Random(3);
	if(r == 0) target = CreateObject(Boompack, x, y, NO_OWNER);
	if(r == 1){target = CreateObject(DynamiteBox, x, y, NO_OWNER); target->SetR(180); }
	if(r == 2){target = CreateObject(Arrow, x, y, NO_OWNER); target->SetObjDrawTransform(1000,0,0,0,1000,-7500);}
		
		
	var balloon = CreateObject(TargetBalloon, x, y-30, NO_OWNER);
	balloon->SetProperty("load",target);
	target->SetAction("Attach", balloon);
	CreateParticle("Flash", x, y - 50, 0, 0, 500, RGB(255, 255, 255));
	AddEffect("HorizontalMoving", balloon, 1, 1, balloon);
	balloon->SetXDir(((Random(2)*2)-1) * (Random(4)+3));
}

global func PlaceEdges()
{
	var x=[213, 780, 285, 285, 220, 220, 235, 414, 404, 668, 676, 684, 781, 694, 781, 229, 772, 364, 534, 380];
	var y=[276, 212, 325, 404, 404, 421, 495, 285, 276, 372, 364, 356, 356, 277, 413, 309, 252, 445, 446, 412];
	var d=[nil, 1, 2, 0, 1, 3, 3, 2, 1, 1, 1, 1, 0, 3, 2, 0, 1, 3, 2, 0];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i] + 5, NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]);
		//edge->PermaEdge();
	}
	return 1;
}

global func PlaceGras()
{
	var x=[747, 475, 474, 359, 244, 216, 324, 705, 754, 758, 828, 828, 235, 266, 266, 269, 177, 194, 204, 223, 348, 273, 284, 365, 369, 375, 379, 285, 281, 274, 233, 390, 229, 401, 388, 414, 476, 468, 463, 457, 422, 482, 493, 615, 609, 625, 631, 700, 704, 687, 761, 763, 771, 777, 619, 615, 621, 696, 789, 766, 356, 228, 188];
	var y=[280, 376, 384, 299, 279, 224, 290, 244, 226, 215, 175, 169, 328, 319, 310, 327, 205, 212, 213, 227, 292, 306, 294, 409, 407, 413, 414, 399, 396, 390, 388, 264, 392, 249, 256, 247, 252, 249, 241, 246, 246, 245, 363, 347, 376, 349, 348, 348, 335, 350, 337, 346, 349, 351, 249, 255, 254, 237, 178, 212, 291, 305, 227];
	var r=[-91, -93, -93, 89, 93, 93, 88, 89, -92, -92, 88, 93, 93, -88, -87, -93, 0, 0, 0, 0, 0, 0, 0, 0, 0, 43, 43, 46, 44, 48, -43, -48, -48, -45, -43, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -48, -44, -45, 48, 46, 48, 45, 0, 0, 0, 0, 0, 0, 0, 44, 0];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(Grass, x[i], y[i] + 5, NO_OWNER);
		edge->SetR(r[i]); 
		edge->Initialize();
	}
	return 1;
}

private func MakeTarget(int x, int y)
{

	var target = CreateObject(DynamiteBox, x, y, NO_OWNER);
	var balloon = CreateObject(TargetBalloon, x, y-30, NO_OWNER);
	balloon->SetProperty("load",target);
	target->SetAction("Attach", balloon);
	CreateParticle("Flash", x, y - 50, 0, 0, 500, RGB(255, 255, 255));

}

// Refill/fill chests.
global func FxIntFillChestsStart(object target, effect, int temporary)
{
	if(temporary) return 1;
	var chests = FindObjects(Find_ID(Chest),Find_InRect(0,0,LandscapeWidth(),610));
	var w_list = [Shield, Javelin, FireballScroll, Bow, Musket, WindScroll, ThunderScroll];
	
	for(var chest in chests)
		for(var i=0; i<4; ++i)
			chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxIntFillChestsTimer()
{
	SetTemperature(100);
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	var w_list = [Javelin, Bow, Musket, Boompack, Dynamite, Shield, WindScroll, FireballScroll, ThunderScroll, Club, Sword];
	var maxcount = [1,1,1,1,2,1,1,2,2,1,1];
	
	var contents;
	for(var i=0; i<chest->GetLength(w_list); i++)
		contents+=chest->ContentsCount(w_list[i]);
	if(contents > 5) return 1;
	
	if(!FindObject(Find_ID(Clonk),Find_Distance(20,chest->GetX(),chest->GetY())) && chest->ContentsCount()>2)
	{
		var r=chest->Random(chest->ContentsCount());
		var remove=true;
		for(var i=0; i<GetLength(w_list); i++)
			if(chest->Contents(r)->GetID() == w_list[i]) remove=false;
		if(remove) chest->Contents(r)->RemoveObject();			
	}
	
	for(var i=0; i<2 ; i++)
	{
		var r = Random(GetLength(w_list));
		if (chest->ContentsCount(w_list[r]) < maxcount[r])
		{
			chest->CreateChestContents(w_list[r]);
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
	var position = [[180,150],[310,320],[600,290],[650,180],[790,110],[440,190]];
	var r=Random(GetLength(position));
	var x = position[r][0], y = position[r][1];
	var relaunch = CreateObject(RelaunchContainer, x, y + 49, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	return;
}

func KillsToRelaunch() { return 0; }
func RelaunchWeaponList() { return [Bow,  Javelin, Musket, FireballScroll, WindScroll, ThunderScroll]; }
