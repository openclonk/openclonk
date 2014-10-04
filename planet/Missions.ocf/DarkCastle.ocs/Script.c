/**
	Evil Castle
	Desc
	
	@authors Sven2
*/

static const S2DD_InitialRelaunchs = 0;

static g_is_initialized;

static g_ruin1, g_ruin2, g_ruin3, g_elev1, g_elev2, g_farmer, g_king;
static npc_pyrit, g_cannon, g_cannoneer;

func DoInit(int first_player)
{
	// Message when first player enters shroom area
	ScheduleCall(nil, Scenario.ShroomCaveCheck, 21, 0xffffff);
	// Scorching village
	g_ruin1->AddScorch(-20,-10, -45, 50, 1500);
	g_ruin2->AddScorch(-15,42, 90, 50, 1200);
	g_ruin3->AddScorch(-12,18, 130, 80, 1300);
	// Horax
	g_king.JumpSpeed = 200;
	// Update AI stuff
	var fx;
	for (var enemy in FindObjects(Find_ID(Clonk), Find_Owner(NO_OWNER)))
		if (fx = S2AI->GetAI(enemy))
		{
			fx.weapon = fx.target = nil;
			S2AI->BindInventory(enemy);
			enemy->DoEnergy(10000);
			enemy->AddEnergyBar();
		}
	g_farmer.portrait = { Source=DialogueCastle };
	// Start intro if not yet started
	StartSequence("Intro", 0, GetCrew(first_player));
	return true;
}

func InitializePlayer(int plr)
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

func EncounterCave(object enemy, object player)
{
	Dialogue->MessageBoxAll("$MsgEncounterCave$", enemy, true);
	return true;
}

func EncounterOutpost(object enemy, object player)
{
	Dialogue->MessageBoxAll("$MsgEncounterOutpost$", enemy, true);
	return true;
}

func EncounterKing(object enemy, object player)
{
	if (!player) player = enemy; // Leads to a funny message, but better than a null pointer.
	Dialogue->MessageBoxAll(Format("$MsgEncounterKing$", player->GetName()), enemy, true);
	return true;
}


/* Mushroom cave encounter */

func ShroomCaveCheck()
{
	var intruder = FindObject(Find_InRect(1252,1342,320,138), Find_OCF(OCF_CrewMember));
	if (!intruder) return true;
	Dialogue->MessageBoxAll("$MsgEncounterShrooms$", intruder, true);
	ClearScheduleCall(nil, Scenario.ShroomCaveCheck);
	return true;
}

func OnGoalsFulfilled()
{
	GainScenarioAchievement("Done");
	GainMissionAccess("S2Castle");
	return false;
}
