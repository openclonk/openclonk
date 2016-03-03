/**
	Stackable objects
	Unit tests for the stackable library.
	
	Invokes tests by calling the global function Test*_OnStart(int plr)
	and iterate through all tests.
	The test is completed once Test*_Completed() returns true.
	Then Test*_OnFinished() is called, to be able to reset the scenario
	for the next test.
	
	@author Maikel (test logic), Marky (test)
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
	effect.result = true;
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
			if (effect.result)
				Log("All tests have passed!");
			else
				Log("At least one test has failed!");
			return -1;
		}
		effect.launched = true;
	}
	else
	{
		effect.launched = false;
		var result = Call(Format("Test%d_Execute", effect.testnr));
		effect.result &= result;
		// Call the test on finished function.
		Call(Format("~Test%d_OnFinished", effect.testnr));
		// Log result and increase test number.
		if (result)
			Log(">> Test %d passed.", effect.testnr);
		else
			Log (">> Test %d failed.", effect.testnr);

		effect.testnr++;
	}
	return FX_OK;
}

global func Test1_OnStart(int plr){ return true;}
global func Test1_OnFinished(){ return; }
global func Test1_Execute()
{
	Log("Test the behaviour of SetStackCount() and GetStackCount()");

	var stackable = CreateObject(Arrow);
	
	var passed = doTest("The stackable object should start with the amount set by InitialStackCount(). Got %d, expected %d", stackable->GetStackCount(), stackable->InitialStackCount());
	passed &= doTest("IsFullStack works correctly: Got %v, expected %v.", stackable->IsFullStack(), true);

	stackable->SetStackCount(stackable->MaxStackCount() + 1);
	passed &= doTest("SetStackCount() can set a value greater than MaxStackCount(). Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount() + 1);

	stackable->SetStackCount(stackable->MaxStackCount() - 1);
	passed &= doTest("SetStackCount() can set a value. Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount() - 1);
	passed &= doTest("IsFullStack works correctly: Got %v, expected %v.", stackable->IsFullStack(), false);

	stackable->SetStackCount(0);
	passed &= doTest("Setting SetStackCount() to 0 should not remove the object. Got %d, expected %d", !!stackable, true);
	passed &= doTest("An object with stack count 0 returns a value nonetheless. Got %d, expected %d", stackable->GetStackCount(), 1);

	stackable->SetStackCount(-1);
	passed &= doTest("SetStackCount() cannot set negative values. Got %d, expected %d", stackable->GetStackCount(), 1);
	
	stackable->RemoveObject();

	return passed;
}

global func Test2_OnStart(int plr){ return true;}
global func Test2_OnFinished(){ return; }
global func Test2_Execute()
{
	Log("Test the behaviour of DoStackCount()");

	var stackable = CreateObject(Arrow);
	
	stackable->SetStackCount(stackable->MaxStackCount());
	stackable->DoStackCount(+1);	
	var passed = doTest("The stackable object can exceed its maximum amount with DoStackCount(). Got %d, expected %d", stackable->GetStackCount(), stackable->MaxStackCount() + 1);
	passed &= doTest("IsFullStack works correctly: Got %v, expected %v.", stackable->IsFullStack(), true);
	
	stackable->SetStackCount(stackable->MaxStackCount());
	stackable->DoStackCount(-1);	
	passed &= doTest("DoStackCount() can reduce the stack. Got %d, expected %d", stackable->GetStackCount(), stackable->MaxStackCount() - 1);
	
	stackable->SetStackCount(1);
	stackable->DoStackCount(-1);
	passed &= doTest("DoStackCount() removes the object if the stack is reduced to 0 items. Got %v, expected %v", !stackable, true);
	
	stackable = CreateObject(Arrow);
	stackable->SetStackCount(0);
	stackable->DoStackCount(-1);
	passed &= doTest("DoStackCount() removes the object if the stack is reduced to less than 0 items. Got %v, expected %v", !stackable, true);

	stackable = CreateObject(Arrow);
	stackable->SetStackCount(0);
	stackable->DoStackCount(1);
	passed &= doTest("SetStackCount(0) should actually set the stack count to 0. After adding to the stack with DoStackCount(+1) we get %d, expected %d", stackable->GetStackCount(), 1);
	
	stackable->RemoveObject();
	return passed;
}

global func doTest(description, returned, expected)
{
	var test = (returned == expected);
	
	var predicate = "[Fail]";
	if (test) predicate = "[Pass]";
	
	Log(Format("%s %s", predicate, description), returned, expected);
	return test;
}
