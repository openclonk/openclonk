/**
	Kill Tracing
	Unit tests for kill tracing. Invokes tests by calling the 
	global function Test*_OnStart(int plr) and iterate through all 
	tests. The test is completed once Test*_Completed() returns
	true. Then Test*_OnFinished() is called, to be able to reset 
	the scenario for the next test.
	
	With LaunchTest(int nr) a specific test can be launched when
	called during runtime. A test can be skipped by calling the
	function SkipTest().
	
	@author Maikel
*/


static plr_victim;
static plr_killer;
static plr_killer_fake;

protected func Initialize()
{
	// Create script players for these tests.
	CreateScriptPlayer("Victim", RGB(0, 0, 255), nil, CSPF_NoEliminationCheck);
	CreateScriptPlayer("Killer", RGB(0, 255, 0), nil, CSPF_NoEliminationCheck);
	CreateScriptPlayer("KillerFake", RGB(255, 0, 0), nil, CSPF_NoEliminationCheck);
	return;
}

protected func InitializePlayer(int plr)
{
	// Initialize script player.
	if (GetPlayerType(plr) == C4PT_Script)
	{
		// Store the player numbers.
		if (GetPlayerName(plr) == "Victim")
			plr_victim = plr;
		else if (GetPlayerName(plr) == "Killer")
			plr_killer = plr;
	 	else if (GetPlayerName(plr) == "KillerFake")
			plr_killer_fake = plr;
		// Make crew of killers invincible.	
		if (plr == plr_killer || plr == plr_killer_fake)
		{
			GetCrew(plr)->MakeInvincible();
			GetCrew(plr)->SetPosition(50, 150);
		}
		return;
	}
	
	// Set zoom to full map size.
	SetPlayerZoomByViewRange(plr, LandscapeWidth(), nil, PLRZOOM_Direct);
	
	// No FoW to see everything happening.
	SetFoW(false, plr);
	
	// Move normal players into a relaunch container.
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2);
	GetCrew(plr)->Enter(relaunch);
	
	// Add test control effect.
	var effect = AddEffect("IntTestControl", nil, 100, 2);
	effect.testnr = 1;
	effect.launched = false;
	effect.plr = plr;
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
	Log("=====================================");
	return FX_OK;
}

global func FxIntTestControlTimer(object target, proplist effect)
{
	// Launch new test if needed.
	if (!effect.launched)
	{
		// Start the test if available, otherwise finish test sequence.
		if (!Call(Format("~Test%d_OnStart", effect.testnr)))
		{
			Log("=====================================");
			Log("All tests have been completed!");
			return FX_Execute_Kill;
		}
		effect.launched = true;
	}
	return FX_OK;
}

global func FxIntTestControlOnDeath(object target, proplist effect, int killer, object clonk)
{
	Log("Test %d (%s): %v", effect.testnr, Call(Format("~Test%d_Log", effect.testnr)), plr_killer == killer);
	effect.launched = false;
	effect.testnr++;
	// Remove all objects except the living crew members.
	RemoveAll(Find_Not(Find_Or(Find_OCF(OCF_CrewMember), Find_ID(RelaunchContainer))), Find_Exclude(clonk));
	// Remove all landscape changes.
	ClearFreeRect(0, 0, LandscapeWidth(), 160);
	return FX_OK;
}

public func OnClonkDeath(object clonk, int killer)
{
	if (clonk->GetOwner() != plr_victim)
		return;
	var effect = GetEffect("IntTestControl");
	EffectCall(nil, effect, "OnDeath", killer, clonk);
	return;
}

public func RelaunchPlayer(int plr)
{
	var clonk = CreateObjectAbove(Clonk, 100, 150, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	clonk->DoEnergy(100000);
	return;
}


/*-- Kill Tracing Tests --*/

global func Test1_Log() { return "Object throw"; }
global func Test1_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(90, 150);
	victim->DoEnergy(-49);
	
	var throwing_obj = killer->CreateContents(Rock);
	killer->ObjectCommand("Throw", throwing_obj, 20, -10);
	return true;
}


global func Test2_Log() { return "Lantern fire"; }
global func Test2_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	var fake_killer = GetCrew(plr_killer_fake);
	
	victim->SetPosition(130, 150);
	victim->DoEnergy(-45);

	var lantern = fake_killer->CreateContents(Lantern);
	lantern->Enter(killer);
	killer->ObjectCommand("Throw", lantern, 20, -10);
	return true;
}

global func Test3_Log() { return "Cannon shot"; }
global func Test3_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	var fake_killer = GetCrew(plr_killer_fake);
	
	victim->SetPosition(140, 150);
	victim->DoEnergy(-45);

	var cannon = killer->CreateObject(Cannon);
	cannon->CreateContents(PowderKeg);
	fake_killer->CreateContents(Shield)->Enter(killer);
	cannon->ControlUseStart(killer, 20, 0);
	cannon->ControlUseStop(killer, 20, 0);
	return true;
}

global func Test4_Log() { return "Wallkit asphyxiation"; }
global func Test4_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(55, 150);
	victim->DoEnergy(-49);
	victim->DoBreath(- 9 * victim.MaxBreath / 10);

	var wallkit = killer->CreateContents(WallKit);
	wallkit->ControlUseStart(killer, 20, 0);
	wallkit->ControlUseStop(killer, 20, 0);
	return true;
}

