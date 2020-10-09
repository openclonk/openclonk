/*-- 
	Thunderous Skies
	Author: Mimmo_O
	
	King of the hill high in the skies.
--*/


static ThunderousSkies_air_particles, ThunderousSkies_air_particles_red;

protected func Initialize()
{
	// Goal.
	CreateObject(Rule_ObjectFade)->DoFadeTime(8 * 36);
	var goal = CreateObject(Goal_KingOfTheHill, 450, 380, NO_OWNER);
	goal->SetRadius(90);
	goal->SetPointLimit(5);
	AddEffect("BlessTheKing",goal, 100, 1, nil);
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_Gravestones);
	GetRelaunchRule()
		->SetLastWeaponUse(false)
		->SetDefaultRelaunchCount(nil);
	
	//Enviroment.
	//SetSkyAdjust(RGBa(250, 250, 255, 128),RGB(200, 200, 220));
	SetSkyParallax(1, 20, 20, 0, 0, nil, nil);
	Sound("Environment::BirdsLoop",true, 100, nil,+1);
		
	CreateObjectAbove(Column, 650, 375);
	CreateObjectAbove(Column, 350, 407);
	CreateObjectAbove(Column, 162, 231);
	CreateObjectAbove(Column, 448, 271);
	CreateObjectAbove(Column, 810, 175);

	// Chests with weapons.
	CreateObjectAbove(Chest, 175, 200, NO_OWNER)->MakeInvincible();
	CreateObjectAbove(Chest, 800, 150, NO_OWNER)->MakeInvincible();
	CreateObjectAbove(Chest, 430, 240, NO_OWNER)->MakeInvincible();
	CreateObjectAbove(Chest, 610, 340, NO_OWNER)->MakeInvincible();
	CreateObjectAbove(Chest, 355, 390, NO_OWNER)->MakeInvincible();
	
	AddEffect("IntFillChests", nil, 100, 2 * 36, nil);
	AddEffect("ChanneledWind", nil, 1, 1, nil);
	AddEffect("Balloons", nil, 100, 100, nil);
	
	// Moving bricks.
	var brick;
	brick = CreateObjectAbove(MovingBrick, 370, 424);
	brick->MoveHorizontal(344, 544);
	brick = CreateObjectAbove(MovingBrick, 550, 250);
	brick->MoveVertical(240, 296);

	// Smooth brick edges.
	DrawMaterialTriangle("Brick-brick", 380, 412, 0);
	
	PlaceGras();
	
	ThunderousSkies_air_particles =
	{
		Prototype = Particles_Air(),
		Size = PV_KeyFrames(0, 0, 0, 100, PV_Random(2, 5), 1000, 0),
		Stretch = PV_Speed(2000, 0),
		OnCollision = PC_Stop(),
		ForceY = -20,
		Attach = ATTACH_Back | ATTACH_MoveRelative
	};
	
	ThunderousSkies_air_particles_red =
	{
		Prototype = ThunderousSkies_air_particles,
		R = 255,
		G = PV_KeyFrames(0, 0, 255, 250, 255, 500, 0),
		B = PV_KeyFrames(0, 0, 255, 250, 255, 500, 0)
	};
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
		AddEffect("Lifedrain",king, 100, 1, nil, nil, damage/3);
	return damage;
}
global func FxLifedrainStart(object target, effect, int temporary, damage)
{
	if (temporary) return 1;
	effect.drain = damage/10;
}
global func FxLifedrainAdd(object target, effect, string new_name, int new_timer, damage)
{
	effect.drain += damage/10;
}
global func FxLifedrainTimer(object target, effect, int timer)
{
	if (effect.drain>0) return -1;
	target->DoEnergy(+100, 1, 0,-1);
	effect.drain += 10;
}
global func FxBlessTheKingTimer(object target, effect, int timer)
{
	if (!FindObject(Find_ID(KingOfTheHill_Location))) return 1;
	if (FindObject(Find_ID(KingOfTheHill_Location))->GetKing() == nil) return 1;
	
	var king = FindObject(Find_ID(KingOfTheHill_Location))->GetKing();
	var particles = ThunderousSkies_air_particles;
	var duration = 10;
	if (GetEffect("Lifedrain", king))
	{
		particles = ThunderousSkies_air_particles_red;
		duration *= 2;
	}
	king->CreateParticle("Air", 0, 8, PV_Random(-10, 10),PV_Random(0, 10), PV_Random(duration, 2 * duration), particles, 4);
	return 1;
}

public func ApplyChanneledWindEffects(x, y, w, h, bottom)
{
	for (var obj in FindObjects(Find_InRect(x, y, w, h)))
	{
		obj->SetYDir(Max(obj->GetYDir()-5,-50));
		var x_dir = -1;
		if (obj->GetXDir() > 0)
			x_dir = +1;
		else if (obj->GetXDir() == 0)
			x_dir = RandomX(-2, 2);
		obj->SetXDir(obj->GetXDir() + x_dir);
	}
	CreateParticle("Air", x + Random(w),bottom, RandomX(-1, 1),-30, PV_Random(10, 30), ThunderousSkies_air_particles);
}

global func FxChanneledWindTimer()
{
	Scenario->ApplyChanneledWindEffects(230, 300, 40, 90, 398);
	Scenario->ApplyChanneledWindEffects(700, 250, 60, 100, 348);
}

global func FxBalloonsTimer()
{
	if (ObjectCount(Find_ID(TargetBalloon)) > 2 )
	{
		return 1;
	}
	if (ObjectCount(Find_ID(TargetBalloon)) )
	{
		if (Random(6)) return 1;	
	}

	if (Random(2)) return 1;
	
	var x = Random(300)+50;
	if (Random(2)) x = LandscapeWidth() - x;
	var y = Random(50) + 100;
	var target;
	
	var r = Random(3);
	if (r == 0) { target = CreateObjectAbove(Boompack, x, y, NO_OWNER); target->SetR(180); }
	if (r == 1) { target = CreateObjectAbove(DynamiteBox, x, y, NO_OWNER); target->SetR(180); }
	if (r == 2) { target = CreateObjectAbove(Arrow, x, y, NO_OWNER); target->SetObjDrawTransform(1000, 0, 0, 0, 1000,-7500); }
		
		
	var balloon = CreateObjectAbove(TargetBalloon, x, y-30, NO_OWNER);
	balloon->SetProperty("load",target);
	target->SetAction("Attach", balloon);
	CreateParticle("Flash", x, y, 0, 0, 8, Particles_Flash());
	AddEffect("HorizontalMoving", balloon, 1, 1, balloon);
	balloon->SetXDir(((Random(2)*2)-1) * (Random(4)+3));
}

global func PlaceGras()
{
	var x=[747, 475, 474, 359, 244, 216, 324, 705, 754, 758, 828, 828, 235, 266, 266, 269, 177, 194, 204, 223, 348, 273, 284, 365, 369, 375, 379, 285, 281, 274, 233, 390, 229, 401, 388, 414, 476, 468, 463, 457, 422, 482, 493, 615, 609, 625, 631, 700, 704, 687, 761, 763, 771, 777, 619, 615, 621, 696, 789, 766, 356, 228, 188];
	var y=[280, 376, 384, 299, 279, 224, 290, 244, 226, 215, 175, 169, 328, 319, 310, 327, 205, 212, 213, 227, 292, 306, 294, 409, 407, 413, 414, 399, 396, 390, 388, 264, 392, 249, 256, 247, 252, 249, 241, 246, 246, 245, 363, 347, 376, 349, 348, 348, 335, 350, 337, 346, 349, 351, 249, 255, 254, 237, 178, 212, 291, 305, 227];
	var r=[-91, -93, -93, 89, 93, 93, 88, 89, -92, -92, 88, 93, 93, -88, -87, -93, 0, 0, 0, 0, 0, 0, 0, 0, 0, 43, 43, 46, 44, 48, -43, -48, -48, -45, -43, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -48, -44, -45, 48, 46, 48, 45, 0, 0, 0, 0, 0, 0, 0, 44, 0];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge = CreateObjectAbove(Grass, x[i], y[i] + 5, NO_OWNER);
		edge->SetR(r[i]); 
		edge->Initialize();
	}
	return 1;
}

// Refill/fill chests.
global func FxIntFillChestsStart(object target, effect, int temporary)
{
	if (temporary) return 1;
	var chests = FindObjects(Find_ID(Chest),Find_InRect(0, 0, LandscapeWidth(),610));
	var w_list = [Shield, Javelin, FireballScroll, Bow, Blunderbuss, WindScroll, ThunderScroll];
	
	for (var chest in chests)
		for (var i = 0; i<4; ++i)
			chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxIntFillChestsTimer()
{
	SetTemperature(100);
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	var w_list = [Javelin, Bow, Blunderbuss, Boompack, IronBomb, Shield, WindScroll, FireballScroll, ThunderScroll, Club, Sword];
	var maxcount = [1, 1, 1, 1, 2, 1, 1, 2, 2, 1, 1];
	
	var contents;
	for (var i = 0; i<chest->GetLength(w_list); i++)
		contents += chest->ContentsCount(w_list[i]);
	if (contents > 5) return 1;
	
	if (!FindObject(Find_ID(Clonk),Find_Distance(20, chest->GetX(),chest->GetY())) && chest->ContentsCount()>2)
	{
		var r = chest->Random(chest->ContentsCount());
		var remove = true;
		for (var i = 0; i<GetLength(w_list); i++)
			if (chest->Contents(r)->GetID() == w_list[i]) remove = false;
		if (remove) chest->Contents(r)->RemoveObject();			
	}
	
	for (var i = 0; i<2 ; i++)
	{
		var r = Random(GetLength(w_list));
		if (chest->ContentsCount(w_list[r]) < maxcount[r])
		{
			chest->CreateChestContents(w_list[r]);
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

protected func InitializePlayer(int plr)
{
	// This scenario does not have shadows.
	SetFoW(false, plr);
}

public func RelaunchPosition()
{
	return [[180, 150],[310, 300],[600, 290],[650, 180],[790, 110],[440, 190]];
}

func KillsToRelaunch() { return 0; }
func RelaunchWeaponList() { return [Bow,  Javelin, Blunderbuss, FireballScroll, WindScroll, ThunderScroll]; }
