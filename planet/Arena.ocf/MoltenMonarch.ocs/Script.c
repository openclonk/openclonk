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
	CreateObject(Rule_Gravestones)->SetFadeOut(3 * 36);
	GetRelaunchRule()->SetLastWeaponUse(false);
	
	//make lava collapse
	CreateObjectAbove(Firestone,625,480);

	// Chests with weapons.
	CreateObjectAbove(Chest, 320, 80, NO_OWNER)->MakeInvincible();
	CreateObjectAbove(Chest, 720, 192, NO_OWNER)->MakeInvincible();
	CreateObjectAbove(Chest, 668, 336, NO_OWNER)->MakeInvincible();
	CreateObjectAbove(Chest, 320, 440, NO_OWNER)->MakeInvincible();
	CreateObjectAbove(Chest, 48, 256, NO_OWNER)->MakeInvincible();
	AddEffect("IntFillChests", nil, 100, 5 * 36);
	
	// Moving bricks.
	var brick;
	brick = CreateObjectAbove(MovingBrick, 542, 176);
	brick->SetSize(3);
	brick->MoveVertical(168, 472, 8);
	brick = CreateObjectAbove(MovingBrick, 588, 192);
	brick->SetSize(3);
	brick->MoveVertical(160, 464, 8);
	
	brick = CreateObjectAbove(MovingBrick, 77, 0);
	brick->MoveVertical(0, LandscapeHeight(), 6);
	AddEffect("LavaBrickReset", brick, 100, 10);
	brick = CreateObjectAbove(MovingBrick, 77, LandscapeHeight() / 3);
	brick->MoveVertical(0, LandscapeHeight(), 6);
	AddEffect("LavaBrickReset", brick, 100, 10);
	brick = CreateObjectAbove(MovingBrick, 77, 2 * LandscapeHeight() / 3);
	brick->MoveVertical(0, LandscapeHeight(), 6);
	AddEffect("LavaBrickReset", brick, 100, 10);
	
	AddEffect("DeathByFire",nil,100,2,nil);
	return;
}

global func FxBlessTheKingStart(target, effect fx, temp)
{
	if (temp) return;
	fx.particles = 
	{
		Prototype = Particles_Fire(),
		Attach = ATTACH_Back
	};
	fx.koth_location = nil;
}

global func FxBlessTheKingTimer(object target, effect fx, int timer)
{
	if (!fx.koth_location)
	{
		fx.koth_location = FindObject(Find_ID(KingOfTheHill_Location));
		if (!fx.koth_location) return FX_OK;
	}
	var king = fx.koth_location->GetKing(); 
	if(king == nil) return 1;

	var item = king->GetHandItem(0);
	if (item) item->~MakeKingSize();

	king->CreateParticle("Fire", PV_Random(-4, 4), PV_Random(-11, 8), PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(10, 30), fx.particles, 10);
	return 1;
}

global func FxDeathByFireStart(object target, effect fx, bool temp)
{
	if (temp) return;
	fx.particles = Particles_Fire();
}

global func FxDeathByFireTimer(object target, effect fx, int timer)
{
	for(var obj in FindObjects(Find_InRect(55,0,50,70), Find_Category(C4D_Object | C4D_Living)))
	{
		if (obj->GetAlive())
			obj->Kill();
		
		if (obj && obj->GetY() <= 30 && obj->GetID() != MovingBrick)
		{
			obj->RemoveObject();
		}
	}

	CreateParticle("Fire", PV_Random(55, 95), PV_Random(0, 40), PV_Random(-1, 1), PV_Random(0, 20), PV_Random(10, 40), fx.particles, 20);
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
	var w_list = [Bow, Blunderbuss, Shield, Sword, Club, Javelin, Bow, Blunderbuss, Shield, Sword, Club, Javelin, DynamiteBox];
	
	for(var chest in chests)
		for(var i=0; i<4; ++i)
			chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxIntFillChestsTimer()
{
	SetTemperature(100); 
	var chests = FindObjects(Find_ID(Chest));
	var w_list = [IronBomb, Rock, IronBomb, Firestone, Firestone, Bow, Blunderbuss, Sword, Javelin];
	for(var chest in chests)
		if (chest->ContentsCount() < 5 )
			chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
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

public func RelaunchPosition()
{
	return [[420,200],[300,440],[130,176],[140,368],[700,192],[670,336],[750,440],[440,392],[45,256]];
}

func RelaunchWeaponList() { return [Bow, Shield, Sword, Javelin, Blunderbuss, Club]; }
