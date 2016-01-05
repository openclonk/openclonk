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

static const TEST_START_NR = 1;

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
	effect.testnr = TEST_START_NR;
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
	Log("Running kill tests: in each test K is");
	Log("supposed to be the killer, V is the  ");
	Log("the victim, and F is a fake killer   ");
	Log("=====================================");
	return FX_OK;
}

global func FxIntTestControlTimer(object target, proplist effect)
{
	// Launch new test if needed.
	if (!effect.launched)
	{
		InitTest();
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
	Log("Test %d [%s]: %v, killer = %s", effect.testnr, Call(Format("~Test%d_Log", effect.testnr)), plr_killer == killer, GetPlayerName(killer));
	// Store the result.
	effect.results[effect.testnr - 1] = (plr_killer == killer);
	effect.launched = false;
	effect.testnr++;
	return FX_OK;
}

global func InitTest()
{
	// Remove all objects except the player crew members and relaunch container they are in.
	for (var obj in FindObjects(Find_Not(Find_ID(RelaunchContainer))))
		if (!((obj->GetOCF() & OCF_CrewMember) && (GetPlayerType(obj->GetOwner()) == C4PT_User || obj->GetOwner() == plr_victim)))
			obj->RemoveObject();

	// Remove all landscape changes.
	DrawMaterialQuad("Brick", 0, 160, LandscapeWidth(), 160, LandscapeWidth(), LandscapeHeight(), 0, LandscapeHeight());
	ClearFreeRect(0, 0, LandscapeWidth(), 160);
	
	// Give script players new crew.
	for (var plr in [plr_victim, plr_killer, plr_killer_fake])
	{	
		if (!GetCrew(plr))
		{
			var clonk = CreateObjectAbove(Clonk, 100, 150, plr);
			clonk->MakeCrewMember(plr);
			clonk->SetDir(DIR_Right);
			SetCursor(plr, clonk);
			clonk->DoEnergy(100000);
		}
	}
	GetCrew(plr_victim)->SetPosition(100, 150);
	GetCrew(plr_killer)->SetPosition(50, 150);
	GetCrew(plr_killer_fake)->SetPosition(20, 150);
	return;
}

global func GetPlayerName(int plr)
{
	if (plr == NO_OWNER)
		return "NO_OWNER";
	return _inherited(plr, ...);
}

public func OnClonkDeath(object clonk, int killer)
{
	if (clonk->GetOwner() != plr_victim)
		return;
	var effect = GetEffect("IntTestControl");
	EffectCall(nil, effect, "OnDeath", killer, clonk);
	return;
}


/*-- Kill Tracing Tests --*/

global func Test1_Log() { return "K throws object on V"; }
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


global func Test2_Log() { return "K throws lantern to V (V dies of lantern fire)"; }
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

global func Test3_Log() { return "K shoots object with cannon at V"; }
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

global func Test4_Log() { return "K placed wallkit on V (V dies of asphyxiation)"; }
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

global func Test5_Log() { return "K strikes V with a sword"; }
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

global func Test6_Log() { return "K strikes V with a club"; }
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

global func Test7_Log() { return "K strikes V with an axe"; }
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

global func Test8_Log() { return "K throws firestone at V (V dies of explosion)"; }
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

global func Test9_Log() { return "K shoots object with club at V"; }
global func Test9_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(260, 150);
	victim->DoEnergy(-45);

	var club = killer->CreateContents(Club);
	CreateObject(Rock, killer->GetX() + 4, killer->GetY() - 4); 
	ScheduleCall(club, "ControlUseStart", 2, 0, killer, 20, -10);
	ScheduleCall(club, "ControlUseHolding", 3, 0, killer, 20, -10);
	ScheduleCall(club, "ControlUseStop", 4, 0, killer, 20, -10);
	return true;
}

global func Test10_Log() { return "K throw firestone at object, object moves to V"; }
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

global func Test11_Log() { return "K explodes firestone material, one of the cascade firestones kills V"; }
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

global func Test12_Log() { return "K shoots a normal arrow at V"; }
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

global func Test13_Log() { return "K shoots a fire arrow at V"; }
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

global func Test14_Log() { return "K shoots a bomb arrow at V"; }
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

global func Test15_Log() { return "K shoots with the musket at V"; }
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

global func Test16_Log() { return "K throws a javelin at V"; }
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

global func Test17_Log() { return "K shoots a grenade (with launcher) at V"; }
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

global func Test18_Log() { return "K throws an activated iron bomb at V (no collection by V)"; }
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

global func Test19_Log() { return "K ignites powder keg (musket shot) near V"; }
global func Test19_OnStart()
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

global func Test20_Log() { return "K throws a fusing dynamite at V (no collection by V)"; }
global func Test20_OnStart()
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

global func Test21_Log() { return "F puts himself on fire, K is incinerated by F and then incinerates V"; }
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

global func Test22_Log() { return "K dumps object from a lorry onto V"; }
global func Test22_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	DrawMaterialQuad("Tunnel", 110, 160, 170, 160, 170, 260, 110, 260);

	victim->SetPosition(144, 250);
	victim->DoEnergy(-48);
	
	killer->SetPosition(100, 150);
	var lorry = CreateObjectAbove(Lorry, 110, 160);
	lorry->CreateContents(Rock, 20);
	lorry->SetR(55);
	killer->SetCommand("Grab", lorry);
	lorry->ControlUseStart(killer, 1, 0);
	ScheduleCall(lorry, "ControlUseHolding", 1, 40, killer, 1, 0);
	return true;
}

global func Test23_Log() { return "K destroys a lorry with objects next to V"; }
global func Test23_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	DrawMaterialQuad("Tunnel", 110, 160, 170, 160, 170, 260, 110, 260);

	victim->SetPosition(124, 250);
	victim->DoEnergy(-45);
	
	killer->SetPosition(100, 150);
	var lorry = CreateObjectAbove(Lorry, 150, 260);
	lorry->DoDamage(95);
	lorry->CreateContents(Rock, 20);
	var firestone = killer->CreateContents(Firestone);
	killer->SetHandAction(0);
	ScheduleCall(killer, "ControlThrow", 4, 0, firestone, 14, -50);
	return true;
}

global func Test24_Log() { return "K shoots an object with the catapult at V"; }
global func Test24_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(240, 150);
	victim->DoEnergy(-45);

	var cannon = killer->CreateObject(Catapult);
	killer->CreateContents(Shield);
	cannon->ControlUseStart(killer, 120, 0);
	cannon->ControlUseStop(killer, 120, 0);
	return true;
}

global func Test25_Log() { return "K lets lava move onto V (trigger: explosion)"; }
global func Test25_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	DrawMaterialQuad("Tunnel", 60, 160, 90, 160, 90, 260, 60, 260);
	DrawMaterialQuad("Earth", 90, 90, 130, 90, 130, 160, 90, 160);
	DrawMaterialQuad("DuroLava", 94, 90, 126, 90, 126, 160, 94, 160);

	victim->SetPosition(75, 250);
	victim->DoEnergy(-45);

	var firestone = killer->CreateContents(Firestone);
	killer->SetHandAction(0);
	ScheduleCall(killer, "ControlThrow", 4, 0, firestone, 14, -6);
	return true;
}

global func Test26_Log() { return "K lets acid move onto V (trigger: explosion)"; }
global func Test26_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	DrawMaterialQuad("Tunnel", 60, 160, 90, 160, 90, 260, 60, 260);
	DrawMaterialQuad("Earth", 90, 90, 130, 90, 130, 160, 90, 160);
	DrawMaterialQuad("Acid", 94, 90, 126, 90, 126, 160, 94, 160);

	victim->SetPosition(75, 250);
	victim->DoEnergy(-45);

	var firestone = killer->CreateContents(Firestone);
	killer->SetHandAction(0);
	ScheduleCall(killer, "ControlThrow", 4, 0, firestone, 14, -6);
	return true;
}

global func Test27_Log() { return "K uses windbag to shoot object at V"; }
global func Test27_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(240, 150);
	victim->DoEnergy(-45);

	var windbag = killer->CreateContents(WindBag);
	windbag->DoFullLoad();
	CreateObject(Rock, killer->GetX() + 10, killer->GetY() - 10); 
	ScheduleCall(windbag, "ControlUse", 4, 0, killer, 20, -8);
	return true;
}

global func Test28_Log() { return "K shoots at V hanging on a balloon (V tumbles out of map)"; }
global func Test28_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	DrawMaterialQuad("Sky:Sky", 100, 0, LandscapeWidth(), 0, LandscapeWidth(), LandscapeHeight(), 100, LandscapeHeight());
	ClearFreeRect(100, 0, LandscapeWidth() - 100, LandscapeHeight());
	
	victim->SetPosition(260, 120);
	var balloon = victim->CreateContents(Balloon);
	balloon->ControlUseStart(victim);

	var musket = killer->CreateContents(Musket);
	musket->CreateContents(LeadShot);
	musket.loaded = true;
	musket->ControlUseStart(killer, victim->GetX() - killer->GetX(), victim->GetY() - killer->GetY());
	musket->ControlUseStop(killer, victim->GetX() - killer->GetX(), victim->GetY() - killer->GetY());
	return true;
}

global func Test29_Log() { return "K shoots at balloon on which V hangs (V tumbles out of map)"; }
global func Test29_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	DrawMaterialQuad("Sky:Sky", 100, 0, LandscapeWidth(), 0, LandscapeWidth(), LandscapeHeight(), 100, LandscapeHeight());
	ClearFreeRect(100, 0, LandscapeWidth() - 100, LandscapeHeight());
	
	victim->SetPosition(260, 120);
	var balloon = victim->CreateContents(Balloon);
	balloon->ControlUseStart(victim);

	var musket = killer->CreateContents(Musket);
	musket->CreateContents(LeadShot);
	musket.loaded = true;
	musket->ControlUseStart(killer, victim->GetActionTarget()->GetX() - killer->GetX(), victim->GetActionTarget()->GetY() - killer->GetY());
	musket->ControlUseStop(killer, victim->GetActionTarget()->GetX() - killer->GetX(), victim->GetActionTarget()->GetY() - killer->GetY());
	return true;
}

global func Test30_Log() { return "K uses windbag to shoot V out of map"; }
global func Test30_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	DrawMaterialQuad("Sky:Sky", 100, 0, LandscapeWidth(), 0, LandscapeWidth(), LandscapeHeight(), 100, LandscapeHeight());
	ClearFreeRect(100, 0, LandscapeWidth() - 100, LandscapeHeight());
	
	victim->SetPosition(60, 150);
	ScheduleCall(victim, "ControlJump", 4, 1);

	var windbag = killer->CreateContents(WindBag);
	windbag->DoFullLoad();
	ScheduleCall(windbag, "ControlUse", 12, 0, killer, 20, -30);
	return true;
}

global func Test31_Log() { return "K launches a boompack at V"; }
global func Test31_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(90, 150);
	victim->DoEnergy(-45);

	var boompack = killer->CreateContents(Boompack);
	ScheduleCall(boompack, "ControlUse", 12, 0, killer, 20, -2);
	ScheduleCall(boompack, "ControlJump", 16, 0, killer);
	return true;
}

global func Test32_Log() { return "F places a dynamite box, K takes igniter and blasts V"; }
global func Test32_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	var fake_killer = GetCrew(plr_killer_fake);
	
	victim->SetPosition(140, 150);
	victim->DoEnergy(-45);

	fake_killer->SetPosition(140, 150);
	var dynamite_box = fake_killer->CreateContents(DynamiteBox);
	ScheduleCall(dynamite_box, "ControlUse", 1, 5, fake_killer, 0, 10);
	ScheduleCall(fake_killer, "ControlThrow", 5, 0, dynamite_box, -14, -6);
	ScheduleCall(killer, "SetCommand", 40, 0, "MoveTo", dynamite_box);
	ScheduleCall(killer, "Collect", 70, 0, dynamite_box);
	ScheduleCall(dynamite_box, "ControlUse", 72, 0, killer, 0, 10);
	return true;
}

global func Test33_Log() { return "K uses teleglove to drop object onto V"; }
global func Test33_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	var fake_killer = GetCrew(plr_killer_fake);
	
	victim->SetPosition(120, 150);
	victim->DoEnergy(-45);

	var teleglove = killer->CreateContents(TeleGlove);
	var rock = CreateObject(Rock, 120, 100);
	ScheduleCall(teleglove, "ControlUseStart", 1, 0, killer, rock->GetX() - killer->GetX(), rock->GetY() - killer->GetY());
	ScheduleCall(teleglove, "ControlUseHolding", 1, 20, killer, rock->GetX() - killer->GetX(), rock->GetY() - killer->GetY());
	ScheduleCall(teleglove, "ControlUseStop", 21, 0, killer, rock->GetX() - killer->GetX(), rock->GetY() - killer->GetY());
	
	fake_killer->SetPosition(180, 150);
	var teleglove_fake = fake_killer->CreateContents(TeleGlove);
	ScheduleCall(teleglove_fake, "ControlUseStart", 1, 0, fake_killer, rock->GetX() - fake_killer->GetX(), rock->GetY() - fake_killer->GetY());
	ScheduleCall(teleglove_fake, "ControlUseHolding", 1, 15, fake_killer, rock->GetX() - fake_killer->GetX(), rock->GetY() - fake_killer->GetY());
	ScheduleCall(teleglove_fake, "ControlUseStop", 16, 0, fake_killer, rock->GetX() - fake_killer->GetX(), rock->GetY() - fake_killer->GetY());
	return true;
}

global func Test34_Log() { return "K blasts object from material which drops on V"; }
global func Test34_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	DrawMaterialQuad("Rock", 80, 0, 160, 0, 160, 120, 80, 120);
	DrawMaterialQuad("Tunnel", 100, 160, 120, 160, 140, 160, 120, 200);
	
	victim->SetPosition(120, 185);
	victim->DoEnergy(-49);	
	
	var cannon = killer->CreateObject(Cannon);
	cannon->CreateContents(PowderKeg);
	killer->CreateContents(Firestone);
	cannon->ControlUseStart(killer, 20, -10);
	cannon->ControlUseStop(killer, 20, -10);
	return true;
}

global func Test35_Log() { return "K throws an activated iron bomb at V (then collected by V)"; }
global func Test35_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(170, 150);
	victim->DoEnergy(-45);
		
	var bomb = killer->CreateContents(IronBomb);
	bomb->ControlUse(killer);
	killer->SetHandAction(0);
	killer->ControlThrow(bomb, 20, -20);
	ScheduleCall(victim, "BeginPickingUp", 70, 0);
	ScheduleCall(victim, "EndPickingUp", 71, 0);
	return true;
}

global func Test36_Log() { return "K throws a fusing dynamite at V (V tries to collect but can't)"; }
global func Test36_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(140, 150);
	victim->DoEnergy(-45);
		
	var dynamite = killer->CreateContents(Dynamite);
	dynamite->ControlUse(killer);
	killer->SetHandAction(0);
	killer->ControlThrow(dynamite, 20, -20);
	ScheduleCall(victim, "BeginPickingUp", 70, 0);
	ScheduleCall(victim, "EndPickingUp", 71, 0);
	return true;
}

global func Test37_Log() { return "K ignites boompack (musket shot) which shoots at V"; }
global func Test37_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	victim->SetPosition(280, 150);
	victim->DoEnergy(-45);
		
	CreateObject(Boompack, 240, 146)->SetR(90);
	var musket = killer->CreateContents(Musket);
	musket->CreateContents(LeadShot);
	musket.loaded = true;
	musket->ControlUseStart(killer, 20, -7);
	musket->ControlUseStop(killer, 20, -7);
	return true;
}

global func Test38_Log() { return "K ignites a battery of boompacks which shoot down V flying an airplane"; }
global func Test38_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	var airplane = CreateObject(Airplane, 80, 50);
	victim->Enter(airplane);
	victim->SetAction("Walk");
	airplane->FaceRight();
	airplane->StartInstantFlight(90, 15);
	airplane->SetXDir(12);
	airplane->SetYDir(-5);
	victim->DoEnergy(-45);
	
	killer->SetPosition(180, 150);	
	for (var i = 0; i < 15; i++)	
		CreateObjectAbove(Boompack, 240, 160);
	var lantern = killer->CreateContents(Lantern);
	killer->ObjectCommand("Throw", lantern, 20, -10);
	return true;
}

global func Test39_Log() { return "K shoots at airship which carries V (V tumbles out of map)"; }
global func Test39_OnStart()
{
	var victim = GetCrew(plr_victim);
	var killer = GetCrew(plr_killer);
	
	ClearFreeRect(100, 0, LandscapeWidth() - 100, LandscapeHeight());
	
	var airship = CreateObjectAbove(Airship, 330, 165);
	airship->DoDamage(20);
	victim->SetPosition(330, 150);

	var musket = killer->CreateContents(Musket);
	musket->CreateContents(LeadShot);
	musket.loaded = true;
	musket->ControlUseStart(killer, airship->GetX() - killer->GetX(), airship->GetY() - killer->GetY() - 12);
	musket->ControlUseStop(killer, airship->GetX() - killer->GetX(), airship->GetY() - killer->GetY() - 12);
	return true;
}


/*-- Wiki Overview Table --*/

global func CreateWikiOverviewTable(array results)
{
	var table = "{| border=\"1\" class=\"wikitable\" style=\"margin: 1em auto 1em auto;\"\n|-\n! #\n! Kill Description\n! Traced?\n";
	for (var index = 1; index <= GetLength(results); index++)
	{
		var result = results[index - 1];
		if (result == nil)
			continue;
		var success = "<span style=\"color:#FF0000\">no</span>";
		if (result)
			success = "<span style=\"color:#00CC00\">yes</span>";
		var entry = Format("|-\n| %d\n| %s\n| %s\n", index, Call(Format("~Test%d_Log", index)), success);
		table = Format("%s%s", table, entry);
	}
	var table_closing = "|-\n|}";
	table = Format("%s%s", table, table_closing);
	Log(table);
}