/*-- 
	ForgottenHeights
	Author: Mimmo_O
	
	Last man Standing on sky islands for up to 12 players.
--*/


static Overcast_air_particles;

protected func Initialize()
{
	// Goal.
	CreateObject(Goal_LastManStanding);
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_Gravestones);
	GetRelaunchRule()->SetLastWeaponUse(false);
	
	//Enviroment.
	Cloud->Place(25);
	SetSkyAdjust(RGBa(250, 250, 255, 128),RGB(200, 200, 220));
	Sound("Environment::BirdsLoop", true, 100, nil, 1);
	
	// Brick edges at horizontal moving bricks.
	var x = [524, 436, 436, 524, 812, 812, 716, 716, 572, 572, 212, 212];
	var y = [692, 676, 692, 676, 652, 668, 500, 484, 500, 484, 676, 692];
	var d = [1, 3, 2, 0, 3, 2, 2, 3, 1, 0, 0, 1];
	for (var i = 0; i < GetLength(x); i++)
		DrawMaterialTriangle("Brick-brick", x[i], y[i], d[i]);
	
	// Vertically moving bricks
	var brick = CreateObjectAbove(MovingBrick, 660, 90);
	brick->SetSize(3);
	brick->MoveVertical(80, 144);
	
	var brick = CreateObjectAbove(MovingBrick, 592, 270);
	brick->SetSize(2);
	brick->MoveVertical(264, 344);
	var brick = CreateObjectAbove(MovingBrick, 632, 285);
	brick->SetSize(2);
	brick->MoveVertical(272, 352);
	var brick = CreateObjectAbove(MovingBrick, 672, 300);
	brick->SetSize(2);
	brick->MoveVertical(280, 360);
	
	var brick = CreateObjectAbove(MovingBrick, 270, 430);
	brick->SetSize(2);
	brick->MoveVertical(416, 488);
	var brick = CreateObjectAbove(MovingBrick, 310, 440);
	brick->SetSize(2);
	brick->MoveVertical(424, 496);
	var brick = CreateObjectAbove(MovingBrick, 350, 450);
	brick->SetSize(2);
	brick->MoveVertical(432, 504);
	
	var brick = CreateObjectAbove(MovingBrick, 932, 240);
	brick->SetSize(4);
	brick->MoveVertical(232, 536);
	
	var brick = CreateObjectAbove(MovingBrick, 368, 200);
	brick->SetSize(3);
	brick->MoveVertical(152, 288);
	
	var brick = CreateObjectAbove(MovingBrick, 184, 320);
	brick->SetSize(2);
	brick->MoveVertical(240, 368);
	
	var brick = CreateObjectAbove(MovingBrick, 498, 560);
	brick->SetSize(2);
	brick->MoveVertical(480, 680);
	DrawMaterialQuad("Tunnel-Brickback", 478, 478, 497, 478, 497, 552, 478, 552);
	DrawMaterialQuad("Tunnel-Brickback", 478, 670, 497, 670, 497, 680, 478, 680);
	
	var brick = CreateObjectAbove(MovingBrick, 170, 600);
	brick->SetSize(2);
	brick->MoveVertical(528, 680);
	DrawMaterialQuad("Tunnel-Brickback", 150, 524, 169, 524, 169, 576, 150, 576);
	DrawMaterialQuad("Tunnel-Brickback", 150, 670, 169, 670, 169, 680, 150, 680);
	
	// Horizontally moving bricks.
	var brick = CreateObjectAbove(MovingBrick, 600, 496);
	brick->SetSize(4);
	brick->MoveHorizontal(552, 736);
	
	var brick = CreateObjectAbove(MovingBrick, 656, 688);
	brick->SetSize(3);
	brick->MoveHorizontal(504, 668);
	var brick = CreateObjectAbove(MovingBrick, 688, 664);
	brick->SetSize(3);
	brick->MoveHorizontal(668, 832);
	
	var brick = CreateObjectAbove(MovingBrick, 320, 688);
	brick->SetSize(3);
	brick->MoveHorizontal(192, 456);
	
	// Chests with weapons.
	var chest;
	chest = CreateObjectAbove(Chest, 764, 128, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObjectAbove(Chest, 732, 336, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObjectAbove(Chest, 116, 400, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObjectAbove(Chest, 272, 152, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObjectAbove(Chest, 424, 480, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObjectAbove(Chest, 872, 520, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillChest", chest, 100, 72);
	chest = CreateObjectAbove(Chest, 72, 208, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillChest", chest, 100, 72);
	
	// Some columns as decoration.
	CreateObjectAbove(Column, 736, 480, NO_OWNER);
	CreateObjectAbove(Column, 860, 392, NO_OWNER);
	CreateObjectAbove(Column, 420, 280, NO_OWNER);
	CreateObjectAbove(Column, 200, 144, NO_OWNER);
	CreateObjectAbove(Column, 556, 480, NO_OWNER);
	CreateObjectAbove(Column, 452, 672, NO_OWNER);
	CreateObjectAbove(Column, 192, 672, NO_OWNER);
	
	// Grass as decoration.
	var x=[813, 820, 856, 833, 827, 838, 844, 869, 872, 882, 853, 861, 506, 466, 527, 524, 514, 510, 517, 502, 436, 437, 441, 448, 457, 463, 177, 139, 121, 126, 199, 190, 181, 213, 206, 133, 147, 196, 104, 175, 116, 108, 126, 135, 155, 201, 195, 188, 43, 52, 58, 406, 410, 551, 554, 550, 547, 538, 723, 838, 802, 808, 836, 690, 781, 787, 745, 869, 861, 825, 829, 849, 720, 910, 908, 901, 795, 803, 819, 875, 855, 830, 813, 869, 894, 886, 844, 796, 789, 787, 729, 764, 779, 770, 755, 749, 741, 574, 533, 574, 566, 559, 546, 426, 429, 414, 407, 397, 390, 387, 380, 376, 218, 210, 97, 104, 118, 123, 131, 139, 144, 154, 212, 194, 209, 205, 198, 188, 181, 174, 216, 221, 291, 313, 298, 305, 307, 296, 301, 284, 269, 103, 84, 132, 124, 148, 146, 140, 118, 107, 91, 332, 330, 328, 314, 298, 281, 184, 236, 227, 213, 268, 292, 307, 315, 322, 365, 362, 370, 378, 578, 582, 589, 731, 734, 740, 759, 773, 787, 833, 826, 806, 818, 832, 842, 864, 851, 887, 892, 906, 877, 732, 729, 709, 704, 757, 769, 717, 892, 899, 922, 907, 857, 842, 832];
	var y=[648, 646, 645, 634, 636, 629, 623, 628, 633, 640, 620, 621, 668, 669, 673, 666, 655, 648, 660, 643, 675, 664, 658, 653, 646, 646, 669, 655, 673, 662, 661, 653, 652, 672, 670, 660, 645, 500, 518, 493, 508, 516, 499, 500, 484, 519, 517, 493, 207, 205, 204, 283, 277, 282, 289, 275, 268, 259, 127, 137, 258, 254, 223, 336, 343, 331, 333, 389, 374, 394, 387, 372, 479, 522, 513, 508, 484, 486, 484, 498, 474, 483, 477, 493, 500, 500, 469, 472, 469, 464, 476, 468, 460, 462, 467, 468, 469, 482, 453, 475, 470, 468, 460, 460, 451, 461, 460, 460, 462, 469, 477, 483, 412, 405, 403, 398, 382, 377, 373, 374, 373, 373, 398, 376, 389, 384, 381, 373, 372, 372, 235, 229, 232, 272, 271, 270, 255, 243, 250, 226, 220, 191, 192, 207, 204, 223, 216, 213, 193, 188, 187, 126, 119, 147, 140, 121, 131, 141, 131, 132, 125, 131, 124, 116, 116, 116, 88, 93, 84, 75, 77, 84, 92, 116, 112, 108, 107, 108, 108, 131, 111, 108, 107, 204, 196, 197, 188, 193, 196, 212, 179, 318, 316, 310, 316, 317, 323, 307, 372, 369, 367, 363, 372, 372, 379];
	var r=[-50, 0, 0, -50, 0, -50, -30, 90, 50, 60, 0, 0, 0, 0, 60, 60, 60, 45, 0, 0, -45, -45, -45, 0, 0, 0, 0, -45, -45, -45, 0, 0, 0, 45, 0, 0, 0, 85, -35, 0, 0, 0, 0, 0, 0, 35, 0, 0, -35, 0, 0, -85, 0, 45, 95, 95, 95, 0, -45, 45, -45, 0, -45, -45, 45, 95, 0, 0, 0, -35, -95, 0, -35, 75, 75, 0, 45, 0, 90, 90, 60, 0, 0, 0, 0, 0, 0, 60, 0, 60, 0, 0, 0, 0, 0, 0, 0, 45, 90, 90, 0, 0, 0, 0, -90, 0, 0, 0, -40, 0, -40, -40, 40, 30, -30, 0, -30, -30, 0, 0, 0, 0, 90, 40, 40, 40, 0, 0, 0, 0, -40, -20, 50, 50, 50, 0, 50, 50, 50, 0, 0, -40, -40, 50, 0, 90, 50, 0, 50, 0, 0, 90, 55, 55, 0, -35, -35, 0, 0, 0, 0, 0, 0, 0, 0, 0, -55, 0, 0, 0, 55, 0, 0, 0, -35, 0, 0, 0, 0, 35, 35, 0, 0, -35, 0, 0, 0, 35, 0, 0, 0, 0, 0, -35, -45, 0, 0, 0, 0, -45, 45, 0, 10, 0, -50];
	for (var i = 0; i < GetLength(x); i++)
	{
		var grass = CreateObjectAbove(Grass, x[i], y[i] + 5, NO_OWNER);
		grass->SetR(r[i]); 
	}
	
	// Blue chest with wind bag
	var chest_blue = CreateObjectAbove(Chest, 850, 648, NO_OWNER);
	chest_blue->SetClrModulation(RGB(100, 180, 255));
	AddEffect("FillBlueChest", chest_blue, 100, 72);
	
	// Red chest with club.
	var chest_red = CreateObjectAbove(Chest, 124, 520, NO_OWNER);
	chest_red->SetClrModulation(RGB(255, 100, 100));
	AddEffect("FillRedChest", chest_red, 100, 72);
		
	// Wind channel.
	AddEffect("WindChannel", nil, 100, 1);
	
	Overcast_air_particles =
	{
		Prototype = Particles_Air(),
		Size = PV_KeyFrames(0, 0, 0, 100, PV_Random(5, 10), 1000, 0),
		OnCollision = PC_Die()
	};
	return;
}

protected func InitializePlayer(int plr)
{
	// This scenario does not have shadows.
	SetFoW(false, plr);
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
		obj->SetYDir(speed - 36, 100);
	}
	CreateParticle("Air", 464 + Random(40), 344 + Random(112), RandomX(-1, 1), -30, PV_Random(10, 40), Overcast_air_particles);
	
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
			obj->SetYDir(speed - 36, 100);
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
		CreateParticle("Air", 464 + Random(40), 280 + Random(10), RandomX(-1, 1), -30, PV_Random(10, 40), Overcast_air_particles);
	if (effect.Divider == 0)
		CreateParticle("Air", 464 + Random(40), 280 + Random(10), RandomX(-20,-10), -20, PV_Random(10, 40), Overcast_air_particles);
	if (effect.Divider == 2)
		CreateParticle("Air", 464 + Random(40), 280 + Random(10), RandomX(10, 20), -20, PV_Random(10, 40), Overcast_air_particles);
		
	// Second shaft with upward wind.
	for (var obj in FindObjects(Find_InRect(464, 96, 40, 144)))
	{
		// Divert at top.
		if (obj->GetY() < 104)
		{
			if (obj->GetX() < 484)
			{
				var speed = obj->GetXDir(100);
				if (speed > -200)
				obj->SetXDir(speed - 16, 100);					
			}
			else
			{
				var speed = obj->GetXDir(100);
				if (speed < 200)
				obj->SetXDir(speed + 16, 100);				
			}			
			//continue;
		}
		var speed = obj->GetYDir(100);
		if (speed < -360)
			continue;
		obj->SetYDir(speed - 36, 100);
	}
	CreateParticle("Air", 464 + Random(40), 160 + Random(96), RandomX(-1, 1), -30, PV_Random(10, 40), Overcast_air_particles);
	
	return 1;
}

/*-- Chest contents --*/

global func FxFillChestStart(object target, proplist effect, int temporary)
{
	if (temporary) 
		return 1;
	var w_list = [Shield, Javelin, FireballScroll, Bow, Blunderbuss, WindScroll, TeleportScroll];
	for (var i = 0; i < 4; i++)
		target->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxFillChestTimer(object target, proplist effect)
{
	if (Random(5))
		return 1;
	var w_list = [Balloon, Boompack, IronBomb, Shield, Javelin, Bow, Blunderbuss, Boompack, IronBomb, Shield, Javelin, Bow, Blunderbuss, TeleportScroll, WindScroll, FireballScroll];

	if (target->ContentsCount() < 6)
		target->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxFillBlueChestStart(object target, proplist effect, int temporary)
{
	if (temporary) 
		return 1;		
	target->CreateContents(WindBag);
	var w_list = [Firestone, Boompack, Balloon, FireballScroll, WindScroll];
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
		
	if (!FindObject(Find_ID(WindBag)))
		target->CreateContents(WindBag);
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
		
	if (!FindObject(Find_ID(Club)))
		target->CreateContents(Club);
		
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

/*-- Player control --*/

// GameCall from RelaunchContainer.
func OnClonkLeftRelaunch(object clonk)
{
	CreateParticle("Air", clonk->GetX(),clonk->GetY(), PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(5, 10), Overcast_air_particles, 25);
	return;
}

public func RelaunchPosition()
{
	return [[432, 270],[136, 382],[200, 134],[864, 190],[856, 382],[840, 518],[408, 86],[536, 470]];
	
}

global func GetRandomSpawn()
{
	return RandomElement(Scenario->RelaunchPosition());
}

func KillsToRelaunch() { return 0; }
func RelaunchWeaponList() { return [Bow, Javelin, Blunderbuss, FireballScroll, WindScroll, TeleportScroll]; }
