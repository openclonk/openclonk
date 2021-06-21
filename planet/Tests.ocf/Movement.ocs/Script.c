/**
	Movement
	
	Movement test for various movement situations.
	
	@author Marky
*/


static script_player;

func Initialize()
{
	// Create a script player for some tests.
	script_player = nil;
	CreateScriptPlayer("Buddy", RGB(0, 0, 255), nil, CSPF_NoEliminationCheck);
}

func InitializePlayer(proplist player)
{
	// Set zoom to full map size.
	player->SetZoomByViewRange(LandscapeWidth(), nil, PLRZOOM_Direct);
	
	// No FoW to see everything happening.
	player->SetFoW(false);
		
	// Initialize script player.
	if (player.Type == C4PT_Script)
	{
		// Store the player number.
		if (script_player == nil)
		{
			script_player = player;
		}
		// No crew needed.
		player->GetCrew()->RemoveObject();
		return;
	}
	else
	{
		// Move player to the start of the scenario.
		player->GetCrew()->SetPosition(265, 180);
		LaunchTest(8);
	}
}

func RemovePlayer(proplist player)
{
	// Remove script player.
	if ((player.Type == C4PT_Script) && (player == script_player))
	{
		script_player = nil;
	}
}


/* --- Test Control --- */

// Aborts the current test and launches the specified test instead.
global func LaunchTest(int nr)
{
	// Get the control test.
	var test = CurrentTest();
	if (test)
	{
		// Finish the currently running test.
		Call(Format("~Test%d_OnFinished", test.testnr));
		// Start the requested test by just setting the test number and setting 
		// test.launched to false, effect will handle the rest.
	}
	else
	{
		// Create a new control effect and launch the test.
		test = CurrentTest(true);
		test.player = GetPlayerByIndex(0, C4PT_User);
		test.global_result = true;
		test.count_total = 0;
		test.count_failed = 0;
		test.count_skipped = 0;
	}

	test.testnr = nr;
	test.launched = false;
}


// Calling this function skips the current test, does not work if last test has been ran already.
global func SkipTest()
{
	// Get the control test.
	var test = CurrentTest();
	if (test)
	{
		// Finish the previous test.
		Call(Format("~Test%d_OnFinished", test.testnr));
		// Start the next test by just increasing the test number and setting 
		// test.launched to false, effect will handle the rest.
		test.testnr++;
		test.launched = false;
		test.count_skipped++;
	}
}


/* --- Test Effect --- */

static const IntTestControl = new Effect
{
	Start = func (int temporary)
	{
		if (!temporary)
		{
			// Set default interval.
			this.Interval = 1;
		}
		return FX_OK;
	},

	Timer = func ()
	{
		// Launch new test if needed.
		if (!this.launched)
		{
			// Start the test if available, otherwise finish test sequence.
			if (!this->HasNextTest())
			{
				Log("Test %d not available, the previous test was the last test.", this.testnr);
				Log("=====================================");
				Log("All tests have been completed!");
				Log("* %d tests total", this.count_total);
				Log("%d tests failed", this.count_failed);
				Log("%d tests skipped", this.count_skipped);
				Log("=====================================");
				if (this.count_skipped == 0 && this.count_failed == 0 && this.count_total > 0)
				{
					Log("All tests passed!");
				}
				else
				{
					Log("At least one test failed or was skipped!");
				}
				return FX_Execute_Kill;
			}
			// Log test start.
			Log("=====================================");
			Log("Test %d started:", this.testnr);
			this.launched = true;
			this.count_total++;
			this.current_result = false;
			this.current_check = true;
		}

		// waiting
		if (this.wait > 0)
		{
			this.wait -= 1;
			return FX_OK;
		}

		// Check whether the current test has been finished.
		if (this->ExecuteTest())
		{
			this.launched = false;

			if (this.current_result)
			{
				Log(">> Test %d passed.", this.testnr);
			}
			else
			{
				Log(">> Test %d failed.", this.testnr);
				this.count_failed++;
			}

			// Update global result
			this.global_result &= this.current_result;

			// Call the test on finished function.
			this->CleanupTest();
			// Log result and increase test number.
			Log("Test %d successfully completed.", this.testnr);
			this.testnr++;
		}
		return FX_OK;
	},


	GetIndex = func () // Get the index of the test
	{
		return this.testnr - 1;
	},

	GetNumber = func () // Get the test number
	{
		return this.testnr;
	},


	HasNextTest = func ()
	{
		return Call(Format("~Test%d_OnStart", this.testnr), this.player);
	},


	ExecuteTest = func ()
	{
		return Call(Format("Test%d_Execute", this.testnr));
	},


	CleanupTest = func ()
	{
		Call(Format("~Test%d_OnFinished", this.testnr));
	},
};


global func doTest(description, returned, expected)
{
	var test;

	if (GetType(returned) == C4V_PropList || GetType(returned) == C4V_Array)
	{
		test = DeepEqual(returned, expected);
	}
	else
	{
		test = (returned == expected);
	}

	var predicate = "[Fail]";
	if (test) predicate = "[Pass]";

	Log(Format("%s %s", predicate, description), returned, expected);

	CurrentTest().current_check &= test;
	return test;
}


global func CurrentTest(bool create)
{
	if (create)
	{
		return Scenario->CreateEffect(IntTestControl, 100, 2);
	}
	else
	{
		return GetEffect("IntTestControl", Scenario);
	}
}

global func Evaluate()
{
	var test = CurrentTest();
	test.current_result = test.current_check;
	return true;
}


global func PassTest()
{
	CurrentTest().current_result = true;
	return true;
}


global func FailTest()
{
	CurrentTest().current_result = false;
	return true;
}


global func Wait(int amount)
{
	CurrentTest().wait = Max(0, amount ?? 10);
	return false;
}


/*-- The actual tests --*/

global func SetPositionAndSpeed(int x, int y, int xdir, int ydir)
{
	SetPosition(x, y);
	SetXDir(xdir);
	SetYDir(ydir);
	this.was_launched = true;
}

//-----------------------------------

global func LaunchTest_Start(def type, int x, int y, int x_dir, int y_dir)
{
	CurrentTest().victim_test = false;
	CurrentTest().victim_died = false;
	var rock = CreateObject(type, x, y, script_player);
	rock->ScheduleCall(rock, Global.SetPositionAndSpeed, 9, 0, x, y, 0, 0); // Reset velocity
	rock->ScheduleCall(rock, Global.SetPositionAndSpeed, 10, 0, x, y, x_dir, y_dir); // Delayed velocity is better, somehow
	CurrentTest().target = rock;
	return true;
}

global func LaunchTest_Execute(int min_x, int max_x, int min_y, int max_y)
{
	var rock = CurrentTest().target;
	if (rock.was_launched && rock->GetSpeed() <= 1)
	{
		Log("%i position is %v", rock->GetID(), rock->GetPosition());
		TestBounds("X coordinate", rock->GetX(), min_x, max_x);
		TestBounds("Y coordinate", rock->GetY(), min_y, max_y);
		if (CurrentTest().victim_test)
		{
			doTest("Victim should be hit, got %v, expected %v", CurrentTest().victim_died, true);
		}
		return Evaluate();
	}
	return Wait(100);
}

global func LaunchTest_Finish()
{
	if (CurrentTest().target)
	{
		CurrentTest().target->RemoveObject();
	}
	if (CurrentTest().victim)
	{
		CurrentTest().victim->RemoveObject();
	}
}


global func LaunchTest_AddVictim(def type, int x, int y)
{
	var victim = CreateObject(type, x, y, script_player);
	victim->DoEnergy(1 - victim->GetEnergy());
	CurrentTest().victim = victim;
	CurrentTest().victim_test = true;
	return true;
}

public func OnClonkDeath(object clonk, proplist killer)
{
	if (clonk->GetOwner() == script_player)
	{
		CurrentTest().victim_died = true;
	}
}

global func TestBounds(string description, int value, int min, int max)
{
	if (min != nil && max != nil)
	{
		doTest(Format("Check %s = %d inside [%d, %d], got %%v, expected %%v", description, value, min, max), Inside(value, min, max), true);
	}
	else if (min != nil)
	{
		doTest(Format("Check %s = %d > %d, got %%v, expected %%v", description, value, min), value > min, true);
	}
	else if (max != nil)
	{
		doTest(Format("Check %s = %d < %d, got %%v, expected %%v", description, value, max), value < max, true);
	}
}

//-----------------------------------

global func Test1_OnStart(proplist player)
{
	Log("Launch a Clonk diagonally with low speed, should move diagonally");
	return LaunchTest_Start(Clonk, 320, 60, 50, 50);
}

global func Test1_Execute()
{
	return LaunchTest_Execute(380, 400, 140, 160);
}

global func Test1_OnFinished()
{
	LaunchTest_Finish();
}

//-----------------------------------

global func Test2_OnStart(proplist player)
{
	Log("Launch a Clonk diagonally with high speed, should move diagonally");
	// With the current implementation (object moves in X direction first)
	// the object will hit the wall with its top vertex and move not diagonally,
	// but rather nearly downward
	return LaunchTest_Start(Clonk, 320, 60, 1000, 1000);
}

global func Test2_Execute()
{
	return LaunchTest_Execute(380, 400, 140, 160);
}

global func Test2_OnFinished()
{
	LaunchTest_Finish();
}

//-----------------------------------

global func Test3_OnStart(proplist player)
{
	// Objects with smaller vertices do not hit the wall, even at high velocity
	return LaunchTest_Start(Rock, 320, 60, 1000, 1000);
}

global func Test3_Execute()
{
	return LaunchTest_Execute(380, nil, 140, 160);
}

global func Test3_OnFinished()
{
	LaunchTest_Finish();
}

//-----------------------------------

global func Test4_OnStart(proplist player)
{
	return LaunchTest_Start(Rock, 444, 233, 100, 100) && LaunchTest_AddVictim(Clonk, 480, 270);
}

global func Test4_Execute()
{
	return LaunchTest_Execute(484, 495, 280, 290);
}

global func Test4_OnFinished()
{
	LaunchTest_Finish();
}

//-----------------------------------

global func Test5_OnStart(proplist player)
{
	return LaunchTest_Start(Rock, 444, 233, 1000, 1000) && LaunchTest_AddVictim(Clonk, 480, 270);
}

global func Test5_Execute()
{
	return LaunchTest_Execute(484, 495, 280, 290);
}

global func Test5_OnFinished()
{
	LaunchTest_Finish();
}

//-----------------------------------

global func Test6_OnStart(proplist player)
{
	return LaunchTest_Start(Rock, 444, 233, 10000, 10000) && LaunchTest_AddVictim(Clonk, 480, 270);
}

global func Test6_Execute()
{
	return LaunchTest_Execute(484, 495, 280, 290);
}

global func Test6_OnFinished()
{
	LaunchTest_Finish();
}

global func Test8_OnStart(proplist player)
{
	return LaunchTest_Start(Rock, 444, 233, 60, 50) && LaunchTest_AddVictim(Clonk, 480, 270);
}

global func Test8_Execute()
{
	return LaunchTest_Execute(484, 495, 280, 290);
}

global func Test8_OnFinished()
{
	LaunchTest_Finish();
}
