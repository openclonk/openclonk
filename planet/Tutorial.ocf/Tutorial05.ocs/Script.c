/*-- 
	Tutorial 05
	Author: Caesar
	
	Basic settlement tutorial: Foundry, Flagpole, Windmill
--*/

static guide; // guide object

protected func Initialize()
{
	// Environment.
	PlaceGrass(85);
	
	return;
}

protected func InitializePlayer(int plr)
{
	var clonk = GetCrew(plr, 0);
	clonk->SetPosition(300, -150);
	clonk->Fling(0, 0);
	clonk->SetDir(DIR_Right);
	clonk->CreateContents(Shovel);
	clonk->CreateContents(Hammer);
	clonk->CreateContents(Axe);
	var effect = AddEffect("ClonkRestore", clonk, 100, 10);
	effect.var1 = 300;
	effect.var2 = -10;
	
	// Create tutorial guide, add messages, show first.
	guide = CreateTutorialGuide(plr);
	guide->AddGuideMessage("@$MsgTutIntro0$");
	guide->ShowGuideMessage(0);
	AddEffect("TutorialIntro1", nil, 1, 36 * 5);
	AddEffect("TutorialFoundrySite", nil, 1, 18);
	AddEffect("TutorialFoundryMaterialUse", nil, 1, 18);
	return;
}

/*-- Guide Messages --*/
// Finds when the Clonk has done 'X', and changes the message.

global func FxTutorialIntro1Stop()
{
	guide->AddGuideMessage("@$MsgTutIntro1$");
	guide->ShowGuideMessage(1);
	AddEffect("TutorialIntro2", nil, 1, 36 * 6);
	return 0;
}

global func FxTutorialIntro2Stop()
{
	guide->AddGuideMessage("@$MsgTutIntro2$");
	guide->ShowGuideMessage(2);
	AddEffect("TutorialIntro3", nil, 1, 36 * 4);
	return 0;
}

global func FxTutorialIntro3Stop()
{
	guide->AddGuideMessage("$MsgTutIntro3$");
	guide->ShowGuideMessage(3);
	guide->AddGuideMessage("$MsgTutFoundrySite$");
	return 1;
}

global func FxTutorialFoundrySiteTimer(object target, effect, int timer)
{
	if (FindObject(Find_ID(ConstructionSite)))
		return -1;
}

global func FxTutorialFoundrySiteStop(object target, effect, int timer)
{	
	while (GetEffect("TutorialIntro*"))
		RemoveEffect("TutorialIntro*");
	guide->ClearGuideMessage();
	guide->AddGuideMessage("$MsgTutFoundryChopTree$");
	AddEffect("TutorialFoundryChopTree", nil, 1, 18);
	return 0;
}	

global func FxTutorialFoundryChopTreeTimer(object target, effect, int timer)
{
	if (FindObject(Find_ID(Tree_Coniferous), Find_Not(Find_Category(C4D_StaticBack))))
		return -1;
}

global func FxTutorialFoundryChopTreeStop(object target, effect, int timer)
{
	guide->AddGuideMessage("$MsgTutFoundryHackTree$");
	AddEffect("TutorialFoundryHackTree", nil, 1, 18);
}

global func FxTutorialFoundryHackTreeTimer(object target, effect, int timer)
{
	if (ObjectCount(Find_ID(Wood)) >= 2)
		return -1;
}

global func FxTutorialFoundryHackTreeStop(object target, effect, int timer)
{
	guide->AddGuideMessage("$MsgTutFoundryStoneBlast$");
	AddEffect("TutorialFoundryStoneBlast", nil, 1, 18);
}

global func FxTutorialFoundryStoneBlastTimer(object target, effect, int timer)
{
	if (ObjectCount(Find_ID(Rock)) >= 4)
		return -1;
}

global func FxTutorialFoundryStoneBlastStop(object target, effect, int timer)
{
	guide->AddGuideMessage("$MsgTutFoundryMaterialUse$");
	AddEffect("TutorialFoundryMaterialUse", nil, 1, 18);
}

global func FxTutorialFoundryMaterialUseTimer(object target, effect, int timer)
{
	if (FindObject(Find_ID(Foundry)))
		return -1;
}

global func FxTutorialFoundryMaterialUseStop(object target, effect, int timer)
{
	guide->AddGuideMessage("$MsgTutFlagpoleSite$");
	for(var i=0; i<GetPlayerCount(); ++i) // probably overkill to loop
		SetPlrKnowledge(GetPlayerByIndex(i), Flagpole);
	RemoveEffect("TutorialFoundrySite");
	RemoveEffect("TutorialFoundryChopTree");
	RemoveEffect("TutorialFoundryHackTree");
	RemoveEffect("TutorialFoundryStoneBlast");
	AddEffect("TutorialFlagpoleSite", nil, 1, 18);
	AddEffect("TutorialFlagpoleBuild", nil, 1, 18);
}

global func FxTutorialFlagpoleSiteTimer(object target, effect, int timer)
{
	for(var site in FindObjects(Find_ID(ConstructionSite)))
		if (site.definition == Flagpole)
			return -1;
}

global func FxTutorialFlagpoleSiteStop(object tg, e, int tm)
{
	guide->AddGuideMessage("$MsgTutFlagpoleIronComponents$");
	AddEffect("TutorialFlagpoleIronComponents", nil, 1, 18);
}

global func FxTutorialFlagpoleIronComponentsTimer(object target, effect, int timer)
{
	if (ObjectCount(Find_ID(Coal)) >= 1 && ObjectCount(Find_ID(Ore)))
		return -1;
}

global func FxTutorialFlagpoleIronComponentsStop(object target, effect, int timer)
{
	guide->AddGuideMessage("$MsgTutFlagpoleIronSmelt$");
	AddEffect("TutorialFlagpoleIronSmelt", nil, 1, 18);
}

global func FxTutorialFlagpoleIronSmeltTimer(object target, effect, int timer)
{
	if (ObjectCount(Find_ID(Metal)) >= 1)
		return -1;
}

global func FxTutorialFlagpoleIronSmeltStop(object target, effect, int timer)
{
	guide->AddGuideMessage("$MsgTutFlagpoleIronPraise$");
	AddEffect("TutorialFlagpoleBuild", nil, 1, 18);
}

global func FxTutorialFlagpoleBuildTimer(object target, effect, int timer)
{
	if (FindObject(Find_ID(Flagpole)))
		return -1;
}

global func FxTutorialFlagpoleBuildStop(object target, effect, int timer)
{
	RemoveEffect("TutorialFlagpoleSite");
	RemoveEffect("TutorialFlagpoleIronComponents");
	RemoveEffect("TutorialFlagpoleIronSmelt");
	for(var i=0; i<GetPlayerCount(); ++i) // probably overkill
		SetPlrKnowledge(GetPlayerByIndex(i), WindGenerator);
	guide->AddGuideMessage("$MsgTutEnergy$");
	AddEffect("TutorialEnergy", nil, 1, 18);
}


global func FxTutorialEnergyTimer(object tg, e, tm) {
	for (var wg in FindObjects(Find_ID(Flagpole)))
		if (wg.lflag && wg.lflag.power_helper && (wg.lflag.power_helper.power_balance > 0))
			return -1;
}

global func FxTutorialEnergyStop(object tg, e, tm) {
	for(var building in [SteamEngine, ToolsWorkshop, Sawmill, Elevator, Pump, Compensator, Windmill, WoodenCabin])
		for(var i = GetPlayerCount(); i--;)
			SetPlrKnowledge(GetPlayerByIndex(i), building);
	guide->ClearGuideMessage();
	GameOver();
}


protected func OnGuideMessageShown(int plr, int index)
{
	// Show where the guide is located in the HUD.
	if (index == 3)	
		TutArrowShowGUIPos(guide->GetX(), guide->GetY() / 2, 0, 10+guide->GetDefHeight());
	// Show the tools
	if (index == 2) //show shovel
		TutArrowShowTarget(FindObject(Find_ID(Shovel)));
	if (index == 2 || index == 4) // show hammer
		TutArrowShowTarget(FindObject(Find_ID(Hammer)));
	if (index == 2 || index == 5) // show axe
		TutArrowShowTarget(FindObject(Find_ID(Axe)));
	// Show a tree, a chopped one if possible
	if (index == 6)
	{
		var tree = FindObject(Find_ID(Tree_Coniferous), Find_Not(Find_Category(C4D_StaticBack)));
		if (!tree) tree = FindObject(Find_ID(Tree_Coniferous), Sort_Distance(GetCursor(plr)->GetX(),GetCursor(plr)->GetY()));
		if (tree) TutArrowShowTarget(tree, 225, 25);
	}	
	// Show foundry
	if (index == 14)
		TutArrowShowTarget(FindObject(Find_ID(Foundry)), 225, 30);
	// Show stone vein and flint close to it
	if (index == 7)
	{
		TutArrowShowPos(200, 430, 225);
		TutArrowShowTarget(FindObject(Find_ID(Firestone), Sort_Distance(200,350/*y arbitrary*/)), 0);
	}
	// Show ore/coal vein and flint close to it
	if (index == 12)
	{
		TutArrowShowPos(540, 450, 135);
		TutArrowShowTarget(FindObject(Find_ID(Firestone), Sort_Distance(540,350/*y arbitrary*/)), 0);
	}
	// Show crafted metal bar
	if (index == 16 && FindObject(Find_ID(Metal)))
		TutArrowShowTarget(FindObject(Find_ID(Metal)), 225);
}

protected func OnGuideMessageRemoved(int plr, int index)
{
	TutArrowClear();
	return;
}

/*-- Clonk restoring --*/

// Relaunches the clonk, from death or removal.
global func FxClonkRestoreStop(object target, effect, int reason, bool  temporary)
{
	if (reason == 3 || reason == 4)
	{
		var restorer = CreateObject(ObjectRestorer, 0, 0, NO_OWNER);
		var x = BoundBy(target->GetX(), 0, LandscapeWidth());
		var y = BoundBy(target->GetY(), 0, LandscapeHeight());
		restorer->SetPosition(x, y);
		var to_x = effect.var1;
		var to_y = effect.var2;
		// Respawn new clonk.
		var plr = target->GetOwner();
		var clonk = CreateObject(Clonk, 0, 0, plr);
		clonk->GrabObjectInfo(target);
		SetCursor(plr, clonk);
		clonk->DoEnergy(100000);
		restorer->SetRestoreObject(clonk, nil, to_x, to_y, "ClonkRestore");
	}
	return 1;
}
