/**
	Cable Cars
	Unit tests for the cable cars. Invokes tests by calling the 
	global function Test*_OnStart(int plr) and iterate through all 
	tests. The test is completed once Test*_Completed() returns
	true. Then Test*_OnFinished() is called, to be able to reset 
	the scenario for the next test.
	
	With LaunchTest(int nr) a specific test can be launched when
	called during runtime. A test can be skipped by calling the
	function SkipTest().
	
	@author Maikel
*/


static script_plr;

protected func Initialize()
{
	// Create a script player for some tests.
	script_plr = nil;
	CreateScriptPlayer("PowerBuddy", RGB(0, 0, 255), nil, CSPF_NoEliminationCheck);
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
		
	// Initialize script player.
	if (GetPlayerType(plr) == C4PT_Script)
	{
		// Store the player number.
		if (script_plr == nil)
			script_plr = plr;
		// No crew needed.
		GetCrew(plr)->RemoveObject();
		return;
	}	
	
	// Move player to the start of the scenario.
	GetCrew(plr)->SetPosition(120, 150);
	
	// Give all knowledge.
	var index = 0, def;
	while (def = GetDefinition(index++))
		SetPlrKnowledge(plr, def);
	
	// Add test control effect.
	var effect = AddEffect("IntTestControl", nil, 100, 2);
	effect.testnr = 1;
	effect.launched = false;
	effect.plr = plr;
	return;
}

protected func RemovePlayer(int plr)
{
	// Remove script player.
	if (GetPlayerType(plr) == C4PT_Script)
	{
		if (plr == script_plr)
			script_plr = nil;
		return;	
	}
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
			return FX_Execute_Kill;
		}
		PrintCableCarNetwork();
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


/*-- Cable Cars Tests --*/

global func Test1_OnStart(int plr)
{
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 90, 160, plr);

	var crossing1 = CreateObjectAbove(CableCrossing, 70, 160, plr);
	var crossing2 = CreateObjectAbove(CableCrossing, 216, 64, plr);
	var crossing3 = CreateObjectAbove(CableCrossing, 272, 64, plr);
	var crossing4 = CreateObjectAbove(CableCrossing, 450, 104, plr);
	
	CreateCableCrossingsConnection(crossing1, crossing2);
	CreateCableCrossingsConnection(crossing2, crossing3);
	CreateCableCrossingsConnection(crossing3, crossing4);
	
	var hoist = crossing4->CreateObject(CableHoist);
	hoist->EngageRail(crossing4);
	var lorry = crossing4->CreateObject(CableLorry);
	hoist->PickupVehicle(lorry);
	lorry->CreateContents(Metal, 2);
	lorry->CreateContents(Wood, 2);
	
	var workshop = CreateObjectAbove(ToolsWorkshop, 40, 160, plr);
	crossing1->CombineWith(workshop);
	workshop->AddToQueue(Shovel, 2);
		
	// Log what the test is about.
	Log("A workshop needs material to produce 2 shovels, lorry in system has materials.");
	return true;
}

global func Test1_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

global func Test1_OnFinished()
{
	RemoveTestObjects();
	return;
}


global func Test2_OnStart(int plr)
{
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 90, 160, plr);
	CreateObjectAbove(WindGenerator, 440, 104, plr);

	var crossing1 = CreateObjectAbove(CableCrossing, 70, 160, plr);
	var crossing2 = CreateObjectAbove(CableCrossing, 216, 64, plr);
	var crossing3 = CreateObjectAbove(CableCrossing, 272, 64, plr);
	var crossing4 = CreateObjectAbove(CableCrossing, 450, 104, plr);
	
	CreateCableCrossingsConnection(crossing1, crossing2);
	CreateCableCrossingsConnection(crossing2, crossing3);
	CreateCableCrossingsConnection(crossing3, crossing4);
	
	var hoist, lorry;
	hoist = crossing3->CreateObject(CableHoist);
	hoist->EngageRail(crossing3);
	lorry = crossing3->CreateObject(CableLorry);
	hoist->PickupVehicle(lorry);
	lorry->CreateContents(Wood, 4);
	
	hoist = crossing2->CreateObject(CableHoist);
	hoist->EngageRail(crossing2);
	lorry = crossing2->CreateObject(CableLorry);
	hoist->PickupVehicle(lorry);
	lorry->CreateContents(Metal, 4);
	
	var workshop;
	workshop = CreateObjectAbove(ToolsWorkshop, 40, 160, plr);
	crossing1->CombineWith(workshop);
	workshop->AddToQueue(Shovel, 2);
	
	var workshop = CreateObjectAbove(ToolsWorkshop, 490, 104, plr);
	crossing4->CombineWith(workshop);
	workshop->AddToQueue(Axe, 2);
	
	// Log what the test is about.
	Log("Workshop (needs 2x metal 2x wood) and another workshop (2x metal 2x wood) battle for 2 lorries (one with 4x metal and other with 4x wood).");
	return true;
}

global func Test2_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2 && ObjectCount(Find_ID(Axe)) >= 2)
		return true;
	return false;
}

global func Test2_OnFinished()
{
	RemoveTestObjects();
	return;
}


global func Test3_OnStart(int plr)
{
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 90, 160, plr);

	var crossing1 = CreateObjectAbove(CableCrossing, 70, 160, plr);
	var crossing2 = CreateObjectAbove(CableCrossing, 216, 64, plr);
	var crossing3 = CreateObjectAbove(CableCrossing, 272, 64, plr);
	var crossing4 = CreateObjectAbove(CableCrossing, 450, 104, plr);
	
	CreateCableCrossingsConnection(crossing1, crossing2);
	var line_to_break = CreateCableCrossingsConnection(crossing2, crossing3);
	CreateCableCrossingsConnection(crossing3, crossing4);
	
	var hoist = crossing4->CreateObject(CableHoist);
	hoist->EngageRail(crossing4);
	var lorry = crossing4->CreateObject(CableLorry);
	hoist->PickupVehicle(lorry);
	lorry->CreateContents(Metal, 2);
	lorry->CreateContents(Wood, 2);
	
	var workshop = CreateObjectAbove(ToolsWorkshop, 40, 160, plr);
	crossing1->CombineWith(workshop);
	workshop->AddToQueue(Shovel, 2);
	
	ScheduleCall(line_to_break, "OnLineBreak", 36, 0, true);
	ScheduleCall(line_to_break, "RemoveObject", 37, 0, true);
	ScheduleCall(nil, "CreateCableCrossingsConnection", 240, 0, crossing2, crossing3);
		
	// Log what the test is about.
	Log("Check if a delivery is continued when a cable breaks and is repaired.");
	return true;
}

global func Test3_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

global func Test3_OnFinished()
{
	RemoveTestObjects();
	return;
}

global func Test4_OnStart(int plr)
{
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 90, 160, plr);

	var crossing1 = CreateObjectAbove(CableCrossing, 70, 160, plr);
	var crossing2 = CreateObjectAbove(CableCrossing, 216, 64, plr);
	var crossing3 = CreateObjectAbove(CableCrossing, 272, 64, plr);
	var crossing4 = CreateObjectAbove(CableCrossing, 450, 104, plr);
	
	CreateCableCrossingsConnection(crossing1, crossing2);
	CreateCableCrossingsConnection(crossing2, crossing3);
	CreateCableCrossingsConnection(crossing3, crossing4);
	
	var hoist = crossing4->CreateObject(CableHoist);
	hoist->EngageRail(crossing4);
	var lorry = crossing4->CreateObject(CableLorry);
	hoist->PickupVehicle(lorry);
	lorry->CreateContents(Metal, 2);
	lorry->CreateContents(Wood, 2);
	
	var workshop = CreateObjectAbove(ToolsWorkshop, 40, 160, plr);
	crossing1->CombineWith(workshop);
	workshop->AddToQueue(Shovel, 2);
	
	ScheduleCall(crossing2, "RemoveObject", 36, 0, true);
	ScheduleCall(nil, "CreateCableCrossingsConnection", 240, 0, crossing1, crossing3);
		
	// Log what the test is about.
	Log("Check if a delivery is continued when a station is destroyed.");
	return true;
}

global func Test4_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

global func Test4_OnFinished()
{
	RemoveTestObjects();
	return;
}


global func Test5_OnStart(int plr)
{
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 90, 160, plr);

	var crossing1 = CreateObjectAbove(CableCrossing, 70, 160, plr);
	var crossing2 = CreateObjectAbove(CableCrossing, 216, 64, plr);
	var crossing3 = CreateObjectAbove(CableCrossing, 272, 64, plr);
	var crossing4 = CreateObjectAbove(CableCrossing, 450, 104, plr);
	var crossing5 = CreateObjectAbove(CableCrossing, 220, 160, plr);
	var crossing6 = CreateObjectAbove(CableCrossing, 280, 160, plr);
	var crossing7 = CreateObjectAbove(CableCrossing, 348, 104, plr);
	
	CreateCableCrossingsConnection(crossing1, crossing2);
	CreateCableCrossingsConnection(crossing2, crossing3);
	CreateCableCrossingsConnection(crossing3, crossing4);
	CreateCableCrossingsConnection(crossing1, crossing5);
	CreateCableCrossingsConnection(crossing5, crossing6);
	CreateCableCrossingsConnection(crossing6, crossing7);
	CreateCableCrossingsConnection(crossing7, crossing4);
	
	var hoist = crossing4->CreateObject(CableHoist);
	hoist->EngageRail(crossing4);
	var lorry = crossing4->CreateObject(CableLorry);
	hoist->PickupVehicle(lorry);
	lorry->CreateContents(Metal, 2);
	lorry->CreateContents(Wood, 2);
	
	var workshop = CreateObjectAbove(ToolsWorkshop, 40, 160, plr);
	crossing1->CombineWith(workshop);
	workshop->AddToQueue(Shovel, 2);
			
	// Log what the test is about.
	Log("Test path finding and car choice (TODO: Extend this test).");
	return true;
}

global func Test5_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

global func Test5_OnFinished()
{
	RemoveTestObjects();
	return;
}

global func Test6_OnStart(int plr)
{
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 30, 160, plr);

	var crossing1 = CreateObjectAbove(CableCrossing, 70, 160, plr);
	var crossing2 = CreateObjectAbove(CableCrossing, 216, 64, plr);
	var crossing3 = CreateObjectAbove(CableCrossing, 272, 64, plr);
	var crossing4 = CreateObjectAbove(CableCrossing, 450, 104, plr);
	var crossing5 = CreateObjectAbove(CableCrossing, 220, 160, plr);
	var crossing6 = CreateObjectAbove(CableCrossing, 280, 160, plr);
	var crossing7 = CreateObjectAbove(CableCrossing, 348, 104, plr);
	var crossing8 = CreateObjectAbove(CableCrossing, 476, 312, plr);
	
	CreateCableCrossingsConnection(crossing1, crossing2);
	CreateCableCrossingsConnection(crossing2, crossing3);
	CreateCableCrossingsConnection(crossing3, crossing4);
	CreateCableCrossingsConnection(crossing1, crossing5);
	CreateCableCrossingsConnection(crossing5, crossing6);
	CreateCableCrossingsConnection(crossing6, crossing7);
	CreateCableCrossingsConnection(crossing7, crossing4);
	CreateCableCrossingsConnection(crossing7, crossing8);
	
	var hoist, lorry;
	hoist = crossing4->CreateObject(CableHoist);
	hoist->EngageRail(crossing4);
	lorry = crossing4->CreateObject(CableLorry);
	hoist->PickupVehicle(lorry);
	lorry->CreateContents(Rock, 4);
	
	hoist = crossing8->CreateObject(CableHoist);
	hoist->EngageRail(crossing8);
	lorry = crossing8->CreateObject(CableLorry);
	hoist->PickupVehicle(lorry);
		
	var foundry = CreateObjectAbove(Foundry, 110, 160, plr);
	foundry->AddToQueue(Concrete, nil, true);
	crossing1->CombineWith(foundry);
		
	var pump = CreateObjectAbove(Pump, 50, 160, plr);
	var source = CreateObjectAbove(Pipe, 168, 292, plr);
	source->ConnectPipeTo(pump, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 240, 100, plr);
	drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	drain->ConnectPipeTo(foundry, PIPE_STATE_Drain);
	
	var pump = CreateObjectAbove(Pump, 250, 160, plr);
	CreateObjectAbove(Flagpole, 280, 160, plr);
	pump->SetMaterialSelection([Concrete]);
	var source = CreateObjectAbove(Pipe, 168, 292, plr);
	source->ConnectPipeTo(pump, PIPE_STATE_Source);
	source->ConnectPipeTo(foundry, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 80, 300, plr);
	drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	
	Schedule(nil, "CreateObject(Dynamite, 482, 266)->Fuse()", 180, 10**6);
	
	// Log what the test is about.
	Log("Test automated concrete production line.");
	return true;
}

global func Test6_Completed()
{
	if (GetMaterial(80, 240) == Material("Granite"))
		return true;
	return false;
}

global func Test6_OnFinished()
{
	RemoveTestObjects();
	RemoveEffect("IntSchedule");
	return;
}

global func Test7_OnStart(int plr)
{
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 90, 160, plr);

	var crossing1 = CreateObjectAbove(CableCrossing, 70, 160, plr);
	var crossing2 = CreateObjectAbove(CableCrossing, 216, 64, plr);
	var crossing3 = CreateObjectAbove(CableCrossing, 272, 64, plr);
	var crossing4 = CreateObjectAbove(CableCrossing, 450, 104, plr);
	var crossing5 = CreateObjectAbove(CableCrossing, 220, 160, plr);
	var crossing6 = CreateObjectAbove(CableCrossing, 280, 160, plr);
	var crossing7 = CreateObjectAbove(CableCrossing, 348, 104, plr);
	
	CreateCableCrossingsConnection(crossing1, crossing2);
	CreateCableCrossingsConnection(crossing2, crossing3);
	CreateCableCrossingsConnection(crossing3, crossing4);
	CreateCableCrossingsConnection(crossing1, crossing5);
	var line_to_break = CreateCableCrossingsConnection(crossing5, crossing6);
	CreateCableCrossingsConnection(crossing6, crossing7);
	CreateCableCrossingsConnection(crossing7, crossing4);
	
	var hoist = crossing4->CreateObject(CableHoist);
	hoist->EngageRail(crossing4);
	var lorry = crossing4->CreateObject(CableLorry);
	hoist->PickupVehicle(lorry);
	lorry->CreateContents(Metal, 2);
	lorry->CreateContents(Wood, 2);
	
	var workshop = CreateObjectAbove(ToolsWorkshop, 40, 160, plr);
	crossing1->CombineWith(workshop);
	workshop->AddToQueue(Shovel, 2);

	ScheduleCall(line_to_break, "OnLineBreak", 36, 0, true);
	ScheduleCall(line_to_break, "RemoveObject", 37, 0);
	ScheduleCall(nil, "PrintCableCarNetwork", 38, 0);
			
	// Log what the test is about.
	Log("Test if the car finds a new path if the current is removed.");
	return true;
}

global func Test7_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

global func Test7_OnFinished()
{
	RemoveTestObjects();
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

global func RemoveTestObjects()
{
	RemoveAll(Find_Or(
		Find_Or(
			Find_ID(WindGenerator),
			Find_ID(Foundry),
			Find_ID(Pump),
			Find_ID(Pipe),
			Find_ID(CableCrossing),
			Find_ID(CableHoist),
			Find_ID(CableLorry),
			Find_ID(Flagpole),
			Find_ID(ToolsWorkshop),
			Find_ID(Rock)
		),
		Find_Or(
			Find_ID(Dynamite)
		)
	));
	return;
}

global func CreateCableCrossingsConnection(object c1, object c2)
{
	var cable = c1->CreateObject(CableLine);
	cable->SetConnectedObjects(c1, c2);
	return cable;
}


global func PrintCableCarNetwork()
{
	Log("Distances between all of the cable crossings:");
	var cable_crossings = FindObjects(Find_Func("IsCableCrossing"));
	var header = "Obj# |";
	var line = "------";
	for (var crossing in cable_crossings)
	{
		header = Format("%s %04d", header, crossing->ObjectNumber());
		line = Format("%s-----", line);
	}
	Log(Format("%s  tot", header));
	Log(Format("%s-----", line));
	for (var crossing1 in cable_crossings)
	{	
		var msg = Format("%04d |", crossing1->ObjectNumber());
		var sum = 0;
		for (var crossing2 in cable_crossings)
		{
			var len = crossing1->GetLengthToTarget(crossing2);
			sum += len;
			msg = Format("%s %04d", msg, len);
		}
		msg = Format("%s %04d", msg, sum);
		Log(msg);
	}
	return;
}
