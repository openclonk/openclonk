/**
	AI
	Tests for the AI system.
	
	@author Maikel
*/


static script_enemy1;
static script_enemy2;

protected func Initialize()
{
	// Script player functions as an opponent
	CreateScriptPlayer("Enemy 1", 0xffff0000, nil, CSPF_FixedAttributes | CSPF_NoEliminationCheck | CSPF_Invisible);
	CreateScriptPlayer("Enemy 2", 0xff000000, nil, CSPF_FixedAttributes | CSPF_NoEliminationCheck | CSPF_Invisible);
	return;
}


/*-- Player Initialization --*/

protected func InitializePlayer(int plr)
{
	// Make all players hostile to each other.
	for (var plr1 in GetPlayers())
		for (var plr2 in GetPlayers())
			if (plr1 != plr2)
			{
				SetHostility(plr1, plr2, true, true);			
				SetHostility(plr2, plr2, true, true);
			}
			
	// Initialize script players differently.	
	if (GetPlayerType(plr) == C4PT_Script)
		return InitializeScriptPlayer(plr);
	
	// Everything visible to the observer.
	SetFoW(false, plr);
	SetPlayerZoomByViewRange(plr, LandscapeWidth(), LandscapeHeight(), PLRZOOM_Direct | PLRZOOM_Set | PLRZOOM_LimitMax);
	var container = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2);
	GetCrew(plr)->Enter(container);
	
	// Add test control effect.
	var fx = AddEffect("IntTestControl", nil, 100, 2);
	fx.testnr = 11;
	fx.launched = false;
	fx.plr = plr;
	return;
}

protected func InitializeScriptPlayer(int plr)
{
	// Remove old crew.
	var index = 0;
	while (GetCrew(plr, index))
	{
		GetCrew(plr, index)->RemoveObject();
		index++;
	}
	
	// Store the script player number.
	if (script_enemy1 == nil)
		script_enemy1 = plr;
	else if (script_enemy2 == nil)
		script_enemy2 = plr;
	return;
}

global func CreateEnemy(id clonktype, int x, int y, int plr, array contents, int life)
{
	var enemy = CreateObjectAbove(clonktype, x, y, plr);
	if (!enemy) return nil;
	enemy->SetDir(DIR_Right);
	enemy->MakeCrewMember(plr);
	enemy->SetMaxEnergy(life * 1000);
	if (contents)
		for (var c in contents)
			enemy->CreateContents(c);
	AI->AddAI(enemy);
	AI->SetMaxAggroDistance(enemy, LandscapeWidth());
	AI->SetGuardRange(enemy, 0, 0, LandscapeWidth(), LandscapeHeight());
	enemy->AddEnergyBar();
	return enemy;
}


/*-- AI Tests --*/

global func Test1_OnStart(int plr)
{
	CreateEnemy(Clonk, 120, 258, script_enemy1, [GrenadeLauncher, IronBomb, IronBomb, IronBomb, IronBomb, IronBomb], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Sword], 50);
	// Log what the test is about.
	Log("AI battle: grenade launcher (p1) vs. sword (p2).");
	return true;
}

global func Test2_OnStart(int plr)
{
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Firestone, Firestone, Firestone, Firestone, Firestone, Firestone, Firestone, Firestone, Firestone], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Lantern, Lantern, Lantern, Lantern, Lantern, Lantern, Lantern, Lantern, Lantern], 50);
	// Log what the test is about.
	Log("AI battle: firestone (p1) vs. lantern (p2).");
	return true;
}

global func Test3_OnStart(int plr)
{
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Blunderbuss, LeadBullet, Sword], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Sword, Bow, Arrow, Shield], 50);
	// Log what the test is about.
	Log("AI battle: blunderbuss (p1) vs. bow (p2).");
	return true;
}

global func Test4_OnStart(int plr)
{
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Axe], 50);
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Axe], 50);	
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Sword], 50);
	// Log what the test is about.
	Log("AI battle: 2x axe (p1) vs. sword (p2).");
	return true;
}

global func Test5_OnStart(int plr)
{
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Bow, FireArrow], 50);
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Bow, BombArrow], 50);	
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Blunderbuss, LeadBullet, Sword], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Blunderbuss, LeadBullet, Sword], 50);
	// Log what the test is about.
	Log("AI battle: 2x bow (p1) vs. 2x blunderbuss.");
	return true;
}

global func Test6_OnStart(int plr)
{
	CreateEnemy(Clonk, 32, 208, script_enemy1, [Bow, FireArrow], 50);
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Bow, BombArrow], 50);
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Club], 50);	
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Sword], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Blunderbuss, LeadBullet, Sword], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [GrenadeLauncher, IronBomb, IronBomb, IronBomb, IronBomb, IronBomb], 50);
	CreateEnemy(Clonk, 480, 208, script_enemy2, [Javelin, Javelin, Javelin, Javelin, Javelin], 50);	
	// Log what the test is about.
	Log("AI battle: lots (p1) vs. lots (p2).");
	return true;
}

global func Test7_OnStart(int plr)
{
	CreateObject(Rule_NoFriendlyFire, LandscapeWidth() / 2, LandscapeHeight() / 2);
	CreateEnemy(Clonk, 32, 208, script_enemy1, [Bow, FireArrow], 50);
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Bow, BombArrow], 50);
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Club], 50);	
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Sword], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Blunderbuss, LeadBullet, Sword], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [GrenadeLauncher, IronBomb, IronBomb, IronBomb, IronBomb, IronBomb], 50);
	var catapulteer = CreateEnemy(Clonk, 480, 208, script_enemy2, nil, 50);
	var catapult = CreateObjectAbove(Catapult, 480, 208, script_enemy2);
	catapult->CreateContents(Firestone, 10);
	catapulteer->SetAction("Push", catapult);
	catapulteer->GetAI().vehicle = catapult;
	// Log what the test is about.
	Log("AI battle: lots (p1) vs. lots (p2).");
	return true;
}

global func Test8_OnStart(int plr)
{
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Club], 50);
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Club], 50);	
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Sword], 50);
	// Log what the test is about.
	Log("AI battle: 2x club (p1) vs. sword (p2).");
	return true;
}

global func Test9_OnStart(int plr)
{
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Blunderbuss, LeadBullet, LeadBullet], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Bread, Sproutberry, Mushroom], 50);
	// Log what the test is about.
	Log("AI battle: blunderbuss (p1) vs. bread (p2).");
	return true;
}

global func Test10_OnStart(int plr)
{
	var catapulteer = CreateEnemy(Clonk, 32, 208, script_enemy1, nil, 50);
	var catapult = CreateObjectAbove(Catapult, 32, 208, script_enemy1);
	catapult->CreateContents(Rock, 20);
	catapulteer->SetAction("Push", catapult);
	catapulteer->GetAI().vehicle = catapult;
	var clubber = CreateEnemy(Clonk, 392, 258, script_enemy2, [Club], 50);
	AI->SetAutoSearchTarget(clubber, false);
	clubber->GetAI().alert = FrameCounter();
	// Log what the test is about.
	Log("AI battle: catapult (p1) vs. club (p2).");
	return true;
}

global func Test11_OnStart(int plr)
{
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Bow, Arrow, Arrow, Arrow], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Bow, Arrow, Arrow, Arrow], 50);
	DrawMaterialQuad("Rock", 246, 200, 266, 200, 266, 400, 246, 400);
	// Log what the test is about.
	Log("AI battle: bow (p1) vs. bow (p2).");
	return true;
}

/*-- Test Control --*/

// Aborts the current test and launches the specified test instead.
global func LaunchTest(int nr)
{
	// Get the control effect.
	var fx = GetEffect("IntTestControl", nil);
	if (!fx)
	{
		// Create a new control effect and launch the test.
		fx = AddEffect("IntTestControl", nil, 100, 2);
		fx.testnr = nr;
		fx.launched = false;
		fx.plr = GetPlayerByIndex(0, C4PT_User);
		return;
	}
	// Finish the currently running test.
	ClearCurrentTest();
	// Start the requested test by just setting the test number and setting 
	// effect.launched to false, effect will handle the rest.
	fx.testnr = nr;
	fx.launched = false;
	return;
}

// Calling this function skips the current test, does not work if last test has been ran already.
global func SkipTest()
{
	// Get the control effect.
	var fx = GetEffect("IntTestControl", nil);
	if (!fx)
		return;
	// Finish the previous test.
	ClearCurrentTest();
	// Start the next test by just increasing the test number and setting 
	// effect.launched to false, effect will handle the rest.
	fx.testnr++;
	fx.launched = false;
	return;
}


/*-- Test Effect --*/

global func FxIntTestControlStart(object target, effect fx, int temporary)
{
	if (temporary)
		return FX_OK;
	// Set default interval.
	fx.Interval = 2;
	return FX_OK;
}

global func FxIntTestControlTimer(object target, effect fx)
{
	// Launch new test if needed.
	if (!fx.launched)
	{
		// Log test start.
		Log("=====================================");
		Log("Test %d started:", fx.testnr);
		// Start the test if available, otherwise finish test sequence.
		if (!Call(Format("~Test%d_OnStart", fx.testnr), fx.plr))
		{
			Log("Test %d not available, the previous test was the last test.", fx.testnr);
			Log("=====================================");
			Log("All tests have been successfully completed!");
			return FX_Execute_Kill;
		}
		fx.launched = true;
	}		
	// Check whether the current test has been finished.
	if (IsTestCompleted())
	{
		fx.launched = false;
		// Log result and increase test number.
		Log("Test %d successfully completed (p%d wins).", fx.testnr, GetTestWinner());
		// Clear current test.
		ClearCurrentTest();
		fx.testnr++;
	}
	return FX_OK;
}

global func ClearCurrentTest()
{
	RemoveAll(Find_Owner(script_enemy1));
	RemoveAll(Find_Owner(script_enemy2));
	RemoveAll(Find_ID(Rule_NoFriendlyFire));
}

global func IsTestCompleted()
{
	if (ObjectCount(Find_OCF(OCF_Alive), Find_Owner(script_enemy1)) == 0 || ObjectCount(Find_OCF(OCF_Alive), Find_Owner(script_enemy2)) == 0)
		return true;
	return false;
}

global func GetTestWinner()
{
	if (ObjectCount(Find_OCF(OCF_Alive), Find_Owner(script_enemy1)) > ObjectCount(Find_OCF(OCF_Alive), Find_Owner(script_enemy2)))
		return 1;
	return 2;
}
