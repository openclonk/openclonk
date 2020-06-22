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
	var passed = true;
	passed &= doTest("Container accepts correct material. Got %v, expected %v.", container->IsLiquidContainerForMaterial("Water"), true);
	passed &= doTest("Container does not accept incorrect material. Got %v, expected %v.", container->IsLiquidContainerForMaterial("Sky"), false);
	passed &= doTest("Container does not accept material 'nil'. Got %v, expected %v.", container->IsLiquidContainerForMaterial(), false);
	
	container->RemoveObject();
	return passed;
}

global func Test2_OnStart(int plr){ return true;}
global func Test2_OnFinished(){ return; }
global func Test2_Execute()
{
	Log("Test the behaviour of liquid objects entering liquid containers");

	var container = CreateObject(Barrel);
	var liquid = CreateObject(Water);
	
    // -----

	Log("Can fill empty barrel with the liquid");
	liquid->SetStackCount(100);
	liquid->Enter(container);
	
	var passed = true;
	passed &= doTest("Liquid can fill empty barrel. Got %v, expected %v.", container->Contents(), liquid);
	passed &= doTest("Barrel contains %d units, expected %d.", container->GetLiquidAmount("Water"), 100);
	passed &= doTest("GetLiquidAmount() can be called with definitions, too. Got %d units, expected %d.", container->GetLiquidAmount(Water), 100);
	passed &= doTest("The liquid returns a max stack count of %d, expected %d.", container->Contents()->MaxStackCount(), 300);
	
    // -----

	Log("Can fill barrel with more liquid, liquid object gets removed");
	liquid = CreateObject(Water);
	liquid->SetStackCount(100);
	liquid->Enter(container);

	passed &= doTest("Liquid can enter barrel and gets removed. Got %v, expected %v.", liquid, nil);
	passed &= doTest("Barrel contains %d units, expected %d.", container->GetLiquidAmount(), 200);

    // -----

	Log("Cannot fill in more than the allowed amount");
	liquid = CreateObject(Water);
	liquid->SetStackCount(200);
	liquid->Enter(container);
	
	passed &= doTest("Liquid cannot enter filled barrel if the capacity is exceeded. Got %v, expected %v.", liquid->Contained(), nil);
	passed &= doTest("Barrel contains %d items, expected %d.", container->ContentsCount(), 1);
	passed &= doTest("Barrel does increase fill level, up to the allowed amount, contains %d units, expected %d.", container->GetLiquidAmount(), 300);
	passed &= doTest("Liquid object still contains %d units, expected %d", liquid->GetLiquidAmount(), 100);

	Log("- Resetting liquid amount to 0");
	liquid->RemoveObject();
	container->Contents()->RemoveObject();

    // -----

	Log("Cannot fill in empty barrel and empty liquid object partially");
	liquid = CreateObject(Water);
	liquid->SetStackCount(500);
	liquid->Enter(container);
	
	passed &= doTest("Liquid cannot enter empty barrel if the capacity is exceeded. Got %v, expected %v.", liquid->Contained(), nil);
	passed &= doTest("Barrel does increase fill level, up to the allowed amount, contains %d units, expected %d.", container->GetLiquidAmount(), 300);
	passed &= doTest("Liquid object still contains %d units, expected %d.", liquid->GetLiquidAmount(), 200);

	Log("- Resetting liquid amount to 200");
	liquid->RemoveObject();
	container->Contents()->SetStackCount(200);

    // -----

	Log("Cannot fill in a different liquid");
	liquid = CreateObject(Oil);
	liquid->SetStackCount(50);
	liquid->Enter(container);
	
	passed &= doTest("Liquid cannot enter filled barrel of a different liquid type. Got %v, epxected %v.", liquid->Contained(), nil);
	passed &= doTest("Barrel does not increase fill level, contains %d units, expected %d.", container->GetLiquidAmount(), 200);

	liquid->RemoveObject();
	
    // -----

	Log("Barrel gets emptied when liquid exits it");
	liquid = container->Contents();
	liquid->Exit();
	
	passed &= doTest("Liquid container should be empty when liquid leaves it. Got %v, expected %v.", container->Contents(), nil);
	passed &= doTest("Liquid container return a liquid amount of 0 when liquid leaves it. Got %v, expected %v.", container->GetLiquidAmount(), 0);
	passed &= doTest("Liquid exists after leaving the container. Got %v, expected %v.", !!liquid , true);
	
	liquid->RemoveObject();

	// -----
	
	Log("Adding single objects via create contents is not possible beyond the container limit");
	
	RemoveAll(Find_ID(Water));
	
	container->PutLiquid("Water", 299);
	container->CreateContents(Water, 2);
	
	passed &= doTest("Liquid container contains %d, should contain %d, when filling in 299 water and adding 2 water contents", container->GetLiquidAmount(), 300);
	passed &= doTest("A total of %d water objects exist outside the container, expected %d", ObjectCount(Find_ID(Water), Find_NoContainer()), 0);
	passed &= doTest("A total of %d water objects exist in the container, expected %d", container->ContentsCount(), 1);

	// Clean up

	RemoveAll(Find_ID(Water));
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
	passed &= doTest("Container returns '0' when inserting 1 pixel of incompatible material. Got %d, expected %d.", container->PutLiquid("Lava", 1, nil), 0);
	passed &= doTest("Container returns 'nil' for contents. Got %v, expected %v.", container->Contents(), nil);

    // -----

	Log("Compatible material");
	passed &= doTest("Container returns '1' when inserting 1 pixel of compatible material. Got %d, expected %d.", container->PutLiquid("Water", 1, nil), 1);
	passed &= doTest("Container has contents Water after inserting 1 pixel of compatible material. Got %v, expected %v.", !!container->FindContents(Water), true);
	if (passed)
	{
		passed &= doTest("Container returns the fill level 1 when inserting 1 pixel of compatible material. Got %d, expected %d.", container->FindContents(Water)->GetLiquidAmount(), 1);
	}
	
	passed &= doTest("Container returns 'the actually inserted amount of material' when inserting more than the volume. Got %d, expected %d.", container->PutLiquid("Water", container->GetLiquidContainerMaxFillLevel(), nil), container->GetLiquidContainerMaxFillLevel() - 1);
	passed &= doTest("Container returns the max fill level when inserting more than the allowed volume. Got %d, expected %d", container->GetLiquidAmount("Water"), container->GetLiquidContainerMaxFillLevel());
	
    // -----
    
    Log("Parameters");
    
    if (container) container->RemoveObject();
    container = CreateObject(Barrel);
    
    passed &= doTest("Container is filled fully if no amount parameter is passed and the container is empty. Got %d, expected %d.", container->PutLiquid("Water"), container->GetLiquidContainerMaxFillLevel());
	
	var filled = 100;
	container->Contents()->SetStackCount(filled);
    passed &= doTest("Container is filled fully if no amount parameter is passed and the container is filled partially. Got %d, expected %d.", container->PutLiquid("Water"), container->GetLiquidContainerMaxFillLevel() - filled);

    if (container) container->RemoveObject();
    container = CreateObject(Barrel);

	passed &= doTest("Container can be filled with definition input. Got %d, expected %d.", container->PutLiquid(Oil, filled), filled);

	container->RemoveObject();
	return passed;
}

global func Test4_OnStart(int plr){ return true;}
global func Test4_OnFinished(){ return; }
global func Test4_Execute()
{
	Log("Test the behaviour of RemoveLiquid");

	var container = CreateObject(Barrel);
	container->PutLiquid("Water", 100);

    // -----

	Log("Incompatible material");
	var returned = container->RemoveLiquid("Lava", 0, nil);
	var expected = ["Lava", 0];

	var passed = doTest("Container returns the requested material when removing incompatible material. Got %s, expected %s.", returned[0], expected[0]);
	passed &= doTest("Container returns no amount when removing incompatible material. Got %d, expected %d.", returned[1], expected[1]);
	passed &= doTest("Container contents do not change when removing incompatible material. Got %d, expected %d.", container->GetLiquidAmount(), 100);

    // -----

	Log("Compatible material");
	returned = container->RemoveLiquid("Water", 1, nil);
	expected = ["Water", 1];

	passed &= doTest("Container returns the extracted material name. Got %s, expected %s.", returned[0], expected[0]);
	passed &= doTest("Container returns the correct amount when removing 1 pixel of compatible material. Got %d, expected %d.", returned[1], expected[1]);
	passed &= doTest("Container contents do change when removing compatible material. Got %d, expected %d.", container->GetLiquidAmount(), 99);

	returned = container->RemoveLiquid("Water", 100, nil);
	expected = ["Water", 99];

	passed &= doTest("Container returns the extracted material name. Got %s, expected %s.", returned[0], expected[0]);
	passed &= doTest("Container returns the correct amount when removing compatible material. Got %d, expected %d.", returned[1], expected[1]);
	passed &= doTest("Container contents do change when removing compatible material. Got %d, expected %d.", container->GetLiquidAmount(), 0);

    // -----

	Log("Request everything");
	container->PutLiquid("Oil", 100);
	
	returned = container->RemoveLiquid(nil, 50, nil);
	expected = ["Oil", 50];

	passed &= doTest("Container returns the contained material when extracting material 'nil'. Got %s, expected %s.", returned[0], expected[0]);
	passed &= doTest("Container returns the correct amount when removing compatible material. Got %d, expected %d.", returned[1], expected[1]);
	passed &= doTest("Container contents do change when removing compatible material. Got %d, expected %d.", container->GetLiquidAmount(), 50);

	container->PutLiquid("Oil", 50);

	returned = container->RemoveLiquid("Oil", nil, nil);
	expected = ["Oil", 100];

	passed &= doTest("Container returns the contained material when extracting material. Got %s, expected %s.", returned[0], expected[0]);
	passed &= doTest("Container returns the correct amount when extracting amount 'nil'. Got %d, expected %d.", returned[1], expected[1]);
	passed &= doTest("Container is empty after removing amount 'nil'. Got %d, expected %d.", container->GetLiquidAmount(), 0);

	container->PutLiquid("Oil", 100);

	returned = container->RemoveLiquid(nil, nil, nil);
	expected = ["Oil", 100];

	passed &= doTest("Container returns the contained material when extracting material and amount 'nil'. Got %s, expected %s.", returned[0], expected[0]);
	passed &= doTest("Container returns the correct amount when extracting material and amount 'nil'. Got %d, expected %d.", returned[1], expected[1]);
	passed &= doTest("Container is empty after removing material and amount 'nil'. Got %d, expected %d.", container->GetLiquidAmount(), 0);

    // -----

	Log("Parameters");
	
	container->PutLiquid(Oil, 100);
	returned = container->RemoveLiquid(Oil, 50, nil);
	expected = [Oil, 50];

	passed &= doTest("Container returns the contained material when extracting material and amount 'nil'. Got %v, expected %v.", returned[0], expected[0]);
	passed &= doTest("Container returns the correct amount when extracting material and amount 'nil'. Got %d, expected %d.", returned[1], expected[1]);
	passed &= doTest("Container is not empty after partially removing material. Got %d, expected %d.", container->GetLiquidAmount(), 50);

	container->RemoveObject();
	return passed;
}


global func Test5_OnStart(int plr)
{
	var effect = GetEffect("IntTestControl", nil);

	effect.pump = CreateObjectAbove(Pump, 100, 200);
	effect.engine = CreateObjectAbove(SteamEngine, 150, 200);
	return true;
}

global func Test5_Execute()
{
	var effect = GetEffect("IntTestControl", nil);
	
	Log("Test the behaviour of connections between pipe and pump");

	var passed = true;
	var pipeA, pipeB;
	
	Log("No connection");
	passed &= Test5_CheckConnections(effect, effect.pump, effect.pump);

	Log("1. Connecting pipe A to pump, pipe B to pump, pipe B to engine");
	pipeA = CreateObject(Pipe);
	pipeB = CreateObject(Pipe);

	pipeA->ConnectPipeTo(effect.pump);
	passed &= Test5_CheckConnections(effect, pipeA, effect.pump);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Source, pipeB, PIPE_STATE_Neutral);
	pipeB->ConnectPipeTo(effect.pump);
	passed &= Test5_CheckConnections(effect, pipeA, pipeB);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Source, pipeB, PIPE_STATE_Drain);
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
	passed &= Test5_CheckConnections(effect, pipeA, effect.pump);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Source, pipeB, PIPE_STATE_Neutral);
	pipeB->ConnectPipeTo(effect.engine);
	passed &= Test5_CheckConnections(effect, pipeA, effect.pump);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Source, pipeB, PIPE_STATE_Drain);
	pipeB->ConnectPipeTo(effect.pump);
	passed &= Test5_CheckConnections(effect, pipeA, effect.engine);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Source, pipeB, PIPE_STATE_Drain);
	
	pipeA->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.engine);
	
	pipeA->RemoveObject();
	pipeB->RemoveObject();

	Log("3. Connecting pipe A to engine, pipe A to pump, pipe B to pump");

	pipeA = CreateObject(Pipe);
	pipeB = CreateObject(Pipe);

	pipeA->ConnectPipeTo(effect.engine);
	passed &= Test5_CheckConnections(effect, effect.pump, effect.pump);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Neutral, pipeB, PIPE_STATE_Neutral);
	pipeA->ConnectPipeTo(effect.pump);
	passed &= Test5_CheckConnections(effect, effect.pump, effect.engine);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Drain, pipeB, PIPE_STATE_Neutral);
	pipeB->ConnectPipeTo(effect.pump);
	passed &= Test5_CheckConnections(effect, pipeB, effect.engine);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Drain, pipeB, PIPE_STATE_Source);
	
	pipeA->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.engine);
	
	pipeA->RemoveObject();
	pipeB->RemoveObject();
	
	Log("4. Connecting pipe A to pump (drain via menu), pipe B to pump, pipe A to engine");
	
	pipeA = CreateObject(Pipe);
	pipeB = CreateObject(Pipe);

	pipeA->ConnectPipeTo(effect.pump, PIPE_STATE_Drain);
	passed &= Test5_CheckConnections(effect, effect.pump, pipeA);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Drain, pipeB, PIPE_STATE_Neutral);
	pipeB->ConnectPipeTo(effect.pump);
	passed &= Test5_CheckConnections(effect, pipeB, pipeA);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Drain, pipeB, PIPE_STATE_Source);
	pipeA->ConnectPipeTo(effect.engine);
	passed &= Test5_CheckConnections(effect, pipeB, effect.engine);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Drain, pipeB, PIPE_STATE_Source);
	
	pipeA->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.pump);
	pipeB->CutLineConnection(effect.engine);
	
	pipeA->RemoveObject();
	pipeB->RemoveObject();

	Log("5. Connecting pipe A to pump (source), pipe A to engine");
	
	pipeA = CreateObject(Pipe);

	pipeA->ConnectPipeTo(effect.pump, PIPE_STATE_Source);
	passed &= Test5_CheckConnections(effect, pipeA, effect.pump);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Source, nil, nil);
	pipeA->ConnectPipeTo(effect.engine);
	passed &= Test5_CheckConnections(effect, effect.engine, effect.pump);
	passed &= Test5_CheckPipes(pipeA, PIPE_STATE_Source, nil, nil);
	
	pipeA->CutLineConnection(effect.pump);
	pipeA->RemoveObject();

	return passed;
}

global func Test5_CheckConnections(proplist effect, object expected_source, object expected_drain)
{
	var passed = doTest("Pump returns source object %v, expected %v.", effect.pump->GetSourceObject(), expected_source);
	passed &= doTest("Pump returns drain object %v, expected %v.", effect.pump->GetDrainObject(), expected_drain);
	return passed;
}

global func Test5_CheckPipes(object pipeA, string stateA, object pipeB, string stateB)
{
	var functionA, functionB;
	var passed = true;

	if (pipeA != nil)
	{
		     if (stateA == PIPE_STATE_Source)  functionA = pipeA.IsSourcePipe;
		else if (stateA == PIPE_STATE_Drain)   functionA = pipeA.IsDrainPipe;
		else if (stateA == PIPE_STATE_Neutral) functionA = pipeA.IsNeutralPipe;
		
		var test = pipeA->Call(functionA);
		passed &= doTest(Format("Pipe A is a %s/%v pipe? %s", stateA, functionA, "Got %v, expected %v.", test, true));
	}
	
	if (pipeB != nil)
	{
		     if (stateB == PIPE_STATE_Source)  functionB = pipeB.IsSourcePipe;
		else if (stateB == PIPE_STATE_Drain)   functionB = pipeB.IsDrainPipe;
		else if (stateB == PIPE_STATE_Neutral) functionB = pipeB.IsNeutralPipe;
	
	
		test = pipeB->Call(functionB);
		passed &= doTest(Format("Pipe B is a %s/%v pipe? %s", stateB, functionB, "Got %v, expected %v."), test, true);
	}
	return passed;
}

global func Test5_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Pump), Find_ID(SteamEngine), Find_ID(Pipe)));
	return true;
}

global func Test6_OnStart(int plr){ return true;}
global func Test6_OnFinished(){ return; }
global func Test6_Execute()
{
	Log("Test the behaviour of liquid entering a Clonk (for example from a producer)");
	
	var crew = CreateObject(Clonk);
	var barrel1 = crew->CreateContents(Barrel);
	var barrel2 = crew->CreateContents(Barrel);
	var barrel3 = crew->CreateContents(Barrel);
	
	barrel3->PutLiquid("Oil", 100);

	barrel1->Enter(crew);
	barrel2->Enter(crew);
	barrel3->Enter(crew);
	
	var liquid = CreateObject(Water);
	liquid->SetStackCount(1000);
	
	// -----
	
	var passed = doTest("Prerequisites: Barrel 3 has contents. Got %v, expected %v.", !!(barrel3->Contents()), true);
	passed &= doTest("Barrel 1 is in the Clonk. Got %v, epxected %v.", barrel1->Contained(), crew);
	passed &= doTest("Barrel 2 is in the Clonk. Got %v, epxected %v.", barrel2->Contained(), crew);
	passed &= doTest("Barrel 3 is in the Clonk. Got %v, epxected %v.", barrel3->Contained(), crew);
	
	liquid->Enter(crew);
	
	passed &= doTest("Liquid cannot enter the Clonk. Container is %v, expected %v.", liquid->Contained(), nil);
	passed &= doTest("Barrel 1 is filled. Got %d, expected %d.", barrel1->GetLiquidAmount(), barrel1->GetLiquidContainerMaxFillLevel());
	passed &= doTest("Barrel 2 is filled. Got %d, expected %d.", barrel2->GetLiquidAmount(), barrel2->GetLiquidContainerMaxFillLevel());
	if (barrel1->Contents()) passed &= doTest("Barrel 1 contains water. Got %s, expected %s.", barrel1->Contents()->GetLiquidType(), "Water");
	if (barrel1->Contents()) passed &= doTest("Barrel 2 contains water. Got %s, expected %s.", barrel2->Contents()->GetLiquidType(), "Water");
	
	passed &= doTest("Barrel 3 does not change. Got %d, expected %d.", barrel3->GetLiquidAmount(), 100);
	passed &= doTest("Barrel 3 contains oil. Got %s, expected %s.", barrel3->Contents()->GetLiquidType(), "Oil");

	passed &= doTest("Liquid amount does change. Got %d, expected %d.", liquid->GetLiquidAmount(), 400);

	barrel1->RemoveObject();
	barrel2->RemoveObject();
	barrel3->RemoveObject();
	liquid->RemoveObject();
	
	passed &= doTest("Liquid objects get removed with the barrels. Got %d, expected %d.", ObjectCount(Find_Func("IsLiquid")), 0);
	
	crew->RemoveObject();
	return passed;
}

// Deactivated: for some reason the (inherited) stacking function returns false
global func Test6_deactivated_OnStart(int plr){ return true;}
global func Test6_deactivated_OnFinished(){ return; }
global func Test6_deactivated_Execute()
{
	Log("Test the behaviour of barrels getting stacked");

	var container1 = CreateObject(Barrel);
	var container2 = CreateObject(Barrel);
	
	// can stack filled barrel with other filled barrel of the same liquid
	container1->PutLiquid("Water", 100);
	container2->PutLiquid("Water", 300);

	var passed = doTest("Barrel can be stacked with other barrel that contains the same liquid. Got %v, expected %v.", container1->CanBeStackedWith(container2), true);
	passed &= doTest("Barrel can be stacked with other barrel that contains the same liquid. Got %v, expected %v.", container2->CanBeStackedWith(container1), true);

	// cannot stack filled barrel with other empty barrel
	container1->Contents()->SetStackCount(100);
	container2->Contents()->RemoveObject();

	passed &= doTest("Filled barrel cannot be stacked with empty barrel. Got %v, expected %v.", container1->CanBeStackedWith(container2), false);
	passed &= doTest("Empty barrel cannot be stacked with filled barrel. Got %v, expected %v.", container2->CanBeStackedWith(container1), false);


	// can stack empty barrel with other empty barrel
	container1->Contents()->RemoveObject();

	passed &= doTest("Empty barrel can be stacked with empty barrel. Got %v, expected %v", container1->CanBeStackedWith(container2), true);

	// cannot stack filled barrel with other filled barrel of different liquid
	container1->PutLiquid("Water", 100);
	container2->PutLiquid("Oil", 100);

	passed &= doTest("Liquid A barrel cannot be stacked with liquid B barrel. Got %v, expected %v.", container1->CanBeStackedWith(container2), false);
	passed &= doTest("Liquid B barrel cannot be stacked with liquid A barrel. Got %v, expected %v.", container2->CanBeStackedWith(container1), false);
	
	container1->RemoveObject();
	container2->RemoveObject();

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
