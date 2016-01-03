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
		// Make crew of killers invincible and set position.	
		if (plr == plr_killer)
		{
			GetCrew(plr)->MakeInvincible(true);
			GetCrew(plr)->SetPosition(50, 150);
		}
		else if (plr == plr_killer_fake)
		{
			GetCrew(plr)->MakeInvincible(true);
			GetCrew(plr)->SetPosition(20, 150);
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
	effect.results = [];
	Log("=====================================");
	return FX_OK;
}

global func FxIntTestControlTimer(object target, proplist effect)
{
	// Launch new test if needed.
	if (!effect.launched)
	{
		// Remove all objects except the living crew members.
		RemoveAll(Find_Not(Find_Or(Find_OCF(OCF_CrewMember), Find_ID(RelaunchContainer))));
		// Remove all landscape changes.
		DrawMaterialQuad("Brick", 0, 160, LandscapeWidth(), 160, LandscapeWidth(), LandscapeHeight(), 0, LandscapeHeight());
		ClearFreeRect(0, 0, LandscapeWidth(), 160);
		// Extinguish all remaining crew members and restore positions.
		for (var crew in FindObjects(Find_OCF(OCF_CrewMember)))
		{
			crew->Extinguish();
			if (crew->GetOwner() == plr_killer)
				crew->SetPosition(50, 150);
			else if (crew->GetOwner() == plr_killer_fake)
				crew->SetPosition(20, 150);
		}
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

global func FxIntTestControlStop(object target, proplist effect, int reason, bool temporary)
{
	if (temporary)
		return FX_OK;
	CreateWikiOverviewTable(effect.results);
	return FX_OK;
}

global func FxIntTestControlOnDeath(object target, proplist effect, int killer, object clonk)
{
	// Log the result.
	Log("Test %d (%s): %v", effect.testnr, Call(Format("~Test%d_Log", effect.testnr)), plr_killer == killer);
	// Store the result.
	effect.results[effect.testnr - 1] = (plr_killer == killer);
	effect.launched = false;
	effect.testnr++;
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
	victim->DoBreath(- 95 * victim.MaxBreath / 100);

	var wallkit = killer->CreateContents(WallKit);
	wallkit->ControlUseStart(killer, 20, 0);
	wallkit->ControlUseStop(killer, 20, 0);
	return true;
}

global func Test5_Log() { return "Sword strike"; }
global func Test5_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(55, 150);
	victim->DoEnergy(-45);

	var sword = killer->CreateContents(Sword);
	sword->ControlUse(killer, 20, 0);
	return true;
}

global func Test6_Log() { return "Club strike"; }
global func Test6_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(55, 150);
	victim->DoEnergy(-45);

	var club = killer->CreateContents(Club);
	club->ControlUseStart(killer, 20, 0);
	club->ControlUseStop(killer, 20, 0);
	return true;
}

global func Test7_Log() { return "Axe strike"; }
global func Test7_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(55, 150);
	victim->DoEnergy(-45);

	var axe = killer->CreateContents(Axe);
	axe->ControlUseStart(killer, 20, 0);
	axe->ControlUseStop(killer, 20, 0);
	return true;
}

global func Test8_Log() { return "Firestone explosion (thrown)"; }
global func Test8_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(110, 150);
	victim->DoEnergy(-40);

	var firestone = killer->CreateContents(Firestone);
	killer->ObjectCommand("Throw", firestone, 20, -10);
	return true;
}

global func Test9_Log() { return "Club shot"; }
global func Test9_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(140, 150);
	victim->DoEnergy(-45);

	var club = killer->CreateContents(Club);
	CreateObject(Rock, killer->GetX() + 5, killer->GetY() - 10); 
	club->ControlUseStart(killer, 20, 0);
	club->ControlUseStop(killer, 20, 0);
	return true;
}

global func Test10_Log() { return "Object moved by explosion"; }
global func Test10_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(276, 150);
	victim->DoEnergy(-45);
		
	CreateObject(Rock, 124, 158);
	var firestone = killer->CreateContents(Firestone);
	killer->SetHandAction(0);
	killer->ControlThrow(firestone, 20, -20);
	return true;
}

global func Test11_Log() { return "Firestone material cascade"; }
global func Test11_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	DrawMaterialQuad("Firestone", 110, 160, 170, 160, 170, 220, 110, 220);
	DrawMaterialQuad("Tunnel", 110, 220, 170, 220, 170, 260, 110, 260);
	
	victim->SetPosition(140, 250);
	victim->DoEnergy(-45);
		
	var firestone = killer->CreateContents(Firestone);
	killer->SetHandAction(0);
	killer->ControlThrow(firestone, 20, -20);
	return true;
}

global func Test12_Log() { return "Bow shot (arrow)"; }
global func Test12_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(240, 150);
	victim->DoEnergy(-45);
		
	var bow = killer->CreateContents(Bow);
	bow->CreateContents(Arrow);
	bow->ControlUseStart(killer, 20, -4);
	bow->ControlUseStop(killer, 20, -4);
	return true;
}

global func Test13_Log() { return "Bow shot (fire arrow)"; }
global func Test13_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(240, 150);
	victim->DoEnergy(-43);
		
	var bow = killer->CreateContents(Bow);
	bow->CreateContents(FireArrow);
	bow->ControlUseStart(killer, 20, -4);
	bow->ControlUseStop(killer, 20, -4);
	return true;
}

global func Test14_Log() { return "Bow shot (bomb arrow)"; }
global func Test14_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(240, 150);
	victim->DoEnergy(-40);
		
	var bow = killer->CreateContents(Bow);
	bow->CreateContents(BombArrow);
	bow->ControlUseStart(killer, 20, -4);
	bow->ControlUseStop(killer, 20, -4);
	return true;
}

global func Test15_Log() { return "Musket shot"; }
global func Test15_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(240, 150);
	victim->DoEnergy(-45);
		
	var musket = killer->CreateContents(Musket);
	musket->CreateContents(LeadShot);
	musket.loaded = true;
	musket->ControlUseStart(killer, 20, -8);
	musket->ControlUseStop(killer, 20, -8);
	return true;
}

global func Test16_Log() { return "Javelin throw"; }
global func Test16_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(200, 150);
	victim->DoEnergy(-35);
		
	var javelin = killer->CreateContents(Javelin);
	javelin->ControlUseStart(killer, 20, -8);
	javelin->ControlUseStop(killer, 20, -8);
	return true;
}

global func Test17_Log() { return "Grenade launcher shot"; }
global func Test17_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(200, 150);
	victim->DoEnergy(-35);
		
	var launcher = killer->CreateContents(GrenadeLauncher);
	launcher->CreateContents(IronBomb);
	launcher.loaded = true;
	launcher->ControlUseStart(killer, 20, -10);
	launcher->ControlUseStop(killer, 20, -10);
	return true;
}

global func Test18_Log() { return "Iron bomb explosion (thrown)"; }
global func Test18_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(170, 150);
	victim->DoEnergy(-35);
		
	var bomb = killer->CreateContents(IronBomb);
	bomb->ControlUse(killer);
	killer->SetHandAction(0);
	killer->ControlThrow(bomb, 20, -20);
	return true;
}

global func Test19_Log() { return "Dynamite explosion (thrown)"; }
global func Test19_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(140, 150);
	victim->DoEnergy(-45);
		
	var dynamite = killer->CreateContents(Dynamite);
	dynamite->ControlUse(killer);
	killer->SetHandAction(0);
	killer->ControlThrow(dynamite, 20, -20);
	return true;
}

global func Test20_Log() { return "Powder keg (shot by musket)"; }
global func Test20_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(250, 150);
	victim->DoEnergy(-45);
		
	CreateObject(PowderKeg, 240, 140);
	var musket = killer->CreateContents(Musket);
	musket->CreateContents(LeadShot);
	musket.loaded = true;
	musket->ControlUseStart(killer, 20, -8);
	musket->ControlUseStop(killer, 20, -8);
	return true;
}

global func Test21_Log() { return "Incineration by burning clonk"; }
global func Test21_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	var fake_killer = GetCrew(plr_killer_fake);

	victim->SetPosition(120, 150);
	victim->DoEnergy(-45);
	
	fake_killer->Incinerate(100, plr_killer_fake);
	killer->SetCommand("MoveTo", nil, fake_killer->GetX(), fake_killer->GetY());
	killer->AppendCommand("Wait", nil, nil, nil, nil, nil, 150);
	killer->AppendCommand("MoveTo", nil, victim->GetX(), victim->GetY());
	killer->AppendCommand("Wait", nil, nil, nil, nil, nil, 150);
	killer->AppendCommand("MoveTo", nil, killer->GetX(), killer->GetY());
	return true;
}

/*-- Wiki Overview Table --*/

global func CreateWikiOverviewTable(array results)
{
	var table = "{| border=\"1\" class=\"wikitable\"\n|-\n! Kill Description\n! Traced?\n";
	for (var index = 1; index <= GetLength(results); index++)
	{
		var result = results[index - 1];
		if (result == nil)
			continue;
		var success = "<span style=\"color:#FF0000\">false</span>";
		if (result)
			success = "<span style=\"color:#00CC00\">true</span>";
		var entry = Format("|-\n| %s\n| %s\n", Call(Format("~Test%d_Log", index)), success);
		table = Format("%s%s", table, entry);
	}
	var table_closing = "|-\n|}";
	table = Format("%s%s", table, table_closing);
	Log(table);
}