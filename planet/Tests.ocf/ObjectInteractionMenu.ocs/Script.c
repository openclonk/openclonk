/**
	Object interaction menu
	Unit tests for the inventory and object actions GUI.
	
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

// Setups:
// Object with QueryRejectDeparture(source)=true is not transferred
// Object with QueryRejectDeparture(destination)=true is transferred
// Transfer 200 stacks of 1 arrow each
// Transfer an object with extra slot
// Transfer items to the same object
// Transfer into an object that removes itself, such as a construction site

global func Test1_OnStart(int plr){ return true;}
global func Test1_OnFinished(){ return; }
global func Test1_Execute()
{
	// setup
	
	var menu = CreateObject(GUI_ObjectInteractionMenu);

	var source = CreateObjectAbove(Armory, 150, 100);

	var container_structure = CreateObjectAbove(Armory, 50, 100);
	var container_living = CreateObjectAbove(Clonk, 50, 100);
	var container_vehicle = CreateObjectAbove(Lorry, 50, 100);
	var container_surrounding = CreateObjectAbove(Helper_Surrounding, 50, 100);
	container_surrounding->InitFor(container_living, menu);
	

	var passed = true;

	// actual test
	
	Log("Test transfer of simple objects metal and wood from a structure (Armory)");

	Log("****** Transfer from source to structure");
	
		Log("*** Function TransferObjectsFromToSimple()");
		passed &= Test1_Transfer(menu, menu.TransferObjectsFromToSimple, source, container_structure, container_structure);

		Log("*** Function TransferObjectsFromTo()");
		passed &= Test1_Transfer(menu, menu.TransferObjectsFromTo, source, container_structure, container_structure);

	Log("****** Transfer from source to living (Clonk)");
	
		Log("*** Function TransferObjectsFromToSimple()");
		passed &= Test1_Transfer(menu, menu.TransferObjectsFromToSimple, source, container_living, container_living);

		Log("*** Function TransferObjectsFromTo()");
		passed &= Test1_Transfer(menu, menu.TransferObjectsFromTo, source, container_living, container_living);

	Log("****** Transfer from source to vehicle (Lorry)");
	
		Log("*** Function TransferObjectsFromToSimple()");
		passed &= Test1_Transfer(menu, menu.TransferObjectsFromToSimple, source, container_vehicle, container_vehicle);

		Log("*** Function TransferObjectsFromTo()");
		passed &= Test1_Transfer(menu, menu.TransferObjectsFromTo, source, container_vehicle, container_vehicle);

	Log("****** Transfer from source to surrounding");
	
		Log("*** Function TransferObjectsFromToSimple()");
		passed &= Test1_Transfer(menu, menu.TransferObjectsFromToSimple, source, container_surrounding, nil);

		Log("*** Function TransferObjectsFromTo()");
		passed &= Test1_Transfer(menu, menu.TransferObjectsFromTo, source, container_surrounding, nil);


	if (container_structure) container_structure->RemoveObject();
	if (container_living) container_living->RemoveObject();
	if (container_vehicle) container_vehicle->RemoveObject();
	if (container_surrounding) container_surrounding->RemoveObject();
	if (source) source->RemoveObject();
	if (menu) menu->RemoveObject();
	return passed;
}

global func Test1_Transfer(object menu, tested_function, object source, object destination, object expected_container)
{
	var passed = true;
	
	var wood = source->CreateContents(Wood);
	var metal = source->CreateContents(Metal);
	var to_transfer = [wood, metal];

	menu->Call(tested_function, to_transfer, source, destination);

	passed &= doTest("Wood is in the destination object. Container is %v, expected %v.", wood->Contained(), expected_container);
	passed &= doTest("Metal is in the destination object. Container is %v, expected %v.", metal->Contained(), expected_container);
	
	if (wood) wood->RemoveObject();
	if (metal) metal->RemoveObject();
	
	return passed;
}


global func TestX_OnStart(int plr){ return true;}
global func TestX_OnFinished(){ return; }
global func TestX_Execute()
{
	// setup
	
	var menu = CreateObject(GUI_ObjectInteractionMenu);

	var source = CreateObject(Dummy);
	
	var container_structure = CreateObjectAbove(Armory, 50, 100);
	var container_living = CreateObjectAbove(Clonk, 50, 100);
	var container_vehicle = CreateObjectAbove(Lorry, 50, 100);
	var container_surrounding = CreateObjectAbove(Helper_Surrounding, 50, 100);
	container_surrounding->InitFor(container_living, menu);

	var passed = true;
	var to_transfer;

	// actual test
	
	Log("Test ");

	Log("****** Transfer from source to structure");
	Log("*** Function TransferObjectsFromToSimple()");
	Log("*** Function TransferObjectsFromTo()");
	Log("****** Transfer from source to living (Clonk)");
	Log("*** Function TransferObjectsFromToSimple()");
	Log("*** Function TransferObjectsFromTo()");
	Log("****** Transfer from source to vehicle (Lorry)");
	Log("*** Function TransferObjectsFromToSimple()");
	Log("*** Function TransferObjectsFromTo()");
	Log("****** Transfer from source to surrounding");
	Log("*** Function TransferObjectsFromToSimple()");
	Log("*** Function TransferObjectsFromTo()");

	if (container_structure) container_structure->RemoveObject();
	if (container_living) container_living->RemoveObject();
	if (container_vehicle) container_vehicle->RemoveObject();
	if (container_surrounding) container_surrounding->RemoveObject();
	if (source) source->RemoveObject();
	if (menu) menu->RemoveObject();
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
