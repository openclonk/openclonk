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
	PlaceEdges();
	MovingBricks();
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
	effect.var0=damage/10;
}
global func FxLifedrainAdd(object target, effect, string new_name, int new_timer, damage)
{
	effect.var0+=damage/10;
}
global func FxLifedrainTimer(object target, effect, int timer)
{
	if(effect.var0>0) return -1;
	target->DoEnergy(+100,1,0,-1);
	effect.var0+=10;
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
		if(GetEffect("Lifedrain",king))
		{
			clr=RGBa(BoundBy(-GetEffect("Lifedrain",king).var0,30,225)+Random(20),160-(BoundBy(-GetEffect("Lifedrain",king).var0/2,10,100))-Random(50),230-BoundBy(-GetEffect("Lifedrain",king).var0,30,225)+Random(20), 230+Random(20));
			var str=Random(BoundBy(-GetEffect("Lifedrain",king).var0/10,2,6));
		}
		CreateParticle("AirIntake",king->GetX()+Sin(r,6-Random(5)),king->GetY()-Cos(r,6-Random(5)),Sin(r + 90,8+Random(4)+str),-Cos(r +90,8+Random(4)+str),20+Random(30) +str*str,clr);
		CreateParticle("AirIntake",king->GetX()+Sin(r + 180,6-Random(5)),king->GetY()-Cos(r+180,6-Random(5)),Sin(r + 90,8+Random(4)+str),-Cos(r +90,8+Random(4)+str),20+Random(30)+str*str,clr);
		
	}
	CreateParticle("AirIntake",king->GetX()+Sin(r + 180,6-Random(5)),king->GetY()-Cos(r+180,6-Random(5)),0,-25,20+Random(20),king->GetPlayerColor());
	return 1;
}

global func MovingBricks()
{
	CreateObject(MovingBrick,370,430)->Room(140,0,0,10,0);
	CreateObject(MovingBrick,550,250)->Room(0,50,0,0,9);
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
	var x=[505, 525, 365, 765, 765, 695, 765, 275, 225, 225, 775, 685, 775, 685, 675, 665, 405, 415, 235, 215, 215, 375, 285, 285];
	var y=[385, 395, 395, 235, 205, 295, 295, 345, 345, 255, 365, 225, 305, 305, 315, 325, 225, 235, 445, 375, 355, 365, 355, 275];
	var d=[3, 2, 3, 3, 1, 1, 0, 0, 1, 0, 2, 3, 0, 1, 1, 1, 1, 2, 3, 3, 1, 0, 0, 2];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge);
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]+50);
		edge->PermaEdge();
	}
	return 1;
}	

global func PlaceGras()
{
	var x=[756, 756, 756, 757, 497, 497, 498, 498, 498, 497, 222, 223, 147, 148, 147, 148, 147, 147, 214, 215, 214, 214, 214, 214, 214, 705, 704, 704, 764, 765, 765, 765, 823, 824, 825, 825, 778, 778, 778, 778, 778, 395, 233, 235, 235, 396, 266, 396, 397, 397, 396, 396, 266, 267, 175, 180, 193, 199, 207, 212, 223, 273, 330, 322, 314, 306, 298, 289, 282, 338, 365, 306, 320, 325, 297, 369, 375, 379, 335, 312, 290, 285, 281, 276, 272, 227, 411, 207, 213, 218, 223, 404, 400, 435, 419, 481, 474, 468, 462, 427, 489, 555, 508, 514, 525, 532, 540, 548, 520, 503, 576, 604, 596, 590, 582, 570, 563, 609, 670, 626, 632, 661, 666, 638, 621, 615, 674, 680, 686, 691, 696, 760, 761, 766, 771, 781, 777, 778, 637, 643, 631, 612, 618, 624, 686, 692, 680, 652, 660, 668, 673, 699, 789, 819, 782, 771, 220, 228, 764, 188];
	var y=[264, 268, 275, 280, 418, 407, 395, 383, 393, 403, 289, 283, 257, 252, 229, 247, 237, 243, 270, 253, 248, 261, 240, 231, 235, 274, 271, 266, 241, 229, 222, 237, 198, 195, 190, 184, 213, 199, 186, 192, 208, 296, 327, 318, 312, 285, 320, 310, 284, 291, 304, 320, 312, 328, 227, 228, 225, 225, 226, 275, 297, 307, 308, 308, 306, 305, 307, 307, 306, 307, 408, 406, 406, 407, 406, 408, 413, 414, 408, 405, 404, 400, 395, 391, 388, 389, 268, 407, 403, 397, 394, 272, 279, 265, 266, 268, 267, 264, 267, 268, 269, 377, 377, 375, 377, 375, 378, 378, 376, 376, 376, 375, 375, 378, 379, 376, 376, 378, 367, 378, 377, 375, 371, 377, 376, 377, 364, 359, 353, 346, 341, 257, 338, 344, 350, 362, 356, 217, 257, 256, 256, 257, 257, 256, 259, 258, 259, 256, 258, 258, 259, 259, 179, 175, 178, 218, 274, 306, 250, 226];
	var r=[-89, -91, -93, -90, -93, -93, -92, -88, -93, -93, 87, 89, -90, -91, -89, -91, -90, -90, 93, 87, 88, 87, 90, 93, 88, 90, 93, 89, -87, -92, -92, -92, 88, 93, 90, 93, -91, -93, -88, -92, -90, -88, 93, 88, 91, -90, -88, -90, -91, -93, -87, -92, -87, -93, 0, 0, 0, 0, 0, 0, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 43, 43, 0, 0, 0, 46, 44, 42, 48, -43, -48, -45, -45, -43, -48, -45, -43, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -48, 0, 0, -43, -44, 0, 0, 0, -42, -42, -48, -44, -45, -89, 48, 46, 48, 44, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 44, -89, 0];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(Grass, x[i], y[i] + 5, NO_OWNER);
		edge->SetR(r[i]); 
		edge->Initialize();
		edge->SetCategory(C4D_StaticBack);
		if(!Random(10))
		{
			var big=Random(50);
			var new = CreateObject(Fern,edge->GetX(),edge->GetY()+(big/2));
			new->DoCon(big);
			new->SetR(r[i]);
			edge->Incinerate();
		}
	}
	return 1;
}

global func MakeGrasFunction(bool fExact)
{
	var x=[];
	var y=[];
	var r=[];
	for(var e in FindObjects(Find_ID(Grass)))
	{
		x[GetLength(x)]=e->GetX();
		y[GetLength(y)]=e->GetY();
		r[GetLength(r)]=e->GetR();
	}
	Log("global func PlaceGras()");
	Log("{");
	Log("	var x=%v;",x);
	Log("	var y=%v;",y);
	Log("	var r=%v;",r);

	Log("	for (var i = 0; i < GetLength(x); i++)");
	Log("	{");
	Log("		var edge=CreateObject(Grass, x[i], y[i] + 5, NO_OWNER);");
	Log("		edge->SetR(r[i]); ");
	Log("		edge->Initialize();"); //additional initialize for anti self blocking

	Log("	}");
	Log("	return 1;");
	Log("}");
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
