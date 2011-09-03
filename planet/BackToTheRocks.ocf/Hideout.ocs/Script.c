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
	goal->SetFlagBase(1, 120, 504);
	goal->SetFlagBase(2, LandscapeWidth() - 120, 504);
	
	// Rules
	CreateObject(Rule_Restart);
	CreateObject(Rule_ObjectFade)->DoFadeTime(5 * 36);
	CreateObject(Rule_KillLogs);
	
	// Doors and spinwheels.
	var gate, wheel;
	gate = CreateObject(StoneDoor, 365, 449, NO_OWNER);
	gate->DoDamage(50);		// Upper doors are easier to destroy
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 1);
	gate = CreateObject(StoneDoor, 341, 585, NO_OWNER);
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 1);
	gate = CreateObject(StoneDoor, 693, 544, NO_OWNER);
	gate->DoDamage(80);		// Middle doors even easier
	wheel = CreateObject(SpinWheel, 660, 552, NO_OWNER);
	wheel->SetStoneDoor(gate);
	
	gate = CreateObject(StoneDoor, LandscapeWidth() - 364, 449, NO_OWNER);
	gate->DoDamage(50);		// Upper doors are easier to destroy
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 2);
	gate = CreateObject(StoneDoor, LandscapeWidth() - 340, 585, NO_OWNER);
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 2);
	gate = CreateObject(StoneDoor, LandscapeWidth() - 692, 544, NO_OWNER);
	gate->DoDamage(80);		// Middle doors even easier
	wheel = CreateObject(SpinWheel, LandscapeWidth() - 660, 552, NO_OWNER);
	wheel->SetStoneDoor(gate);
	
	// Chests with weapons.
	var chest;
	chest = CreateObject(Chest, 110, 592, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 6 * 36,nil,nil,true);
	chest = CreateObject(Chest, 25, 464, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 6 * 36,nil,nil,false);
	chest = CreateObject(Chest, 730, 408, NO_OWNER);
	AddEffect("FillOtherChest", chest, 100, 6 * 36);
	chest = CreateObject(Chest, LandscapeWidth() - 110, 592, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 6 * 36,nil,nil,true);
	chest = CreateObject(Chest, LandscapeWidth() - 25, 464, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 6 * 36,nil,nil,false);
	chest = CreateObject(Chest, LandscapeWidth() - 730, 408, NO_OWNER);
	AddEffect("FillOtherChest", chest, 100, 6 * 36);
	
	chest = CreateObject(Chest, LandscapeWidth()/2, 512, NO_OWNER);
	AddEffect("FillSpecialChest", chest, 100, 4 * 36);
	
	/*
	// No Cannons
	// Cannons loaded with 12 shots.
	var cannon;
	cannon = CreateObject(Cannon, 429, 444, NO_OWNER);
	cannon->SetDir(DIR_Right);
	cannon->SetR(15);
	cannon->CreateContents(PowderKeg);
	cannon = CreateObject(Cannon, LandscapeWidth() - 429, 444, NO_OWNER);
	cannon->SetDir(DIR_Left);
	cannon->SetR(-15);
	cannon->CreateContents(PowderKeg);
	*/
	// Brick edges, notice the symmetric landscape.
	PlaceEdges();

	return;
}

global func PlaceEdges()
{
	var x=[613, 565, 596, 676, 637, 381, 252, 268, 284, 109, 102, 45, 188, 149, 396, 332, 261, 245, 229, 213, 101, 93, 85, 316, 629, 621, 652, 668, 676, 556, 516, 684, 556, 613, 660, 94, 109, 277, 324];
	var y=[468, 412, 412, 493, 492, 389, 484, 476, 468, 365, 372, 468, 548, 588, 444, 452, 572, 564, 556, 548, 492, 484, 476, 549, 484, 476, 436, 420, 412, 556, 564, 548, 429, 429, 428, 380, 500, 580, 460];
	var d=[0, 0, 1, 3, 0, 2, 1, 1, 1, 2, 2, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 1, 1, 1, 1, 1, 1, 3, 2, 1, 2, 0, 0, 1];
	var o=[1,0,3,2];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i], NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]);
		edge->PermaEdge();
		
		var edge=CreateObject(BrickEdge, x[i], y[i], NO_OWNER);
		edge->Initialize();
		edge->SetP(o[d[i]]);
		edge->SetPosition(LandscapeWidth()-x[i],y[i]);
		edge->PermaEdge();
	}
	return 1;
}
protected func InitializePlayer(int plr)
{
	SetPlayerZoomByViewRange(plr, 600, nil, PLRZOOM_Direct);
	return;
}

// Gamecall from CTF goal, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	var relaunch = CreateObject(RelaunchContainer, clonk->GetX(), clonk->GetY(), clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	relaunch->SetRelaunchTime(8, true);
	return;
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
		var w_list = [Sword, Javelin, Musket, ShieldGem, PyreGem];
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
	
	var clr = RGB(0,255,0);
	if (r == 1) clr = RGB(255,0,0);
	if (r == 2) clr = RGB(255,128,0);
	CastParticles("AnouncingFire",75,60,target->GetX(),target->GetY(),100,150,clr);
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
	if (obj_id == GrappleBow)
		AddEffect("NotTooLong",obj,100,36);
	
	obj->Enter(this);
	
	return;
}

protected func CaptureFlagCount() { return (4 + GetPlayerCount()) / 2; }

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
		var gem=clonk->CreateObject(LifeGem, 0, 0, killed_by);
		if(GetPlayerTeam(killed_by) == 1)
			gem->SetGraphics("E");
	}
	return _inherited(clonk, killed_by);
}