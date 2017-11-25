/*-- 
	Hideout
	Author: Maikel
	
	A capture the flag scenario for two teams, both teams occupy a hideout and must steal the flag from
	the opposing team. The hideout is protected by doors and various weapons are there to fight with.
--*/


protected func Initialize()
{
	// Environment 
	PlaceGrass(185);
	
	// Goal: Capture the flag, with bases in both hideouts.
	var goal = CreateObject(Goal_CaptureTheFlag, 0, 0, NO_OWNER);
	goal->SetFlagBase(1, 120, 506);
	goal->SetFlagBase(2, LandscapeWidth() - 120, 506);
	
	// Rules
	var relaunch_rule = GetRelaunchRule();
	relaunch_rule->SetAllowPlayerRestart(true);
	relaunch_rule->SetRespawnDelay(8);
	relaunch_rule->SetLastWeaponUse(false);
	CreateObject(Rule_ObjectFade)->DoFadeTime(5 * 36);
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_Gravestones);
		
	var lwidth = LandscapeWidth();
	// Doors and spinwheels.
	var gate, wheel;
	gate = CreateObjectAbove(StoneDoor, 364, 448, NO_OWNER);
	DrawMaterialQuad("Tunnel-brickback", 360, 446, 360, 448, 368, 448, 368, 446);
	gate->DoDamage(50);		// Upper doors are easier to destroy
	gate->SetAutoControl(1);
	gate = CreateObjectAbove(StoneDoor, 340, 584, NO_OWNER);
	DrawMaterialQuad("Tunnel-brickback", 336, 582, 336, 584, 344, 584, 344, 582);
	gate->SetAutoControl(1);
	gate = CreateObjectAbove(StoneDoor, 692, 544, NO_OWNER);
	DrawMaterialQuad("Tunnel-brickback", 688, 542, 688, 544, 696, 544, 696, 542);
	gate->DoDamage(80);		// Middle doors even easier
	wheel = CreateObjectAbove(SpinWheel, 660, 552, NO_OWNER);
	wheel->SetSwitchTarget(gate);
	
	gate = CreateObjectAbove(StoneDoor, lwidth - 364, 448, NO_OWNER);
	DrawMaterialQuad("Tunnel-brickback", lwidth - 361, 446, lwidth - 361, 448, lwidth - 365, 448, lwidth - 365, 446);
	gate->DoDamage(50);		// Upper doors are easier to destroy
	gate->SetAutoControl(2);
	gate = CreateObjectAbove(StoneDoor, lwidth - 340, 584, NO_OWNER);
	DrawMaterialQuad("Tunnel-brickback", lwidth - 337, 582, lwidth - 337, 584, lwidth - 345, 584, lwidth - 345, 582);
	gate->SetAutoControl(2);
	gate = CreateObjectAbove(StoneDoor, lwidth - 692, 544, NO_OWNER);
	DrawMaterialQuad("Tunnel-brickback", lwidth - 689, 542, lwidth - 689, 544, lwidth - 697, 544, lwidth - 697, 542);
	gate->DoDamage(80);		// Middle doors even easier
	wheel = CreateObjectAbove(SpinWheel, lwidth - 660, 552, NO_OWNER);
	wheel->SetSwitchTarget(gate);
	
	// Chests with weapons.
	var chest;
	chest = CreateObjectAbove(Chest, 110, 592, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillBaseChest", chest, 100, 6 * 36,nil,nil,true);
	chest = CreateObjectAbove(Chest, 25, 464, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillBaseChest", chest, 100, 6 * 36,nil,nil,false);
	chest = CreateObjectAbove(Chest, 730, 408, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillOtherChest", chest, 100, 6 * 36);
	chest = CreateObjectAbove(Chest, lwidth - 110, 592, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillBaseChest", chest, 100, 6 * 36,nil,nil,true);
	chest = CreateObjectAbove(Chest, lwidth - 25, 464, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillBaseChest", chest, 100, 6 * 36,nil,nil,false);
	chest = CreateObjectAbove(Chest, lwidth - 730, 408, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillOtherChest", chest, 100, 6 * 36);
	chest = CreateObjectAbove(Chest, lwidth/2, 512, NO_OWNER);
	chest->MakeInvincible();
	AddEffect("FillSpecialChest", chest, 100, 4 * 36);
	
	/*
	// No Cannons
	// Cannons loaded with 12 shots.
	var cannon;
	cannon = CreateObjectAbove(Cannon, 429, 444, NO_OWNER);
	cannon->SetDir(DIR_Right);
	cannon->SetR(15);
	cannon->CreateContents(PowderKeg);
	cannon = CreateObjectAbove(Cannon, lwidth - 429, 444, NO_OWNER);
	cannon->SetDir(DIR_Left);
	cannon->SetR(-15);
	cannon->CreateContents(PowderKeg);
	*/

	return;
}

protected func InitializePlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 600, nil, PLRZOOM_Direct);
	return;
}

// Game call from RelaunchContainer when a Clonk has left the respawn.
public func OnClonkLeftRelaunch(object clonk)
{
	// Randomize the Clonk's position a bit.
	var x = clonk->GetX();
	var y = clonk->GetY();
	clonk->SetPosition(x + RandomX(-20, 20), y + RandomX(-20, 10));
	if (clonk->Stuck()) clonk->SetPosition(x, y);
	
	// Some small spark-like particles that just fly around.
	var sparks =
	{
		Prototype = Particles_Glimmer(),
		Size = PV_Random(0, 1),
		R = 200, G = 0, B = 200,
		ForceX = PV_Random(-5, 5, 10),
		ForceY = PV_Random(-5, 5, 10),
		Attach = ATTACH_Front
	};
	
	if (GetPlayerTeam(clonk->GetOwner()) == 1)
	{
		sparks.R = 255;
		sparks.B = 0;
	}
	clonk->CreateParticle("Flash", 0, 0, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(10, 400), sparks, 100);
	
	// And a larger flash as if the Clonk had just been teleported there.
	var flash =
	{
		Prototype = Particles_Flash(),
		Size = 10,
		Stretch = PV_Linear(40000, 0),
		R = sparks.R, G = sparks.G, B = sparks.B
	};
	clonk->CreateParticle("StarSpark", 0, 0, 0, 0, 10, flash, 1);
}

func RelaunchWeaponList() { return [Bow, Shield, Sword, Javelin, Shovel, Firestone, Dynamite, Loam]; }

/*-- Chest filler effects --*/

global func FxFillBaseChestStart(object target, effect, int temporary, bool supply)
{
	if (temporary) 
		return 1;
		
	effect.supply_type=supply;
	if(effect.supply_type) 
		var w_list = [Firestone, Dynamite, Ropeladder, ShieldGem];
	else
		var w_list = [Bow, Sword, Javelin, PyreGem];
	for(var i=0; i<4; i++)
		target->CreateChestContents(w_list[i]);
	return 1;
}
global func FxFillBaseChestTimer(object target, effect)
{
	if(effect.supply_type)
	{ 
		var w_list = [Firestone, Dynamite, Shovel, Loam, Ropeladder, SlowGem, ShieldGem];
		var maxcount = [2,2,1,2,1,2,1];
	}
	else
	{
		var w_list = [Sword, Javelin, Blunderbuss, ShieldGem, PyreGem];
		var maxcount = [1,2,1,1,1];
	}
	
	var contents;
	for(var i=0; i<target->GetLength(w_list); i++)
		contents+=target->ContentsCount(w_list[i]);
	if(contents > 5) return 1;
	
	for(var i=0; i<2 ; i++)
	{
		var r = Random(GetLength(w_list));
		if (target->ContentsCount(w_list[r]) < maxcount[r])
		{
			target->CreateChestContents(w_list[r]);
			i=3;
		}
	}
	return 1;
}

global func FxFillOtherChestStart(object target, effect, int temporary)
{
	if (temporary) 
		return 1;
	var w_list = [Shield, Sword, Javelin, Club, Firestone, Dynamite];
	if (target->ContentsCount() < 5)
		target->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxFillOtherChestTimer(object target)
{
	var w_list = [Shield ,Sword, Club, Bow, Dynamite, Firestone, SlowGem, ShieldGem, PyreGem];
	var maxcount = [2,1,1,1,2,2,1,2,2];

	
	var contents;
	for(var i=0; i<target->GetLength(w_list); i++)
		contents+=target->ContentsCount(w_list[i]);
	if(contents > 6) return 1;
	
	for(var i=0; i<2 ; i++)
	{
		var r = Random(GetLength(w_list));
		if (target->ContentsCount(w_list[r]) < maxcount[r])
		{
			target->CreateChestContents(w_list[r]);
			i=3;
		}
	}
	return 1;
}

global func FxFillSpecialChestTimer(object target)
{
	if (Random(2)) return 1;
	
	var w_list = [PyreGem, ShieldGem, SlowGem];
	var r=Random(3);
	if (target->ContentsCount() < 4)
		target->CreateChestContents(w_list[r]);
	
	var w_list = [GrappleBow, DynamiteBox, Boompack];
	var r=Random(3);
	for(var i=0; i < GetLength(w_list); i++)
		if (FindObject(Find_ID(w_list[i]))) return 1;
	target->CreateChestContents(w_list[r]);
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
	if (obj_id == GrappleBow)
		AddEffect("NotTooLong",obj,100,36);
	
	obj->Enter(this);
	
	return;
}

protected func CaptureFlagCount() { return 2;} //4 + GetPlayerCount()) / 2; }

global func FxNotTooLongTimer(object target, effect)
{	if (!(target->Contained())) return 1;
	if (target->Contained()->GetID() == Clonk) effect.inClonk_time++;
	if (effect.inClonk_time > 40) return target->RemoveObject();
	else if (effect.inClonk_time > 35) target->Message("@<c ff%x%x>%d",(41-effect.inClonk_time)*50,(41-effect.inClonk_time)*50,41-effect.inClonk_time);
}

func OnClonkDeath(object clonk, int killed_by)
{
	// create a magic healing gem on Clonk death
	if(Hostile(clonk->GetOwner(), killed_by))
	{
		var gem=clonk->CreateObjectAbove(LifeGem, 0, 0, killed_by);
		if(GetPlayerTeam(killed_by) == 1)
			gem->SetGraphics("E");
	}
	return _inherited(clonk, killed_by);
}
