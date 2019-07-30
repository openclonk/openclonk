/** 
	Rock Bottom
	An arena like chaotic last man standing round in a well.

	@author Mimmo_O, Maikel
*/


protected func Initialize()
{
	// Goal and rules.
	CreateObject(Goal_LastManStanding, 0, 0, NO_OWNER);
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_Gravestones);
	GetRelaunchRule()->SetLastWeaponUse(false);
	
	// Chests with weapons.
	var chest = CreateObjectAbove(Chest, 108, 248);
	chest->MakeInvincible();
	AddEffect("IntFillChests", nil, 100, 36, nil, nil, chest);

	// Objects fade after 7 seconds.
	CreateObject(Rule_ObjectFade)->DoFadeTime(7 * 36);
	
	// Some decoration trunks ranks and a waterfall.
	var trunk = CreateObjectAbove(Trunk, 76, 324);
	trunk->SetR(60); trunk.Plane = 510;
	trunk.MeshTransformation = [-731, 0, 682, 0, 0, 1000, 0, 0, -682, 0, -731, 0];
	trunk = CreateObjectAbove(Trunk, 123, 68);
	trunk->SetR(115); trunk.Plane = 510;
	trunk.MeshTransformation = [469, 0, 883, 0, 0, 1000, 0, 0, -883, 0, 469, 0];
	trunk = CreateObjectAbove(Trunk, 172, 134);
	trunk->SetR(-110); trunk.Plane = 510;
	trunk.MeshTransformation = [-545, 0, -839, 0, 0, 1000, 0, 0, 839, 0, -545, 0];
	
	var waterfall;
	waterfall = CreateWaterfall(130, 53, 2, "Water");
	waterfall->SetDirection(6, 3, 2, 3);
	waterfall = CreateWaterfall(144, 50, 8, "Water");
	waterfall->SetDirection(9, 3, 3, 3);
	CreateLiquidDrain(100, 315, 10);
	CreateLiquidDrain(130, 315, 10);
	CreateLiquidDrain(160, 315, 10);
	
	CreateObjectAbove(Fern, 48, 114);
	CreateObjectAbove(Fern, 284, 128);
	CreateObjectAbove(Lorry, 294, 128)->SetR(20);
	CreateObjectAbove(Pickaxe, 260, 128)->SetR(-45); 
	CreateObjectAbove(Mushroom, 271, 136);
	
	CreateObjectAbove(Branch, 146, 316)->SetR(180);
	CreateObjectAbove(Branch, 192, 198)->SetR(225);
	CreateObjectAbove(Branch, 54, 76)->SetR(180);
	CreateObjectAbove(Branch, 50, 232)->SetR(120);
	CreateObjectAbove(Branch, 264, 238)->SetR(-120);
	
	for (var i = 0; i < 2 + Random(6); i++) 
		CreateObjectAbove(Branch, 121, 10 + Random(140))->SetR(RandomX(60, 120));
	for (var i = 0; i < 2 + Random(6); i++) 
		CreateObjectAbove(Branch, 183, 10 + Random(140))->SetR(-RandomX(60, 120));
	
	// Some lights to have the well visible at all times.
	CreateLight(152, 40, 80, Fx_Light.LGT_Constant);
	CreateLight(152, 120, 80, Fx_Light.LGT_Constant);
	CreateLight(152, 200, 80, Fx_Light.LGT_Constant);
	CreateLight(152, 340, 80, Fx_Light.LGT_Constant);
	var torch1 = CreateObjectAbove(Torch, 60, 256);
	torch1->AttachToWall(true);
	var torch2 = CreateObjectAbove(Torch, 224, 256);
	torch2->AttachToWall(true);
	return;
}

protected func InitializePlayer(int plr)
{
	// Set player zoom to maximally the landscape height.
	SetPlayerZoomByViewRange(plr, nil, LandscapeHeight(), PLRZOOM_LimitMax);
	// Set player zoom to minimally the half the landscape width.
	SetPlayerZoomByViewRange(plr, LandscapeWidth() / 2, nil, PLRZOOM_LimitMin);
	// Set player zoom to be standard the landscape width.
	SetPlayerZoomByViewRange(plr, LandscapeWidth(), nil, PLRZOOM_Direct);
	SetPlayerViewLock(plr, true);
	return;
}

// Refill/fill chests.
global func FxIntFillChestsStart(object target, proplist effect, int temporary, object chest)
{
	if (temporary)
		return 1;
	// Store weapon list and chest.		
	effect.w_list = [Dynamite, Dynamite, Firestone, Firestone, Bow, Blunderbuss, Club, Sword, Javelin, IronBomb, PowderKeg];
	effect.chest = chest;
	// Fill the chest with ten items.
	for (var i = 0; i < 10; i++)
		effect.chest->CreateChestContents(effect.w_list[Random(GetLength(effect.w_list))]);
	return 1;
}

global func FxIntFillChestsTimer(object target, proplist effect, int time)
{
	// Refill the chest with up to ten items.
	if (effect.chest->ContentsCount() < 10 || !Random(effect.chest->ContentsCount()))
		effect.chest->CreateChestContents(effect.w_list[Random(GetLength(effect.w_list))]);
	return 1;
}

global func CreateChestContents(id obj_id)
{
	if (!this)
		return;
	var obj = CreateObjectAbove(obj_id);
	if (obj_id == Bow)
		obj->CreateContents([Arrow, BombArrow][Random(2)]);
	if (obj_id == Blunderbuss)
		obj->CreateContents(LeadBullet);
	obj->Enter(this);
	return;
}

// GameCall from RelaunchContainer.
public func OnClonkEnteredRelaunch(object clonk)
{
	clonk->CreateContents(Sword);
}

// GameCall from RelaunchContainer.
public func OnClonkLeftRelaunch(object clonk)
{
	clonk->SetPosition(RandomX(120, 160), -20);
	clonk->Fling(0, 5);
	return;
}

public func RelaunchPosition()
{
	return [LandscapeWidth() / 2, LandscapeHeight() / 2];
}

public func KillsToRelaunch() { return 0; }
public func RelaunchWeaponList() { return [Bow, Shield, Firestone, Dynamite, Javelin, Blunderbuss]; }
