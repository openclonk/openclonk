/**
	Explode Container
	Unit tests for the explosions and containers. Invokes tests by
	calling the global function Test*_OnStart(int plr) and iterate 
	through all tests. The test is completed once Test*_Completed()
	returns	true. Then Test*_OnFinished() is called, to be able to 
	reset the scenario for the next test.
	
	@author Maikel
*/


protected func Initialize()
{

	return;
}

protected func InitializePlayer(int plr)
{
	// Set zoom and move player to the middle of the scenario.
	SetPlayerZoomByViewRange(plr, LandscapeWidth(), nil, PLRZOOM_Direct);
	SetFoW(false, plr);
	GetCrew(plr)->SetPosition(120, 190);
	GetCrew(plr)->MakeInvincible();
	
	// Add test control effect.
	var effect = AddEffect("IntTestControl", nil, 100, 2);
	effect.testnr = 1;
	effect.launched = false;
	effect.plr = plr;
	return true;
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


/*-- Tests --*/

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
			Log("Test %d not available, this was the last test.", effect.testnr);
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

global func Test1_OnStart(int plr)
{
	var lorry1 = CreateObject(Lorry, 200, 200);
	CreateObject(Wood, 200, 206);
	var lorry2 = CreateObject(Lorry, 300, 200);
	CreateObject(Wood, 300, 206);
	lorry1->CreateContents(Dynamite)->Fuse();
	lorry1->CreateContents(Dynamite)->Fuse();
	lorry2.ContainBlast = true;
	lorry2->CreateContents(Dynamite)->Fuse();
	lorry2->CreateContents(Dynamite)->Fuse();
	Log("Tests difference between a ContainBlast container and a normal container.");
	return true;
}

global func Test1_Completed()
{
	if (ObjectCount(Find_ID(Wood)) == 1)
		return true;
	return false;
}

global func Test1_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Lorry), Find_ID(Wood)));
	DrawMaterialQuad("Earth", 0, LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight(), 0, LandscapeHeight(), true); 
	return;
}

global func Test2_OnStart(int plr)
{
	var lorry1 = CreateObject(Lorry, 200, 200);
	var lorry2 = CreateObject(Lorry, 200, 200);
	lorry1->Enter(lorry2);
	CreateObject(Wood, 200, 206);
	lorry1->CreateContents(Dynamite)->Fuse();
	lorry1->CreateContents(Dynamite)->Fuse();
	Log("Tests explosion propagation through two containers.");
	return true;
}

global func Test2_Completed()
{
	if (ObjectCount(Find_ID(Wood)) == 0)
		return true;
	return false;
}

global func Test2_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Lorry), Find_ID(Wood)));
	DrawMaterialQuad("Earth", 0, LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight(), 0, LandscapeHeight(), true); 
	return;
}

global func Test3_OnStart(int plr)
{
	var lorry1 = CreateObject(Lorry, 200, 200);
	var lorry2 = CreateObject(Lorry, 200, 200);
	lorry1->Enter(lorry2);
	lorry1.ContainBlast = true;
	lorry1->CreateContents(Dynamite)->Fuse();
	lorry1->CreateContents(Dynamite, 3);
	Log("Tests a nested container of which the inner one explodes but contains some blasts.");
	return true;
}

global func Test3_Completed()
{
	if (ObjectCount(Find_ID(Lorry)) == 1)
		return true;
	return false;
}

global func Test3_OnFinished()
{
	RemoveAll(Find_ID(Lorry));
	DrawMaterialQuad("Earth", 0, LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight(), 0, LandscapeHeight(), true); 
	return;
}

global func Test4_OnStart(int plr)
{
	var lorry1 = CreateObject(Lorry, 200, 200);
	var lorry2 = CreateObject(Lorry, 200, 200);
	var lorry3 = CreateObject(Lorry, 200, 200);
	lorry1->Enter(lorry2);
	lorry2->Enter(lorry3);
	lorry2.ContainBlast = true;
	lorry1->CreateContents(Rock);
	lorry2->CreateContents(Rock);
	lorry3->CreateContents(Rock);
	lorry1->CreateContents(Dynamite)->Fuse();
	lorry2->CreateContents(Dynamite)->Fuse();
	lorry3->CreateContents(Dynamite)->Fuse();
	Log("Tests damage done to nested containers and objects inside.");
	return true;
}

global func Test4_Completed()
{
	if (ObjectCount(Find_ID(Lorry), Find_Damage(78)) == 1 &&
	    ObjectCount(Find_ID(Lorry), Find_Damage(52)) == 1 &&
	    ObjectCount(Find_ID(Lorry), Find_Damage(26)) == 1 &&
	    ObjectCount(Find_ID(Rock), Find_Damage(52)) == 1 &&
	    ObjectCount(Find_ID(Rock), Find_Damage(26)) == 2)
		return true;
	return false;
}

global func Test4_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Lorry), Find_ID(Rock)));
	DrawMaterialQuad("Earth", 0, LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight(), 0, LandscapeHeight(), true); 
	return;
}

global func Test5_OnStart(int plr)
{
	var lorry1 = CreateObject(Lorry, 200, 200);
	lorry1.ContainBlast = true;
	lorry1->CreateContents(Rock);
	lorry1->CreateContents(Dynamite, 2)->Fuse();
	lorry1->CreateContents(Dynamite)->Fuse();
	lorry1->CreateContents(Dynamite)->Fuse();
	Log("Test handling of the destruction of a contained blast container.");
	return true;
}

global func Test5_Completed()
{
	if (ObjectCount(Find_ID(Rock), Find_Damage(4 * 26)) == 1 && ObjectCount(Find_ID(LorryFragment)) == 0)
		return true;
	return false;
}

global func Test5_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Lorry), Find_ID(Rock)));
	DrawMaterialQuad("Earth", 0, LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight(), 0, LandscapeHeight(), true); 
	return;
}

global func Test6_OnStart(int plr)
{
	var lorry1 = CreateObject(Lorry, 200, 200);
	var lorry2 = CreateObject(Lorry, 200, 200);
	var lorry3 = CreateObject(Lorry, 200, 200);
	lorry1->Enter(lorry2);
	lorry2->Enter(lorry3);
	lorry2.ContainBlast = true;
	lorry1->CreateContents(Rock);
	lorry2->CreateContents(Rock);
	lorry3->CreateContents(Rock);
	lorry1->CreateContents(Dynamite)->Fuse();
	lorry2->CreateContents(Dynamite)->Fuse();
	lorry2->CreateContents(Dynamite);
	lorry3->CreateContents(Dynamite)->Fuse();
	Log("Tests damage done to nested containers and objects inside, when one in the middle explodes.");
	return true;
}

global func Test6_Completed()
{
	// Open question on expected behaviour.
	return false;
}

global func Test6_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Lorry), Find_ID(Rock)));
	DrawMaterialQuad("Earth", 0, LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight() / 2, LandscapeWidth(), LandscapeHeight(), 0, LandscapeHeight(), true); 
	return;
}

/*-- Helper Functions --*/

global func Find_Damage(int amount)
{
	return [C4FO_Func, "Find_DamageCheck", amount];
}

global func Find_DamageCheck(int amount)
{
	return GetDamage() == amount;
}
