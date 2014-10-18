/**
	Power System
	Unit tests for the power system. Invoke a test by calling the global function Test*()
	and iterate through all tests.
	
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
	GetCrew(plr)->SetPosition(400, 206);
	
	// Add test control effect.
	var effect = AddEffect("IntTestControl", nil, 100, 10);
	effect.testnr = 1;
	effect.testcount = 5;
	effect.launched = false;
	effect.plr = plr;
	return true;
}


/*-- Tests --*/

global func FxIntTestControlTimer(object target, proplist effect)
{
	// Check if all tests have been completed.
	if (effect.testnr > effect.testcount)
	{
		Log("All tests have been successfully completed!");
		return -1;
	}
	
	// Launch new test if needed.
	if (!effect.launched)
	{
		effect.launched = true;
		Call(Format("Test%d", effect.testnr), effect.plr);
	}
		
	// Check whether the current test has been finished.
	if (Call(Format("Test%d_Completed", effect.testnr)))
	{
		effect.launched = false;
		RemoveTest();
		// Log result and increase test number.
		Log("Test %d successfully completed", effect.testnr);
		effect.testnr++;
	}
	return FX_OK;
}

// Simple test of one steady source and one steady consumer.
global func Test1(int plr)
{
	// Power source: one wind generator.
	SetWindFixed(50);
	CreateObject(WindGenerator, 400, 216, plr);
	
	// Power consumer: one workshop.
	var workshop = CreateObject(ToolsWorkshop, 440, 216, plr);
	workshop->CreateContents(Wood, 2);
	workshop->CreateContents(Metal, 2);
	workshop->AddToQueue(Shovel, 2);
	return;
}

global func Test1_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

// Simple test of one steady source and two consumers, only sufficient power for one at a time.
global func Test2(int plr)
{
	// Power source: one wind generator.
	SetWindFixed(50);
	CreateObject(WindGenerator, 400, 216, plr);
	
	// Power consumers: one workshop, one chemical lab.
	var workshop = CreateObject(ToolsWorkshop, 440, 216, plr);
	workshop->CreateContents(Wood, 2);
	workshop->CreateContents(Metal, 2);
	workshop->AddToQueue(Shovel, 2);
	var chemlab = CreateObject(ChemicalLab, 360, 216, plr);
	chemlab->CreateContents(Coal, 2);
	chemlab->CreateContents(Firestone, 2);
	chemlab->AddToQueue(Dynamite, 2);
	return;
}

global func Test2_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2 && ObjectCount(Find_ID(Dynamite)) >= 2)
		return true;
	return false;
}

// Test for one source on demand and one consumer.
global func Test3(int plr)
{
	// Power source: one steam engine.
	var engine = CreateObject(SteamEngine, 380, 216, plr);
	engine->CreateContents(Coal, 1);
	
	// Power consumer: one workshop.
	var workshop = CreateObject(ToolsWorkshop, 440, 216, plr);
	workshop->CreateContents(Wood, 2);
	workshop->CreateContents(Metal, 2);
	workshop->AddToQueue(Shovel, 2);
	return;
}

global func Test3_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

// Test one underproducing steady source, storage and one consumer.
global func Test4(int plr)
{
	// Power source: one wind generator.
	SetWindFixed(25);
	CreateObject(WindGenerator, 400, 216, plr);
	CreateObject(Compensator, 380, 216, plr);
	
	// Power consumer: one workshop.
	var workshop = CreateObject(ToolsWorkshop, 440, 216, plr);
	workshop->CreateContents(Wood, 2);
	workshop->CreateContents(Metal, 2);
	workshop->AddToQueue(Shovel, 2);

	return;
}

global func Test4_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 2)
		return true;
	return false;
}

// Test for alternating steady source, storage and one consumer.
global func Test5(int plr)
{
	// Power source: one wind generator.
	var effect = AddEffect("IntAlternatingWind", nil, 100, 180);
	effect.strength = 100;
	CreateObject(WindGenerator, 400, 216, plr);
	CreateObject(Compensator, 380, 216, plr);
	CreateObject(Compensator, 360, 216, plr);
	
	// Power consumer: one workshop.
	var workshop = CreateObject(ToolsWorkshop, 440, 216, plr);
	workshop->CreateContents(Wood, 4);
	workshop->CreateContents(Metal, 4);
	workshop->AddToQueue(Shovel, 4);
	return;
}

global func Test5_Completed()
{
	if (ObjectCount(Find_ID(Shovel)) >= 4)
		return true;
	return false;
}

/*-- Helper Functions --*/

global func RemoveTest()
{
	// Remove all objects besides the clonk.
	RemoveAll(Find_Not(Find_OCF(OCF_CrewMember)));
	
	// Remove all effects besides the test control effect.
	// TODO
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
	