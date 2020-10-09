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

func InitializePlayer(int player)
{
	// Set zoom to full map size.
	SetPlayerZoomByViewRange(player, LandscapeWidth(), nil, PLRZOOM_Direct);
	
	// No FoW to see everything happening.
	SetFoW(false, player);
		
	// Initialize script player.
	if (GetPlayerType(player) == C4PT_Script)
	{
		// Store the player number.
		if (script_player == nil)
		{
			script_player = player;
		}
		// No crew needed.
		GetCrew(player)->RemoveObject();
		return;
	}
	else
	{
		// Move player to the start of the scenario.
		GetCrew(player)->SetPosition(265, 180);
		LaunchTest(1);
	}
}

func RemovePlayer(int player)
{
	// Remove script player.
	if ((GetPlayerType(player) == C4PT_Script) && (player == script_player))
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

global func Test1_OnStart(int player)
{
	Log("Launch a Clonk diagonally with low speed, should move diagonally");
	var clonk = CreateObject(Clonk, 320, 60, script_player);
	clonk->ScheduleCall(clonk, Global.SetPositionAndSpeed, 9, 0, 320, 60, 0, 0); // Reset velocity
	clonk->ScheduleCall(clonk, Global.SetPositionAndSpeed, 10, 0, 320, 60, 50, 50); // Delayed velocity is better, somehow
	CurrentTest().target = clonk;
	return true;
}

global func Test1_Execute()
{
	var clonk = CurrentTest().target;
	if (clonk.was_launched && clonk->GetSpeed() <= 1)
	{
		Log("Clonk position is %v", clonk->GetPosition());
		doTest("Check X coordinate inside [380, 400], got %v, expected %v", Inside(clonk->GetX(), 380, 400), true);
		doTest("Check Y coordinate inside [140, 160], got %v, expected %v", Inside(clonk->GetY(), 140, 160), true);
		return Evaluate();
	}
	return Wait(100);
}

global func Test1_OnFinished()
{
	if (CurrentTest().target)
	{
		CurrentTest().target->RemoveObject();
	}
}

//-----------------------------------

global func Test2_OnStart(int player)
{
	Log("Launch a Clonk diagonally with high speed, should move diagonally");
	// With the current implementation (object moves in X direction first)
	// the object will hit the wall with its top vertex and move not diagonally,
	// but rather nearly downward
	var clonk = CreateObject(Clonk, 320, 60, script_player);
	clonk->ScheduleCall(clonk, Global.SetPositionAndSpeed, 9, 0, 320, 60, 0, 0); // Reset velocity
	clonk->ScheduleCall(clonk, Global.SetPositionAndSpeed, 10, 0, 320, 60, 1000, 1000); // Delayed velocity is better, somehow
	CurrentTest().target = clonk;
	return true;
}

global func Test2_Execute()
{
	var clonk = CurrentTest().target;
	if (clonk.was_launched && clonk->GetSpeed() <= 1)
	{
		Log("Clonk position is %v", clonk->GetPosition());
		doTest("Check X coordinate inside [380, 400], got %v, expected %v", Inside(clonk->GetX(), 380, 400), true);
		doTest("Check Y coordinate inside [140, 160], got %v, expected %v", Inside(clonk->GetY(), 140, 160), true);
		return Evaluate();
	}
	return Wait(100);
}

global func Test2_OnFinished()
{
	if (CurrentTest().target)
	{
		CurrentTest().target->RemoveObject();
	}
}

//-----------------------------------

global func Test3_OnStart(int player)
{
	Log("Launch a rock diagonally with high speed, should move diagonally");
	// Objects with smaller vertices do not hit the wall, even at high velocity
	var rock = CreateObject(Rock, 320, 60, script_player);
	rock->ScheduleCall(rock, Global.SetPositionAndSpeed, 9, 0, 320, 60, 0, 0); // Reset velocity
	rock->ScheduleCall(rock, Global.SetPositionAndSpeed, 10, 0, 320, 60, 1000, 1000); // Delayed velocity is better, somehow
	CurrentTest().target = rock;
	return true;
}

global func Test3_Execute()
{
	var rock = CurrentTest().target;
	if (rock.was_launched && rock->GetSpeed() <= 1)
	{
		Log("Rock position is %v", rock->GetPosition());
		doTest("Check X coordinate > 380, got %v, expected %v", rock->GetX() > 380, true); // Rock moves upward as redirected movement
		doTest("Check Y coordinate inside [140, 160], got %v, expected %v", Inside(rock->GetY(), 140, 160), true);
		return Evaluate();
	}
	return Wait(100);
}

global func Test3_OnFinished()
{
	if (CurrentTest().target)
	{
		CurrentTest().target->RemoveObject();
	}
}

