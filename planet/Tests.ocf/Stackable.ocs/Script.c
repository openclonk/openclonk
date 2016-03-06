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

	Log("****** Initial stack count");

	var stackable = CreateObject(Arrow);
	
	var passed = doTest("The stackable object should start with the amount set by InitialStackCount(). Got %d, expected %d", stackable->GetStackCount(), stackable->InitialStackCount());
	passed &= doTest("IsFullStack works correctly: Got %v, expected %v.", stackable->IsFullStack(), true);

	Log("****** Setting a value greater than MaxStackCount() with SetStackCount()");

	stackable->SetStackCount(stackable->MaxStackCount() + 1);
	passed &= doTest("SetStackCount() can set a value greater than MaxStackCount(). Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount() + 1);

	Log("****** Setting a value less than MaxStackCount() with SetStackCount()");

	stackable->SetStackCount(stackable->MaxStackCount() - 1);
	passed &= doTest("SetStackCount() can set a value. Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount() - 1);
	passed &= doTest("IsFullStack works correctly: Got %v, expected %v.", stackable->IsFullStack(), false);

	Log("****** Setting the stack count to 0 with SetStackCount()");

	stackable->SetStackCount(0);
	passed &= doTest("Setting SetStackCount() to 0 should not remove the object. Got %d, expected %d", !!stackable, true);
	passed &= doTest("An object with stack count 0 returns a value nonetheless. Got %d, expected %d", stackable->GetStackCount(), 1);

	Log("****** Setting negative values with SetStackCount()");

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

	Log("****** Increasing the stack count beyond MaxStackCount()");

	var stackable = CreateObject(Arrow);
	
	stackable->SetStackCount(stackable->MaxStackCount());
	stackable->DoStackCount(+1);	
	var passed = doTest("The stackable object can exceed its maximum amount with DoStackCount(). Got %d, expected %d", stackable->GetStackCount(), stackable->MaxStackCount() + 1);
	passed &= doTest("IsFullStack works correctly: Got %v, expected %v.", stackable->IsFullStack(), true);
	
	Log("****** Decreasing the stack count below MaxStackCount()");

	stackable->SetStackCount(stackable->MaxStackCount());
	stackable->DoStackCount(-1);	
	passed &= doTest("DoStackCount() can reduce the stack. Got %d, expected %d", stackable->GetStackCount(), stackable->MaxStackCount() - 1);
	
	Log("****** Decreasing the stack count to 0 with DoStackCount()");

	stackable->SetStackCount(1);
	stackable->DoStackCount(-1);
	passed &= doTest("DoStackCount() removes the object if the stack is reduced to 0 items. Got %v, expected %v", !stackable, true);
	
	Log("****** Decreasing the stack count below 0 with DoStackCount()");

	stackable = CreateObject(Arrow);
	stackable->SetStackCount(0);
	stackable->DoStackCount(-1);
	passed &= doTest("DoStackCount() removes the object if the stack is reduced to less than 0 items. Got %v, expected %v", !stackable, true);

	Log("****** Consistency of SetStackCount() and GetStackCount()");

	stackable = CreateObject(Arrow);
	stackable->SetStackCount(0);
	stackable->DoStackCount(1);
	passed &= doTest("SetStackCount(0) should actually set the stack count to 0. After adding to the stack with DoStackCount(+1) we get %d, expected %d", stackable->GetStackCount(), 1);
	
	stackable->RemoveObject();
	return passed;
}


global func Test3_OnStart(int plr){ return true;}
global func Test3_OnFinished(){ return; }
global func Test3_Execute()
{
	Log("Test the behaviour of CalcValue()");

	var stackable = CreateObject(Arrow);
	var passed = true;
	
	for (var i = 1; i < 11; ++i)
	{	
		stackable->SetStackCount(i);
		var comparison = "Got %d, expected %d.";
		var description = Format("A stack with %d object(s) should have %d times the value of the definition. %s", i, i, comparison);
		var passed = doTest(description, stackable->CalcValue(), (Arrow->GetValue() * i) / Arrow->MaxStackCount());
	}

	stackable->RemoveObject();
	return passed;
}

global func Test4_OnStart(int plr){ return true;}
global func Test4_OnFinished(){ return; }
global func Test4_Execute()
{
	Log("Test the behaviour of TakeObject");

	Log("****** Taking an object from a stack inside a container");
	
	var container = CreateObject(Dummy);

	var stackable = CreateObject(Arrow);
	stackable->Enter(container);
	stackable->SetStackCount(3);
	var item = stackable->TakeObject();
	
	var passed = doTest("The stackable object is contained. Got %v, expected %v.", stackable->Contained(), container);

	passed &= doTest("Taking an object from a stack should return an object. Got %v, expected %v.", !!item, true);
	passed &= doTest("Taking an object from a stack should return a new object. Got %v, expected %v.", item != stackable, true);
	passed &= doTest("Taking an object should reduce the stack count. Got %d, expected %d.", stackable->GetStackCount(), 2);
	passed &= doTest("The taken object should not contained. Got %v, expected %v.", item->Contained(), nil);
	passed &= doTest("The taken object should be a single object. Got %v, expected %v.", item->GetStackCount(), 1);

	Log("****** Taking the last object from a stack inside a container");

	item->RemoveObject();
	stackable->SetStackCount(1);
	var item = stackable->TakeObject();
	
	passed &= doTest("Taking an object from a one-object stack should return the stack itself. Got %v, expected %v.", item, stackable);	
	passed &= doTest("The taken object should not be contained. Got %v, expected %v.", item->Contained(), nil);
	passed &= doTest("The taken object should be a single object. Got %v, expected %v.", item->GetStackCount(), 1);

    item->DoStackCount(1);
    passed &= doTest("The taken object should not have an internal stack count of 0. It increases the stack count correctly. Got %d, expected %d.", item->GetStackCount(), 2);

	Log("****** Taking an object from a stack with 0 objects");

	stackable->Enter(container);
	stackable->SetStackCount(0);
	item = stackable->TakeObject();

	passed &= doTest("The stackable object should be in a container when moving it there. Got %v, expected %v.", stackable->Contained(), container);
	passed &= doTest("Taking an object from a stack with 0 objects should not return an object. Got %v, expected %v.", item, nil);
	passed &= doTest("Taking an object from a stack with 0 objects should preserve the stack. Got %v, expected %v.", !!stackable, true);
	passed &= doTest("Taking an object from a stack with 0 objects should not eject the stack from the container. Got %v, expected %v.", stackable->Contained(), container);

	stackable->RemoveObject();
	container->RemoveObject();
	return passed;
}

// Test for stackable inside extra slot inside extra slot: arrow in bow in cannon, arrow in bow in barrel in cannon

global func Test5_OnStart(int plr){ return true;}
global func Test5_OnFinished(){ return; }
global func Test5_Execute()
{
	Log("Test the behaviour of Stack()");
	
	Log("****** Stack() a full stack onto a full stack");
	
	var stackable = CreateObject(Arrow);
	var other = CreateObject(Arrow);
	stackable->SetStackCount(stackable->MaxStackCount());
	other->SetStackCount(other->MaxStackCount());
	
	var passed = doTest("Stacking two full objects has no effect. Stack() returns %d, expected %d.", stackable->Stack(other), 0);
	passed &= doTest("Stack count of original object should not change. Got %d, expected %d.", stackable->GetStackCount(), stackable->InitialStackCount());
	passed &= doTest("Stack count of other object should not change. Got %d, expected %d.", other->GetStackCount(), other->InitialStackCount());
	
	Log("****** Stack() a full stack onto a stack with 0 items");

	other->SetStackCount(other->MaxStackCount());
	stackable->SetStackCount(0);
	passed &= doTest("Stacking an full stack onto a stack with stack count 0 should transfer everything. Got %d, expected %d.", stackable->Stack(other), stackable->InitialStackCount());
	passed &= doTest("The original object should be full. Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount());
	passed &= doTest("The other object should be removed. Got %v, expected %v.", other, nil);
	
	Log("****** Stack() a stack with 0 items onto a stack with 1 items");

	other = CreateObject(Arrow);
	other->SetStackCount(0);
	stackable->SetStackCount(1);
	passed &= doTest("Stacking an empty object onto a full stack should transfer everything. Got %d, expected %d.", stackable->Stack(other), 0);
	passed &= doTest("The original object should not change. Got %d, expected %d.", stackable->GetStackCount(), 1);
	passed &= doTest("The other object should be removed. Got %v, expected %v.", other, nil);

	Log("****** Stack() a stack with negative items onto a full stack");

	other = CreateObject(Arrow);
	other->SetStackCount(-5);
	stackable->SetStackCount(stackable->MaxStackCount());
	passed &= doTest("Stacking an negative amount object onto a full stack should transfer everything. Got %d, expected %d.", stackable->Stack(other), 0);
	passed &= doTest("The original object should be full. Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount());
	passed &= doTest("The other object should be removed. Got %v, expected %v.", other, nil);

	Log("****** Stack() a full stack onto a partial stack");

	other = CreateObject(Arrow);
	other->SetStackCount(other->MaxStackCount());
	stackable->SetStackCount(8);
	passed &= doTest("Stacking a full object fills the partial stack. Got %d remaining in the (previously) full stack, expected %d.", stackable->Stack(other), 7);
	passed &= doTest("The original object should be full. Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount());
	passed &= doTest("The stacked object should still exist and be partially filled. Got %d, expected %d.", other->GetStackCount(), 8);

	Log("****** Stack() a partial stack onto itself");

	stackable->SetStackCount(2);
	passed &= doTest("Stacking an object on itself does nothing. Got %d, expected %d.", stackable->Stack(stackable), 0);
	passed &= doTest("Stack count does not change. Got %d, expected %d.", stackable->GetStackCount(), 2);

	stackable->RemoveObject();
	other->RemoveObject();

	return passed;
}

global func Test6_OnStart(int plr){ return true;}
global func Test6_OnFinished(){ return; }
global func Test6_Execute()
{
	Log("Test the behaviour of TryAddToStack()");
	
	Log("****** TryAddToStack() a full stack onto a full stack");

	var stackable = CreateObject(Arrow);
	var other = CreateObject(Arrow);
	stackable->SetStackCount(stackable->MaxStackCount());
	other->SetStackCount(other->MaxStackCount());
	
	var passed = doTest("Stacking two full objects has no effect. TryAddToStack() returns %d, expected %d.", other->TryAddToStack(stackable), false);
	passed &= doTest("Stack count of original object should not change. Got %d, expected %d.", stackable->GetStackCount(), stackable->InitialStackCount());
	passed &= doTest("Stack count of other object should not change. Got %d, expected %d.", other->GetStackCount(), other->InitialStackCount());
	
	Log("****** TryAddToStack() a full stack onto a stack with 0 items");

	other->SetStackCount(other->MaxStackCount());
	stackable->SetStackCount(0);
	passed &= doTest("Stacking an full stack onto a stack with stack count 0 should transfer everything. Got %d, expected %d.", other->TryAddToStack(stackable), true);
	passed &= doTest("The original object should be full. Got %d, expected %d.", stackable->GetStackCount(), stackable->InitialStackCount());
	passed &= doTest("The other object should be removed. Got %v, expected %v.", other, nil);
	
	Log("****** TryAddToStack() a stack with 0 items onto a stack with 1 items");

	other = CreateObject(Arrow);
	other->SetStackCount(0);
	stackable->SetStackCount(1);
	passed &= doTest("Stacking an empty object onto a full stack should transfer everything. Got %d, expected %d.", other->TryAddToStack(stackable), true);
	passed &= doTest("The original object should not change. Got %d, expected %d.", stackable->GetStackCount(), 1);
	passed &= doTest("The other object should be removed. Got %v, expected %v.", other, nil);

	Log("****** TryAddToStack() a stack with negative items onto a full stack");

	other = CreateObject(Arrow);
	other->SetStackCount(-5);
	stackable->SetStackCount(stackable->MaxStackCount());
	passed &= doTest("Stacking an negative amount object onto a full stack should transfer everything. Got %d, expected %d.", other->TryAddToStack(stackable), true);
	passed &= doTest("The original object should be full. Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount());
	passed &= doTest("The other object should be removed. Got %v, expected %v.", other, nil);

	Log("****** TryAddToStack() a full stack onto a partial stack");

	other = CreateObject(Arrow);
	other->SetStackCount(other->MaxStackCount());
	stackable->SetStackCount(8);
	passed &= doTest("Stacking a full object fills the partial stack. Got %d remaining in the (previously) full stack, expected %d.", other->TryAddToStack(stackable), true);
	passed &= doTest("The original object should be full. Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount());
	passed &= doTest("The stacked object should still exist and be partially filled. Got %d, expected %d.", other->GetStackCount(), 8);

	Log("****** TryAddToStack() a partial stack onto itself");

	stackable->SetStackCount(2);
	passed &= doTest("Stacking an object on itself does nothing. Got %d, expected %d.", stackable->TryAddToStack(stackable), false);
	passed &= doTest("Stack count does not change. Got %d, expected %d.", stackable->GetStackCount(), 2);

	stackable->RemoveObject();
	other->RemoveObject();
	
	return passed;
}

global func Test7_OnStart(int plr){ return true;}
global func Test7_OnFinished(){ return; }
global func Test7_Execute()
{
	Log("Test the behaviour of TryPutInto() with empty objects");
	var container = CreateObject(Dummy);
	
	Log("****** TryPutInto() a single object stack into an object");

	var stackable = CreateObject(Arrow);
	stackable->SetStackCount(1);

	var passed = doTest("TryPutInto() a single object stack into an object. The collection should not be handled by TryPutInto(). Got %v, expected %v.", stackable->TryPutInto(container), false);
	passed &= doTest("The function should not actually make an object enter the container. The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count does not change. Got %d, expected %d", stackable->GetStackCount(), 1);

	stackable->RemoveObject();
	
	Log("****** TryPutInto() a full stack into an object");
	
	stackable = CreateObject(Arrow);
	stackable->SetStackCount(stackable->MaxStackCount());

	passed = doTest("TryPutInto() an empty stack into an object. Got %v, expected %v.", stackable->TryPutInto(container), false);
	passed &= doTest("The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count does not change. Got %d, expected %d", stackable->GetStackCount(), stackable->MaxStackCount());

	stackable->RemoveObject();

	Log("****** TryPutInto() a partial stack into an object that contains a full stack");

	stackable = CreateObject(Arrow);
	var other = CreateObject(Arrow);
	stackable->SetStackCount(5);
	other->SetStackCount(other->MaxStackCount());
	other->Enter(container);

	passed = doTest("TryPutInto() a partial stack into an object with a full stack. Got %v, expected %v.", stackable->TryPutInto(container), false);
	passed &= doTest("The container of the stack is %d, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count of the added stack does not change. Got %d, expected %d", stackable->GetStackCount(), 5);
	passed &= doTest("The stack count of the original stack does not change. Got %d, expected %d", other->GetStackCount(), other->MaxStackCount());

	stackable->RemoveObject();

	Log("****** TryPutInto() a full stack into an object that contains a partial stack");

	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	stackable->SetStackCount(stackable->MaxStackCount());
	other->SetStackCount(5);
	other->Enter(container);

	passed = doTest("TryPutInto() a full stack into an object with a partial stack. Got %v, expected %v.", stackable->TryPutInto(container), true);
	passed &= doTest("The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count of the added stack does change. Got %d, expected %d", stackable->GetStackCount(), 5);
	passed &= doTest("The stack count of the original stack does change. Got %d, expected %d", other->GetStackCount(), other->MaxStackCount());

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
