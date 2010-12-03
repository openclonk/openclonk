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
	goal->SetFlagBase(1, 120, 500);
	goal->SetFlagBase(2, LandscapeWidth() - 120, 500);
	
	// Rules
	CreateObject(Rule_Restart);
	CreateObject(Rule_ObjectFade)->DoFadeTime(5 * 36);
	
	// Doors and spinwheels.
	var gate, wheel;
	gate = CreateObject(StoneDoor, 366, 420, NO_OWNER);
	gate->DoDamage(50);		//Upper doors are easier to destroy
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 1);
	//wheel = CreateObject(SpinWheel, 320, 460, NO_OWNER);
	//wheel->SetStoneDoor(gate);
	gate = CreateObject(StoneDoor, 346, 550, NO_OWNER);
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 1);
	//wheel = CreateObject(SpinWheel, 280, 580, NO_OWNER);
	//wheel->SetStoneDoor(gate);
	gate = CreateObject(StoneDoor, 846, 480, NO_OWNER);
	gate->DoDamage(80);		//Middle dors even easier
	wheel = CreateObject(SpinWheel, 780, 480, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate = CreateObject(StoneDoor, LandscapeWidth() - 364, 420, NO_OWNER);
	gate->DoDamage(50);		//Upper doors are easier to destroy
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 2);
	//wheel = CreateObject(SpinWheel, LandscapeWidth() - 320, 460, NO_OWNER);
	//wheel->SetStoneDoor(gate);
	gate = CreateObject(StoneDoor, LandscapeWidth() - 344, 550, NO_OWNER);
	AddEffect("AutoControl", gate, 100, 3, gate, nil, 2);
	//wheel = CreateObject(SpinWheel, LandscapeWidth() - 280, 580, NO_OWNER);
	//wheel->SetStoneDoor(gate);
	gate = CreateObject(StoneDoor, LandscapeWidth() - 844, 480, NO_OWNER);
	gate->DoDamage(80);		//Middle dors even easier
	wheel = CreateObject(SpinWheel, LandscapeWidth() - 780, 480, NO_OWNER);
	wheel->SetStoneDoor(gate);
	
	// Chests with weapons.
	var chest;
	chest = CreateObject(Chest, 110, 590, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 2 * 36,nil,nil,true);
	chest = CreateObject(Chest, 25, 460, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 2 * 36,nil,nil,false);
	chest = CreateObject(Chest, 810, 600, NO_OWNER);
	AddEffect("FillOtherChest", chest, 100, 2 * 36);
	chest = CreateObject(Chest, 860, 350, NO_OWNER);
	AddEffect("FillOtherChest", chest, 100, 2 * 36);
	chest = CreateObject(Chest, LandscapeWidth() - 110, 590, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 2 * 36,nil,nil,true);
	chest = CreateObject(Chest, LandscapeWidth() - 25, 460, NO_OWNER);
	AddEffect("FillBaseChest", chest, 100, 2 * 36,nil,nil,false);
	chest = CreateObject(Chest, LandscapeWidth() - 810, 600, NO_OWNER);
	AddEffect("FillOtherChest", chest, 100, 2 * 36);
	chest = CreateObject(Chest, LandscapeWidth() - 860, 350, NO_OWNER);
	AddEffect("FillOtherChest", chest, 100, 2 * 36);
	
	chest = CreateObject(Chest, LandscapeWidth()/2, 0, NO_OWNER);
	AddEffect("FillSpecialChest", chest, 100, 4 * 36);
	
	// Cannons loaded with 12 shots.
	var cannon;
	cannon = CreateObject(Cannon, 429, 444, NO_OWNER);
	cannon->SetDir(DIR_Right);
	cannon->SetR(15);
	cannon->CreateContents(PowderKeg);
	cannon = CreateObject(Cannon, 1772, 444, NO_OWNER);
	cannon->SetDir(DIR_Left);
	cannon->SetR(-15);
	cannon->CreateContents(PowderKeg);
	
	// Brick edges, notice the symmetric landscape.
	PlaceEdges();

	return;
}




global func PlaceEdges()
{
	var x=[695, 655, 385, 345, 255, 275, 295, 105, 95, 45, 185, 155, 145, 825, 815, 805, 755, 765, 775, 785, 625, 615, 395, 335, 265, 245, 225, 215, 105, 95, 75];
	var y=[515, 545, 385, 405, 485, 475, 465, 365, 375, 465, 545, 575, 585, 355, 365, 375, 405, 415, 425, 435, 555, 565, 445, 455, 575, 565, 555, 545, 495, 485, 475];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i] + 5, NO_OWNER);
		edge->Initialize();
		edge->PermaEdge();
		var edge=CreateObject(BrickEdge, LandscapeWidth()-x[i]+5, y[i] + 5, NO_OWNER);
		edge->Initialize();
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

global func FxFillBaseChestStart(object target, int num, int temporary, bool supply)
{
	if (temporary) 
		return 1;
		
	EffectVar(0, target, num)=supply;
	if(EffectVar(0, target, num)) 
		var w_list = [Firestone, Dynamite, Ropeladder, ShieldGem];
	else
		var w_list = [Bow, Sword, Javelin, PyreGem];
	for(var i=0; i<4; i++)
		target->CreateChestContents(w_list[i]);
	return 1;
}
global func FxFillBaseChestTimer(object target, int num)
{
	if(EffectVar(0, target, num))
	{ 
		var w_list = [Firestone, Dynamite, Shovel, Loam, Ropeladder, SlowGem, ShieldGem];
		var maxcount = [2,2,1,2,1,2,1];
	}
	else
	{
		var w_list = [ Sword, Javelin, Musket, ShieldGem, PyreGem];
		var maxcount = [1,2,1,1,2];
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

global func FxFillOtherChestStart(object target, int num, int temporary)
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

global func FxNotTooLongTimer(object target, int num)
{	if (!(target->Contained())) return 1;
	if (target->Contained()->GetID() == Clonk) EffectVar(0, target, num)++;
	if (EffectVar(0, target, num) > 40) return target->RemoveObject();
	else if (EffectVar(0, target, num) > 35) target->Message("@<c ff%x%x>%d",(41-EffectVar(0, target, num))*50,(41-EffectVar(0, target, num))*50,41-EffectVar(0, target, num));
}

func OnClonkDeath(object clonk, int killed_by)
{
	// create a magic healing gem on Clonk death
	clonk->CreateObject(LifeGem, 0, 0, killed_by);
	return _inherited(clonk, killed_by);
}