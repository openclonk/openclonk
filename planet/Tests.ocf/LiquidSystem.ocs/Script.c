/**
	Liquid System
	Unit tests for the liquid system. Invokes tests by calling the 
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
	// Add the no power rule, this is about liquids.
	CreateObject(Rule_NoPowerNeed);	
	// Create a script player for some tests.
	script_plr = nil;
	CreateScriptPlayer("Buddy", RGB(0, 0, 255), nil, CSPF_NoEliminationCheck);
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


/*-- Liquid Tests --*/

global func Test1_OnStart(int plr)
{
	var foundry = CreateObjectAbove(Foundry, 110, 160, plr);
	foundry->CreateContents(Earth, 10);
	foundry->AddToQueue(Loam, 2);
	
	var pump = CreateObjectAbove(Pump, 84, 160, plr);
	var source = CreateObjectAbove(Pipe, 168, 292, plr);
	source->ConnectPipeTo(pump, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 240, 100, plr);
	drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	drain->ConnectPipeTo(foundry, PIPE_STATE_Drain);
	
	// Log what the test is about.
	Log("Water supply to foundry.");
	return true;
}

global func Test1_Completed()
{
	if (ObjectCount(Find_ID(Loam)) >= 2)
		return true;
	return false;
}

global func Test1_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Pump), Find_ID(Loam)));
	RestoreWaterLevels();
	return;
}


global func Test2_OnStart(int plr)
{
	var foundry = CreateObjectAbove(Foundry, 110, 160, plr);
	foundry->CreateContents(Rock, 60);
	foundry->AddToQueue(Concrete, nil, true);
	
	var pump = CreateObjectAbove(Pump, 84, 160, plr);
	var source = CreateObjectAbove(Pipe, 168, 292, plr);
	source->ConnectPipeTo(pump, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 240, 100, plr);
	drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	drain->ConnectPipeTo(foundry, PIPE_STATE_Drain);
	
	var pump = CreateObjectAbove(Pump, 240, 160, plr);
	pump->SetMaterialSelection([Concrete]);
	var source = CreateObjectAbove(Pipe, 168, 292, plr);
	source->ConnectPipeTo(pump, PIPE_STATE_Source);
	source->ConnectPipeTo(foundry, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 240, 100, plr);
	drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	
	// Log what the test is about.
	Log("Water supply to foundry and concrete supply from foundry.");
	return true;
}

global func Test2_Completed()
{
	if (GetMaterial(240, 60) == Material("Granite"))
		return true;
	return false;
}

global func Test2_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Pump), Find_ID(Pipe)));
	RestoreWaterLevels();
	return;
}


global func Test3_OnStart(int plr)
{
	DrawMaterialQuad("Oil", 144, 168, 208 + 1, 168, 208 + 1, 304, 144, 304, true);
	
	var engine1 = CreateObjectAbove(SteamEngine, 70, 160, plr);
	var engine2 = CreateObjectAbove(SteamEngine, 240, 160, plr);
	var pump1 = CreateObjectAbove(Pump, 16, 160, plr);
	var pump2 = CreateObjectAbove(Pump, 124, 160, plr);
	
	var source = CreateObjectAbove(Pipe, 168, 292, plr);
	source->ConnectPipeTo(pump1, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 240, 100, plr);
	drain->ConnectPipeTo(pump1, PIPE_STATE_Drain);
	drain->ConnectPipeTo(engine1, PIPE_STATE_Drain);
	
	var source = CreateObjectAbove(Pipe, 168, 292, plr);
	source->ConnectPipeTo(pump2, PIPE_STATE_Source);
	source->ConnectPipeTo(engine1, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 240, 100, plr);
	drain->ConnectPipeTo(pump2, PIPE_STATE_Drain);
	drain->ConnectPipeTo(engine2, PIPE_STATE_Drain);
	
	// Log what the test is about.
	Log("Supply two steam engines with oil in a chain.");
	return true;
}

global func Test3_Completed()
{
	for (var engine in FindObjects(Find_ID(SteamEngine)))
		if (engine->GetLiquidAmount("Oil") < 300)
			return false;
	return true;
}

global func Test3_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(Pump), Find_ID(Pipe)));
	RestoreWaterLevels();
	return;
}

global func Test4_OnStart(int plr)
{	
	DrawMatBasin("DuroLava", 20, 120);

	var foundry = CreateObjectAbove(Foundry, 110, 160, plr);
	foundry->CreateContents(Earth, 10);
	foundry->AddToQueue(Loam, 5);
	
	var pump = CreateObjectAbove(Pump, 84, 160, plr);
	var source = CreateObject(Pipe, 168, 292, plr);
	source->ConnectPipeTo(pump, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 240, 100, plr);
	drain->ConnectPipeTo(pump, PIPE_STATE_Drain);
	drain->ConnectPipeTo(foundry, PIPE_STATE_Drain);
	
	ScheduleCall(source, "SetPosition", 100, 0, 20, 120);
	ScheduleCall(foundry, "DoCutPipe", 200, 0, drain);
	
	// Log what the test is about.
	Log("Test air switching pumping materials after connection changes.");
	return true;
}

global func Test4_Completed()
{
	// The test fails if lava is left in the new source rectangle,
	// or if production can be completed (the foundry can produce
	// only 2 loam instead of 5 if the connection changes)
	var could_pump_lava = GetMaterial(20, 125) == Material("Sky");
	var cannot_finish_production = ObjectCount(Find_ID(Loam)) <= 2;
	var producing = FindObject(Find_ID(Foundry))->IsProducing();
	return !producing && could_pump_lava && cannot_finish_production;
}

global func Test4_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(Foundry), Find_ID(Pump), Find_ID(Loam)));
	RestoreWaterLevels();
	return;
}


global func Test5_OnStart(int plr)
{	
	RemoveAll(Find_ID(Rule_NoPowerNeed));	
	
	var engine = CreateObjectAbove(SteamEngine, 40, 160, plr);
	engine->CreateContents(Coal, 10);
	
	var pump = CreateObjectAbove(Pump, 84, 160, plr);
	var helmet = GetHiRank(plr)->CreateContents(DivingHelmet);
	var drain = CreateObjectAbove(Pipe, 240, 100, plr);
	drain->ConnectPipeTo(helmet);
	drain->ConnectPipeTo(pump, drain->GetPipeState());
	
	// Move player character over the basin
	GetHiRank()->SetPosition(175, 150);
	GetHiRank()->SetComDir(COMD_Down);
	helmet->ControlUse(GetHiRank(plr));

	// Log what the test is about.
	Log("Test air supply to diving helmet - dive down towards the ground");
	return true;
}

global func Test5_Completed()
{
	var is_down = GetHiRank()->GetY() > 280;
	if (is_down)
	{
		GetHiRank()->SetComDir(COMD_Up);
		var breath_before = GetHiRank()->GetBreath();
		GetHiRank()->DoBreath(1000);
		var breath_used = GetHiRank().MaxBreath - breath_before;

		var pump = FindObject(Find_ID(Pump));
		var is_pumping = pump->GetAction() == "Pump";
		var has_air_pipe = pump->IsAirPipeConnected();
		Log("The clonk used %d air, pump is pumping %v, has air pipe %v", breath_used, is_pumping, has_air_pipe);
		if (breath_used <= 70 && is_pumping && has_air_pipe)
		{
			return true;
		}
		Log("There was no air supply to the diving helmet - skipping the failed test");
		SkipTest();
	}
	return false;
}

global func Test5_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(DivingHelmet), Find_ID(Pump), Find_ID(Pipe), Find_ID(SteamEngine)));
	RestoreWaterLevels();
	CreateObject(Rule_NoPowerNeed);
	return;
}

global func Test6_OnStart(int plr)
{
	var tank = CreateObjectAbove(LiquidTank, 70, 160, plr);
	var pump1 = CreateObjectAbove(Pump, 16, 160, plr);
	var pump2 = CreateObjectAbove(Pump, 124, 160, plr);
	
	var source = CreateObjectAbove(Pipe, 168, 292, plr);
	source->ConnectPipeTo(pump1, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 240, 100, plr);
	drain->ConnectPipeTo(pump1, PIPE_STATE_Drain);
	drain->ConnectPipeTo(tank, PIPE_STATE_Drain);
	
	var source = CreateObjectAbove(Pipe, 168, 292, plr);
	source->ConnectPipeTo(pump2, PIPE_STATE_Source);
	source->ConnectPipeTo(tank, PIPE_STATE_Source);
	var drain = CreateObjectAbove(Pipe, 240, 100, plr);
	drain->ConnectPipeTo(pump2, PIPE_STATE_Drain);
	
	// Log what the test is about.
	Log("Test pumping into and from a liquid tank");
	return true;
}

global func Test6_Completed()
{
	if (GetMaterial(240, 60) == Material("Water"))
		return true;
	return false;	
}

global func Test6_OnFinished()
{
	RemoveAll(Find_Or(Find_ID(LiquidTank), Find_ID(Pump), Find_ID(Pipe)));
	RestoreWaterLevels();
	return;
}


/*-- Helper Functions --*/

global func RestoreWaterLevels()
{
	// Restore water levels.
	DrawMaterialQuad("Water", 144, 168, 208 + 1, 168, 208 + 1, 304, 144, 304, true);
	for (var x = 216; x <= 280; x++)
		for (var y = 24; y <= 120; y++)
			if (GetMaterial(x, y) != Material("BrickSoft"))
				ClearFreeRect(x, y, 1, 1);
	return;
}

global func DrawMatBasin(string mat, int x, int y)
{
	DrawMaterialQuad("Brick", x - 10, y - 10, x - 10, y + 10, x + 10, y + 10, x + 10, y - 10);
	DrawMaterialQuad(mat, x - 6, y - 6, x - 6, y + 6, x + 6, y + 6, x + 6, y - 6);
	return;
}

global func RemoveWater()
{
	for (var x = 144; x <= 208 + 1; x++)
		for (var y = 168; y <= 304; y++)
			if (GetMaterial(x, y) != Material("BrickSoft"))
				ClearFreeRect(x, y, 1, 1);
	return;
}
