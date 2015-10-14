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
	//for (var plr1 in GetPlayers())
	//	for (var plr2 in GetPlayers())
	Log("%d", plr);
			
	// Initialize script players differently.	
	if (GetPlayerType(plr) == C4PT_Script)
		return InitializeScriptPlayer(plr);
	
	// Everything visible to the observer.
	SetFoW(false, plr);
	SetPlayerZoomByViewRange(plr, LandscapeWidth(), LandscapeHeight(), PLRZOOM_Direct | PLRZOOM_Set | PLRZOOM_LimitMax);
	var container = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2);
	GetCrew(plr)->Enter(container);
	
	// Add test control effect.
	var effect = AddEffect("IntTestControl", nil, 100, 2);
	effect.testnr = 1;
	effect.launched = false;
	effect.plr = plr;
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

global func CreateEnemy(id clonktype, int x,int y, int plr, array contents, int life)
{
	var enemy = CreateObjectAbove(clonktype, x, y, plr);
	if (!enemy) return nil;
	enemy->SetDir(DIR_Right);
	enemy->MakeCrewMember(plr);
	enemy->SetMaxEnergy(life);
	if (contents) for (var c in contents) enemy->CreateContents(c);
	AI->AddAI(enemy);
	enemy->AddEnergyBar();
	return enemy;
}

/*-- AI Tests --*/

global func Test1_OnStart(int plr)
{
	CreateEnemy(Clonk, 120, 258, script_enemy1, [GrenadeLauncher, IronBomb, IronBomb, IronBomb, IronBomb, IronBomb], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Sword], 50);
	// Log what the test is about.
	Log("AI battle: grenade launcher vs. sword.");
	return true;
}

global func Test1_Completed()
{
	if (ObjectCount(Find_OCF(OCF_Alive), Find_Owner(script_enemy1)) == 0 || ObjectCount(Find_OCF(OCF_Alive), Find_Owner(script_enemy2)) == 0)
		return true;
	return false;
}

global func Test1_OnFinished()
{
	RemoveAll(Find_Owner(script_enemy1));
	RemoveAll(Find_Owner(script_enemy2));
	return;
}


global func Test2_OnStart(int plr)
{
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Firestone, Firestone, Firestone, Firestone, Firestone, Firestone, Firestone, Firestone, Firestone], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Lantern, Lantern, Lantern, Lantern, Lantern, Lantern, Lantern, Lantern, Lantern], 50);
	// Log what the test is about.
	Log("AI battle: firestone vs. lantern.");
	return true;
}

global func Test2_Completed()
{
	if (ObjectCount(Find_OCF(OCF_Alive), Find_Owner(script_enemy1)) == 0 || ObjectCount(Find_OCF(OCF_Alive), Find_Owner(script_enemy2)) == 0)
		return true;
	return false;
}

global func Test2_OnFinished()
{
	RemoveAll(Find_Owner(script_enemy1));
	RemoveAll(Find_Owner(script_enemy2));
	return;
}


global func Test3_OnStart(int plr)
{
	CreateEnemy(Clonk, 120, 258, script_enemy1, [Musket, LeadShot, Sword], 50);
	CreateEnemy(Clonk, 392, 258, script_enemy2, [Sword, Bow, Arrow], 50);
	// Log what the test is about.
	Log("AI battle: musket vs. bow.");
	return true;
}

global func Test3_Completed()
{
	if (ObjectCount(Find_OCF(OCF_Alive), Find_Owner(script_enemy1)) == 0 || ObjectCount(Find_OCF(OCF_Alive), Find_Owner(script_enemy2)) == 0)
		return true;
	return false;
}

global func Test3_OnFinished()
{
	RemoveAll(Find_Owner(script_enemy1));
	RemoveAll(Find_Owner(script_enemy2));
	return;
}



/*-- Test Control --*/

// Aborts the current test and launches the specified test instead.
global func LaunchTest(int nr)
{
	// Get the control effect.
	var effect = GetEffect("IntTestControl", nil);
	if (!effect)
	{
		// Create a new control effect and launch the test.
		effect = AddEffect("IntTestControl", nil, 100, 2);
		effect.testnr = nr;
		effect.launched = false;
		effect.plr = GetPlayerByIndex(0, C4PT_User);
		return;
	}
	// Finish the currently running test.
	Call(Format("~Test%d_OnFinished", effect.testnr));
	// Start the requested test by just setting the test number and setting 
	// effect.launched to false, effect will handle the rest.
	effect.testnr = nr;
	effect.launched = false;
	return;
}

// Calling this function skips the current test, does not work if last test has been ran already.
global func SkipTest()
{
	// Get the control effect.
	var effect = GetEffect("IntTestControl", nil);
	if (!effect)
		return;
	// Finish the previous test.
	Call(Format("~Test%d_OnFinished", effect.testnr));
	// Start the next test by just increasing the test number and setting 
	// effect.launched to false, effect will handle the rest.
	effect.testnr++;
	effect.launched = false;
	return;
}


/*-- Test Effect --*/

global func FxIntTestControlStart(object target, proplist effect, int temporary)
{
	if (temporary)
		return FX_OK;
	// Set default interval.
	effect.Interval = 2;
	return FX_OK;
}

global func FxIntTestControlTimer(object target, proplist effect)
{
	// Launch new test if needed.
	if (!effect.launched)
	{
		// Log test start.
		Log("=====================================");
		Log("Test %d started:", effect.testnr);
		// Start the test if available, otherwise finish test sequence.
		if (!Call(Format("~Test%d_OnStart", effect.testnr), effect.plr))
		{
			Log("Test %d not available, the previous test was the last test.", effect.testnr);
			Log("=====================================");
			Log("All tests have been successfully completed!");
			return -1;
		}
		effect.launched = true;
	}		
	// Check whether the current test has been finished.
	if (Call(Format("Test%d_Completed", effect.testnr)))
	{
		effect.launched = false;
		//RemoveTest();
		// Call the test on finished function.
		Call(Format("~Test%d_OnFinished", effect.testnr));
		// Log result and increase test number.
		Log("Test %d successfully completed.", effect.testnr);
		effect.testnr++;
	}
	return FX_OK;
}
