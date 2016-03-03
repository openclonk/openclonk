/**
	Liquid Container
	Unit tests for the containers that can store liquid.
	
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
	Log("Test the behaviour of IsLiquidContainerForMaterial");

	// a loop would be cool, but that would work only with runtime overloadable functions
	var container = CreateObject(Barrel);
	
	var test1 = container->IsLiquidContainerForMaterial("Water");
	var test2 = !container->IsLiquidContainerForMaterial("Sky");
	var test3 = !container->IsLiquidContainerForMaterial();
	
	Log("- Container returns 'true' if liquid parameter is correct: %v", test1);
	Log("- Container returns 'false' if liquid parameter is incorrect: %v", test2);
	Log("- Container returns 'false' if liquid parameter is nil: %v", test3);
	
	container->RemoveObject();
	return test1 && test2 && test3;
}

global func Test2_OnStart(int plr){ return true;}
global func Test2_OnFinished(){ return; }
global func Test2_Execute()
{
	Log("Test the behaviour of liquid objects entering liquid containers");

	var container = CreateObject(Barrel);
	var liquid = CreateObject(Liquid_Water);
	
    // -----

	Log("Can fill empty barrel with the liquid");
	liquid->SetStackCount(100);
	liquid->Enter(container);
	
	var passed = true;
	var returned = container->Contents();
	var test = (returned == liquid); passed &= test;
	Log("- Liquid can fill empty barrel: %v", test);
	returned = container->GetLiquidAmount("Water");
	test = (100 == returned); passed &= test;
	Log("- Barrel contains %d units, expected %d: %v", returned, 100, test);
	returned = container->Contents()->MaxStackCount();
	test = (300 == returned); passed &= test;
	Log("- The liquid returns a max stack count of %d, expected %d: %v", returned, 300, test);
	
    // -----

	Log("Can fill barrel with more liquid, liquid object gets removed");
	liquid = CreateObject(Liquid_Water);
	liquid->SetStackCount(100);
	liquid->Enter(container);

	test = (liquid == nil); passed &= test;
	Log("- Liquid can enter filled barrel, liquid got removed: %v", test);
	returned = container->GetLiquidAmount();
	test = (200 == returned); passed &= test;
	Log("- Barrel contains %d units, expected %d: %v", returned, 200, test);

    // -----

	Log("Cannot fill in more than the allowed amount");
	liquid = CreateObject(Liquid_Water);
	liquid->SetStackCount(200);
	liquid->Enter(container);
	
	returned = liquid->Contained();
	test = (returned == nil); passed &= test;
	Log("- Liquid cannot enter filled barrel if the capacity is exceeded: %v", test);
	returned = container->ContentsCount();
	test = (1 == returned); passed &= test;
	Log("- Barrel contains %d items, expected %d: %v", returned, 1, test);
	returned = container->GetLiquidAmount();
	test = (300 == returned); passed &= test;
	Log("- Barrel does increase fill level, up to the allowed amount, contains %d units, expected %d: %v", returned, 300, test);
	returned = liquid->GetLiquidAmount();
	test = (100 == returned); passed &= test;
	Log("- Liquid object still contains %d units, expected %d: %v", returned, 100, test);

	Log("- Resetting liquid amount to 0");
	liquid->RemoveObject();
	container->Contents()->RemoveObject();

    // -----

	Log("Cannot fill in empty barrel and empty liquid object partially");
	liquid = CreateObject(Liquid_Water);
	liquid->SetStackCount(500);
	liquid->Enter(container);
	
	returned = liquid->Contained();
	test = (returned == nil); passed &= test;
	Log("- Liquid cannot enter empty barrel if the capacity is exceeded: %v", test);
	returned = container->GetLiquidAmount();
	test = (300 == returned); passed &= test;
	Log("- Barrel does increase fill level, up to the allowed amount, contains %d units, expected %d: %v", returned, 300, test);
	returned = liquid->GetLiquidAmount();
	test = (200 == returned); passed &= test;
	Log("- Liquid object still contains %d units, expected %d: %v", returned, 200, test);

	Log("- Resetting liquid amount to 200");
	liquid->RemoveObject();
	container->Contents()->SetStackCount(200);

    // -----

	Log("Cannot fill in a different liquid");
	liquid = CreateObject(Liquid_Oil);
	liquid->SetStackCount(50);
	liquid->Enter(container);
	
	returned = liquid->Contained();
	test = (returned == nil); passed &= test;
	Log("- Liquid cannot enter filled barrel of a different liquid type: %v", test);
	returned = container->GetLiquidAmount();
	test = (200 == returned); passed &= test;
	Log("- Barrel does not increase fill level, contains %d units, expected %d: %v", returned, 200, test);

	liquid->RemoveObject();
	
    // -----

	Log("Barrel gets emptied when liquid exits it");
	liquid = container->Contents();
	liquid->Exit();
	
	returned = container->Contents();
	test = (returned == nil); passed &= test;
	Log("- Liquid container should be empty when liquid leaves it: %v", test);
	returned = container->GetLiquidAmount();
	test = (returned == 0); passed &= test;
	Log("- Liquid container return a liquid amount of 0 when liquid leaves it: %v", test);
	test = (liquid != nil); passed &= test;
	Log("- Liquid exists after leaving the container: %v", test);
	
	liquid->RemoveObject();
	container->RemoveObject();

	return passed;
}


global func Test3_OnStart(int plr){ return true;}
global func Test3_OnFinished(){ return; }
global func Test3_Execute()
{
	Log("Test the behaviour of PutLiquid");

	var container = CreateObject(Barrel);
	var passed = true;
	
    // -----

	Log("Incompatible material");
	var test = (container->PutLiquid("Lava", 1, nil) == 0);
	passed &= test;
	Log("- Container returns '0' when inserting 1 pixel of incompatible material: %v", test);
	test = container->Contents() == nil; passed &= test;
	Log("- Container returns 'nil' for contents: %v, %v", test, container->Contents());

    // -----

	Log("Compatible material");
	test = (container->PutLiquid("Water", 1, nil) == 1);
	Log("- Container returns '1' when inserting 1 pixel of compatible material: %v", test);
	test = container->FindContents(Liquid_Water) != nil; passed &= test;
	Log("- Container has contents Liquid_Water when inserting 1 pixel of compatible material: %v", test);
	if (passed)
	{
		test = container->FindContents(Liquid_Water)->GetLiquidAmount() == 1; passed &= test;
		Log("- Container returns the fill level 1 when inserting 1 pixel of compatible material: %d, %v", container->FindContents(Liquid_Water)->GetLiquidAmount(), test);
	}
	
	var returned = container->PutLiquid("Water", container->GetLiquidContainerMaxFillLevel(), nil);
	var expected = (container->GetLiquidContainerMaxFillLevel() - 1);
	test = (returned == expected);
	passed &= test;
	Log("- Container returns 'the actually inserted amount of material' %d when inserting more than the volume, returned %d: %v", expected, returned, test);
	returned = container->GetLiquidAmount("Water");
	expected = container->GetLiquidContainerMaxFillLevel();
	test = (returned == expected); passed &= test;
	Log("- Container returns the fill level %d, expected %d, when inserting more than the volume: %v", returned, expected, test);

	container->RemoveObject();
	return passed;
}

global func Test4_OnStart(int plr){ return true;}
global func Test4_OnFinished(){ return; }
global func Test4_Execute()
{
	Log("Test the behaviour of RemoveLiquid");

	var container = CreateObject(Barrel);
	var passed = true;
	
	container->PutLiquid("Water", 100);

    // -----

	Log("Incompatible material");
	var returned = container->RemoveLiquid("Lava", 0, nil);
	var expected = ["Lava", 0];
	var test = (returned[0] == expected[0]);
	passed &= test;
	Log("- Container returns the requested material (was %s, expected %s) when removing incompatible material: %v", returned[0], expected[0], test);
	test = (returned[1] == expected[1]); passed &= test;
	Log("- Container returns no amount when removing incompatible material (was %d, expected %d) : %v", returned[1], expected[1], test);
	returned = container->GetLiquidAmount();
	expected = 100;
	test = (returned == expected); passed &= test;
	Log("- Container contents do not change when removing incompatible material (was %d, expected %d): %v", returned, expected, test);

    // -----

	Log("Compatible material");
	returned = container->RemoveLiquid("Water", 1, nil);
	expected = ["Water", 1];
	test = (returned[0] == expected[0]);
	Log("- Container returns the extracted material name (was %s, expected %s): %v", returned[0], expected[0], test);
	test = returned[1] == expected[1]; passed &= test;
	Log("- Container returns the correct amount (was %d, expected %d) when removing 1 pixel of compatible material: %v", returned[1], expected[1], test);
	returned = container->GetLiquidAmount();
	expected = 99;
	test = (returned == expected); passed &= test;
	Log("- Container contents do change when removing compatible material (was %d, expected %d): %v", returned, expected, test);
	
	returned = container->RemoveLiquid("Water", 100, nil);
	expected = ["Water", 99];
	test = (returned[0] == expected[0]);
	Log("- Container returns the extracted material name (was %s, expected %s): %v", returned[0], expected[0], test);
	test = returned[1] == expected[1]; passed &= test;
	Log("- Container returns the correct amount (was %d, expected %d) when removing compatible material: %v", returned[1], expected[1], test);
	returned = container->GetLiquidAmount();
	expected = 0;
	test = (returned == expected); passed &= test;
	Log("- Container contents do change when removing compatible material (was %d, expected %d): %v", returned, expected, test);

    // -----

	Log("Request everything");
	container->PutLiquid("Oil", 100);
	
	returned = container->RemoveLiquid(nil, 50, nil);
	expected = ["Oil", 50];
	test = (returned[0] == expected[0]);
	Log("- Container returns the contained material (was %s, expected %s) when extracting material 'nil': %v", returned[0], expected[0], test);
	test = returned[1] == expected[1]; passed &= test;
	Log("- Container returns the correct amount (was %d, expected %d) when removing compatible material: %v", returned[1], expected[1], test);
	returned = container->GetLiquidAmount();
	expected = 50;
	test = (returned == expected); passed &= test;
	Log("- Container contents do change when removing compatible material (was %d, expected %d): %v", returned, expected, test);

	container->PutLiquid("Oil", 50);

	returned = container->RemoveLiquid("Oil", nil, nil);
	expected = ["Oil", 100];
	test = (returned[0] == expected[0]);
	Log("- Container returns the contained material (was %s, expected %s) when extracting amount 'nil': %v", returned[0], expected[0], test);
	test = returned[1] == expected[1]; passed &= test;
	Log("- Container returns the contained amount (was %d, expected %d) when extracting amount 'nil': %v", returned[1], expected[1], test);
	returned = container->GetLiquidAmount();
	expected = 0;
	test = (returned == expected); passed &= test;
	Log("- Container is empty after removing amount 'nil' (was %d, expected %d): %v", returned, expected, test);

	container->PutLiquid("Oil", 100);

	returned = container->RemoveLiquid(nil, nil, nil);
	expected = ["Oil", 100];
	test = (returned[0] == returned[0]);
	Log("- Container returns the contained material (was %s, expected %s) when extracting material and amount 'nil': %v", returned[0], expected[0], test);
	test = returned[1] == expected[1]; passed &= test;
	Log("- Container returns the contained amount (was %d, expected %d) when extracting material and amount 'nil': %v", returned[1], expected[1], test);
	returned = container->GetLiquidAmount();
	expected = 0;
	test = (returned == expected); passed &= test;
	Log("- Container is empty after removing amount material and amount 'nil' (was %d, expected %d): %v", returned, expected, test);

	container->RemoveObject();
	return passed;
}





global func Test10_OnStart(int plr)
{
	var effect = GetEffect("IntTestControl", nil);

	effect.pump = CreateObjectAbove(Pump, 100, 200);
	effect.engine = CreateObjectAbove(SteamEngine, 150, 200);
	return true;
}

global func Test10_Execute()
{
	var effect = GetEffect("IntTestControl", nil);
	
	Log("Test the behaviour of connections between pipe and pump");

	var passed = true;
	var pipeA, pipeB;
	
	Log("No connection");
	passed &= Test10_CheckConnections(effect, effect.pump, effect.pump);

	Log("1. Connecting pipe A to pump, pipe B to pump, pipe B to engine");
	pipeA = CreateObject(Pipe);
	pipeB = CreateObject(Pipe);

	pipeA->ConnectPipeTo(effect.pump);
	passed &= Test10_CheckConnections(effect, pipeA, effect.pump);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Source, pipeB, PIPE_STATE_Neutral);
	pipeB->ConnectPipeTo(effect.pump);
	passed &= Test10_CheckConnections(effect, pipeA, pipeB);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Source, pipeB, PIPE_STATE_Drain);
	pipeB->ConnectPipeTo(effect.engine);
	
	pipeA->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.engine);
	
	pipeA->RemoveObject();
	pipeB->RemoveObject();

	Log("2. Connecting pipe A to pump, pipe B to engine, pipe B to pump");
	
	pipeA = CreateObject(Pipe);
	pipeB = CreateObject(Pipe);

	pipeA->ConnectPipeTo(effect.pump);
	passed &= Test10_CheckConnections(effect, pipeA, effect.pump);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Source, pipeB, PIPE_STATE_Neutral);
	pipeB->ConnectPipeTo(effect.engine);
	passed &= Test10_CheckConnections(effect, pipeA, effect.pump);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Source, pipeB, PIPE_STATE_Neutral);
	pipeB->ConnectPipeTo(effect.pump);
	passed &= Test10_CheckConnections(effect, pipeA, effect.engine);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Source, pipeB, PIPE_STATE_Drain);
	
	pipeA->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.engine);
	
	pipeA->RemoveObject();
	pipeB->RemoveObject();

	Log("3. Connecting pipe A to engine, pipe A to pump, pipe B to pump");

	pipeA = CreateObject(Pipe);
	pipeB = CreateObject(Pipe);

	pipeA->ConnectPipeTo(effect.engine);
	passed &= Test10_CheckConnections(effect, effect.pump, effect.pump);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Neutral, pipeB, PIPE_STATE_Neutral);
	pipeA->ConnectPipeTo(effect.pump);
	passed &= Test10_CheckConnections(effect, effect.pump, effect.engine);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Drain, pipeB, PIPE_STATE_Neutral);
	pipeB->ConnectPipeTo(effect.pump);
	passed &= Test10_CheckConnections(effect, pipeB, effect.engine);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Drain, pipeB, PIPE_STATE_Source);
	
	pipeA->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.engine);
	
	pipeA->RemoveObject();
	pipeB->RemoveObject();
	
	Log("4. Connecting pipe A to pump (drain via menu), pipe B to pump, pipe A to engine");
	
	pipeA = CreateObject(Pipe);
	pipeB = CreateObject(Pipe);

	pipeA->ConnectPipeTo(effect.pump, PIPE_STATE_Drain);
	passed &= Test10_CheckConnections(effect, effect.pump, pipeA);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Drain, pipeB, PIPE_STATE_Neutral);
	pipeB->ConnectPipeTo(effect.pump);
	passed &= Test10_CheckConnections(effect, pipeB, pipeA);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Drain, pipeB, PIPE_STATE_Source);
	pipeA->ConnectPipeTo(effect.engine);
	passed &= Test10_CheckConnections(effect, pipeB, effect.engine);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Drain, pipeB, PIPE_STATE_Source);
	
	pipeA->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.engine);
	
	pipeA->RemoveObject();
	pipeB->RemoveObject();

	Log("5. Connecting pipe A to pump (source), pipe A to engine");
	
	pipeA = CreateObject(Pipe);

	pipeA->ConnectPipeTo(effect.pump, PIPE_STATE_Source);
	passed &= Test10_CheckConnections(effect, pipeA, effect.pump);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Source, nil, nil);
	pipeA->ConnectPipeTo(effect.engine);
	passed &= Test10_CheckConnections(effect, pipeA, effect.pump);
	passed &= Test10_CheckPipes(pipeA, PIPE_STATE_Source, nil, nil);
	
	pipeA->CutLineConnection(effect.pump);
	pipeA->RemoveObject();

	return passed;
}

global func Test10_CheckConnections(proplist effect, object expected_source, object expected_drain)
{
	var passed = true;
	var returned = effect.pump->GetSourceObject();
	var test = returned == expected_source ; passed &= test;
	Log("- Pump returns source object %v: %v (returned %v)", expected_source, test, returned);
	returned = effect.pump->GetDrainObject();
	test = returned == expected_drain ; passed &= test;
	Log("- Pump returns drain object %v: %v (returned %v)", expected_drain, test, returned);
	return passed;
}

global func Test10_CheckPipes(object pipeA, string stateA, object pipeB, string stateB)
{
	var functionA, functionB;
	var passed = true;

	if (pipeA != nil)
	{
		     if (stateA == PIPE_STATE_Source)  functionA = pipeA.IsSourcePipe;
		else if (stateA == PIPE_STATE_Drain)   functionA = pipeA.IsDrainPipe;
		else if (stateA == PIPE_STATE_Neutral) functionA = pipeA.IsNeutralPipe;
		
		var test = pipeA->Call(functionA);
		passed &= test;
		Log("- Pipe A is %s pipe: %v", stateA, test);
	}
	
	if (pipeB != nil)
	{
		     if (stateB == PIPE_STATE_Source)  functionB = pipeB.IsSourcePipe;
		else if (stateB == PIPE_STATE_Drain)   functionB = pipeB.IsDrainPipe;
		else if (stateB == PIPE_STATE_Neutral) functionB = pipeB.IsNeutralPipe;
	
	
		test = pipeB->Call(functionB);
		passed &= test;
		Log("- Pipe B is %s pipe: %v", stateB, test);
	}
	return passed;
}

global func Test10_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Pump), Find_ID(SteamEngine), Find_ID(Pipe)));
	return true;
}

global func Test11_OnStart(int plr){ return true;}
global func Test11_OnFinished(){ return; }
global func Test11_Execute()
{
	Log("Test the behaviour of barrels getting stacked");

	var container1 = CreateObject(Barrel);
	var container2 = CreateObject(Barrel);
	
	// can stack filled barrel with other filled barrel of the same liquid
	container1->CreateContents(Liquid_Water, 100);
	container2->CreateContents(Liquid_Water, 300);

	var passed = true;
	var returned = container1->CanBeStackedWith(container2);
	var test = returned == true; passed &= test;
	Log("- Barrel can be stacked with other barrel that contains the same liquid: %v", test);
	returned = container2->CanBeStackedWith(container1);
	test = returned == true; passed &= test;
	Log("- Barrel can be stacked with other barrel that contains the same liquid: %v", test);

	// cannot stack filled barrel with other empty barrel
	container1->Contents()->SetStackCount(100);
	container2->Contents()->RemoveObject();

	returned = container1->CanBeStackedWith(container2);
	test = returned == false; passed &= test;
	Log("- Filled barrel cannot be stacked with empty barrel: %v", test);
	returned = container2->CanBeStackedWith(container1);
	test = returned == false; passed &= test;
	Log("- Empty barrel cannot be stacked with filled barrel: %v", test);

	// can stack empty barrel with other empty barrel
	container1->Contents()->RemoveObject();

	returned = container1->CanBeStackedWith(container2);
	test = returned == true; passed &= test;
	Log("- Empty barrel can be stacked with empty barrel: %v", test);

	// cannot stack filled barrel with other filled barrel of different liquid
	container1->CreateContents(Liquid_Water, 100);
	container2->CreateContents(Liquid_Oil, 100);

	returned = container1->CanBeStackedWith(container2);
	test = returned == false; passed &= test;
	Log("- Liquid A barrel cannot be stacked with liquid B barrel: %v", test);
	returned = container2->CanBeStackedWith(container1);
	test = returned == false; passed &= test;
	Log("- Liquid B barrel cannot be stacked with liquid A barrel: %v", test);
	
	container1->RemoveObject();
	container2->RemoveObject();

	return passed;
}