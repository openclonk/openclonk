/**
	Producer System
	Unit tests for the producers system. Invokes tests by calling the 
	global function Test*_OnStart(int plr) and iterate through all 
	tests. The test is completed once Test*_Completed() returns
	true. Then Test*_OnFinished() is called, to be able to reset 
	the scenario for the next test.
	
	With LaunchTest(int nr) a specific test can be launched when
	called during runtime. A test can be skipped by calling the
	function SkipTest().
	
	@author Maikel (unit test logic), Marky (tests)
*/


protected func Initialize()
{
	// Energy is not our concern in this test
	CreateObject(Rule_NoPowerNeed);
	return;
}

protected func InitializePlayer(int plr)
{
	// Set zoom to full map size.
	SetPlayerZoomByViewRange(plr, LandscapeWidth(), nil, PLRZOOM_Direct);
	
	// No FoW to see everything happening.
	SetFoW(false, plr);
	
	// All players belong to the first team.
	// The second team only exists for testing.
	SetPlayerTeam(plr, 1);
		
	// Move player to the start of the scenario.
	GetCrew(plr)->SetPosition(120, 150);
	
	// Some knowledge to construct a flagpole.
	GetCrew(plr)->CreateContents(Hammer);
	SetPlrKnowledge(plr, Flagpole);
	
	// Add test control effect.
	var effect = AddEffect("IntTestControl", nil, 100, 2);
	effect.testnr = 1;
	effect.launched = false;
	effect.plr = plr;
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


/*-- Producer Tests --*/

// Producer with liquid need and pseudo liquid object.
global func Test1_OnStart(int plr)
{
	// Producer: Foundry
	var producer = CreateObjectAbove(Foundry, 75, 160, plr);
	producer->CreateContents(Earth, 10);
	producer->CreateContents(Ice, 2); // contain a total of 400 water
	producer->AddToQueue(Loam, 5); // needs 300 water

	// Log what the test is about.
	Log("Objects with liquid need (loam), can be produced with pseudo liquid objects (ice)");
	return true;
}

global func Test1_Completed()
{
	SetTemperature(-10);
	if (ObjectCount(Find_ID(Loam)) >= 5)
		return true;
	return false;
}

global func Test1_OnFinished()
{
	// Remove the created objects
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Loam)));
	return;
}

// Producer with liquid need and liquid container.
global func Test2_OnStart(int plr)
{
	// Producer: Foundry
	var producer = CreateObjectAbove(Foundry, 75, 160, plr);
	producer->CreateContents(Earth, 10);
	var barrel = CreateObject(Barrel);
	barrel->PutLiquid("Water", 300); // contains 300 water
	producer->AddToQueue(Loam, 5); // needs 300 water
	producer->Collect(barrel, true);

	// Log what the test is about.
	Log("Objects with liquid need (loam), can be produced with liquid containers (barrel). The liquid container must not be removed.");
	return true;
}

global func Test2_Completed()
{
    // The barrel must not be removed.
	if (ObjectCount(Find_ID(Loam)) >= 5 && ObjectCount(Find_ID(Barrel)) >= 1)
		return true;
	return false;
}

global func Test2_OnFinished()
{
	// Remove wind generator, compensator and workshop.
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Loam), Find_ID(Barrel)));
	return;
}


// Producer with liquid need and liquid object.
global func Test3_OnStart(int plr)
{
	// Producer: Foundry
	var producer = CreateObjectAbove(Foundry, 75, 160, plr);
	producer->CreateContents(Earth, 10);
	var water = CreateObject(Liquid_Water);
	water->SetStackCount(400);
	producer->AddToQueue(Loam, 5); // needs 300 water
	water->Enter(producer);

	// Log what the test is about.
	Log("Objects with liquid need (loam), can be produced with liquid containers (barrel). The liquid container must not be removed.");
	return true;
}

global func Test3_Completed()
{
    // The liquid must not be removed.
	if (ObjectCount(Find_ID(Loam)) >= 5 && ObjectCount(Find_ID(Liquid_Water)) >= 1)
		return true;
	return false;
}

global func Test3_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Loam), Find_ID(Liquid_Water)));
	return;
}


// Producer with fuel need, fuel object
global func Test4_OnStart(int plr)
{
	// Producer: Foundry
	var producer = CreateObjectAbove(Foundry, 75, 160, plr);
	producer->CreateContents(Wood, 10);
	producer->CreateContents(Ore, 5);
	producer->AddToQueue(Metal, 5);

	// Log what the test is about.
	Log("Objects with fuel need (metal), can be produced with fuel objects (wood). The fuel is used up.");
	return true;
}

global func Test4_Completed()
{
	if (ObjectCount(Find_ID(Metal)) >= 5 && ObjectCount(Find_ID(Wood)) <= 0)
		return true;
	return false;
}

global func Test4_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Metal)));
	return;
}


// Producer with fuel need, liquid container
global func Test5_OnStart(int plr)
{
	// Producer: Foundry
	var producer = CreateObjectAbove(Foundry, 75, 160, plr);
	producer->CreateContents(Ore, 5);
	producer->AddToQueue(Metal, 5);
	
	var barrelA = CreateObject(Barrel);
	var barrelB = CreateObject(Barrel);
	barrelA->PutLiquid("Oil", 300);
	barrelB->PutLiquid("Oil", 300);

	producer->Collect(barrelA, true);
	producer->Collect(barrelB, true);

	// Log what the test is about.
	Log("Objects with fuel need (metal), can be produced with fuel from a liquid container (barrel with oil). The fuel is used up.");
	return true;
}

global func Test5_Completed()
{
	if (ObjectCount(Find_ID(Metal)) >= 5 // metal is produced
	 && ObjectCount(Find_ID(Barrel)) == 2 // barrels stay
	 && FindObject(Find_ID(Liquid_Oil)) // oil remains
	 && FindObject(Find_ID(Liquid_Oil))->GetLiquidAmount() == 100) // the correct amount was used
		return true;
	return false;
}

global func Test5_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Metal), Find_ID(Barrel), Find_ID(Liquid_Oil)));
	return;
}


// Fuel functions behave correclty
global func Test6_OnStart(int plr)
{
	// Log what the test is about.
	Log("Objects that are fuel should return a value > 0 if no parameter is passed to GetFuelAmount().");
	// Get the control effect.
	var effect = GetEffect("IntTestControl", nil);
	if (!effect) return false;

	var passed = true;
	var def;
	for (var i = 0; def = GetDefinition(i); ++i)
	if (def->~IsFuel())
	{
		var instance = CreateObject(def);
		var amount = instance->~GetFuelAmount();
		Log("Default fuel amount in %i: %d > 0?", def, amount);
		passed &= (amount > 0);
		if (instance) instance->RemoveObject();
	}
	
	effect.passed_test_6 = passed;

	return true;
}

global func Test6_Completed()
{
	return GetEffect("IntTestControl", nil).passed_test_6;
}

global func Test6_OnFinished()
{
	return;
}

// Producer stops production on insufficient material
global func Test7_OnStart(int plr)
{
	var producer = CreateObjectAbove(ToolsWorkshop, 75, 160, plr);
	producer->CreateContents(Wood, 10);
	producer->CreateContents(Metal, 1);
	producer->AddToQueue(Barrel, 5);

	// Log what the test is about.
	Log("Producer halts production if materials are insufficient, continues when new material comes in.");

	GetEffect("IntTestControl", nil).timer = 0;
	return true;
}

global func Test7_Completed()
{
	if (ObjectCount(Find_ID(Barrel)) >= 5) return true;

	var fx = GetEffect("IntTestControl", nil);
	fx.timer++;
	
	if (fx.timer >= 250)
	{
		FindObject(Find_ID(ToolsWorkshop))->CreateContents(Metal);
		fx.timer = 200;
	}
	
	return false;
}

global func Test7_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Metal), Find_ID(Barrel), Find_ID(Liquid_Oil)));
	return;
}


/*-- Helper Functions --*/

global func SetWindFixed(int strength)
{
	strength = BoundBy(strength, -100, 100);
	var effect = GetEffect("IntFixedWind");
	if (!effect)
		effect = AddEffect("IntFixedWind", nil, 100, 1);
	effect.strength = strength;
	return;
}

global func RestoreWaterLevels()
{
	// Restore water levels.
	DrawMaterialQuad("Water", 144, 168, 208 + 1, 168, 208 + 1, 304, 144, 304, true);
	for (var x = 216; x <= 280; x++)
		for (var y = 24; y <= 120; y++)
			if (GetMaterial(x, y) == Material("Water"))
				ClearFreeRect(x, y, 1, 1);
	return;
}

global func RemoveWater()
{
	for (var x = 144; x <= 208 + 1; x++)
		for (var y = 168; y <= 304; y++)
			if (GetMaterial(x, y) == Material("Water"))
				ClearFreeRect(x, y, 1, 1);
	return;
}

global func FxIntFixedWindTimer(object target, proplist effect)
{
	SetWind(effect.strength);
	return FX_OK;
}

global func FxIntAlternatingWindTimer(object target, proplist effect, int time)
{
	if (((time / effect.Interval) % 2) == 0)
		SetWindFixed(effect.strength);
	else
		SetWindFixed(0);
	return FX_OK;
}
	
