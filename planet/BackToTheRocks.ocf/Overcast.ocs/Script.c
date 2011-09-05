/*-- 
	ForgottenHeights
	Author: Mimmo_O
	
	Last man Standing on sky islands for up to 12 players.
--*/



protected func Initialize()
{

	// Goal.
	CreateObject(Goal_LastManStanding);
	CreateObject(Rule_KillLogs);
	
	//Enviroment.
	CreateObject(Environment_Clouds);
	CreateObject(Environment_Clouds); //Double the clouds.
	SetSkyAdjust(RGBa(250,250,255,128),RGB(200,200,220));
	Sound("BirdsLoop.ogg", true, 100, nil, 1);
	
	// Smooth brick edges.
	var x=[557, 565, 124, 213, 213, 172, 149, 909, 804, 909, 828, 692, 804, 812, 820, 828, 836, 844, 852, 860, 901, 893, 885, 877, 829, 837, 724, 581, 573, 364, 372, 372, 380, 589, 508, 461, 412, 508, 452, 517, 461, 781, 773, 829, 837, 549, 325, 292, 221, 180, 44, 109, 117, 125, 133, 261, 269, 277, 285, 293, 236, 228, 220, 212, 309, 197, 205, 213, 221, 229, 124, 100, 92, 380, 573, 573, 716, 789, 797, 805, 813, 821, 829, 909, 852, 716, 812, 812, 868, 525, 436, 436, 525];
	var y=[517, 509, 676, 692, 677, 525, 524, 445, 277, 396, 396, 340, 260, 252, 244, 236, 228, 220, 212, 204, 228, 220, 212, 204, 165, 157, 132, 45, 53, 109, 117, 45, 53, 109, 100, 100, 293, 284, 237, 237, 284, 348, 340, 132, 140, 284, 148, 148, 148, 148, 212, 212, 220, 228, 236, 236, 244, 252, 260, 268, 236, 244, 252, 260, 285, 396, 404, 412, 420, 428, 396, 404, 412, 484, 485, 500, 485, 484, 492, 500, 508, 516, 524, 524, 524, 500, 668, 653, 644, 677, 692, 677, 692];
	var d=[2, 2, 1, 2, 0, 1, 0, 2, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 2, 1, 2, 2, 3, 3, 3, 3, 2, 1, 0, 3, 1, 3, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 1, 1, 0, 3, 1, 2];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i], NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]);
		edge->PermaEdge();
	}
	
	// Vertically moving bricks
	var brick = CreateObject(MovingBrick, 660, 90);
	brick->SetSize(3);
	brick->MoveVertical(80, 144);
	
	var brick = CreateObject(MovingBrick, 592, 270);
	brick->SetSize(2);
	brick->MoveVertical(264, 344);
	var brick = CreateObject(MovingBrick, 632, 285);
	brick->SetSize(2);
	brick->MoveVertical(272, 352);
	var brick = CreateObject(MovingBrick, 672, 300);
	brick->SetSize(2);
	brick->MoveVertical(280, 360);
	
	var brick = CreateObject(MovingBrick, 270, 430);
	brick->SetSize(2);
	brick->MoveVertical(416, 488);
	var brick = CreateObject(MovingBrick, 310, 440);
	brick->SetSize(2);
	brick->MoveVertical(424, 496);
	var brick = CreateObject(MovingBrick, 350, 450);
	brick->SetSize(2);
	brick->MoveVertical(432, 504);
	
	var brick = CreateObject(MovingBrick, 932, 240);
	brick->SetSize(4);
	brick->MoveVertical(232, 536);
	
	var brick = CreateObject(MovingBrick, 368, 200);
	brick->SetSize(3);
	brick->MoveVertical(152, 288);
	
	var brick = CreateObject(MovingBrick, 184, 320);
	brick->SetSize(2);
	brick->MoveVertical(240, 368);
	
	var brick = CreateObject(MovingBrick, 498, 560);
	brick->SetSize(2);
	brick->MoveVertical(480, 680);
	DrawMaterialQuad("Tunnel-Brickback", 478, 480, 497, 480, 497, 552, 478, 552);
	DrawMaterialQuad("Tunnel-Brickback", 478, 672, 497, 672, 497, 680, 478, 680);
	
	var brick = CreateObject(MovingBrick, 170, 600);
	brick->SetSize(2);
	brick->MoveVertical(528, 680);
	DrawMaterialQuad("Tunnel-Brickback", 150, 524, 169, 524, 169, 576, 150, 576);
	DrawMaterialQuad("Tunnel-Brickback", 150, 672, 169, 672, 169, 680, 150, 680);
	
	// Horizontally moving bricks.
	var brick = CreateObject(MovingBrick, 600, 496);
	brick->SetSize(4);
	brick->MoveHorizontal(552, 736);
	
	var brick = CreateObject(MovingBrick, 656, 688);
	brick->SetSize(3);
	brick->MoveHorizontal(504, 668);
	var brick = CreateObject(MovingBrick, 688, 664);
	brick->SetSize(3);
	brick->MoveHorizontal(668, 832);
	
	var brick = CreateObject(MovingBrick, 320, 688);
	brick->SetSize(3);
	brick->MoveHorizontal(192, 456);
	
	// Chests with weapons.
	var chest;
	chest = CreateObject(Chest, 764, 128, NO_OWNER);
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObject(Chest, 732, 336, NO_OWNER);
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObject(Chest, 116, 400, NO_OWNER);
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObject(Chest, 272, 152, NO_OWNER);
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObject(Chest, 424, 480, NO_OWNER);
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObject(Chest, 872, 520, NO_OWNER);
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObject(Chest, 72, 208, NO_OWNER);
	AddEffect("FillChest", chest, 100, 72);
	
	// Blue chest with jar of winds.
	var chest_blue = CreateObject(Chest, 850, 648, NO_OWNER);
	chest_blue->SetClrModulation(RGB(100,180,255));
	AddEffect("FillBlueChest", chest_blue, 100, 72);
	
	// Red chest with club.
	var chest_red = CreateObject(Chest, 124, 520, NO_OWNER);
	chest_red->SetClrModulation(RGB(255, 100, 100));
	AddEffect("FillRedChest", chest_red, 100, 72);
		
	// Wind channel.
	AddEffect("WindChannel", nil, 100, 1);
	return;
}

/*-- Wind channel --*/

global func FxWindChannelStart(object target, proplist effect, int temporary)
{
	if (temporary == 0)
		effect.Divider = 1; 
	return 1;
}

global func FxWindChannelTimer(object target, proplist effect)
{
	// Channel divided into three parts.
	
	// First shaft with upward wind.
	for (var obj in FindObjects(Find_InRect(464, 288, 40, 152)))
	{
		var speed = obj->GetYDir(100);
		if (speed < -360)
			continue;
		obj->SetYDir(speed - 32, 100);
	}
	CreateParticle("AirIntake", 464+Random(40), 344+Random(112), RandomX(-1,1), -30, 60+Random(10), RGB(100+Random(25),128+Random(20),255));
	
	// Divider with random wind.
	if (!Random(100))
		effect.Divider = Random(3);
	for (var obj in FindObjects(Find_InRect(464, 240, 40, 48)))
	{
		if (effect.Divider == 1)
		{
			var speed = obj->GetYDir(100);
			if (speed < -360)
				continue;
			obj->SetYDir(speed - 32, 100);
		}
		if (effect.Divider == 0)
		{
			var speed = obj->GetXDir(100);
			if (speed > -360)
				obj->SetXDir(speed - 20, 100);			
		}
		if (effect.Divider == 2)
		{
			var speed = obj->GetXDir(100);
			if (speed < 360)
				obj->SetXDir(speed + 20, 100);			
		}
	}
	if (effect.Divider == 1)
		CreateParticle("AirIntake", 464+Random(40), 280+Random(10), RandomX(-1,1), -30, 60+Random(10), RGB(100+Random(25),128+Random(20),255));
	if (effect.Divider == 0)
		CreateParticle("AirIntake", 464+Random(40), 280+Random(10), RandomX(-20,-10), -20, 60+Random(10), RGB(100+Random(25),128+Random(20),255));
	if (effect.Divider == 2)
		CreateParticle("AirIntake", 464+Random(40), 280+Random(10), RandomX(10,20), -20, 60+Random(10), RGB(100+Random(25),128+Random(20),255));
		
	// Second shaft with upward wind.
	for (var obj in FindObjects(Find_InRect(464, 104, 40, 136)))
	{
		var speed = obj->GetYDir(100);
		if (speed < -360)
			continue;
		obj->SetYDir(speed - 32, 100);
	}
	CreateParticle("AirIntake", 464+Random(40), 160+Random(96), RandomX(-1,1), -30, 60+Random(10), RGB(100+Random(25),128+Random(20),255));
	
	return 1;
}

/*-- Chest contents --*/

global func FxFillChestStart(object target, proplist effect, int temporary)
{
	if (temporary) 
		return 1;
	var w_list = [Shield, Javelin, FireballScroll, Bow, Musket, WindScroll, TeleportScroll];
	for (var i = 0; i < 4; i++)
		target->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxFillChestTimer(object target, proplist effect)
{
	if (Random(5))
		return 1;
	var w_list = [Boompack, Dynamite, Shield, Javelin, Bow, Musket, Boompack, Dynamite, Shield, Javelin, Bow, Musket, TeleportScroll, WindScroll, FireballScroll];

	if (target->ContentsCount() < 6)
		target->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxFillBlueChestStart(object target, proplist effect, int temporary)
{
	if (temporary) 
		return 1;		
	target->CreateContents(JarOfWinds);
	var w_list = [Firestone, Boompack, FireballScroll, WindScroll];
	for (var i = 0; i < 4; i++)
		target->CreateContents(w_list[Random(GetLength(w_list))]);	
	return 1;
}

global func FxFillBlueChestTimer(object target, proplist effect)
{
	if (Random(5))
		return 1;
	var w_list = [Firestone, Boompack, FireballScroll, WindScroll];

	if (target->ContentsCount() < 6)
		target->CreateContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxFillRedChestStart(object target, proplist effect, int temporary)
{
	if (temporary) 
		return 1;
	target->CreateContents(Club);
	var w_list = [Firestone, Boompack, FireballScroll, WindScroll];
	for (var i = 0; i < 4; i++)
		target->CreateContents(w_list[Random(GetLength(w_list))]);	
	return 1;
}

global func FxFillRedChestTimer(object target, proplist effect)
{
	if (Random(5))
		return 1;
	var w_list = [Firestone, Boompack, FireballScroll, WindScroll];

	if (target->ContentsCount() < 6)
		target->CreateContents(w_list[Random(GetLength(w_list))]);
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

/*-- Player control --*/

// GameCall from RelaunchContainer.
func OnClonkLeftRelaunch(object clonk)
{
	var pos = GetRandomSpawn();
	clonk->SetPosition(pos[0],pos[1]);
	CastParticles("Magic",36,12,pos[0],pos[1],30,60,clonk->GetColor(),clonk->GetColor(),clonk);
	return;
}

global func GetRandomSpawn()
{
	var spawns = [[432,270],[136,382],[200,134],[864,190],[856,382],[840,518],[408,86],[536,470]];
	var rand = Random(GetLength(spawns));
	return spawns[rand];
}


// Gamecall from LastManStanding goal, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	return;
}

func KillsToRelaunch() { return 0; }
func RelaunchWeaponList() { return [Bow, Javelin, Musket, FireballScroll, WindScroll, TeleportScroll]; }
