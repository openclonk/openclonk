/**
	Power System
	Unit tests for the power system. Invokes tests by calling the 
	global function Test*_OnStart(int plr) and iterate through all 
	tests. The test is completed once Test*_Completed() returns
	true. Then Test*_OnFinished() is called, to be able to reset 
	the scenario for the next test.
	
	@author Maikel
*/


protected func Initialize()
{

	return;
}

protected func InitializePlayer(int plr)
{
	// Set zoom and move player to the middle of the scenario.
	SetPlayerZoomByViewRange(plr, LandscapeWidth(), nil, PLRZOOM_Direct);
	GetCrew(plr)->SetPosition(120, 150);
	
	// Add test control effect.
	var effect = AddEffect("IntTestControl", nil, 100, 10);
	effect.testnr = 1;
	effect.launched = false;
	effect.plr = plr;
	return true;
}


/*-- Tests --*/

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
			Log("All tests have been successfully completed!");
			return -1;
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

// Simple test for one steady source and one steady consumer.
global func Test1_OnStart(int plr)
{
	// Power source: one wind generator.
	SetWindFixed(50);
	CreateObject(WindGenerator, 88, 160, plr);
	
	// Power consumer: one pump.
	var pump = CreateObject(Pump, 124, 160, plr);
	var source = CreateObject(Pipe, 176, 292, plr);
	var source_pipe = CreateObject(PipeLine, 144, 160, plr);
	source_pipe->SetActionTargets(source, pump);
	pump->SetSource(source_pipe);
	var drain = CreateObject(Pipe, 248, 100, plr);
	var drain_pipe = CreateObject(PipeLine, 224, 48, plr);
	drain_pipe->AddVertex(208, 48);
	drain_pipe->SetActionTargets(drain, pump);
	pump->SetDrain(drain_pipe);
	
	// Log what the test is about.
	Log("A steady power source (wind generator) supplying a steady power consumer (pump).");
	return true;
}

global func Test1_Completed()
{
	if (GetMaterial(248, 84) == Material("Water"))
		return true;
	return false;
}

global func Test1_OnFinished()
{
	// Restore water levels.
	DrawMaterialQuad("Water", 144, 168, 208 + 1, 168, 208 + 1, 304, 144, 304, true);
	for (var x = 216; x <= 280; x++)
		for (var y = 24; y <= 120; y++)
			if (GetMaterial(x, y) == Material("Water"))
				ClearFreeRect(x, y, 1, 1);
	// Remove wind generator, pump and the pipes.
	RemoveAll(Find_Or(Find_ID(WindGenerator), Find_ID(Pump), Find_ID(Pipe)));
	return;
}

// Test for one on-demand source and one steady consumer.
global func Test2_OnStart(int plr)
{
	// Power source: one steam engine.
	var engine = CreateObject(SteamEngine, 40, 160, plr);
	engine->CreateContents(Coal, 1);
	
	// Power consumer: three pumps.
	for (var i = 0; i < 3; i++)
	{
		var pump = CreateObject(Pump, 80 + i * 20, 160, plr);
		var source = CreateObject(Pipe, 176, 292, plr);
		var source_pipe = CreateObject(PipeLine, 144, 160, plr);
		source_pipe->SetActionTargets(source, pump);
		pump->SetSource(source_pipe);
		var drain = CreateObject(Pipe, 248, 100, plr);
		var drain_pipe = CreateObject(PipeLine, 224, 48, plr);
		drain_pipe->AddVertex(208, 48);
		drain_pipe->SetActionTargets(drain, pump);
		pump->SetDrain(drain_pipe);
	}
	
	// Log what the test is about.
	Log("An on-demand power source (steam engine) supplying a few steady power consumers (pumps).");
	return true;
}

global func Test2_Completed()
{
	if (GetMaterial(248, 48) == Material("Water"))
		return true;
	return false;
}

global func Test2_OnFinished()
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
global func Test3_OnStart(int plr)
{
	// Power source: one wind generator.
	SetWindFixed(25);
	CreateObject(WindGenerator, 40, 160, plr);
	
	// Power consumer: one pump.
	var pump = CreateObject(Pump, 124, 160, plr);
	var source = CreateObject(Pipe, 176, 292, plr);
	var source_pipe = CreateObject(PipeLine, 144, 160, plr);
	source_pipe->SetActionTargets(source, pump);
	pump->SetSource(source_pipe);
	var drain = CreateObject(Pipe, 248, 100, plr);
	var drain_pipe = CreateObject(PipeLine, 224, 48, plr);
	drain_pipe->AddVertex(208, 48);
	drain_pipe->SetActionTargets(drain, pump);
	pump->SetDrain(drain_pipe);
	
	// Power connection: flagpole.
	CreateObject(Flagpole, 304, 140, plr);	 
	
	// Power consumer: one elevator.
	var elevator = CreateObject(Elevator, 372, 104, plr);
	ScheduleCall(elevator.case, "SetMoveDirection", 20, 0, ElevatorCase_down, true);
	
	// Log what the test is about.
	Log("A steady power source (wind generator) supplying a steady power consumer (pump), while an on-demand consumer is turned on and should be prioritized.");
	return true;
}

global func Test3_Completed()
{
	if (FindObject(Find_ID(ElevatorCase), Find_InRect(372, 218, 40, 40)))
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
	// Remove wind generator, pump, the pipes, flagpole and elevator.
	RemoveAll(Find_Or(Find_ID(WindGenerator), Find_ID(Pump), Find_ID(Pipe), Find_ID(Flagpole), Find_ID(Elevator)));
	// Remove burning wood which is created when the elevator case is removed.
	Schedule(nil, "RemoveAll(Find_ID(Wood))", 1);
	return;
}

// Basic test for power storage: one steady supplier, one storage and one consumer which needs energy from both sources.
global func Test4_OnStart(int plr)
{
	// Power source: one wind generator.
	SetWindFixed(25);
	CreateObject(WindGenerator, 40, 160, plr);
	
	// Power storage: one compensator
	CreateObject(Compensator, 70, 160, plr);
	
	// Power consumer: one workshop.
	var workshop = CreateObject(ToolsWorkshop, 110, 160, plr);
	workshop->CreateContents(Wood, 2);
	workshop->CreateContents(Metal, 2);
	workshop->AddToQueue(Shovel, 2);
	
	// Log what the test is about.
	Log("A steady power source (wind generator) connected to a power storage (compensator) which are both needed to supply a steady consumer (workshop).");
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
	// Remove wind generator, compensator and workshop.
	RemoveAll(Find_Or(Find_ID(WindGenerator), Find_ID(Compensator), Find_ID(ToolsWorkshop)));
	return;
}

// Test one overproducing on-demand producer with power storage and one steady consumer.
global func Test5_OnStart(int plr)
{
	// Power source: one steam engine.
	var engine = CreateObject(SteamEngine, 40, 160, plr);
	var coal = engine->CreateContents(Coal, 1);
	coal->SetCon(50);
	
	// Power storage: four compensators.
	CreateObject(Compensator, 20, 224, plr);
	CreateObject(Compensator, 45, 224, plr);
	CreateObject(Compensator, 70, 224, plr);
	CreateObject(Compensator, 95, 224, plr);
	
	// Power consumer: one workshop.
	var workshop = CreateObject(ToolsWorkshop, 110, 160, plr);
	workshop->CreateContents(Wood, 16);
	workshop->CreateContents(Metal, 16);
	workshop->AddToQueue(Shovel, 16);
	
	// Log what the test is about.
	Log("An on-demand power source (steam engine) whose over-production gets stored by power storage (compensators) which should provide a steady consumer (workshop).");
	return true;
}

global func Test5_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 16)
		return true;
	return false;
}

global func Test5_OnFinished()
{
	// Remove steam engine, compensators and workshop.
	RemoveAll(Find_Or(Find_ID(SteamEngine), Find_ID(Compensator), Find_ID(ToolsWorkshop)));
	return;
}

/*-- Helper Functions --*/

global func RemoveTest()
{
	// Remove all objects besides the clonk.
	RemoveAll(Find_Not(Find_OCF(OCF_CrewMember)));
	
	// Remove all effects besides the test control effect.
	/*var effect;
	var index = 0;
	while (effect = GetEffect("*", nil, index))
	{
		Log("%v", effect);
		index++;
	}*/
	return;
}

global func SetWindFixed(int strength)
{
	strength = BoundBy(strength, -100, 100);
	var effect = GetEffect("IntFixedWind");
	if (!effect)
		effect = AddEffect("IntFixedWind", nil, 100, 5);
	effect.strength = strength;
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
	