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
	Log("Test the behaviour of GetFillLevel and SetFillLevel");

	var container = CreateObject(Barrel);
	var passed = true;
	var test_data = [nil, -1, 0, 1, container->GetLiquidContainerMaxFillLevel()/2, container->GetLiquidContainerMaxFillLevel(), container->GetLiquidContainerMaxFillLevel() + 1];
	
	for (var value in test_data)
	{
		var expected_value = value;
		container->SetLiquidFillLevel(value);
		var returned = container->GetLiquidFillLevel();
		if (value == nil || value == -1) expected_value = 0; // accept 0 as a return value in this case.
		var test = (expected_value == returned); passed &= test;
		Log("- Container returns %d (expected %d) if fill level is set to %d, values should be equal: %v", returned, expected_value, value, test);
	}
	
	container->RemoveObject();
	return passed;
}


global func Test3_OnStart(int plr){ return true;}
global func Test3_OnFinished(){ return; }
global func Test3_Execute()
{
	Log("Test the behaviour of GetLiquidType and SetLiquidType");

	var container = CreateObject(Barrel);
	var passed = true;
	var test_data = [nil, "Water", "Lava", "123", "#24942fwijvri"];
	// set a special test function that accepts other material, too
	container.IsLiquidContainerForMaterial = Barrel.Test3_IsLiquidContainerForMaterial;
	
	for (var value in test_data)
	{
		container->SetLiquidType(value);
		var returned = container->GetLiquidType();
		var test = (value == returned); passed &= test;
		Log("- Container returns %s if liquid name is set to %s, values should be equal", returned, value);
	}
	
	container->RemoveObject();
	return passed;
}





global func Test4_OnStart(int plr){ return true;}
global func Test4_OnFinished(){ return; }
global func Test4_Execute()
{
	Log("Test the behaviour of LiquidContainerIsEmpty");

	// a loop would be cool, but that would work only with runtime overloadable functions
	var container = CreateObject(Barrel);
	Log("Max fill level for container is %d", container->GetLiquidContainerMaxFillLevel());
	
	container->SetLiquidFillLevel(0);
	var test1 = container->LiquidContainerIsEmpty();
	Log("- Container fill level: %v", container->GetLiquidFillLevel());
	Log("- Container returns 'true' if liquid fill level is 0: %v", test1);

	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel() / 2);
	var test2 = container->LiquidContainerIsEmpty();
	Log("- Container fill level: %v", container->GetLiquidFillLevel());
	Log("- Container returns 'false' if liquid fill level is 50%: %v", !test2);

	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel());
	var test3 = container->LiquidContainerIsEmpty();
	Log("- Container fill level: %v", container->GetLiquidFillLevel());
	Log("- Container returns 'false' if liquid fill level is 100%: %v", !test3);

	container->RemoveObject();
	return test1 && !test2 && !test3;
}

global func Test5_OnStart(int plr){ return true;}
global func Test5_OnFinished(){ return; }
global func Test5_Execute()
{
	Log("Test the behaviour of LiquidContainerIsFull");

	// a loop would be cool, but that would work only with runtime overloadable functions
	var container = CreateObject(Barrel);

	container->SetLiquidFillLevel(0);
	var test1 = !container->LiquidContainerIsFull();

	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel() / 2);
	var test2 = !container->LiquidContainerIsFull();

	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel());
	var test3 = container->LiquidContainerIsFull();
	
	Log("- Container returns 'false' if liquid fill level is 0: %v", test1);
	Log("- Container returns 'false' if liquid fill level is 50%: %v", test2);
	Log("- Container returns 'true' if liquid fill level is 100%: %v", test3);

	container->RemoveObject();
	return test1 && test2 && test3;
}


global func Test6_OnStart(int plr){ return true;}
global func Test6_OnFinished(){ return; }
global func Test6_Execute()
{
	Log("Test the behaviour of LiquidContainerAccepts");

	var container = CreateObject(Barrel);
	var passed = true;
	
	// incompatible material
	
	var test = !container->LiquidContainerAccepts("Dummy"); passed &= test;
	Log("- Container returns 'false' if material is wrong: %v", test);

	// fill level

	container->SetLiquidFillLevel(0);
	test = container->LiquidContainerAccepts("Water");	passed &= test;
	Log("- Container returns 'true' if liquid fill level is 0% and material is ok: %v", test);

	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel() / 2);
	test = !container->LiquidContainerAccepts("Water");	passed &= test;
	Log("- Container returns 'false' if liquid fill level is 50% and contained material is 'nil': %v", test);
	
	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel());
	test = !container->LiquidContainerAccepts("Water");	passed &= test;
	Log("- Container returns 'false' if liquid fill level is 100% and material is ok: %v", test);

 	// material
 	Log("Setting container to be filled with a material");
 	container->SetLiquidType("Oil");
 	Log("- Fill material is %s", container->GetLiquidType());

	container->SetLiquidFillLevel(0);
 	container->SetLiquidType("Oil");
	test = container->LiquidContainerAccepts("Water");	passed &= test;
	Log("- Container returns 'true' if filled with material and liquid fill level is 0% and other material is ok: %v", test);

	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel() / 2);
 	container->SetLiquidType("Oil");
	test = !container->LiquidContainerAccepts("Water");	passed &= test;
	Log("- Container returns 'false' if filled with material and liquid fill level is 50% and other material is ok: %v", test);
	
	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel() / 2);
 	container->SetLiquidType("Water");
	test = container->LiquidContainerAccepts("Water");	passed &= test;
	Log("- Container returns 'true' if liquid fill level is 50% and material is ok: %v", test);

	container->RemoveObject();
	return passed;
}

global func Test7_OnStart(int plr){ return true;}
global func Test7_OnFinished(){ return; }
global func Test7_Execute()
{
	Log("Test the behaviour of PutLiquid");

	var container = CreateObject(Barrel);
	var passed = true;
	
	// incompatible material
	var test = (container->PutLiquid("Lava", 1, nil) == 0);
	passed &= test;
	Log("- Container returns '0' when inserting 1 pixel of incompatible material: %v", test);
	test = container->GetLiquidType() == nil; passed &= test;
	Log("- Container returns 'nil' for material name: %v, %v", test, container->GetLiquidType());
	test = container->GetLiquidFillLevel() == 0; passed &= test;
	Log("- Container returns '0' for fill level: %v", test);

	// compatible material
	test = (container->PutLiquid("Water", 1, nil) == 1);
	Log("- Container returns '1' when inserting 1 pixel of compatible material: %v", test);
	test = container->GetLiquidType() == "Water"; passed &= test;
	Log("- Container returns the liquid name when inserting 1 pixel of compatible material: %v", test);
	test = container->GetLiquidFillLevel() == 1; passed &= test;
	Log("- Container returns the fill level 1 when inserting 1 pixel of compatible material: %d, %v", container->GetLiquidFillLevel(), test);
	
	test = (container->PutLiquid("Water", container->GetLiquidContainerMaxFillLevel(), nil)  == (container->GetLiquidContainerMaxFillLevel() - 1));
	passed &= test;
	Log("- Container returns 'the actually inserted material' when inserting more than the volume: %v", test);
	test = container->GetLiquidFillLevel() == container->GetLiquidContainerMaxFillLevel(); passed &= test;
	Log("- Container returns the fill level when inserting more than the volume: %v", test);

	container->RemoveObject();
	return passed;
}

global func Test8_OnStart(int plr){ return true;}
global func Test8_OnFinished(){ return; }
global func Test8_Execute()
{
	Log("Test the behaviour of RemoveLiquid");

	var container = CreateObject(Barrel);
	var passed = true;
	
	container->SetLiquidContainer("Water", 100);
	
	// incompatible material
	var returned = container->RemoveLiquid("Lava", 0, nil);
	var test = (returned[0] == "Water");
	passed &= test;
	Log("- Container returns the contained material when removing incompatible material: %v", test);
	test = (returned[1] == 0); passed &= test;
	Log("- Container returns no amount when removing incompatible material: %v", test);
	test = (container->GetLiquidFillLevel() == 100);
	Log("- Container contents do not change when removing incompatible material: %v", test);

	// compatible material
	returned = container->RemoveLiquid("Water", 1, nil);
	test = (returned[0] == "Water");
	Log("- Container returns the extracted material name: %v", test);
	test = returned[1] == 1; passed &= test;
	Log("- Container returns the correct amount when removing 1 pixel of compatible material: %v", test);
	test = (container->GetLiquidFillLevel() == 99);
	Log("- Container contents do change when removing compatible material: %v", test);
	
	returned = container->RemoveLiquid("Water", 100, nil);
	test = (returned[0] == "Water");
	Log("- Container returns the extracted material name: %v", test);
	test = returned[1] == 99; passed &= test;
	Log("- Container returns the correct amount when removing compatible material: %v", test);
	test = (container->GetLiquidFillLevel() == 0);
	Log("- Container contents do change when removing compatible material: %v", test);

	// request everything
	var material_alternative = "Oil";
	container->SetLiquidContainer(material_alternative, 100);
	
	returned = container->RemoveLiquid(nil, 50, nil);
	test = (returned[0] == material_alternative);
	Log("- Container returns the contained material when extracting material 'nil': %v", test);
	test = returned[1] == 50; passed &= test;
	Log("- Container returns the correct amount when removing compatible material: %v", test);
	test = (container->GetLiquidFillLevel() == 50);
	Log("- Container contents do change when removing compatible material: %v", test);

	container->SetLiquidContainer(material_alternative, 100);

	returned = container->RemoveLiquid(material_alternative, nil, nil);
	test = (returned[0] == material_alternative);
	Log("- Container returns the contained material when extracting amount 'nil': %v", test);
	test = returned[1] == 100; passed &= test;
	Log("- Container returns the contained amount when extracting amount 'nil': %v", test);
	test = (container->GetLiquidFillLevel() == 0);
	Log("- Container is empty after removing amount 'nil': %v", test);

	container->SetLiquidContainer(material_alternative, 100);

	returned = container->RemoveLiquid(nil, nil, nil);
	test = (returned[0] == material_alternative);
	Log("- Container returns the contained material when extracting material and amount 'nil': %v", test);
	test = returned[1] == 100; passed &= test;
	Log("- Container returns the contained amount when extracting material and amount 'nil': %v", test);
	test = (container->GetLiquidFillLevel() == 0);
	Log("- Container is empty after removing amount material and amount 'nil': %v", test);

	container->RemoveObject();
	return passed;
}


global func Test9_OnStart(int plr){ return true;}
global func Test9_OnFinished(){ return; }
global func Test9_Execute()
{
	Log("Test the behaviour of SetLiquidFillLevel and SetLiquidType in combination");

	var container = CreateObject(Barrel);
	var passed = true;
	
	var liquid = "Water";
	container->SetLiquidType(liquid);
	var returned = container->GetLiquidType();
	var test = (liquid == returned); passed &= test;
	Log("- Container returns %s if liquid name is set to %s, values should be equal", returned, liquid);
	
	var level = 0;
	returned = container->GetLiquidFillLevel();
	test = (level == returned); passed &= test;
	Log("- Container returns %d, expected %d, values should be equal", returned, level);

	// ----
	Log("- Changing fill level now");

	level = 100;
	container->SetLiquidFillLevel(level);
	returned = container->GetLiquidFillLevel();
	test = (level == returned); passed &= test;
	Log("- Container returns %d if liquid level is set to %d, values should be equal", returned, level);

	returned = container->GetLiquidType();
	test = (liquid == returned); passed &= test;
	Log("- Container returns %s, expected %s, values should not change if level changes", returned, liquid);
	
	// ----
	Log("Changing liquid now");

	liquid = "Oil";
	container->SetLiquidType(liquid);
	returned = container->GetLiquidType();
	test = (liquid == returned); passed &= test;
	Log("- Container returns %s if liquid name is set to %s, values should be equal", returned, liquid);

	returned = container->GetLiquidFillLevel();
	test = (level == returned); passed &= test;
	Log("- Container returns %d, expected %d, values should not change if liquid changes", returned, level);
	
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
	container1->SetLiquidContainer("Water", 100);
	container2->SetLiquidContainer("Water", 300);

	var passed = true;
	var returned = container1->CanBeStackedWith(container2);
	var test = returned == true; passed &= test;
	Log("- Barrel can be stacked with other barrel that contains the same liquid: %v", test);
	returned = container2->CanBeStackedWith(container1);
	test = returned == true; passed &= test;
	Log("- Barrel can be stacked with other barrel that contains the same liquid: %v", test);

	// cannot stack filled barrel with other empty barrel
	container1->SetLiquidContainer("Water", 100);
	container2->SetLiquidFillLevel(0);

	returned = container1->CanBeStackedWith(container2);
	test = returned == false; passed &= test;
	Log("- Filled barrel cannot be stacked with empty barrel: %v", test);
	returned = container2->CanBeStackedWith(container1);
	test = returned == false; passed &= test;
	Log("- Empty barrel cannot be stacked with filled barrel: %v", test);

	// can stack empty barrel with other empty barrel
	container1->SetLiquidFillLevel(0);
	container2->SetLiquidFillLevel(0);

	returned = container1->CanBeStackedWith(container2);
	test = returned == true; passed &= test;
	Log("- Empty barrel can be stacked with empty barrel: %v", test);

	// cannot stack filled barrel with other filled barrel of different liquid
	container1->SetLiquidContainer("Water", 100);
	container2->SetLiquidContainer("Oil", 100);

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

global func Test12_OnStart(int plr){ return true;}
global func Test12_OnFinished(){ return; }
global func Test12_Execute()
{
	Log("Test the behaviour of liquid objects entering liquid containers");

	var container = CreateObject(Barrel);
	var liquid = CreateObject(Liquid_Water);
	
	// can fill empty barrel with the liquid
	liquid->SetLiquidAmount(100);
	liquid->Enter(container);
	
	var passed = true;
	var returned = container->GetLiquidItem();
	var test = (returned == liquid); passed &= test;
	Log("- Liquid can fill empty barrel: %v", test);
	returned = container->GetLiquidFillLevel();
	test = (100 == returned); passed &= test;
	Log("- Barrel contains %d units, expected %d: %v", returned, 100, test);
	
	// can fill barrel with more liquid, liquid object gets removed
	liquid = CreateObject(Liquid_Water);
	liquid->SetLiquidAmount(100);
	liquid->Enter(container);

	test = (liquid == nil); passed &= test;
	Log("- Liquid can enter filled barrel, liquid got removed: %v", test);
	returned = container->GetLiquidFillLevel();
	test = (200 == returned); passed &= test;
	Log("- Barrel contains %d units, expected %d: %v", returned, 200, test);

	// cannot fill in more than the allowed amount
	liquid = CreateObject(Liquid_Water);
	liquid->SetLiquidAmount(200);
	liquid->Enter(container);
	
	returned = liquid->Contained();
	test = (returned == nil); passed &= test;
	Log("- Liquid cannot enter filled barrel if the capacity is exceeded: %v", test);
	returned = container->GetLiquidFillLevel();
	test = (300 == returned); passed &= test;
	Log("- Barrel does increase fill level, up to the allowed amount, contains %d units, expected %d: %v", returned, 300, test);
	returned = liquid->GetLiquidAmount();
	test = (100 == returned); passed &= test;
	Log("- Liquid object still contains %d units, expected %d: %v", returned, 100, test);

	Log("- Resetting liquid amount to 0");
	liquid->RemoveObject();
	container->GetLiquidItem()->RemoveObject();

	// cannot fill in empty barrel and empty liquid object partially
	liquid = CreateObject(Liquid_Water);
	liquid->SetLiquidAmount(500);
	liquid->Enter(container);
	
	returned = liquid->Contained();
	test = (returned == nil); passed &= test;
	Log("- Liquid cannot enter empty barrel if the capacity is exceeded: %v", test);
	returned = container->GetLiquidFillLevel();
	test = (300 == returned); passed &= test;
	Log("- Barrel does increase fill level, up to the allowed amount, contains %d units, expected %d: %v", returned, 300, test);
	returned = liquid->GetLiquidAmount();
	test = (200 == returned); passed &= test;
	Log("- Liquid object still contains %d units, expected %d: %v", returned, 200, test);

	Log("- Resetting liquid amount to 200");
	liquid->RemoveObject();
	container->SetLiquidFillLevel(200);

	// cannot fill in a different liquid
	liquid = CreateObject(Liquid_Oil);
	liquid->SetLiquidAmount(50);
	liquid->Enter(container);
	
	returned = liquid->Contained();
	test = (returned == nil); passed &= test;
	Log("- Liquid cannot enter filled barrel of a different liquid type: %v", test);
	returned = container->GetLiquidFillLevel();
	test = (200 == returned); passed &= test;
	Log("- Barrel does not increase fill level, contains %d units, expected %d: %v", returned, 200, test);

	liquid->RemoveObject();
	
	// barrel gets emptied when liquid exits it
	liquid = container->GetLiquidItem();
	liquid->Exit();
	
	returned = container->LiquidContainerIsEmpty();
	test = returned; passed &= test;
	Log("- Liquid container should be empty when liquid leaves it: %v", test);
	returned = container->GetLiquidItem();
	test = (returned == nil); passed &= test;
	Log("- Liquid container should not have a liquid item when liquid leaves it: %v", test);
	test = (liquid != nil); passed &= test;
	Log("- Liquid exists after leaving the container: %v", test);
	
	liquid->RemoveObject();
	container->RemoveObject();

	return passed;
}
