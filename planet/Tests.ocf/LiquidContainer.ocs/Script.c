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
	
	// Add pump
	CreateObjectAbove(Pump, 100, 200);
	CreateObjectAbove(SteamEngine, 150, 200);
	GetCrew(plr)->CreateContents(Pipe);
	GetCrew(plr)->CreateContents(Pipe);
	GetCrew(plr)->CreateContents(Pipe);
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
global func Test1_OnFinish(){ return; }
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
global func Test2_OnFinish(){ return; }
global func Test2_Execute()
{
	Log("Test the behaviour of GetFillLevel and SetFillLevel");

	var container = CreateObject(Barrel);
	var passed = true;
	var test_data = [nil, -1, 0, 1, container->GetLiquidContainerMaxFillLevel()/2, container->GetLiquidContainerMaxFillLevel(), container->GetLiquidContainerMaxFillLevel() + 1];
	
	for (var value in test_data)
	{
		container->SetLiquidFillLevel(value);
		var returned = container->GetLiquidFillLevel();
		if (value == nil) value = 0; // accept 0 as a return value in this case.
		var test = (value == returned); passed &= test;
		Log("- Container returns %d if fill level is set to %d, values should be equal: %v", returned, value, test);
	}
	
	container->RemoveObject();
	return passed;
}


global func Test3_OnStart(int plr){ return true;}
global func Test3_OnFinish(){ return; }
global func Test3_Execute()
{
	Log("Test the behaviour of GetLiquidType and SetLiquidType");

	var container = CreateObject(Barrel);
	var passed = true;
	var test_data = [nil, "Water", "Lava", "123", "#24942fwijvri"];
	
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
global func Test4_OnFinish(){ return; }
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
global func Test5_OnFinish(){ return; }
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
global func Test6_OnFinish(){ return; }
global func Test6_Execute()
{
	Log("Test the behaviour of LiquidContainerAccepts");

	var container = CreateObject(Barrel);
	var passed = true;
	
	// incompatible material
	
	var test = !container->LiquidContainerAccepts("Dummy"); passed &= test;
	Log("- Container returns 'false' if material is wrong: %v", test);

	// fill level

	//container->SetLiquidType("Water");
	container->SetLiquidFillLevel(0);
	test = container->LiquidContainerAccepts("Water");	passed &= test;
	Log("- Container returns 'true' if liquid fill level is 0% and material is ok: %v", test);

	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel() / 2);
	test = !container->LiquidContainerAccepts("Water");	passed &= test;
// 	Log("-- Debug: %v", container->IsLiquidContainerForMaterial("Water"));
// 	Log("-- Debug: %v", container->LiquidContainerIsEmpty());
// 	Log("-- Debug: %v, %s", container->GetLiquidName() == "Water", container->GetLiquidName());
	Log("- Container returns 'false' if liquid fill level is 50% and contained material is 'nil': %v", test);
	
	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel());
	test = !container->LiquidContainerAccepts("Water");	passed &= test;
	Log("- Container returns 'false' if liquid fill level is 100% and material is ok: %v", test);

 	// material
 	Log("Setting container to be filled with a material");
 	container->SetLiquidType("Lava");
 	Log("- Fill material is %s", container->GetLiquidType());

	container->SetLiquidFillLevel(0);
 	container->SetLiquidType("Lava");
	test = container->LiquidContainerAccepts("Water");	passed &= test;
	Log("- Container returns 'true' if filled with material and liquid fill level is 0% and other material is ok: %v", test);

	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel() / 2);
 	container->SetLiquidType("Lava");
	test = !container->LiquidContainerAccepts("Water");	passed &= test;
	Log("- Container returns 'false' if filled with material and liquid fill level is 50% and other material is ok: %v", test);
	
	container->SetLiquidFillLevel(container->GetLiquidContainerMaxFillLevel() / 2);
 	container->SetLiquidType("Water");
	test = container->LiquidContainerAccepts("Water");	passed &= test;
// 	Log("-- Debug: %v", container->IsLiquidContainerForMaterial("Lava"));
// 	Log("-- Debug: %v", container->LiquidContainerIsEmpty());
// 	Log("-- Debug: %v, %s", container->GetLiquidName() == "Lava", container->GetLiquidName());
	Log("- Container returns 'true' if liquid fill level is 50% and material is ok: %v", test);

	container->RemoveObject();
	return passed;
}

global func Test7_OnStart(int plr){ return true;}
global func Test7_OnFinish(){ return; }
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
	Log("- Container returns the fill level when inserting 1 pixel of compatible material: %v", test);
	
	test = (container->PutLiquid("Water", container->GetLiquidContainerMaxFillLevel(), nil)  == (container->GetLiquidContainerMaxFillLevel() - 1));
	passed &= test;
	Log("- Container returns 'the actually inserted material' when inserting more than the volume: %v", test);
	test = container->GetLiquidFillLevel() == container->GetLiquidContainerMaxFillLevel(); passed &= test;
	Log("- Container returns the fill level when inserting more than the volume: %v", test);


	container->RemoveObject();
	return passed;
}

global func Test8_OnStart(int plr){ return true;}
global func Test8_OnFinish(){ return; }
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
	container->SetLiquidContainer("Lava", 100);
	
	returned = container->RemoveLiquid(nil, 50, nil);
	test = (returned[0] == "Lava");
	Log("- Container returns the contained material when extracting material 'nil': %v", test);
	test = returned[1] == 50; passed &= test;
	Log("- Container returns the correct amount when removing compatible material: %v", test);
	test = (container->GetLiquidFillLevel() == 50);
	Log("- Container contents do change when removing compatible material: %v", test);

	container->SetLiquidContainer("Lava", 100);

	returned = container->RemoveLiquid("Lava", nil, nil);
	test = (returned[0] == "Lava");
	Log("- Container returns the contained material when extracting amount 'nil': %v", test);
	test = returned[1] == 100; passed &= test;
	Log("- Container returns the contained amount when extracting amount 'nil': %v", test);
	test = (container->GetLiquidFillLevel() == 0);
	Log("- Container is empty after removing amount 'nil': %v", test);

	container->SetLiquidContainer("Lava", 100);

	returned = container->RemoveLiquid(nil, nil, nil);
	test = (returned[0] == "Lava");
	Log("- Container returns the contained material when extracting material and amount 'nil': %v", test);
	test = returned[1] == 100; passed &= test;
	Log("- Container returns the contained amount when extracting material and amount 'nil': %v", test);
	test = (container->GetLiquidFillLevel() == 0);
	Log("- Container is empty after removing amount material and amount 'nil': %v", test);

	container->RemoveObject();
	return passed;
}
