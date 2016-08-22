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

	stackable->SetStackCount(1);
	passed &= doTest("SetStackCount() can set the minimum value. Got %d, expected %d.", stackable->GetStackCount(), 1);

	Log("****** Setting the stack count to 0 with SetStackCount()");

	stackable->SetStackCount(0);
	passed &= doTest("Setting SetStackCount() to 0 should remove the object. Got %v, expected %v", stackable, nil);

	if (stackable) stackable->RemoveObject();

	Log("****** Setting negative values with SetStackCount()");

	stackable = CreateObject(Arrow);
	stackable->SetStackCount(-1);
	passed &= doTest("SetStackCount() cannot to negative values removes the object. Got %v, expected %v", stackable, nil);
	
	if (stackable) stackable->RemoveObject();

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
	stackable->SetStackCount(1);
	stackable->DoStackCount(-2);
	passed &= doTest("DoStackCount() removes the object if the stack is reduced to less than 0 items. Got %v, expected %v", !stackable, true);

	//Log("****** Consistency of SetStackCount() and GetStackCount()");
    //
	//stackable = CreateObject(Arrow);
	//stackable->SetStackCount(0);
	//stackable->DoStackCount(1);
	//passed &= doTest("SetStackCount(1) should actually set the stack count to 0. After adding to the stack with DoStackCount(+1) we get %d, expected %d", stackable->GetStackCount(), 1);
	
	if (stackable) stackable->RemoveObject();
	return passed;
}


global func Test3_OnStart(int plr){ return true;}
global func Test3_OnFinished(){ return; }
global func Test3_Execute()
{
	Log("Test the behaviour of CalcValue() and UpdateMass()");

	var stackable = CreateObject(Arrow);
	var passed = true;
	
	for (var i = 1; i < 11; ++i)
	{	
		stackable->SetStackCount(i);
		var comparison = "Got %d, expected %d.";
		var description = Format("A stack with %d object(s) should have value proportional to the value of the definition. %s", i, comparison);
		passed &= doTest(description, stackable->CalcValue(), (Arrow->GetValue() * i) / Arrow->InitialStackCount());

		description = Format("A stack with %d object(s) should have mass proportional to that of the definition. %s", i, comparison);
		passed &= doTest(description, stackable->GetMass(), Max(1, (Arrow->GetMass() * i) / Arrow->InitialStackCount()));
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

	//Log("****** Taking an object from a stack with 0 objects");
    //
	//stackable->Enter(container);
	//stackable->SetStackCount(0);
	//item = stackable->TakeObject();
    //
	//passed &= doTest("The stackable object should be in a container when moving it there. Got %v, expected %v.", stackable->Contained(), container);
	//passed &= doTest("Taking an object from a stack with 0 objects should not return an object. Got %v, expected %v.", item, nil);
	//passed &= doTest("Taking an object from a stack with 0 objects should preserve the stack. Got %v, expected %v.", !!stackable, true);
	//passed &= doTest("Taking an object from a stack with 0 objects should not eject the stack from the container. Got %v, expected %v.", stackable->Contained(), container);

	stackable->RemoveObject();
	container->RemoveObject();
	return passed;
}


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
	
	//Log("****** Stack() a full stack onto a stack with 0 items");
    //
	//other->SetStackCount(other->MaxStackCount());
	//stackable->SetStackCount(0);
	//passed &= doTest("Stacking an full stack onto a stack with stack count 1 should transfer everything. Got %d, expected %d.", stackable->Stack(other), stackable->InitialStackCount());
	//passed &= doTest("The original object should be full. Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount());
	//passed &= doTest("The other object should be removed. Got %v, expected %v.", other, nil);
	
	
	Log("****** Stack() a stack with 1 items onto a stack with 1 items");

	other->RemoveObject();
	other = CreateObject(Arrow);
	other->SetStackCount(1);
	stackable->SetStackCount(1);
	passed &= doTest("Stacking an single object onto a single stack should transfer everything. Got %d, expected %d.", stackable->Stack(other), 1);
	passed &= doTest("The original object should increase its stack count. Got %d, expected %d.", stackable->GetStackCount(), 2);
	passed &= doTest("The other object should still exist. Got %v, expected %v.", !!other, true); // TODO: The expected behavior was, that the other object gets removed. Try this out again later.

	//Log("****** Stack() a stack with negative items onto a full stack");
    //
	//other->RemoveObject();
	//other = CreateObject(Arrow);
	//other->SetStackCount(-5);
	//stackable->SetStackCount(stackable->MaxStackCount());
	//passed &= doTest("Stacking an negative amount object onto a full stack should transfer everything. Got %d, expected %d.", stackable->Stack(other), 0);
	//passed &= doTest("The original object should be full. Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount());
	//passed &= doTest("The other object should be removed. Got %v, expected %v.", other, nil);

	Log("****** Stack() a full stack onto a partial stack");

	other->RemoveObject();
	other = CreateObject(Arrow);
	other->SetStackCount(other->MaxStackCount());
	stackable->SetStackCount(8);
	passed &= doTest("Stacking a full object fills the partial stack. Got %d remaining in the (previously) full stack, expected %d.", stackable->Stack(other), 7);
	passed &= doTest("The original object should be full. Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount());
	passed &= doTest("The stacked object should still exist and be full. Got %d, expected %d.", other->GetStackCount(), other->InitialStackCount()); // TODO: the expected behavior was, that the other object contains 8 items. Try this out again later.

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
	
	//Log("****** TryAddToStack() a full stack onto a stack with 0 items");
    //
	//other->SetStackCount(other->MaxStackCount());
	//stackable->SetStackCount(0);
	//passed &= doTest("Stacking an full stack onto a stack with stack count 0 should transfer everything. Got %d, expected %d.", other->TryAddToStack(stackable), true);
	//passed &= doTest("The original object should be full. Got %d, expected %d.", stackable->GetStackCount(), stackable->InitialStackCount());
	//passed &= doTest("The other object should be removed. Got %v, expected %v.", other, nil);
	
	Log("****** TryAddToStack() a stack with 1 items onto a stack with 1 items");

	other->RemoveObject();
	other = CreateObject(Arrow);
	other->SetStackCount(1);
	stackable->SetStackCount(1);
	passed &= doTest("Stacking an single stack onto a single stack should transfer everything. Got %v, expected %v.", other->TryAddToStack(stackable), true);
	passed &= doTest("The original object should change. Got %d, expected %d.", stackable->GetStackCount(), 2);
	passed &= doTest("The other object should be removed. Got %v, expected %v.", other, nil);

	//Log("****** TryAddToStack() a stack with negative items onto a full stack");
    //
	//if (other) other->RemoveObject();
	//other = CreateObject(Arrow);
	//other->SetStackCount(-5);
	//stackable->SetStackCount(stackable->MaxStackCount());
	//passed &= doTest("Stacking an negative amount object onto a full stack should transfer everything. Got %d, expected %d.", other->TryAddToStack(stackable), true);
	//passed &= doTest("The original object should be full. Got %d, expected %d.", stackable->GetStackCount(), stackable->MaxStackCount());
	//passed &= doTest("The other object should be removed. Got %v, expected %v.", other, nil);

	Log("****** TryAddToStack() a full stack onto a partial stack");

	if (other) other->RemoveObject();
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
	Log("Test the behaviour of MergeWithStacksIn() with empty objects");
	var container = CreateObject(Dummy);
	
	Log("****** MergeWithStacksIn() a single object stack into an object");

	var stackable = CreateObject(Arrow);
	stackable->SetStackCount(1);

	var passed = doTest("MergeWithStacksIn() a single object stack into an object. The collection should not be handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(container), false);
	passed &= doTest("The function should not actually make an object enter the container. The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count does not change. Got %d, expected %d", stackable->GetStackCount(), 1);

	stackable->RemoveObject();
	
	Log("****** MergeWithStacksIn() a full stack into an object");
	
	stackable = CreateObject(Arrow);
	stackable->SetStackCount(stackable->MaxStackCount());

	passed &= doTest("MergeWithStacksIn() an empty stack into an object. Got %v, expected %v.", stackable->MergeWithStacksIn(container), false);
	passed &= doTest("The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count does not change. Got %d, expected %d", stackable->GetStackCount(), stackable->MaxStackCount());

	stackable->RemoveObject();

	Log("****** MergeWithStacksIn() a partial stack into an object that contains a full stack");

	stackable = CreateObject(Arrow);
	var other = CreateObject(Arrow);
	stackable->SetStackCount(5);
	other->SetStackCount(other->MaxStackCount());
	other->Enter(container);

	passed &= doTest("MergeWithStacksIn() a partial stack into an object with a full stack. Got %v, expected %v.", stackable->MergeWithStacksIn(container), false);
	passed &= doTest("The container of the stack is %d, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count of the added stack does not change. Got %d, expected %d", stackable->GetStackCount(), 5);
	passed &= doTest("The stack count of the original stack does not change. Got %d, expected %d", other->GetStackCount(), other->MaxStackCount());

	other->RemoveObject();
	stackable->RemoveObject();

	Log("****** MergeWithStacksIn() a full stack into an object that contains a partial stack");

	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	stackable->SetStackCount(stackable->MaxStackCount());
	other->SetStackCount(5);
	other->Enter(container);

	passed &= doTest("MergeWithStacksIn() counts the operation as not handled. Got %v, expected %v.", stackable->MergeWithStacksIn(container), false); // TODO: was true, but this prevents recursive stacking
	passed &= doTest("The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count of the added stack does change. Got %d, expected %d", stackable->GetStackCount(), 5);
	passed &= doTest("The stack count of the original stack does change. Got %d, expected %d", other->GetStackCount(), other->MaxStackCount());

	other->RemoveObject();
	stackable->RemoveObject();

	return passed;
}

global func Test8_OnStart(int plr){ return true;}
global func Test8_OnFinished(){ return; }
global func Test8_Execute()
{
	Log("Test the behaviour of MergeWithStacksIn() with objects that contain an object with a useable extra slot");
	var container = CreateObject(Dummy);
	var bow = container->CreateContents(Bow);

	Log("****** MergeWithStacksIn() a single object stack into an object");

	var stackable = CreateObject(Arrow);
	stackable->SetStackCount(1);

	var passed = doTest("MergeWithStacksIn() a single object stack into an object. The collection should not be handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(container), false);
	passed &= doTest("The function should not actually make an object enter the container. The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count does not change. Got %d, expected %d", stackable->GetStackCount(), 1);

	stackable->RemoveObject();
	
	Log("****** MergeWithStacksIn() a full stack into an object");
	
	stackable = CreateObject(Arrow);
	stackable->SetStackCount(stackable->MaxStackCount());

	passed &= doTest("MergeWithStacksIn() an empty stack into an object. Got %v, expected %v.", stackable->MergeWithStacksIn(container), false);
	passed &= doTest("The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count does not change. Got %d, expected %d", stackable->GetStackCount(), stackable->MaxStackCount());

	stackable->RemoveObject();

	Log("****** MergeWithStacksIn() a partial stack into an object that contains a full stack");

	stackable = CreateObject(Arrow);
	var other = CreateObject(Arrow);
	stackable->SetStackCount(5);
	other->SetStackCount(other->MaxStackCount());
	other->Enter(container);

	passed &= doTest("MergeWithStacksIn() a partial stack into an object with a full stack. Got %v, expected %v.", stackable->MergeWithStacksIn(container), false);
	passed &= doTest("The container of the stack is %d, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count of the added stack does not change. Got %d, expected %d", stackable->GetStackCount(), 5);
	passed &= doTest("The stack count of the original stack does not change. Got %d, expected %d", other->GetStackCount(), other->MaxStackCount());

	other->RemoveObject();
	stackable->RemoveObject();

	Log("****** MergeWithStacksIn() a full stack into an object that contains a partial stack");

	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	stackable->SetStackCount(stackable->MaxStackCount());
	other->SetStackCount(5);
	other->Enter(container);

	passed &= doTest("MergeWithStacksIn() does not handled the operation. Got %v, expected %v.", stackable->MergeWithStacksIn(container), false); //TODO
	passed &= doTest("The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count of the added stack does change. Got %d, expected %d", stackable->GetStackCount(), 5);
	passed &= doTest("The stack count of the original stack does change. Got %d, expected %d", other->GetStackCount(), other->MaxStackCount());

	other->RemoveObject();
	stackable->RemoveObject();

	Log("****** MergeWithStacksIn() a partial stack into a container that contains a partial stack");
	
	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	var ammo = CreateObject(Arrow);
	
	stackable->SetStackCount(5);
	other->SetStackCount(5);
	ammo->SetStackCount(5);
	
	other->Enter(container);
	ammo->Enter(bow);
	
	passed &= doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), bow);

	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(container), true);
	passed &= doTest("The object got removed. Got %v, expected %v.", stackable, nil);
	passed &= doTest("The stack inside the weapon inside the container is served first. Got %d, expected %d.", ammo->GetStackCount(), 10);
	passed &= doTest("The stack inside the container is not served. Got %d, expected %d.", other->GetStackCount(), 5);

	other->RemoveObject();
	ammo->RemoveObject();
	if (stackable) stackable->RemoveObject();

	Log("****** MergeWithStacksIn() a partial stack into an object inside a container that contains a partial stack");
	
	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	ammo = CreateObject(Arrow);
	
	stackable->SetStackCount(5);
	other->SetStackCount(5);
	ammo->SetStackCount(5);
	
	other->Enter(container);
	ammo->Enter(bow);
	
	passed &= doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), bow);

	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(bow), true);
	passed &= doTest("The object got removed. Got %v, expected %v.", stackable, nil);
	passed &= doTest("The stack inside the weapon inside the container is served. Got %d, expected %d.", ammo->GetStackCount(), 10);
	passed &= doTest("The stack inside the container is not served. Got %d, expected %d.", other->GetStackCount(), 5);

	other->RemoveObject();
	ammo->RemoveObject();
	if (stackable) stackable->RemoveObject();

	Log("****** MergeWithStacksIn() an overfull stack into a container that contains a partial stack");
	
	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	ammo = CreateObject(Arrow);
	
	stackable->SetStackCount(stackable->MaxStackCount() + 5);
	other->SetStackCount(5);
	ammo->SetStackCount(5);
	
	other->Enter(container);
	ammo->Enter(bow);
	
	passed &= doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), bow);

	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(container), true);
	passed &= doTest("The object is not removed. Got %v, expected %v.", stackable, nil);
	passed &= doTest("The stack inside the weapon inside the container is served first. Got %d, expected %d.", ammo->GetStackCount(), ammo->MaxStackCount());
	passed &= doTest("The stack inside the container is not served second. Got %d, expected %d.", other->GetStackCount(), 15);

	other->RemoveObject();
	ammo->RemoveObject();
	if (stackable) stackable->RemoveObject();

	Log("****** MergeWithStacksIn() an overfull stack into an object inisde a container that contains a partial stack");
	
	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	ammo = CreateObject(Arrow);
	
	stackable->SetStackCount(stackable->MaxStackCount() + 5);
	other->SetStackCount(5);
	ammo->SetStackCount(5);
	
	other->Enter(container);
	ammo->Enter(bow);
	
	passed &= doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), bow);

//	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(bow), true); // TODO: this prevents recursive stacking
	passed &= doTest("The entrance is not handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(bow), false);
	passed &= doTest("The object did not get removed and is not contained. Got %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack inside the weapon inside the container is served. Got %d, expected %d.", ammo->GetStackCount(), ammo->MaxStackCount());
	passed &= doTest("The stack inside the container is not served. Got %d, expected %d.", other->GetStackCount(), 5);
	passed &= doTest("The stack changes amount correctly. Got %d, expected %d.", stackable->GetStackCount(), 10);

	other->RemoveObject();
	ammo->RemoveObject();
	if (stackable) stackable->RemoveObject();

	return passed;
}



global func Test9_OnStart(int plr){ return true;}
global func Test9_OnFinished(){ return; }
global func Test9_Execute()
{
	Log("Test the behaviour of MergeWithStacksIn() with objects that contain an object without extra slot");
	var container = CreateObject(Dummy);
	var lorry = container->CreateContents(Lorry);

	Log("****** MergeWithStacksIn() a single object stack into an object");

	var stackable = CreateObject(Arrow);
	stackable->SetStackCount(1);

	var passed = doTest("MergeWithStacksIn() a single object stack into an object. The collection should not be handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(container), false);
	passed &= doTest("The function should not actually make an object enter the container. The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count does not change. Got %d, expected %d", stackable->GetStackCount(), 1);

	stackable->RemoveObject();
	
	Log("****** MergeWithStacksIn() a full stack into an object");
	
	stackable = CreateObject(Arrow);
	stackable->SetStackCount(stackable->MaxStackCount());

	passed &= doTest("MergeWithStacksIn() an empty stack into an object. Got %v, expected %v.", stackable->MergeWithStacksIn(container), false);
	passed &= doTest("The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count does not change. Got %d, expected %d", stackable->GetStackCount(), stackable->MaxStackCount());

	stackable->RemoveObject();

	Log("****** MergeWithStacksIn() a partial stack into an object that contains a full stack");

	stackable = CreateObject(Arrow);
	var other = CreateObject(Arrow);
	stackable->SetStackCount(5);
	other->SetStackCount(other->MaxStackCount());
	other->Enter(container);

	passed &= doTest("MergeWithStacksIn() a partial stack into an object with a full stack. Got %v, expected %v.", stackable->MergeWithStacksIn(container), false);
	passed &= doTest("The container of the stack is %d, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count of the added stack does not change. Got %d, expected %d", stackable->GetStackCount(), 5);
	passed &= doTest("The stack count of the original stack does not change. Got %d, expected %d", other->GetStackCount(), other->MaxStackCount());

	other->RemoveObject();
	stackable->RemoveObject();

	Log("****** MergeWithStacksIn() a full stack into an object that contains a partial stack");

	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	stackable->SetStackCount(stackable->MaxStackCount());
	other->SetStackCount(5);
	other->Enter(container);

	//passed &= doTest("MergeWithStacksIn() a full stack into an object with a partial stack. Got %v, expected %v.", stackable->MergeWithStacksIn(container), true);
	passed &= doTest("MergeWithStacksIn() counts the operation as not handled. Got %v, expected %v.", stackable->MergeWithStacksIn(container), false);
	passed &= doTest("The container of the stack is %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack count of the added stack does change. Got %d, expected %d", stackable->GetStackCount(), 5);
	passed &= doTest("The stack count of the original stack does change. Got %d, expected %d", other->GetStackCount(), other->MaxStackCount());

	other->RemoveObject();
	stackable->RemoveObject();

	Log("****** MergeWithStacksIn() a partial stack into a container that contains a partial stack");
	
	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	var ammo = CreateObject(Arrow);
	
	stackable->SetStackCount(5);
	other->SetStackCount(5);
	ammo->SetStackCount(5);
	
	other->Enter(container);
	ammo->Enter(lorry);

	passed &= doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), lorry);

	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(container), true);
	passed &= doTest("The object got removed. Got %v, expected %v.", stackable, nil);
	passed &= doTest("The stack inside the weapon inside the container is not served. Got %d, expected %d.", ammo->GetStackCount(), 5);
	passed &= doTest("The stack inside the container is served. Got %d, expected %d.", other->GetStackCount(), 10);

	other->RemoveObject();
	ammo->RemoveObject();

	Log("****** MergeWithStacksIn() a partial stack into an object inside a container that contains a partial stack");
	
	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	ammo = CreateObject(Arrow);
	
	stackable->SetStackCount(5);
	other->SetStackCount(5);
	ammo->SetStackCount(5);
	
	other->Enter(container);
	ammo->Enter(lorry);
	
	passed &= doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), lorry);

	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(lorry), true);
	passed &= doTest("The object got removed. Got %v, expected %v.", stackable, nil);
	passed &= doTest("The stack inside the weapon inside the container is served. Got %d, expected %d.", ammo->GetStackCount(), 10);
	passed &= doTest("The stack inside the container is not served. Got %d, expected %d.", other->GetStackCount(), 5);

	other->RemoveObject();
	ammo->RemoveObject();

	Log("****** MergeWithStacksIn() an overfull stack into a container that contains a partial stack");
	
	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	ammo = CreateObject(Arrow);
	
	stackable->SetStackCount(stackable->MaxStackCount() + 5);
	other->SetStackCount(5);
	ammo->SetStackCount(5);
	
	other->Enter(container);
	ammo->Enter(lorry);
	
	passed &= doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), lorry);

	//passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(container), true);
	passed &= doTest("The entrance is not handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(container), false);
	passed &= doTest("The stack inside the weapon inside the container is not served. Got %d, expected %d.", ammo->GetStackCount(), 5);
	passed &= doTest("The stack inside the container is served. Got %d, expected %d.", other->GetStackCount(), other->MaxStackCount());
	passed &= doTest("The stack changed correctly. Got %d, expected %d.", stackable->GetStackCount(), 10);

	other->RemoveObject();
	ammo->RemoveObject();
	stackable->RemoveObject();

	Log("****** MergeWithStacksIn() an overfull stack into an object inisde a container that contains a partial stack");
	
	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	ammo = CreateObject(Arrow);
	
	stackable->SetStackCount(stackable->MaxStackCount() + 5);
	other->SetStackCount(5);
	ammo->SetStackCount(5);

	other->Enter(container);
	ammo->Enter(lorry);
	
	passed &= doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), lorry);

	//passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(lorry), true);
	passed &= doTest("The entrance is not handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(lorry), false);
	passed &= doTest("The object did not get removed and is not contained. Got %v, expected %v.", stackable->Contained(), nil);
	passed &= doTest("The stack inside the weapon inside the container is served. Got %d, expected %d.", ammo->GetStackCount(), ammo->MaxStackCount());
	passed &= doTest("The stack inside the container is not served. Got %d, expected %d.", other->GetStackCount(), 5);
	passed &= doTest("The stack changes amount correctly. Got %d, expected %d.", stackable->GetStackCount(), 10);

	other->RemoveObject();
	ammo->RemoveObject();
	stackable->RemoveObject();

	return passed;
}


global func Test10_OnStart(int plr){ return true;}
global func Test10_OnFinished(){ return; }
global func Test10_Execute()
{
	Log("Test the behaviour of MergeWithStacksIn() with deeply nested extra-slot objects");
	var container = CreateObject(Dummy);
	var cannon = container->CreateContents(Cannon);
	var bow = cannon->CreateContents(Bow);


	Log("****** MergeWithStacksIn() a partial stack -> container");

	var stackable = CreateObject(Arrow);
	var other = CreateObject(Arrow);
	var ammo = CreateObject(Arrow);
	var arrows = CreateObject(Arrow);
	
	stackable->SetStackCount(5);
	other->SetStackCount(5);
	ammo->SetStackCount(5);
	arrows->SetStackCount(5);
	
	other->Enter(container);
	ammo->Enter(cannon);
	arrows->Enter(bow);

	var passed = doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), cannon);
	passed &= doTest("Prerequisite: Arrows object is in %v, expected %v.", arrows->Contained(), bow);

	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(container), true);
	passed &= doTest("The object got removed. Got %v, expected %v.", stackable, nil);
	passed &= doTest("The bow stack is served. Got %d, expected %d.", arrows->GetStackCount(), 10);
	passed &= doTest("The cannon stack is not served. Got %d, expected %d.", ammo->GetStackCount(), 5);
	passed &= doTest("The container stack is not served. Got %d, expected %d.", other->GetStackCount(), 5);

	arrows->RemoveObject();
	other->RemoveObject();
	ammo->RemoveObject();

	Log("****** MergeWithStacksIn() a partial stack -> cannon");

	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	ammo = CreateObject(Arrow);
	arrows = CreateObject(Arrow);
	
	stackable->SetStackCount(5);
	other->SetStackCount(5);
	ammo->SetStackCount(5);
	arrows->SetStackCount(5);
	
	other->Enter(container);
	ammo->Enter(cannon);
	arrows->Enter(bow);

	passed &= doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), cannon);
	passed &= doTest("Prerequisite: Arrows object is in %v, expected %v.", arrows->Contained(), bow);

	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(cannon), true);
	passed &= doTest("The object got removed. Got %v, expected %v.", stackable, nil);
	passed &= doTest("The bow stack is served. Got %d, expected %d.", arrows->GetStackCount(), 10);
	passed &= doTest("The cannon stack is not served. Got %d, expected %d.", ammo->GetStackCount(), 5);
	passed &= doTest("The container stack is not served. Got %d, expected %d.", other->GetStackCount(), 5);

	arrows->RemoveObject();
	other->RemoveObject();
	ammo->RemoveObject();

	Log("****** MergeWithStacksIn() a partial stack -> bow");

	stackable = CreateObject(Arrow);
	other = CreateObject(Arrow);
	ammo = CreateObject(Arrow);
	arrows = CreateObject(Arrow);
	
	stackable->SetStackCount(5);
	other->SetStackCount(5);
	ammo->SetStackCount(5);
	arrows->SetStackCount(5);
	
	other->Enter(container);
	ammo->Enter(cannon);
	arrows->Enter(bow);

	passed &= doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), cannon);
	passed &= doTest("Prerequisite: Arrows object is in %v, expected %v.", arrows->Contained(), bow);

	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", stackable->MergeWithStacksIn(bow), true);
	passed &= doTest("The object got removed. Got %v, expected %v.", stackable, nil);
	passed &= doTest("The bow stack is served. Got %d, expected %d.", arrows->GetStackCount(), 10);
	passed &= doTest("The cannon stack is not served. Got %d, expected %d.", ammo->GetStackCount(), 5);
	passed &= doTest("The container stack is not served. Got %d, expected %d.", other->GetStackCount(), 5);

	arrows->RemoveObject();
	other->RemoveObject();
	ammo->RemoveObject();

	return passed;
}


global func Test11_OnStart(int plr){ return true;}
global func Test11_OnFinished(){ return; }
global func Test11_Execute()
{
	Log("Test the use case #1: Clonk collects stack while having an incomplete stack in the inventory.");

	var crew = CreateObject(Clonk);
	var stackable = CreateObject(Arrow);
	var arrows = crew->CreateContents(Arrow);
	
	var diff = 5;
	arrows->SetStackCount(arrows->InitialStackCount() - diff);
	stackable->SetStackCount(diff);
	
	crew->Collect(stackable);
	var passed = doTest("Arrow object is filled. Got %d, expected %d.", arrows->GetStackCount(), arrows->InitialStackCount());
	passed &= doTest("Stackable object is removed. Got %v, expected %v.", stackable, nil);
	
	arrows->RemoveObject();
	crew->RemoveObject();
	return passed;
}


global func Test12_OnStart(int plr){ return true;}
global func Test12_OnFinished(){ return; }
global func Test12_Execute()
{
	Log("Test the use case #2: Clonk collects stack and it fills his incomplete stacks.");

	var crew = CreateObject(Clonk);
	var stackable = CreateObject(Arrow);
	var arrows = crew->CreateContents(Arrow);
	var bow = crew->CreateContents(Bow);
	var ammo = bow->CreateContents(Arrow);
	
	var to_ammo = 5;
	var to_arrows = 2;
	var remaining = arrows->InitialStackCount() - to_ammo - to_arrows;
	
	ammo->SetStackCount(ammo->MaxStackCount() - to_ammo);
	arrows->SetStackCount(arrows->MaxStackCount() - to_arrows);

	Log("****** Prerequisites");
	
	var passed = doTest("Ammo is in the bow. Got %v, expected %v.", ammo->Contained(), bow);
	passed &= doTest("Arrows are in the crew member. Got %v, expected %v.", arrows->Contained(), crew);
	
	Log("****** Collect stackable");
	
	crew->Collect(stackable);
	passed &= doTest("The arrow stack in the bow is filled. Got %d, expected %d.", ammo->GetStackCount(), ammo->MaxStackCount());
	passed &= doTest("The arrow stack in the inventory is filled. Got %d, expected %d.", arrows->GetStackCount(), arrows->MaxStackCount());
	passed &= doTest("The arrow stack that was collected has the correct count. Got %d, expected %d.", stackable->GetStackCount(), remaining);
	passed &= doTest("The arrow stack is in the crew member inventory. Got %v, expected %v.", stackable->Contained(), crew);
	
	ammo->RemoveObject();
	bow->RemoveObject();
	arrows->RemoveObject();
	stackable->RemoveObject();
	crew->RemoveObject();
	
	return passed;
}


global func Test13_OnStart(int plr){ return true;}
global func Test13_OnFinished(){ return; }
global func Test13_Execute()
{
	Log("Test the use case #2: Clonk collects stack and it fills his incomplete stacks, but does not enter the inventory.");

	var crew = CreateObject(Clonk);
	var stackable = CreateObject(Arrow);
	var arrows = crew->CreateContents(Arrow);
	var bow = crew->CreateContents(Bow);
	var ammo = bow->CreateContents(Arrow);
	
	var to_ammo = 5;
	var to_arrows = 2;
	var remaining = arrows->InitialStackCount() - to_ammo - to_arrows;
	
	ammo->SetStackCount(ammo->MaxStackCount() - to_ammo);
	arrows->SetStackCount(arrows->MaxStackCount() - to_arrows);
	
	crew->CreateContents(Rock, 3);

	Log("****** Prerequisites");
	
	var passed = doTest("Ammo is in the bow. Got %v, expected %v.", ammo->Contained(), bow);
	passed &= doTest("Arrows are in the crew member. Got %v, expected %v.", arrows->Contained(), crew);
	
	Log("****** Collect stackable");
	
	crew->Collect(stackable);
	passed &= doTest("The arrow stack in the bow is filled. Got %d, expected %d.", ammo->GetStackCount(), ammo->MaxStackCount());
	passed &= doTest("The arrow stack in the inventory is filled. Got %d, expected %d.", arrows->GetStackCount(), arrows->MaxStackCount());
	passed &= doTest("The arrow stack that was collected has the correct count. Got %d, expected %d.", stackable->GetStackCount(), remaining);
	passed &= doTest("The arrow stack is not in the crew member inventory. Got %v, expected %v.", stackable->Contained(), nil);
	
	RemoveAll(Find_ID(Rock));
	ammo->RemoveObject();
	bow->RemoveObject();
	arrows->RemoveObject();
	stackable->RemoveObject();
	crew->RemoveObject();
	
	return passed;
}


global func Test14_OnStart(int plr){ return true;}
global func Test14_OnFinished(){ return; }
global func Test14_Execute()
{
	Log("Test infinite stack count: Basics");

	var stackable = CreateObject(Arrow);
	var expected_infinite_count = 50;

	var passed = doTest("Object starts with finite stack count. Got %v, expected %v.", stackable->IsInfiniteStackCount(), false);
	
	Log("****** Set infinite stack count");
	
	stackable->SetInfiniteStackCount();
	
	var expected_value = (stackable->GetID()->GetValue() * expected_infinite_count) / (stackable->InitialStackCount());
	var expected_mass = (stackable->GetID()->GetMass() * expected_infinite_count) / (stackable->InitialStackCount());

	passed &= doTest("Object should have infinite stack count. Got %v, expected %v.", stackable->IsInfiniteStackCount(), true);
	passed &= doTest("Get the stack count. Got %d, expected %d.", stackable->GetStackCount(), expected_infinite_count);
	passed &= doTest("Get the object mass. Got %d, expected %d.", stackable->GetMass(), expected_mass);
	passed &= doTest("Get the value of the object. Got %d, expected %d.", stackable->CalcValue(), expected_value);
	passed &= doTest("The infinite stack counts as full. Got %v, expected %v.", stackable->IsFullStack(), true);

	Log("****** Take objects");
	
	var limit = 5000;
	for (var i = 0; i < limit; ++i)
	{
		var taken = stackable->TakeObject();
		
		if (!taken || taken == stackable)
		{
			passed &= doTest("It should be possible to take %d objects from an infinite stack, got %d", limit, i);
			if (taken) taken->RemoveObject();
			break;
		}
		
		taken->RemoveObject();
	}

	Log("****** SetStackCount() sets the stack count to finite.");
	
	var max = 2147483647;
	stackable->SetStackCount(max);
	passed &= doTest("Object should not have an infinite stack count. Got %v, expected %v.", stackable->IsInfiniteStackCount(), false);
	passed &= doTest("Object should return the correct stack count. Got %d, expected %d.", stackable->GetStackCount(), max);
	
	Log("****** DoStackCount() does not set the stack count to finite.");
	
	stackable->SetInfiniteStackCount();
	passed &= doTest("Object should have an infinite stack count. Got %v, expected %v.", stackable->IsInfiniteStackCount(), true);

	stackable->DoStackCount(-1);
	passed &= doTest("Object should still have an infinite stack count. Got %d, expected %d.", stackable->IsInfiniteStackCount(), true);
	passed &= doTest("Get the stack count. Got %d, expected %d.", stackable->GetStackCount(), expected_infinite_count);

	Log("****** DoStackCount(-GetStackCount()) does not remove the object.");

	stackable->DoStackCount(-(stackable->GetStackCount()));
	passed &= doTest("Object should still exist. Got %v, expected %v.", !!stackable, true);
	if (stackable) passed &= doTest("Get the stack count. Got %d, expected %d.", stackable->GetStackCount(), expected_infinite_count);

	if (stackable) stackable->RemoveObject();
	// Get interaction menu amount

	return passed;
}
	

global func Test15_OnStart(int plr){ return true;}
global func Test15_OnFinished(){ return; }
global func Test15_Execute()
{
	Log("Test infinite stack count: Stacking");

	Log("****** Stack finite object onto an infinite object");

	var infinite = CreateObject(Arrow);
	infinite->SetInfiniteStackCount();
	var finite = CreateObject(Arrow);

	var passed = doTest("Get everything from the finite object. Got %d, expected %d.", infinite->Stack(finite), Arrow->InitialStackCount());
	passed &= doTest("The finite stack is not removed. Got %v, expected %v.", !!finite, true);
	passed &= doTest("The infinite object is still infinite. Got %v, expected %v.", infinite->IsInfiniteStackCount(), true);

	if (infinite) infinite->RemoveObject();
	if (finite) finite->RemoveObject();

	Log("****** Stack infinite object onto a finite object");

	infinite = CreateObject(Arrow);
	infinite->SetInfiniteStackCount();
	finite = CreateObject(Arrow);

	passed &= doTest("Get everything from the infinite object. Got %d, expected %d.", finite->Stack(infinite), 50);
	passed &= doTest("The infinite stack is not removed. Got %v, expected %v.", !!infinite, true);
	passed &= doTest("The finite object is now infinite. Got %v, expected %v.", finite->IsInfiniteStackCount(), true);

	if (infinite) infinite->RemoveObject();
	if (finite) finite->RemoveObject();

	Log("****** CanBeStackedWith() ?");
	
	var infinite_one = CreateObject(Arrow);
	infinite_one->SetInfiniteStackCount();
	var infinite_two = CreateObject(Arrow);
	infinite_two->SetInfiniteStackCount();
	var finite_one = CreateObject(Arrow);
	var finite_two = CreateObject(Arrow);

	passed &= doTest("Can stack infinite stack with itself? Got %v, expected %v.", infinite_one->CanBeStackedWith(infinite_one), true);
	passed &= doTest("Can stack infinite stack with infinite stack? Got %v, expected %v.", infinite_one->CanBeStackedWith(infinite_two), true);
	passed &= doTest("Can stack infinite stack with finite stack? Got %v, expected %v.", infinite_one->CanBeStackedWith(finite_one), false);

	passed &= doTest("Can stack finite stack with itself? Got %v, expected %v.", finite_one->CanBeStackedWith(finite_one), true);
	passed &= doTest("Can stack finite stack with infinite stack? Got %v, expected %v.", finite_one->CanBeStackedWith(infinite_one), false);
	passed &= doTest("Can stack finite stack with finite stack? Got %v, expected %v.", finite_one->CanBeStackedWith(finite_two), true);

	if (infinite_one) infinite_one->RemoveObject();
	if (infinite_two) infinite_two->RemoveObject();
	if (finite_one) finite_one->RemoveObject();
	if (finite_two) finite_two->RemoveObject();


	
	if (infinite) infinite->RemoveObject();

	return passed;
}

global func Test16_OnStart(int plr){ return true;}
global func Test16_OnFinished(){ return; }
global func Test16_Execute()
{		
	Log("Test infinite stack count: MergeWithStacksIn() with objects that contain an object with a useable extra slot");
	var container = CreateObject(Dummy);
	var bow = container->CreateContents(Bow);

	Log("****** MergeWithStacksIn() an infinite stack into an object");

	var infinite = CreateObject(Arrow);
	infinite->SetInfiniteStackCount();

	var passed = doTest("MergeWithStacksIn() an infinite stack into an object. The collection should not be handled by MergeWithStacksIn(). Got %v, expected %v.", infinite->MergeWithStacksIn(container), false);
	passed &= doTest("The function should not actually make an object enter the container. The container of the stack is %v, expected %v.", infinite->Contained(), nil);
	passed &= doTest("The stack count does not change. Got %v, expected %v", infinite->IsInfiniteStackCount(), true);

	infinite->RemoveObject();
	
	Log("****** MergeWithStacksIn() an infinite stack into an object that contains an infinite stack");

	infinite = CreateObject(Arrow);
	var other = CreateObject(Arrow);
	infinite->SetInfiniteStackCount();
	other->SetInfiniteStackCount();
	other->Enter(container);

	passed &= doTest("MergeWithStacksIn() an infinite stack into an object with an infinite stack. Got %v, expected %v.", infinite->MergeWithStacksIn(container), true);
	passed &= doTest("The added stack is removed. Got %v, expected %v.", infinite, nil);
	passed &= doTest("The stack count of the original stack does not change. Got %v, expected %v", other->IsInfiniteStackCount(), true);

	other->RemoveObject();
	if (infinite) infinite->RemoveObject();

	Log("****** MergeWithStacksIn() an infinite stack into an object that contains a container that contains an infinite stack");
	
	infinite= CreateObject(Arrow);
	other = CreateObject(Arrow);
	var ammo = CreateObject(Arrow);
	
	infinite->SetInfiniteStackCount();
	other->SetInfiniteStackCount();
	ammo->SetInfiniteStackCount();
	
	other->Enter(container);
	ammo->Enter(bow);
	
	passed &= doTest("Prerequisite: Other object is in %v, expected %v.", other->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), bow);

	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", infinite->MergeWithStacksIn(container), true);
	passed &= doTest("The object got removed. Got %v, expected %v.", infinite, nil);
	passed &= doTest("The stack inside the weapon inside the container is served. Got %v, expected %v.", ammo->IsInfiniteStackCount(), true);
	passed &= doTest("The stack inside the container is not served. Got %v, expected %v.", other->IsInfiniteStackCount(), true);

	other->RemoveObject();
	ammo->RemoveObject();
	if (infinite) infinite->RemoveObject();

	return passed;
}


global func Test17_OnStart(int plr){ return true;}
global func Test17_OnFinished(){ return; }
global func Test17_Execute()
{		
	Log("Test infinite stack count: MergeWithStacksIn() with objects that contain an object with a useable extra slot");
	var container = CreateObject(Dummy);
	var bow = container->CreateContents(Bow);
	
	Log("****** MergeWithStacksIn() an infinite stack into an object that contains a finite stack");

	var infinite = CreateObject(Arrow);
	var finite = CreateObject(Arrow);
	infinite->SetInfiniteStackCount();
	finite->Enter(container);

	var passed = doTest("MergeWithStacksIn() an infinite stack into an object with a finite stack. Got %v, expected %v.", infinite->MergeWithStacksIn(container), true);
	passed &= doTest("The added stack is removed. Got %v, expected %v.", infinite, nil);
	passed &= doTest("The original stack becomes infinite. Got %v, expected %v", finite->IsInfiniteStackCount(), true);

	finite->RemoveObject();
	if (infinite) infinite->RemoveObject();

	Log("****** MergeWithStacksIn() an infinite stack into an object inside a container that contains a finite stack");
	
	infinite = CreateObject(Arrow);
	finite = CreateObject(Arrow);
	var ammo = CreateObject(Arrow);
	
	infinite->SetInfiniteStackCount();
	
	finite->Enter(container);
	ammo->Enter(bow);
	
	passed &= doTest("Prerequisite: Finite object is in %v, expected %v.", finite->Contained(), container);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), bow);

	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", infinite->MergeWithStacksIn(bow), true);
	passed &= doTest("The object got removed. Got %v, expected %v.", infinite, nil);
	passed &= doTest("The stack inside the weapon inside the container is not infinite. Got %v, expected %v.", ammo->IsInfiniteStackCount(), true);
	passed &= doTest("The stack inside the container is infinite. Got %v, expected %v.", finite->IsInfiniteStackCount(), false);

	finite->RemoveObject();
	ammo->RemoveObject();
	if (infinite) infinite->RemoveObject();

	Log("****** MergeWithStacksIn() a finite stack into an object that contains an infinite stack");

	infinite = CreateObject(Arrow);
	finite = CreateObject(Arrow);
	infinite->SetInfiniteStackCount();
	infinite->Enter(container);

	passed &= doTest("MergeWithStacksIn() a finite stack into an object with an infinite stack. Got %v, expected %v.", finite->MergeWithStacksIn(container), true);
	passed &= doTest("The added stack is removed. Got %v, expected %v.", finite, nil);
	passed &= doTest("The original stack stays infinite. Got %v, expected %v", infinite->IsInfiniteStackCount(), true);

	if (finite) finite->RemoveObject();
	infinite->RemoveObject();

	Log("****** MergeWithStacksIn() a finite stack into an object inside a container that contains an infinite stack");
	
	infinite = CreateObject(Arrow);
	finite = CreateObject(Arrow);
	var ammo = CreateObject(Arrow);
	
	infinite->SetInfiniteStackCount();
	
	ammo->Enter(container);
	infinite->Enter(bow);
	
	passed &= doTest("Prerequisite: Infinite object is in %v, expected %v.", infinite->Contained(), bow);
	passed &= doTest("Prerequisite: Ammo object is in %v, expected %v.", ammo->Contained(), container);

	passed &= doTest("The entrance gets handled by MergeWithStacksIn(). Got %v, expected %v.", finite->MergeWithStacksIn(container), true);
	passed &= doTest("The object got removed. Got %v, expected %v.", finite, nil);
	passed &= doTest("The stack inside the weapon inside the container is infinite. Got %v, expected %v.", infinite->IsInfiniteStackCount(), true);
	passed &= doTest("The stack inside the container is not infinite. Got %v, expected %v.", ammo->IsInfiniteStackCount(), false);

	if (finite) finite->RemoveObject();
	ammo->RemoveObject();
	infinite->RemoveObject();

	return passed;
}


global func Test18_OnStart(int plr){ return true;}
global func Test18_OnFinished(){ return; }
global func Test18_Execute()
{
	Log("Test the use case #1 with infinite objects");

	Log("****** Clonk collects an infinite stack while having a incomplete finite stack in the inventory.");

	var crew = CreateObject(Clonk);
	var infinite = CreateObject(Arrow);
	infinite->SetInfiniteStackCount();
	var arrows = crew->CreateContents(Arrow);
	
	var diff = 5;
	arrows->SetStackCount(arrows->InitialStackCount() - diff);
	
	crew->Collect(infinite);
	// TODO: This was an old idea
	//var passed = doTest("Arrow object is not filled. Got %d, expected %d.", arrows->GetStackCount(), arrows->InitialStackCount() - diff);
	//passed &= doTest("Infinite object is not removed. Got %v, expected %v.", !!infinite, true);
	//if (infinite) passed &= doTest("Infinite object is in the clonk. Got %v, expected %v.", infinite->Contained(), crew);

	var passed = doTest("Arrow object is infinite. Got %v, expected %v.", arrows->IsInfiniteStackCount(), true);
	passed &= doTest("Infinite object is removed. Got %v, expected %v.", infinite, nil);

	if (infinite) infinite->RemoveObject();
	arrows->RemoveObject();
	crew->RemoveObject();

	Log("****** Clonk collects a finite stack while having an infinite stack in the inventory.");

	crew = CreateObject(Clonk);
	infinite = crew->CreateContents(Arrow);
	infinite->SetInfiniteStackCount();
	arrows = CreateObject(Arrow);

	arrows->SetStackCount(arrows->InitialStackCount() - diff);

	crew->Collect(arrows);
	passed &= doTest("Infinite stack stays infinite. Got %v, expected %v.", infinite->IsInfiniteStackCount(), true);
	passed &= doTest("Arrow object is removed. Got %v, expected %v.", arrows, nil);

	infinite->RemoveObject();
	if (arrows) arrows->RemoveObject();
	crew->RemoveObject();

	return passed;
}


global func Test19_OnStart(int plr){ return true;}
global func Test19_OnFinished(){ return; }
global func Test19_Execute()
{
	Log("Test the use case #2 with infinite objects");

	Log("****** Infinite stack collected by Clonk");

	var crew = CreateObject(Clonk);
	var infinite = CreateObject(Arrow);
	var arrows = crew->CreateContents(Arrow);
	var bow = crew->CreateContents(Bow);
	var ammo = bow->CreateContents(Arrow);

	var to_ammo = 5;
	var to_arrows = 2;
	var remaining = arrows->InitialStackCount() - to_ammo - to_arrows;
	
	ammo->SetStackCount(ammo->MaxStackCount() - to_ammo);
	arrows->SetStackCount(arrows->MaxStackCount() - to_arrows);
	infinite->SetInfiniteStackCount();

	var passed = doTest("Prerequisites: Ammo is in the bow. Got %v, expected %v.", ammo->Contained(), bow);
	passed &= doTest("Prerequisites: Arrows are in the crew member. Got %v, expected %v.", arrows->Contained(), crew);
	
	// TODO: this was the old idea - infinite object fills the contents, but stays where it is. Sort of like an infinite ammo dispenser
	// crew->Collect(infinite);
	// passed &= doTest("The arrow stack in the bow is filled. Got %d, expected %d.", ammo->GetStackCount(), ammo->MaxStackCount());
	// passed &= doTest("The arrow stack in the inventory is filled. Got %d, expected %d.", arrows->GetStackCount(), arrows->MaxStackCount());
	// if (infinite)
	// {
	// 	passed &= doTest("The arrow stack that was collected has the correct count. Got %d, expected %d.", infinite->GetStackCount(), remaining);
	// 	passed &= doTest("The arrow stack is in the crew member inventory. Got %v, expected %v.", infinite->Contained(), crew);
	// }
	// else
	// {
	// 	passed = false; Log("[Fail] The infinite stack was removed, but should not be removed");
	// }

	crew->Collect(infinite);
	passed &= doTest("The arrow stack in the bow is infinite. Got %v, expected %v.", ammo->IsInfiniteStackCount(), true);
	passed &= doTest("The arrow stack in the inventory is not filled. Got %d, expected %d.", arrows->GetStackCount(), 13);
	passed &= doTest("The arrow stack in the inventory is finite. Got %v, expected %v.", arrows->IsInfiniteStackCount(), false);
	passed &= doTest("The arrow stack that was collected is removed. Got %v, expected %v.", infinite, nil);

	ammo->RemoveObject();
	bow->RemoveObject();
	arrows->RemoveObject();
	if (infinite) infinite->RemoveObject();
	crew->RemoveObject();
	
	return passed;
}


global func Test20_OnStart(int plr){ return true;}
global func Test20_OnFinished(){ return; }
global func Test20_Execute()
{
	Log("Test the initialization of all stackable objects");
	
	var passed = true;
	var def;
	for (var i = 0; def = GetDefinition(i); ++i)
	{
		if (def->~IsStackable())
		{
			var instance = CreateObject(def);
			passed &= doTest(Format("Definition %i has the correct stack count after initialization? %s", def, "Got %d, expected %d."), instance->GetStackCount(), def->InitialStackCount());
			if (instance) instance->RemoveObject();
		}
	}
	return passed;
}


global func Test21_OnStart(int plr){ return true;}
global func Test21_OnFinished(){ return; }
global func Test21_Execute()
{
	Log("Test use case: Clonk gets liquid from a producer, the liquid the liquid should fill empty barrels in the Clonk, but not enter the Clonk.");

	var passed = true;

	var producer = CreateObject(Kitchen);
	var crew = CreateObject(Clonk);
	
	
	Log("****** Clonk tries to collect liquid without having a barrel");
	var liquid = producer->CreateContents(Water);
	liquid->SetStackCount(300);
	
	crew->Collect(liquid, true);
	
	passed &= doTest("Liquid is in the producer. Got %v, expected %v.", liquid->Contained(), producer);
	passed &= doTest("Liquid is not in the Clonk. Got %v, expected %v.", !!crew->FindContents(Water), false);
	
	if (liquid) liquid->RemoveObject();

	Log("****** Clonk tries to collect liquid with a barrel");
	
	var barrel = crew->CreateContents(Barrel);
	liquid = producer->CreateContents(Water);
	liquid->SetStackCount(600);
	
	crew->Collect(liquid, true);
	
	passed &= doTest("Liquid is in the producer. Got %v, expected %v.", liquid->Contained(), producer);
	passed &= doTest("Liquid is in the barrel. Got %v, expected %v.", !!barrel->FindContents(Water), true);
	passed &= doTest("Liquid is not in the Clonk. Got %v, expected %v.", !!crew->FindContents(Water), false);

	if (barrel) barrel->RemoveObject();
	if (liquid) liquid->RemoveObject();
	if (crew) crew->RemoveObject();
	if (producer) producer->RemoveObject();

	return passed;
}

global func Test22_OnStart(int plr){ return true;}
global func Test22_OnFinished(){ return; }
global func Test22_Execute()
{
	Log("Test stacking of objects with extra slots");

	var passed = true;

	Log("****** Blunderbusses with various contents");

	var blunderbuss_a = CreateObject(Blunderbuss);
	var blunderbuss_b = CreateObject(Blunderbuss);
	var blunderbuss_c = CreateObject(Blunderbuss);
	var blunderbuss_d = CreateObject(Blunderbuss);
	var blunderbuss_e = CreateObject(Blunderbuss);
	
	blunderbuss_a->CreateContents(LeadBullet);
	blunderbuss_b->CreateContents(LeadBullet);
	blunderbuss_c->CreateContents(LeadBullet)->SetStackCount(7);
	blunderbuss_d->CreateContents(LeadBullet)->SetInfiniteStackCount();

	passed &= doTest("Blunderbusses with 8 shots can be stacked with blunderbuss with 8 shots. Got %v, expected %v.", blunderbuss_a->CanBeStackedWith(blunderbuss_b), true);
	passed &= doTest("Blunderbusses with 8 shots cannot be stacked with blunderbuss with 7 shots. Got %v, expected %v.", blunderbuss_a->CanBeStackedWith(blunderbuss_c), false);
	passed &= doTest("Blunderbusses with 8 shots cannot be stacked with blunderbuss with infinite shots. Got %v, expected %v.", blunderbuss_a->CanBeStackedWith(blunderbuss_d), false);
	passed &= doTest("Blunderbusses with 8 shots cannot be stacked with empty blunderbuss. Got %v, expected %v.", blunderbuss_a->CanBeStackedWith(blunderbuss_e), false);

	passed &= doTest("Blunderbusses with 7 shots cannot be stacked with blunderbuss with 8 shots. Got %v, expected %v.", blunderbuss_e->CanBeStackedWith(blunderbuss_a), false);
	passed &= doTest("Blunderbusses with infinite shots cannot be stacked with blunderbuss with 8 shots. Got %v, expected %v.", blunderbuss_d->CanBeStackedWith(blunderbuss_a), false);
	passed &= doTest("Empty blunderbuss cannot be stacked with blunderbuss with 8 shots. Got %v, expected %v.", blunderbuss_e->CanBeStackedWith(blunderbuss_a), false);

	if (blunderbuss_a) blunderbuss_a->RemoveObject();
	if (blunderbuss_b) blunderbuss_b->RemoveObject();
	if (blunderbuss_c) blunderbuss_c->RemoveObject();
	if (blunderbuss_d) blunderbuss_d->RemoveObject();
	if (blunderbuss_e) blunderbuss_e->RemoveObject();

	Log("****** Grenade launcher with various contents");
	
	var grenade_launcher_a = CreateObject(GrenadeLauncher);
	var grenade_launcher_b = CreateObject(GrenadeLauncher);
	var grenade_launcher_c = CreateObject(GrenadeLauncher);
	
	grenade_launcher_a->CreateContents(Dynamite);
	grenade_launcher_b->CreateContents(Dynamite);
	grenade_launcher_c->CreateContents(IronBomb);

	passed &= doTest("Grenade launcher with dynamite can be stacked with the grenade launcher with dynamite. Got %v, expected %v.", grenade_launcher_a->CanBeStackedWith(grenade_launcher_b), true);
	passed &= doTest("Grenade launcher with dynamite cannot be stacked with grenade launcher with iron bomb. Got %v, expected %v.", grenade_launcher_a->CanBeStackedWith(grenade_launcher_c), false);

	if (grenade_launcher_a) grenade_launcher_a->RemoveObject();
	if (grenade_launcher_b) grenade_launcher_b->RemoveObject();
	if (grenade_launcher_c) grenade_launcher_c->RemoveObject();
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
