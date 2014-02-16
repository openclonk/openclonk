/**
	Evil Castle
	Desc
	
	@authors Sven2
*/

static const S2DD_InitialRelaunchs = 0;

static g_is_initialized;

func DoInit(int first_player)
{
	// Goal
	var goal = CreateObject(Goal_Assassination);
	if (goal) goal->SetVictim(g_king);
	// Elevators
	// Top
	g_elev2->SetNoPowerNeed(true);
	g_elev2->CreateShaft(470);
	// Left
	g_elev1->SetNoPowerNeed(true);
	g_elev1->CreateShaft(100);
	// Shrooms
	g_shroom1->AddPoisonEffect(0,0); // floor left
	g_shroom2->AddPoisonEffect(0,0); // ceiling left
	g_shroom3->AddPoisonEffect(-20,0); // floor right
	g_shroom4->AddPoisonEffect(10,-10); // ceiling right
	// Message when first player enters shroom area
	ScheduleCall(nil, Scenario.ShroomCaveCheck, 21, 0xffffff);
	// Scorching village
	g_ruin1->AddScorch(-20,-10, -45, 50, 1500);
	g_ruin2->AddScorch(-15,42, 90, 50, 1200);
	g_ruin3->AddScorch(-12,18, 130, 80, 1300);
	// Rules
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_NoPowerNeed);
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
	// Intro. Message 250 frames + regular message time
	g_farmer.portrait = "Farmer2";
	DialogueCastle->MessageBoxAll("$MsgIntro1$", g_farmer, true);
	Schedule(nil, "DialogueCastle->MessageBoxAll(\"$MsgIntro1$\", g_farmer)", 250, 1);
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
	JoinPlayer(plr);
	return true;
}

func RelaunchPlayer(int plr)
{
	var clonk = CreateObject(Clonk, 50, 1000, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	JoinPlayer(plr);
	return true;
}

func JoinPlayer(int plr)
{
	// Place in village
	var crew;
	for(var index = 0; crew = GetCrew(plr, index); ++index)
	{
		var x = 35 + Random(10);
		var y = 1140;
		crew->SetPosition(x , y);
		crew->SetDir(DIR_Right);
		crew->DoEnergy(1000);
		// First crew member gets shovel and hammer. Try to get those items from the corpse if they still exist
		// so we don't end up with dozens of useless shovels
		if (!index)
		{
			for (var equip_id in [Shovel, Hammer])
			{
				var obj = FindObject(Find_ID(equip_id), Find_Owner(plr));
				if (obj) obj->Enter(crew); else crew->CreateContents(equip_id);
			}
		}
	}
	return true;
}


/* Enemy encounter messages */

func EncounterCave(object enemy, object player)
{
	DialogueCastle->MessageBoxAll("$MsgEncounterCave$", enemy);
	return true;
}

func EncounterOutpost(object enemy, object player)
{
	DialogueCastle->MessageBoxAll("$MsgEncounterOutpost$", enemy);
	return true;
}

func EncounterKing(object enemy, object player)
{
	if (!player) player = enemy; // Leads to a funny message, but better than a null pointer.
	DialogueCastle->MessageBoxAll(Format("$MsgEncounterKing$", player->GetName()), enemy);
	return true;
}


/* Mushroom cave encounter */

func ShroomCaveCheck()
{
	var intruder = FindObject(Find_InRect(1252,1342,320,138), Find_OCF(OCF_CrewMember));
	if (!intruder) return true;
	DialogueCastle->MessageBoxAll("$MsgEncounterShrooms$", intruder);
	ClearScheduleCall(nil, Scenario.ShroomCaveCheck);
	return true;
}