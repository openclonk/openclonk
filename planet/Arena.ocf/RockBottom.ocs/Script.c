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
	
	// Chests with weapons.
	var chest = CreateObject(Chest, 108, 248);
	chest->MakeInvincible();
	AddEffect("IntFillChests", nil, 100, 36, nil, nil, chest);

	// Objects fade after 5 seconds.
	CreateObject(Rule_ObjectFade)->DoFadeTime(7 * 36);
	
	// Some decoration trunks ranks and a waterfall.
	var trunk = CreateObject(Trunk, 76, 324);
	trunk->SetR(60); trunk.Plane = 510;
	trunk.MeshTransformation = [-731, 0, 682, 0, 0, 1000, 0, 0, -682, 0, -731, 0];
	trunk = CreateObject(Trunk, 123, 68);
	trunk->SetR(115); trunk.Plane = 510;
	trunk.MeshTransformation = [469, 0, 883, 0, 0, 1000, 0, 0, -883, 0, 469, 0];
	trunk = CreateObject(Trunk, 172, 134);
	trunk->SetR(-110); trunk.Plane = 510;
	trunk.MeshTransformation = [-545, 0, -839, 0, 0, 1000, 0, 0, 839, 0, -545, 0];
	
	var waterfall;
	waterfall = CreateWaterfall(130, 53, 2, "Water");
	waterfall->SetDirection(4, 0, 3, 6);
	waterfall = CreateWaterfall(144, 50, 8, "Water");
	waterfall->SetDirection(6, 0, 5, 6);
	CreateLiquidDrain(100, 315, 10);
	CreateLiquidDrain(130, 315, 10);
	CreateLiquidDrain(160, 315, 10);
	
	CreateObject(Fern, 48, 114);
	CreateObject(Fern, 284, 128);
	CreateObject(Lorry, 294, 128)->SetR(20);
	CreateObject(Pickaxe, 260, 128)->SetR(-45); 
	CreateObject(Mushroom, 271, 136);
	
	CreateObject(Rank, 146, 302)->SetR(180);
	CreateObject(Rank, 198, 190)->SetR(225);
	CreateObject(Rank, 54, 66)->SetR(180);
	CreateObject(Rank, 42, 232)->SetR(120);
	CreateObject(Rank, 269, 230)->SetR(-120);
	
	for (var i = 0; i < 2 + Random(6); i++) 
		CreateObject(Rank, 114, 10 + Random(140))->SetR(RandomX(60, 120));
	for (var i = 0; i < 2 + Random(6); i++) 
		CreateObject(Rank, 190, 10 + Random(140))->SetR(-RandomX(60, 120));

	return;
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
global func FxIntFillChestsStart(object target, proplist effect, int temporary, object chest)
{
	if (temporary)
		return 1;
	// Store weapon list and chest.		
	effect.w_list = [Dynamite, Dynamite, Firestone, Firestone, Bow, Musket, Club, Sword, Javelin, IronBomb, PowderKeg];
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
	var obj = CreateObject(obj_id);
	if (obj_id == Bow)
		obj->CreateContents([Arrow, BombArrow][Random(2)]);
	if (obj_id == Musket)
		obj->CreateContents(LeadShot);
	obj->Enter(this);
	return;
}

// GameCall from RelaunchContainer.
public func OnClonkLeftRelaunch(object clonk)
{
	clonk->SetPosition(RandomX(120, 160), -20);
	clonk->Fling(0,5);
	return;
}

public func KillsToRelaunch() { return 0; }
public func RelaunchWeaponList() { return [Bow, Shield, Sword, Firestone, Dynamite, Javelin, Musket]; }
