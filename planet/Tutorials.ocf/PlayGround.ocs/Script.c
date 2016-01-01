/** 
	Playground
	Author: Maikel
	
	Reward for completing the tutorials, a small playground where the player can test all ingame objects.
	
	TODO:
	* Multiple map options.
	* Options for animals, vegetation. 
*/


protected func Initialize()
{
	// Tutorial goal.
	var goal = CreateObject(Goal_Tutorial);
	goal.Name = "$MsgGoalName$";
	goal.Description = "$MsgGoalDescription$";
	
	// Add an effect to track the easter egg for all players.
	AddEffect("EasterEgg", nil, 100, 36);
	
	// Initialize different scenario parts.
	InitVegetation(3);
	InitAnimals(SCENPAR_Animals);
	return;
}

private func InitVegetation(int type)
{
	// No vegetation.
	if (type <= 1)
		return;
	
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();

	// Place surface vegetation.
	Tree_Coniferous->Place(10);
	Tree_Coniferous2->Place(10);
	return;
}

private func InitAnimals(int type)
{
	// No animals.
	if (type <= 1)
		return;
	
	var wdt = LandscapeWidth();
	var hgt = LandscapeHeight();
		
	// Sky animals.
	if (FindLocation(Loc_Sky()))
	{
		var amount = BoundBy(wdt * hgt - GetTotalMaterialCount() / 20000, 4, 40);
		Butterfly->Place(amount);
		Mosquito->Place(amount / 2, 10);
		if (type >= 3)
		{
			Zaphive->Place(amount / 2);
		}
	}
	
	// Land animals.
	if (FindLocation(Loc_Sky(), Loc_Wall(CNAT_Bottom)))
	{
		var amount = BoundBy(wdt / 200, 4, 40);
		if (type >= 3)
		{
			Chippie_Egg->Place(amount / 2);
		}
	}

	// Underwater animals.
	if (FindLocation(Loc_Material("Water")))
	{
		var amount = BoundBy(GetMaterialCount(Material("Water")) / 1500, 4, 40);
		Fish->Place(amount);	
		if (type >= 3)
		{
			Piranha->Place(amount / 2);		
		}	
	}
	return;
}


/*-- Player Handling --*/

protected func InitializePlayer(int plr)
{
	// Position player's clonk.
	var clonk = GetCrew(plr, 0);
	clonk->SetPosition(624, 526);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	effect.to_x = 624;
	effect.to_y = 526;
	
	// Standard player zoom for tutorials, set max zoom roughly up to scenario boundaries.
	SetPlayerViewLock(plr, true);
	SetPlayerZoomByViewRange(plr, 400, nil, PLRZOOM_Direct);
	SetPlayerZoomByViewRange(plr, 1200, nil, PLRZOOM_LimitMax);
	
	// Give the player all construction plans.
	var index = 0, def;
	while (def = GetDefinition(index++))
		SetPlrKnowledge(plr, def);

	// Create tutorial guide, add messages, show first.
	var guide = CreateObject(TutorialGuide, 0, 0 , plr);
	guide->AddGuideMessage("$MsgPlaygroundWelcome$");
	guide->ShowGuideMessage();
	AddEffect("RemoveGuide", clonk, 100, 36 * 10);
	
	// Achievement: Playground is completed by just playing it.
	GainScenarioAchievement("TutorialCompleted", 3);
	return;
}

global func FxRemoveGuideTimer(object target, proplist effect, int time)
{
	return -1;
}

global func FxRemoveGuideStop(object target, proplist effect, int reason, bool  temporary)
{
	if (temporary)
		return;
	
	var guide = FindObject(Find_ID(TutorialGuide));
	if (guide)
	{
		guide->HideGuide();
		guide->RemoveObject();	
	}
}


/*-- Easter Egg --*/

global func FxEasterEggTimer(object target, proplist effect, int time)
{
	// Check whether the landscape has been completely destroyed.	
	if (GetBlastableMaterialCount() < 1000)
	{
		GainScenarioAchievement("TutorialEasterEgg");
		return FX_Execute_Kill;
	}
	return FX_OK;
}

global func GetBlastableMaterialCount()
{
	var total = 0;
	for (var index = 0; index < 256; index++)
	{
		// Only for solids.
		if (GetMaterialVal("Density", "Material", index) < 50)
			continue;
		if (!GetMaterialVal("BlastFree", "Material", index) || !GetMaterialVal("DigFree", "Material", index))
			continue;
		var mat_cnt = GetMaterialCount(index);
		if (mat_cnt != -1)
			total += mat_cnt;
	}
	return total;
}

global func GetTotalMaterialCount()
{	var total = 0;
	for (var index = 0; index < 256; index++)
	{
		var mat_cnt = GetMaterialCount(index);
		if (mat_cnt != -1)
			total += mat_cnt;
	}
	return total;
}


/*-- Clonk Restoring --*/

global func FxClonkRestoreTimer(object target, proplist effect, int time)
{
	return 1;
}

// Relaunches the clonk, from death or removal.
global func FxClonkRestoreStop(object target, effect, int reason, bool  temporary)
{
	if (reason == 3 || reason == 4)
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = effect.to_x;
		var to_y = effect.to_y;
		// Respawn new clonk.
		var plr = target->GetOwner();
		var clonk = CreateObject(Clonk, 0, 0, plr);
		clonk->GrabObjectInfo(target);
		SetCursor(plr, clonk);
		clonk->DoEnergy(100000);
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, 0, "ClonkRestore");
	}
	return 1;
}