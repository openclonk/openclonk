/**
	Power System
	Unit tests for the power system. Invokes tests by calling the 
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


/*-- Power Tests --*/

// Simple test for one steady source and one steady consumer.
global func Test1_OnStart(int plr)
{
	// Power source: one wind generator.
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 72, 160, plr);
	
	// Power consumer: one workshop.
	var workshop = CreateObjectAbove(ToolsWorkshop, 110, 160, plr);
	workshop->CreateContents(Wood, 2);
	workshop->CreateContents(Metal, 2);
	workshop->AddToQueue(Shovel, 2);
	
	// Log what the test is about.
	Log("A steady power source (wind generator) supplying an on-demand consumer (workshop).");
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
	// Remove wind generator, compensator and workshop.
	RemoveAll(Find_Or(Find_ID(WindGenerator), Find_ID(Compensator), Find_ID(ToolsWorkshop)));
	return;
}

// Test for one on-demand source and a few consumers.
global func Test2_OnStart(int plr)
{
	// Power source: one steam engine.
	var engine = CreateObjectAbove(SteamEngine, 100, 160, plr);
	var coal = engine->CreateContents(Coal, 1);
	coal->SetCon(12);
	
	// Power consumer: sawmill.
	CreateObjectAbove(Sawmill, 40, 160, plr);
	CreateObjectAbove(Tree_Coconut, 40, 160)->ChopDown();
	
	// Power consumer: armory.
	var armory = CreateObjectAbove(Armory, 280, 160, plr);
	armory->CreateContents(Firestone, 5);
	armory->CreateContents(Metal, 5);
	armory->AddToQueue(IronBomb, 5);
	
	// Log what the test is about.
	Log("An on-demand power source (steam engine) supplying a few on-demand power consumers (sawmill, armory).");
	return true;	
}

global func Test2_Completed()
{
	// One wood is being burned as fuel by the steam engine.
	if (ObjectCount(Find_ID(Wood)) >= 3 && ObjectCount(Find_ID(IronBomb)) >= 5)
		return true;
	return false;
}

global func Test2_OnFinished()
{
	// Remove steam engine, sawmill (possibly with wood), armory.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(Sawmill), Find_ID(Wood), Find_ID(Armory), Find_ID(Tree_Coconut)));
	return;
}

// Test for one on-demand source and one steady consumer.
global func Test3_OnStart(int plr)
{
	// Power source: one steam engine.
	var engine = CreateObjectAbove(SteamEngine, 40, 160, plr);
	engine->CreateContents(Coal, 1);
	
	// Power consumer: three pumps.
	for (var i = 0; i < 3; i++)
	{
		var pump = CreateObjectAbove(Pump, 80 + i * 20, 160, plr);
		var source = CreateObjectAbove(Pipe, 176, 292, plr);
		source->ConnectPipeTo(pump, PIPE_STATE_Source);
		var drain = CreateObjectAbove(Pipe, 248, 100, plr);
		drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
		drain->GetConnectedLine()->AddVertex(208, 48);
	}
	
	// Log what the test is about.
	Log("An on-demand power source (steam engine) supplying a few steady power consumers (pumps).");
	return true;
}

global func Test3_Completed()
{
	if (GetMaterial(248, 48) == Material("Water"))
		return true;
	return false;
}

global func Test3_OnFinished()
{
	// Restore water levels.
	DrawMaterialQuad("Water", 144, 168, 208 + 1, 168, 208 + 1, 304, 144, 304, true);
	for (var x = 216; x <= 280; x++)
		for (var y = 24; y <= 120; y++)
			if (GetMaterial(x, y) == Material("Water"))
				ClearFreeRect(x, y, 1, 1);
	// Remove steam engine, pump and the pipes.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(Pump), Find_ID(Pipe)));
	return;
}

// Test one steady source with one steady consumer and a prioritized on-demand consumer.
global func Test4_OnStart(int plr)
{
	// Power source: one wind generator.
	SetWindFixed(25);
	CreateObjectAbove(WindGenerator, 40, 160, plr);
	
	// Power consumer: one pump.
	var pump = CreateObjectAbove(Pump, 124, 160, plr);
	var source = CreateObjectAbove(Pipe, 176, 292, plr);
	source->ConnectPipeTo(pump, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 248, 100, plr);
	drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	drain->GetConnectedLine()->AddVertex(208, 48);

	// Power connection: flagpole.
	CreateObjectAbove(Flagpole, 304, 140, plr);	 
	
	// Power consumer: one elevator.
	var elevator = CreateObjectAbove(Elevator, 372, 104, plr);
	ScheduleCall(elevator.case, "SetMoveDirection", 4 * 36, 0, COMD_Down, true);
	
	// Log what the test is about.
	Log("A steady power source (wind generator) supplying a steady power consumer (pump), while an on-demand consumer (elevator) is turned on and should be prioritized.");
	return true;
}

global func Test4_Completed()
{
	if (FindObject(Find_ID(ElevatorCase), Find_InRect(372, 230, 40, 40)))
		return true;
	return false;
}

global func Test4_OnFinished()
{
	// Restore water levels.
	DrawMaterialQuad("Water", 144, 168, 208 + 1, 168, 208 + 1, 304, 144, 304, true);
	for (var x = 216; x <= 280; x++)
		for (var y = 24; y <= 120; y++)
			if (GetMaterial(x, y) == Material("Water"))
				ClearFreeRect(x, y, 1, 1);
	// Remove wind generator, pump, the pipes, flagpole and elevator.
	RemoveAll(Find_Or(Find_ID(WindGenerator), Find_ID(Pump), Find_ID(Pipe), Find_ID(Flagpole), Find_ID(Elevator)));
	// Remove burning wood which is created when the elevator case is removed.
	Schedule(nil, "RemoveAll(Find_ID(Wood))", 1);
	return;
}

// Test one steady source with one steady consumer and a prioritized on-demand consumer (double elevators).
global func Test5_OnStart(int plr)
{
	// Power source: one wind generator.
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 40, 160, plr);
	
	// Power consumer: one pump.
	var pump = CreateObjectAbove(Pump, 124, 160, plr);
	var source = CreateObjectAbove(Pipe, 176, 292, plr);
	source->ConnectPipeTo(pump, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 248, 100, plr);
	drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	drain->GetConnectedLine()->AddVertex(208, 48);
	
	// Power connection: flagpole.
	CreateObjectAbove(Flagpole, 304, 140, plr);	 
	
	// Power consumer: double elevator.
	var elevator1 = CreateObjectAbove(Elevator, 372, 104, plr);
	var elevator2 = CreateObjectAbove(Elevator, 434, 104, plr);
	elevator2->SetDir(DIR_Right);
	elevator2->LetsBecomeFriends(elevator1);
	ScheduleCall(elevator1.case, "SetMoveDirection", 4 * 36, 0, COMD_Down, true);
	
	// Log what the test is about.
	Log("A steady power source (wind generator) supplying a steady power consumer (pump), while an on-demand consumer (double elevator) is turned on and should be prioritized.");
	return true;
}

global func Test5_Completed()
{
	if (FindObject(Find_ID(ElevatorCase), Find_InRect(372, 230, 40, 40)))
		return true;
	return false;
}

global func Test5_OnFinished()
{
	// Restore water levels.
	DrawMaterialQuad("Water", 144, 168, 208 + 1, 168, 208 + 1, 304, 144, 304, true);
	for (var x = 216; x <= 280; x++)
		for (var y = 24; y <= 120; y++)
			if (GetMaterial(x, y) == Material("Water"))
				ClearFreeRect(x, y, 1, 1);
	// Remove wind generator, pump, the pipes, flagpole and elevator.
	RemoveAll(Find_Or(Find_ID(WindGenerator), Find_ID(Pump), Find_ID(Pipe), Find_ID(Flagpole), Find_ID(Elevator)));
	// Remove burning wood which is created when the elevator case is removed.
	Schedule(nil, "RemoveAll(Find_ID(Wood))", 1);
	return;
}

// Basic test for power storage: one steady supplier, one storage and one consumer which needs energy from both sources.
global func Test6_OnStart(int plr)
{
	// Power source: one wind generator.
	SetWindFixed(25);
	CreateObjectAbove(WindGenerator, 40, 160, plr);
	
	// Power storage: one compensator
	CreateObjectAbove(Compensator, 70, 160, plr);
	
	// Power consumer: one workshop.
	var workshop = CreateObjectAbove(ToolsWorkshop, 110, 160, plr);
	workshop->CreateContents(Wood, 2);
	workshop->CreateContents(Metal, 2);
	workshop->AddToQueue(Shovel, 2);
	
	// Log what the test is about.
	Log("A steady power source (wind generator) connected to a power storage (compensator) which are both needed to supply a steady consumer (workshop).");
	return true;
}

global func Test6_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

global func Test6_OnFinished()
{
	// Remove wind generator, compensator and workshop.
	RemoveAll(Find_Or(Find_ID(WindGenerator), Find_ID(Compensator), Find_ID(ToolsWorkshop)));
	return;
}

// Test one overproducing on-demand producer with power storage and one steady consumer.
global func Test7_OnStart(int plr)
{
	// Power source: one steam engine.
	var engine = CreateObjectAbove(SteamEngine, 40, 160, plr);
	var coal = engine->CreateContents(Coal, 1);
	coal->SetCon(50);
	
	// Power storage: four compensators.
	CreateObjectAbove(Compensator, 20, 224, plr);
	CreateObjectAbove(Compensator, 45, 224, plr);
	CreateObjectAbove(Compensator, 70, 224, plr);
	CreateObjectAbove(Compensator, 95, 224, plr);
	
	// Power consumer: one workshop.
	var workshop = CreateObjectAbove(ToolsWorkshop, 110, 160, plr);
	workshop->CreateContents(Wood, 18);
	workshop->CreateContents(Metal, 18);
	workshop->AddToQueue(Shovel, 18);
	
	// Log what the test is about.
	Log("An on-demand power source (steam engine) whose over-production gets stored by power storage (compensators) which should provide a steady consumer (workshop).");
	return true;
}

global func Test7_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 18)
		return true;
	return false;
}

global func Test7_OnFinished()
{
	// Remove steam engine, compensators and workshop.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(Compensator), Find_ID(ToolsWorkshop)));
	return;
}

// Test an on-demand producer as back-up for a steady producer with for steady consumers.
global func Test8_OnStart(int plr)
{
	// Power source: one steam engine.
	var engine = CreateObjectAbove(SteamEngine, 40, 160, plr);
	engine->CreateContents(Coal, 1);
	
	// Power source: wind generators which are turned on and off again due to wind.
	CreateObjectAbove(WindGenerator, 356, 104, plr);
	SetWindFixed(80);
	Schedule(nil, "SetWindFixed(0)", 5 * 36);
	Schedule(nil, "SetWindFixed(80)", 8 * 36);
	
	// Power connection: flagpole.
	CreateObjectAbove(Flagpole, 328, 120, plr);
	
	// Power consumer: two pumps.
	for (var i = 0; i < 2; i++)
	{
		var pump = CreateObjectAbove(Pump, 92 + i * 20, 160, plr);
		var source = CreateObjectAbove(Pipe, 176, 292, plr);
		source->ConnectPipeTo(pump, PIPE_STATE_Source);
		var drain = CreateObjectAbove(Pipe, 248, 100, plr);
		drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
		drain->GetConnectedLine()->AddVertex(208, 48);
	}
	
	// Log what the test is about.
	Log("An on-demand producer (steam engine) as back-up for a steady producer (wind generator) with for steady consumers (pumps).");
	return true;
}

global func Test8_Completed()
{
	if (GetMaterial(248, 48) == Material("Water"))
		return true;
	return false;
}

global func Test8_OnFinished()
{
	// Restore water levels.
	RestoreWaterLevels();
	// Remove steam engine, wind generator, flagpole, pump and the pipes.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(WindGenerator), Find_ID(Flagpole), Find_ID(Pump), Find_ID(Pipe)));
	return;
}

// Test the reduced on-demand consumer (wind mill) with an on-demand producer to always have power.
global func Test9_OnStart(int plr)
{
	// Power source: one steam engine.
	var engine = CreateObjectAbove(SteamEngine, 40, 160, plr);
	engine->CreateContents(Coal, 1);
	
	// Change the windlevels so that the engine is needed from time to time.
	SetWindFixed(50);
	Schedule(nil, "SetWindFixed(25)", 5 * 36);
	Schedule(nil, "SetWindFixed(0)", 10 * 36);
	Schedule(nil, "SetWindFixed(50)", 15 * 36);
	Schedule(nil, "SetWindFixed(0)", 20 * 36);
	
	// Power consumer: one wind mill.
	var windmill = CreateObjectAbove(Windmill, 116, 160, plr);
	windmill->CreateContents(Seeds, 3);
	windmill->AddToQueue(Flour, 3);

	// Log what the test is about.
	Log("An on-demand producer (steam engine) always provides power to a reduced on-demand consumer (wind mill).");
	return true;
}

global func Test9_Completed()
{
	if (ObjectCount(Find_ID(Flour)) >= 3)
		return true;
	return false;
}

global func Test9_OnFinished()
{
	// Remove steam engine, wind mill and flour.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(Windmill), Find_ID(Flour)));
	return;
}

// Test a double separated network and power producing pumps.
global func Test10_OnStart(int plr)
{
	// Power source (network 1): one wind generator.
	SetWindFixed(100);
	CreateObjectAbove(WindGenerator, 40, 160, plr);
	
	// Power consumer (network 1): four pumps.
	for (var i = 0; i < 4; i++)
	{
		var pump = CreateObjectAbove(Pump, 80 + i * 12, 160, plr);
		var source = CreateObjectAbove(Pipe, 168, 292, plr);
		source->ConnectPipeTo(pump, PIPE_STATE_Source);
		var drain = CreateObjectAbove(Pipe, 240, 100, plr);
		drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
		drain->GetConnectedLine()->AddVertex(208, 48);
	}
	
	// Power source (network 2): four pumps.
	for (var i = 0; i < 4; i++)
	{
		var pump = CreateObjectAbove(Pump, 228 + i * 12, 160, plr);
		var source = CreateObjectAbove(Pipe, 256, 100, plr);
		source->ConnectPipeTo(pump, PIPE_STATE_Source);
		source->GetConnectedLine()->AddVertex(288, 24);
		source->GetConnectedLine()->AddVertex(288, 114);
		source->GetConnectedLine()->AddVertex(288, 120);
		var drain = CreateObjectAbove(Pipe, 184, 292, plr);
		drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	}
	
	// Power connection (network 2): flagpole.
	CreateObjectAbove(Flagpole, 364, 104, plr);
	
	// Power consumer (network 2): one sawmill.
	CreateObjectAbove(Sawmill, 400, 248, plr);
	for (var i = 0; i < 2; i++)
		CreateObjectAbove(Tree_Coconut, 400, 248 - 30)->ChopDown();
	
	// Log what the test is about.
	Log("Network 1 (steady producer (wind generator) supplying steady consumers (pumps) connected to network 2 where steady producers (pumps) supply an on-demand consumer (sawmill).");
	return true;
}

global func Test10_Completed()
{
	if (ObjectCount(Find_ID(Wood), Find_NoContainer()) >= 5)
		return true;
	return false;
}

global func Test10_OnFinished()
{
	// Restore water levels.
	RestoreWaterLevels();
	// Remove wind generator, sawmill, wood flagpole, pump and the pipes.
	RemoveAll(Find_Or(Find_ID(WindGenerator), Find_ID(Sawmill), Find_ID(Wood), Find_ID(Flagpole), Find_ID(Pump), Find_ID(Pipe), Find_ID(Tree_Coconut)));
	return;
}

// Test connecting two networks by different allied players and then hostility change, team switch and elimination of one players.
global func Test11_OnStart(int plr)
{
	// First network is owned by the player.
	var steam_engine = CreateObjectAbove(SteamEngine, 40, 160, plr);
	steam_engine->CreateContents(Coal, 4);
	
	// Second network is owned by the script player.
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 410, 248, script_plr);
	var shipyard = CreateObjectAbove(Shipyard, 450, 248, script_plr);
	shipyard->CreateContents(Wood, 8);
	shipyard->CreateContents(Metal, 8);
	shipyard->CreateContents(Cloth, 4);
	shipyard->AddToQueue(Airship, 2);
	
	// Networks are disconnected so let the script player bridge the gap.
	ScheduleCall(nil, "CreateObjectAbove", 3 * 36, 0, Compensator, 272, 160, script_plr);
	
	// Make the players hostile for a short time.
	ScheduleCall(nil, "SetHostility", 6 * 36, 0, plr, script_plr, true);
	ScheduleCall(nil, "SetHostility", 6 * 36, 0, script_plr, plr, true);
	
	// Make the players allies again.
	ScheduleCall(nil, "SetHostility", 9 * 36, 0, plr, script_plr, false);
	ScheduleCall(nil, "SetHostility", 9 * 36, 0, script_plr, plr, false);
	
	// Switch the team of the script player.
	var team = GetPlayerTeam(script_plr);
	ScheduleCall(nil, "SetPlayerTeam", 12 * 36, 0, script_plr, team + 1);

	// And switch the team of the script player back again.
	ScheduleCall(nil, "SetPlayerTeam", 15 * 36, 0, script_plr, team);
	
	// Eliminate the script player and see if the normal player takes over the network correctly.
	if (script_plr != nil)
	{
		ScheduleCall(nil, "EliminatePlayer", 18 * 36, 0, script_plr);
		// Rejoin the script player for other tests.
		ScheduleCall(nil, "CreateScriptPlayer", 21 * 36, 0, "PowerBuddy", RGB(0, 0, 255), nil, CSPF_NoEliminationCheck);	
	}
	
	// Log what the test is about.
	Log("Two connected networks by different allied players with hostility change, team switch and elimination of one players.");
	return true;
}

global func Test11_Completed()
{
	if (ObjectCount(Find_ID(Airship)) >= 2)
		return true;
	return false;
}

global func Test11_OnFinished()
{
	// Ensure the script player exists or is created.
	ClearScheduleCall(nil, "EliminatePlayer");
	ClearScheduleCall(nil, "CreateScriptPlayer");	
	if (script_plr == nil)
		CreateScriptPlayer("PowerBuddy", RGB(0, 0, 255), nil, CSPF_NoEliminationCheck);	
	// Remove wind generator, steam engine, flagpole, shipyard, airship.
	RemoveAll(Find_Or(Find_ID(WindGenerator), Find_ID(SteamEngine), Find_ID(Compensator), Find_ID(Shipyard), Find_ID(Airship)));
	return;
}

// Test for the no power need rule and functionality.
global func Test12_OnStart(int plr)
{
	// Power source: one steam engine.
	var steam_engine = CreateObjectAbove(SteamEngine, 40, 160, plr);
	steam_engine->CreateContents(Coal, 4);
	
	// Power consumers: one workshop, one inventor's lab.
	var workshop = CreateObjectAbove(ToolsWorkshop, 110, 160, plr);
	workshop->CreateContents(Wood, 4);
	workshop->CreateContents(Metal, 4);
	workshop->AddToQueue(Shovel, 4);
	var lab = CreateObjectAbove(InventorsLab, 450, 248, plr);
	lab->CreateContents(Metal, 8);
	lab->CreateContents(Diamond, 4);
	lab->AddToQueue(TeleGlove, 4);
	lab->SetNoPowerNeed(true);
	ScheduleCall(nil, "Log", 1, 0, "Lab has no power need (per script).");
	
	// Power connection: flagpole.
	CreateObjectAbove(Flagpole, 304, 140, plr);

	// Let the lab have a power need per script.
	ScheduleCall(lab, "SetNoPowerNeed", 3 * 36, 0, false);
	ScheduleCall(nil, "Log", 3 * 36, 0, "Lab has a power need (per script).");
	
	// Create the no power need rule.
	ScheduleCall(nil, "CreateObject", 6 * 36, 0, Rule_NoPowerNeed);
	ScheduleCall(nil, "Log", 6 * 36, 0, "No power need rule activated.");
	
	// Remove the no power need rule.
	Schedule(nil, "RemoveAll(Find_ID(Rule_NoPowerNeed))", 9 * 36, 0);
	ScheduleCall(nil, "Log", 9 * 36, 0, "No power need rule removed.");

	// Log what the test is about.
	Log("No power need rule and no power need script functionality tested for a simple network.");
	return true;
}

global func Test12_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 4 && ObjectCount(Find_ID(TeleGlove)) >= 4)
		return true;
	return false;
}

global func Test12_OnFinished()
{
	// Remove wind generator, steam engine, flagpole, shipyard, airship.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(ToolsWorkshop), Find_ID(InventorsLab), Find_ID(Flagpole), Find_ID(Rule_NoPowerNeed)));
	return;
}

// A network which is split up in the middle by removing a flagpol.
// TODO: this test should actually reproduce the network error.
global func Test13_OnStart(int plr)
{	
	// Power source: one steam engine.
	var steam_engine = CreateObjectAbove(SteamEngine, 36, 160, plr);
	steam_engine->CreateContents(Coal, 4);

	// Power consumer: one workshop.
	var workshop = CreateObjectAbove(ToolsWorkshop, 340, 248, plr);
	workshop->CreateContents(Wood, 2);
	workshop->CreateContents(Metal, 2);
	workshop->AddToQueue(Shovel, 2);
	
	// Power connection: one flagpole.
	CreateObjectAbove(Flagpole, 248, 280, plr);
	Schedule(nil, "RemoveAll(Find_ID(Flagpole))", 3 * 36, 0);
	ScheduleCall(nil, "CreateObjectAbove", 6 * 36, 0, Flagpole, 248, 280, plr);
	
	// Log what the test is about.
	Log("Flagpole which connects two networks is removed and created again to test network merging.");
	return true;
}

global func Test13_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

global func Test13_OnFinished()
{
	// Remove all the structures.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(ToolsWorkshop), Find_ID(Flagpole)));
	return;
}

static POWER_SYSTEM_Test14_Time;

// Massive test which tests a lot of power structures and the performance of the system.
global func Test14_OnStart(int plr)
{
	// Start the script profiler for this test.
	StartScriptProfiler();
	POWER_SYSTEM_Test14_Time = GetTime();
	
	// Power source: one steam engine.
	var steam_engine1 = CreateObjectAbove(SteamEngine, 36, 160, plr);
	steam_engine1->CreateContents(Coal, 12);

	// Power source: one steam engine.
	var steam_engine2 = CreateObjectAbove(SteamEngine, 472, 312, plr);
	
	// Power source: three wind generators.
	SetWindFixed(75);
	CreateObjectAbove(WindGenerator, 480, 104, plr);
	CreateObjectAbove(WindGenerator, 344, 104, plr);
	CreateObjectAbove(WindGenerator, 480, 248, plr);

	// Power connection: one flagpole.
	CreateObjectAbove(Flagpole, 248, 280, plr);

	// Power consumer: four pumps.
	for (var i = 0; i < 4; i++)
	{
		var pump = CreateObjectAbove(Pump, 84 + i * 12, 160, plr);
		var source = CreateObjectAbove(Pipe, 168, 292, plr);
		source->ConnectPipeTo(pump, PIPE_STATE_Source);
		var drain = CreateObjectAbove(Pipe, 240, 100, plr);
		drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
		drain->GetConnectedLine()->AddVertex(208, 48);
	}
	
	// Power source: four pumps.
	for (var i = 0; i < 4; i++)
	{
		var pump = CreateObjectAbove(Pump, 228 + i * 12, 160, plr);
		var source = CreateObjectAbove(Pipe, 256, 100, plr);
		source->ConnectPipeTo(pump, PIPE_STATE_Source);
		source->GetConnectedLine()->AddVertex(288, 24);
		source->GetConnectedLine()->AddVertex(288, 114);
		source->GetConnectedLine()->AddVertex(282, 120);
		var drain = CreateObjectAbove(Pipe, 184, 292, plr);
		drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	}

	// Power storage: four compensators.
	CreateObjectAbove(Compensator, 20, 224, plr);
	CreateObjectAbove(Compensator, 45, 224, plr);
	CreateObjectAbove(Compensator, 70, 224, plr);
	CreateObjectAbove(Compensator, 95, 224, plr);
	CreateObjectAbove(Compensator, 70, 312, plr);
	CreateObjectAbove(Compensator, 95, 312, plr);
	
	// Power consumer: double elevator.
	var elevator1 = CreateObjectAbove(Elevator, 372, 104, plr);
	var elevator2 = CreateObjectAbove(Elevator, 434, 104, plr);
	elevator2->SetDir(DIR_Right);
	elevator2->LetsBecomeFriends(elevator1);
	ScheduleCall(elevator1.case, "SetMoveDirection", 3 * 36, 1000, COMD_Down, true);
	ScheduleCall(elevator1.case, "SetMoveDirection", 4 * 36, 1000, COMD_Up, true);

	// Power consumer: one workshop.
	var workshop = CreateObjectAbove(ToolsWorkshop, 340, 248, plr);
	workshop->CreateContents(Wood, 12);
	workshop->CreateContents(Metal, 12);
	workshop->AddToQueue(Shovel, 12);
	
	// Power consumer: one chemical lab.
	var chemical_lab = CreateObjectAbove(ChemicalLab, 28, 312, plr);
	chemical_lab->CreateContents(Firestone, 12);
	chemical_lab->CreateContents(Coal, 12);
	chemical_lab->AddToQueue(Dynamite, 12);
	
	// Power consumer: sawmill.
	CreateObjectAbove(Sawmill, 408, 312, plr);
	CreateObjectAbove(Tree_Coconut, 408, 312)->ChopDown();
	CreateObjectAbove(Tree_Coconut, 408, 312)->ChopDown();

	// Power consumer: one armory.
	var armory = CreateObjectAbove(Armory, 340, 312, plr);
	armory->CreateContents(Metal, 24);
	armory->CreateContents(Wood, 8);
	armory->AddToQueue(GrenadeLauncher, 8);

	// Power consumer: inventor's lab.
	var lab = CreateObjectAbove(InventorsLab, 430, 248, plr);
	lab->CreateContents(Metal, 40);
	lab->CreateContents(Amethyst, 20);
	lab->AddToQueue(TeleGlove, 20);

	// Log what the test is about.
	Log("Massive amount of power structures to test the performance of the power system.");
	return true;
}

global func Test14_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 12 && ObjectCount(Find_ID(Dynamite)) >= 12 && ObjectCount(Find_ID(GrenadeLauncher)) >= 8 && ObjectCount(Find_ID(TeleGlove)) >= 20)
		return true;
	return false;
}

global func Test14_OnFinished()
{
	// Stop the script profiler for this test and log the total time.
	var time = GetTime() - POWER_SYSTEM_Test14_Time;
	Log("The test ran for %d ms and these functions have been consuming an amount of time:", time);
	StopScriptProfiler();
	// Restore water levels.
	RestoreWaterLevels();	
	// Remove all the structures.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(WindGenerator), Find_ID(ToolsWorkshop), Find_ID(ChemicalLab), Find_ID(Armory), Find_ID(Sawmill), Find_ID(Wood)));
	RemoveAll(Find_Or(Find_ID(Pump), Find_ID(Pipe), Find_ID(Compensator), Find_ID(Elevator), Find_ID(InventorsLab), Find_ID(Flagpole), Find_ID(Tree_Coconut)));
	// Remove burning wood which is created when the elevator case is removed.
	Schedule(nil, "RemoveAll(Find_ID(Wood))", 1);
	return;
}

// Test for a pump which is continuously powered but does not always have liquid to pump.
global func Test15_OnStart(int plr)
{
	// Power source: wind generator producing the power difference between the two pumps.
	SetWindFixed(100);
	CreateObjectAbove(WindGenerator, 50, 160, plr);
	
	// Power storage: four compensators.
	CreateObjectAbove(Compensator, 20, 224, plr);
	CreateObjectAbove(Compensator, 45, 224, plr);
	CreateObjectAbove(Compensator, 70, 224, plr);
	CreateObjectAbove(Compensator, 95, 224, plr);
		
	// Power consumer: a single pump.	
	var pump = CreateObjectAbove(Pump, 84, 160, plr);
	var source = CreateObjectAbove(Pipe, 168, 292, plr);
	source->ConnectPipeTo(pump, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 240, 100, plr);
	drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	drain->GetConnectedLine()->AddVertex(208, 48);

	// Change the water levels.
	Schedule(nil, "RemoveWater()", 2 * 36, 0);
	Schedule(nil, "RestoreWaterLevels()", 4 * 36, 0);
	Schedule(nil, "RemoveWater()", 6 * 36, 0);
	Schedule(nil, "RestoreWaterLevels()", 8 * 36, 0);
	
	// Log what the test is about.
	Log("A pump which is continuously powered but does not always have liquid to pump.");
	return true;
}

global func Test15_Completed()
{
	if (GetMaterial(248, 48) == Material("Water"))
		return true;
	return false;
}

global func Test15_OnFinished()
{
	// Restore water levels.
	RestoreWaterLevels();
	// Remove steam engine, pump and the pipes.
	RemoveAll(Find_Or(Find_ID(Compensator), Find_ID(WindGenerator), Find_ID(Pump), Find_ID(Pipe)));
	return;
}

static POWER_SYSTEM_Test16_Start;

// Test for underproduction of power not meeting a single demand, which should not lead to producing any power at all.
global func Test16_OnStart(int plr)
{
	// Store the current frame.
	POWER_SYSTEM_Test16_Start = FrameCounter();
	
	// Power source: one steam engine.
	var engine = CreateObjectAbove(SteamEngine, 40, 160, plr);
	var coal = engine->CreateContents(Coal, 1);
	
	// Power storage: four compensators.
	CreateObjectAbove(Compensator, 20, 224, plr);
	CreateObjectAbove(Compensator, 45, 224, plr);
	CreateObjectAbove(Compensator, 70, 224, plr);
	CreateObjectAbove(Compensator, 95, 224, plr);
	
	// Power consumer: one workshop.
	var workshop = CreateObjectAbove(ToolsWorkshop, 110, 160, plr);
	workshop.PowerNeed = Scenario.Test16_PowerNeed;
	workshop->CreateContents(Wood, 1);
	workshop->CreateContents(Metal, 1);
	workshop->AddToQueue(Shovel, 1);
	
	// Log what the test is about.
	Log("Underproduction of power not meeting a single demand, which should not lead to producing any power at all.");
	return true;
}

public func Test16_PowerNeed(...) { return 320; }

global func Test16_Completed()
{
	var engine = FindObject(Find_ID(SteamEngine));
	if (!engine)
		return false;
	// Completed if the engine still has its fuel after 20 seconds.
	if (FrameCounter() > 20 * 36 + POWER_SYSTEM_Test16_Start)
		if (FindObject(Find_Container(engine), Find_ID(Coal)) || engine->GetFuelAmount() >= 100 / 2)
			return true;
	return false;
}

global func Test16_OnFinished()
{
	// Remove steam engine, compensators and workshop.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(Compensator), Find_ID(ToolsWorkshop)));
	return;
}

// Test a bug where an elevator is connected with a flagpole to an existing settlement with two players.
global func Test17_OnStart(int plr)
{
	// Power source: wind generator producing the power difference between the two pumps.
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 40, 160, plr);
	CreateObjectAbove(Flagpole, 12, 160, plr);
	
	// Power consumers.
	CreateObjectAbove(ChemicalLab, 70, 160, plr);
	var workshop = CreateObjectAbove(ToolsWorkshop, 110, 160, plr);
	workshop->CreateContents(Wood, 2);
	workshop->CreateContents(Metal, 2);
	
	// Create elevator and flagpole by script player.
	ScheduleCall(nil, "CreateObjectAbove", 2 * 36, 0, Elevator, 372, 104, script_plr);
	ScheduleCall(nil, "CreateObjectAbove", 4 * 36, 0, Flagpole, 300, 160, script_plr);
		
	// Start a task in the workshop.
	ScheduleCall(workshop, "AddToQueue", 8 * 36, 0, Shovel, 2);
	
	// Log what the test is about.
	Log("An elevator is connected with a flagpole to an existing settlement with two players");
	return true;
}

global func Test17_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

global func Test17_OnFinished()
{
	// Remove structures.
	RemoveAll(Find_Or(Find_ID(WindGenerator), Find_ID(Flagpole), Find_ID(ChemicalLab), Find_ID(ToolsWorkshop), Find_ID(Elevator)));
	return;
}

// Test for one steady source and one steady consumer, where the consumer moves out of the energy range.
global func Test18_OnStart(int plr)
{
	// Power source: one wind generator.
	SetWindFixed(50);
	CreateObjectAbove(WindGenerator, 72, 160, plr);
	
	// Power consumer: one workshop.
	var workshop = CreateObjectAbove(ToolsWorkshop, 110, 160, plr);
	workshop->CreateContents(Wood, 2);
	workshop->CreateContents(Metal, 2);
	workshop->AddToQueue(Shovel, 2);
	
	// Move workshop.
	ScheduleCall(workshop, "SetPosition", 2 * 36, 0, 250, 140);
	ScheduleCall(workshop, "SetPosition", 8 * 36, 0, 110, 140);
	ScheduleCall(workshop, "SetPosition", 12 * 36, 0, 250, 140);
	ScheduleCall(workshop, "SetPosition", 16 * 36, 0, 110, 140);
	
	// Log what the test is about.
	Log("A steady power source (wind generator) supplying an on-demand consumer (workshop), the consumer moves out and into the power range.");
	return true;
}

global func Test18_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

global func Test18_OnFinished()
{
	// Remove wind generator, compensator and workshop.
	RemoveAll(Find_Or(Find_ID(WindGenerator), Find_ID(Compensator), Find_ID(ToolsWorkshop)));
	return;
}

// Test for the supported infinite pump loop, with two pumps pumping in opposite directions.
global func Test19_OnStart(int plr)
{
	// Power source: wind generator producing the power difference between the two pumps.
	SetWindFixed(10);
	CreateObjectAbove(WindGenerator, 50, 160, plr);
		
	// Power consumer: two pumps.
	for (var i = 0; i < 2; i++)
	{
		var pump = CreateObjectAbove(Pump, 80 + i * 30, 160, plr);
		var source = CreateObjectAbove(Pipe, 176, 292, plr);
		source->ConnectPipeTo(pump, [PIPE_STATE_Source, PIPE_STATE_Drain][i]);
		var drain = CreateObjectAbove(Pipe, 248, 100, plr);
		drain->ConnectPipeTo(pump, [PIPE_STATE_Drain, PIPE_STATE_Source][i]);
		drain->GetConnectedLine()->AddVertex(208, 48);
	}
	
	// Some initial potential energy from water.
	CastPXS("Water", 200, 40, 248, 80);
	
	// Log what the test is about.
	Log("A supported (wind generator) infinite pump loop, with two pumps pumping in opposite directions.");
	return true;
}

global func Test19_Completed()
{
	if (GetMaterial(248, 97) == Material("Water") && ObjectCount(Find_ID(Pump), Find_Action("Pump")) == 2)
		return true;
	return false;
}

global func Test19_OnFinished()
{
	// Restore water levels.
	RestoreWaterLevels();
	// Remove steam engine, pump and the pipes.
	RemoveAll(Find_Or(Find_ID(Compensator), Find_ID(WindGenerator), Find_ID(Pump), Find_ID(Pipe)));
	return;
}

// Test for steam engine fueled by oil barrels.
global func Test20_OnStart(int plr)
{
	// Power source: one steam engine.
	var engine = CreateObjectAbove(SteamEngine, 100, 160, plr);

	for (var i = 0; i < 3; ++i)
	{
		var barrel = CreateObject(Barrel, 1);
		barrel->CreateContents(Oil, 10);
		engine->Collect(barrel, true);
	}
	
	// Power consumer: armory.
	var armory = CreateObjectAbove(Armory, 280, 160, plr);
	armory->CreateContents(Firestone, 5);
	armory->CreateContents(Metal, 5);
	armory->AddToQueue(IronBomb, 5);

	// Log what the test is about.
	Log("A steam engine fueled by oil barrels.");
	return true;
}

global func Test20_Completed()
{
	// One wood is being burned as fuel by the steam engine.
	if (ObjectCount(Find_ID(Barrel), Find_NoContainer()) >= 3 && ObjectCount(Find_ID(IronBomb)) >= 5)
		return true;
	return false;
}

global func Test20_OnFinished()
{
	// Remove steam engine, barrels, armory.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(Barrel), Find_ID(Armory)));
	return;
}

// Test for steam engine fueled by oil field and pump.
global func Test21_OnStart(int plr)
{
	// Oil field
	DrawMaterialQuad("Oil", 144, 168, 208 + 1, 168, 208 + 1, 304, 144, 304, true);

	// Power source: one steam engine.
	var engine = CreateObjectAbove(SteamEngine, 70, 160, plr);
	engine->CreateContents(Oil, 10);
	
	// Power consumer: one pump.
	var pump = CreateObjectAbove(Pump, 124, 160, plr);
	var source = CreateObjectAbove(Pipe, 176, 292, plr);
	source->ConnectPipeTo(pump, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 100, 160, plr);
	drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	drain->ConnectPipeTo(engine);
	
	// Power consumer: armory.
	var armory = CreateObjectAbove(Armory, 255, 160, plr);
	armory->CreateContents(Firestone, 20);
	armory->CreateContents(Metal, 20);
	armory->AddToQueue(IronBomb, 20);

	// Log what the test is about.
	Log("A steam engine fueled by an oil field via pump.");
	return true;
}

global func Test21_Completed()
{
	// One wood is being burned as fuel by the steam engine.
	if (ObjectCount(Find_ID(IronBomb)) >= 20)
		return true;
	return false;
}

global func Test21_OnFinished()
{
	// Restore water
	RestoreWaterLevels();
	// Remove steam engine, armory, pump.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(Armory), Find_ID(Pipe), Find_ID(Pump)));
	return;
}

static POWER_SYSTEM_Test22_Time;

// Test of pumping and power system with massive amounts of pumps.
global func Test22_OnStart(int plr)
{
	// Start the script profiler for this test.
	StartScriptProfiler();
	POWER_SYSTEM_Test22_Time = GetTime();
	
	// Power source: one steam engine.
	var steam_engine1 = CreateObjectAbove(SteamEngine, 36, 160, plr);
	steam_engine1->CreateContents(Coal, 12);
	
	// Power source: wind generators.
	SetWindFixed(100);
	CreateObjectAbove(WindGenerator, 440, 104, plr);
	CreateObjectAbove(WindGenerator, 460, 104, plr);
	CreateObjectAbove(WindGenerator, 480, 104, plr);
	CreateObjectAbove(WindGenerator, 500, 104, plr);	

	// Power connection: one flagpole.
	CreateObjectAbove(Flagpole, 248, 280, plr);

	// Power consumer: pumps.
	for (var i = 0; i < 5; i++)
	{
		var pump = CreateObjectAbove(Pump, 84 + i * 10, 160, plr);
		var source = CreateObjectAbove(Pipe, 168 - 2 * i, 292, plr);
		source->ConnectPipeTo(pump, PIPE_STATE_Source);
		var drain = CreateObjectAbove(Pipe, 240 - 2 * i, 100, plr);
		drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
		drain->GetConnectedLine()->AddVertex(208, 48);
	}
	
	// Power source: pumps.
	for (var i = 0; i < 5; i++)
	{
		var pump = CreateObjectAbove(Pump, 228 + i * 10, 160, plr);
		var source = CreateObjectAbove(Pipe, 256 + 2 * i, 100, plr);
		source->ConnectPipeTo(pump, PIPE_STATE_Source);
		source->GetConnectedLine()->AddVertex(288, 24);
		source->GetConnectedLine()->AddVertex(288, 114);
		source->GetConnectedLine()->AddVertex(282, 120);
		var drain = CreateObjectAbove(Pipe, 184 + 2 * i, 292, plr);
		drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	}

	// Power storage: compensators.
	CreateObjectAbove(Compensator, 20, 224, plr);
	CreateObjectAbove(Compensator, 45, 224, plr);
	CreateObjectAbove(Compensator, 70, 224, plr);
	CreateObjectAbove(Compensator, 95, 224, plr);
	CreateObjectAbove(Compensator, 20, 312, plr);
	CreateObjectAbove(Compensator, 45, 312, plr);
	CreateObjectAbove(Compensator, 70, 312, plr);
	CreateObjectAbove(Compensator, 95, 312, plr);

	// Log what the test is about.
	Log("Massive amount of pumps to test the performance of pumps and the power system.");
	return true;
}

global func Test22_Completed()
{
	if (GetTime() - POWER_SYSTEM_Test22_Time > 50000)
		return true;
	return false;
}

global func Test22_OnFinished()
{
	// Stop the script profiler for this test and log the total time.
	var time = GetTime() - POWER_SYSTEM_Test22_Time;
	Log("The test ran for %d ms and these functions have been consuming an amount of time:", time);
	StopScriptProfiler();
	// Restore water levels.
	RestoreWaterLevels();	
	// Remove all the structures.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(WindGenerator)));
	RemoveAll(Find_Or(Find_ID(Pump), Find_ID(Pipe), Find_ID(Compensator), Find_ID(Flagpole)));
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
	
