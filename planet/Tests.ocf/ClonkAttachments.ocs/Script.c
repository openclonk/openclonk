/**
	Clonk Attachments
	
	Unit tests for displaying wearables and tools on the Clonk,
	related to bug #1974 - this is not in the issues folder, though,
	because the test logic is a lot different - namely you cannot
	do everything in one function call for most tests.
	
	Invokes tests by calling the global function Test*_OnStart(int plr)
	and iterate through all  tests.
	The test is completed once Test*_Completed() returns true.
	Then Test*_OnFinished() is called, to be able to reset 
	the scenario for the next test.
	
	With LaunchTest(int nr) a specific test can be launched when
	called during runtime. A test can be skipped by calling the
	function SkipTest().
	
	@author Maikel (test logic), Marky (actual tests)
*/

static const HLINE = "=====================================";

static script_plr, test;

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
	test = CreateEffect(IntTestControl, 100, 2);
	test.testnr = 1;
	test.launched = false;
	test.plr = plr;
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
	if (!test)
	{
		// Create a new control effect and launch the test.
		test = CreateEffect(IntTestControl, 100, 2);
		test.testnr = nr;
		test.launched = false;
		test.plr = GetPlayerByIndex(0, C4PT_User);
		return;
	}
	// Finish the currently running test.
	Call(Format("~Test%d_OnFinished", test.testnr));
	// Start the requested test by just setting the test number and setting 
	// test.launched to false, effect will handle the rest.
	test.testnr = nr;
	test.launched = false;
	return;
}

// Calling this function skips the current test, does not work if last test has been ran already.
global func SkipTest()
{
	if (!test)
		return;
	// Finish the previous test.
	Call(Format("~Test%d_OnFinished", test.testnr));
	// Start the next test by just increasing the test number and setting 
	// test.launched to false, effect will handle the rest.
	test.testnr++;
	test.launched = false;
	return;
}


/*-- Test Effect --*/


static const IntTestControl = new Effect
{
	Timer = func ()
	{
		// Launch new test if needed.
		if (!this.launched)
		{
			// Log test start.
			Log(HLINE);
			Log("Test %d started:", this.testnr);
			// Start the test if available, otherwise finish test sequence.
			if (!Call(Format("~Test%d_OnStart", this.testnr), this.plr))
			{
				Log("Test %d not available, the previous test was the last test.", this.testnr);
				Log(HLINE);
				Log("All tests have been successfully completed!");
				return FX_Execute_Kill;
			}
			this.launched = true;
		}
		// Check whether the current test has been finished.
		if (Call(Format("Test%d_Completed", this.testnr)))
		{
			this.launched = false;
			//RemoveTest();
			// Call the test on finished function.
			Call(Format("~Test%d_OnFinished", this.testnr));
			// Log result and increase test number.
			Log("Test %d successfully completed.", this.testnr);
			this.testnr++;
		}
		return FX_OK;
	},
};


global func IsWaiting()
{
	if (test)
	{
		var wait = test.wait > 0;
		test.wait -= 1;
		return wait;	
	}
	return false;
}

global func Wait(int amount)
{
	test.wait = Max(0, amount);
}

/*-- Liquid Tests --*/

global func Test1_OnStart(int plr)
{
	Log("Tools carried visibly on the Clonk when dying should be visible when picked up again");
	
	if (test == nil)
	{
		Log("No test context");
		return false;
	}
	
	// Get all definitions that are collectible
	test.test1_collectible_defs = [];
	for (var i = 0, def; def = GetDefinition(i); ++i)
	{
		if (def.Collectible		// Only collectible items
		 && def.GetCarryMode) 	// Only things that can be displayed on the Clonk -> so, rocks, etc. are sorted out for now
		{
			PushBack(test.test1_collectible_defs, def);
		}
	}
	
	// init data
	test.test1_length = GetLength(test.test1_collectible_defs);
	test.test1_index = 0;
	test.test1_item = [];
	test.test1_corpse = [];
	test.test1_corpse_display = [];
	test.test1_collector = [];
	test.test1_collector_display = [];
	test.test1_started = [];
	test.test1_finished = [];
	test.test1_passed = [];
	test.test1_comment = [];
	test.test1_store = CreateObject(Dummy);

	return true;
}

global func Test1_Completed()
{
	GetCursor(test.plr)->Message("Test %d / %d", test.test1_index + 1, test.test1_length);

	if (IsWaiting()) return false;

	if (test.test1_index >= test.test1_length)
	{
		Log("Tested all collectible definitions with display, exiting now");
		return Test1_EvaluateResult();
	}
	Test1_Run();
	return false;
}

global func Test1_OnFinished()
{
	if (test.test1_store)
	{
		test.test1_store->RemoveObject();
	}

	return;
}

global func Test1_Run()
{
	var cursor = GetCursor(test.plr);
	var index = test.test1_index;
	var corpse = test.test1_corpse[index];
	var item = test.test1_item[index];
	var collector = test.test1_collector[index];

	// Already done?
	if (test.test1_finished[index])
	{
		if (corpse) corpse.Visibility = VIS_None;
		if (collector) collector.Visibility = VIS_None;
		if (item) item->Enter(test.test1_store);
		RemoveAll(Find_ID(Attacher));
		test.test1_index += 1;
		return;
	}
	
	// Create an item
	if (!item || !test.test1_started[index])
	{
		test.test1_item[index] = item = CreateObject(test.test1_collectible_defs[index], 0, 0, script_plr);
		test.test1_started[index] = true;
	}
	
	var collect_delay = 10;
	if (item->~IsCarryHeavy()) collect_delay += 30;
	
	// Create someone to collect it later
	if (!collector)
	{
		test.test1_collector[index] = collector = CreateObject(Clonk, cursor->GetX() + 50, cursor->GetY(), script_plr);
	}

	// Create someone that drops the item while it is on his back
	if (corpse)
	{
		// Kill the Clonk
		if (corpse->GetAlive())
		{
			corpse.silent_death = true; // We don't want sound every time ;)
			corpse->Kill();
			Wait(15);
			return;
		}
		
		if (!item)
		{
			test.test1_passed[index] = true;
			test.test1_comment[index] = "Item got removed after death";
			test.test1_finished[index] = true;
			return;
		}
		
		var attachment_id = 2; // The item is usually attached with this ID, because 1 is the backpack
		
		if (test.test1_corpse_display[index] == nil)
		{
			test.test1_corpse_display[index] = Test1_IsAttached(corpse, attachment_id + 1); // the corpse also had the attacher
		}
		
		if (item->Contained() != collector)
		{
			item->SetPosition(collector->GetX(), collector->GetY());
			collector->Collect(item);
			Wait(collect_delay);
			return;
		}
		
		if (test.test1_collector_display[index] == nil)
		{
			test.test1_collector_display[index] = Test1_IsAttached(collector, attachment_id);
		}
		
		test.test1_passed[index] = !test.test1_corpse_display[index] && test.test1_collector_display[index]; // Should not be on the corpse, but on the guy who is holding it
		test.test1_finished[index] = true;
		test.test1_comment[index] = Format("Item is displayed: on the corpse (%v), on the collector (%v)", test.test1_corpse_display[index], test.test1_collector_display[index]);
	}
	else
	{
		test.test1_corpse[index] = corpse = CreateObject(Clonk, cursor->GetX() + 100, cursor->GetY(), script_plr);
		item->SetPosition(corpse->GetX(), corpse->GetY());
		corpse->Collect(test.test1_item[index]);
		corpse->CreateContents(Attacher); // Create something else, so that the actual item is displayed in the quick slot
		corpse->ShiftContents(nil, Attacher);
		//corpse->FindContents(Attacher).Visibility = VIS_None;
		Wait(collect_delay);
	}
}


global func Test1_EvaluateResult()
{
	Log(HLINE);
	Log("Evaluating test 1 for the following definitions");
	
	var result = true;
	for (var i = 0; i < GetLength(test.test1_collectible_defs); ++i)
	{
		var predicate;
		if (test.test1_passed[i])
		{
			predicate = "Pass";
		}
		else
		{
			predicate = "Fail";
			result = false;
		}
	
		Log("- [%s] Definition: %i - %s", predicate, test.test1_collectible_defs[i], test.test1_comment[i]);
	}
	Log(HLINE);
	if (!result)
	{
		SkipTest();
	}
	return result;
}


global func Test1_IsAttached(object clonk, int attachment)
{
	return clonk->SetAttachTransform(attachment, Trans_Identity()); // Returns false if the attachment does not exist
}
