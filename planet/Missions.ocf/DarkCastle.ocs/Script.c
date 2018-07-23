/**
	Evil Castle
	Desc
	
	@authors Sven2
*/

static g_is_initialized;

static npc_pyrit;

private func DoInit(int first_player)
{
	var relaunch_rule = GetRelaunchRule();
	relaunch_rule->SetInventoryTransfer(true);
	relaunch_rule->SetFreeCrew(true);
	relaunch_rule->SetRespawnDelay(1);
	relaunch_rule->SetBaseRespawn(true);
	relaunch_rule->SetDefaultRelaunchCount(nil);
	relaunch_rule->SetAllowPlayerRestart(true);
	relaunch_rule->SetLastClonkRespawn(true);	
	relaunch_rule->SetInitialRelaunch(false);
	// Message when first player enters shroom area
	ScheduleCall(nil, Scenario.ShroomCaveCheck, 21, 0xffffff);
	// Scorching village
	g_ruin1->AddScorch(-20,-10, -45, 50, 1500);
	g_ruin2->AddScorch(-15,42, 90, 50, 1200);
	g_ruin3->AddScorch(-12,18, 130, 80, 1300);
	// Horax
	g_king.JumpSpeed = 200;
	// Update AI stuff
	for (var enemy in FindObjects(Find_ID(Clonk), Find_Owner(NO_OWNER)))
	{
		var fx = AI->GetAI(enemy);
		if (fx)
		{
			fx.weapon = fx.target = nil;
			AI->BindInventory(enemy);
			enemy->DoEnergy(10000);
			enemy->AddEnergyBar();
			SetSpecialDeathMessage(enemy);
			
			SetSpecialDeathMessage(enemy);
		}
	}
	g_farmer.portrait = { Source=DialogueCastle };
	// Start intro if not yet started
	StartSequence("Intro", 0, GetCrew(first_player));
	return true;
}

private func SetSpecialDeathMessage(object clonk)
{
	var name = clonk->GetName();
	if (name == "Horst") clonk.SpecialDeathMessage = "$DeathOfHorst$";
	if (name == "Hanniball") clonk.SpecialDeathMessage = "$DeathOfHanniball$";
	if (name == "Twonky") clonk.SpecialDeathMessage = "$DeathOfTwonky$";
	if (name == "Sven") clonk.SpecialDeathMessage = "$DeathOfSven$";
	if (name == "Luki") clonk.SpecialDeathMessage = "$DeathOfLuki$";
	if (name == "Anna") clonk.SpecialDeathMessage = "$DeathOfAnna$";
	if (name == "Cindy") clonk.SpecialDeathMessage = "$DeathOfCindy$";
	if (name == "Sabrina") clonk.SpecialDeathMessage = "$DeathOfSabrina$";
	if (name == "Laura") clonk.SpecialDeathMessage = "$DeathOfLaura$";
	if (name == "Wolfgang") clonk.SpecialDeathMessage = "$DeathOfWolfgang$";
	if (name == "Hans") clonk.SpecialDeathMessage = "$DeathOfHans$";
	if (name == "Joki") clonk.SpecialDeathMessage = "$DeathOfJoki$";
	if (name == "Archibald") clonk.SpecialDeathMessage = "$DeathOfArchibald$";
}

private func InitializePlayer(int plr)
{
	// Players only
	if (GetPlayerType(plr)!=C4PT_User) return;
	// Scenario init
	if (!g_is_initialized) g_is_initialized = DoInit(plr);
	// Harsh zoom range
	for (var flag in [PLRZOOM_LimitMax, PLRZOOM_Direct])
		SetPlayerZoomByViewRange(plr,400,250,flag);
	SetPlayerViewLock(plr, true);
	// Initial join
	var crew = GetCrew(plr);
	crew->SetPosition(35 + Random(10) , 1140);
	crew->SetDir(DIR_Right);
	crew->CreateContents(Shovel);
	crew->CreateContents(Hammer);
	crew->CreateContents(Axe);
	return true;
}


/* Enemy encounter messages */

public func EncounterCave(object enemy, object player)
{
	Dialogue->MessageBoxAll("$MsgEncounterCave$", enemy, true);
	return true;
}

public func EncounterOutpost(object enemy, object player)
{
	Dialogue->MessageBoxAll("$MsgEncounterOutpost$", enemy, true);
	return true;
}

public func EncounterKing(object enemy, object player)
{
	if (!player) player = enemy; // Leads to a funny message, but better than a null pointer.
	Dialogue->MessageBoxAll(Format("$MsgEncounterKing$", player->GetName()), enemy, true);
	return true;
}


/* Mushroom cave encounter */

public func ShroomCaveCheck()
{
	var intruder = FindObject(Find_InRect(1252,1342,320,138), Find_OCF(OCF_CrewMember));
	if (!intruder) return true;
	Dialogue->MessageBoxAll("$MsgEncounterShrooms$", intruder, true);
	ClearScheduleCall(nil, Scenario.ShroomCaveCheck);
	return true;
}

public func OnGoalsFulfilled()
{
	GainScenarioAchievement("Done");
	GainScenarioAccess("S2Castle");
	return false;
}
