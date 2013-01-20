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
	if (goal) goal->SetVictim(Object(3816));
	// Elevators
	// Top
	Object(332)->SetNoPowerNeed(true);
	Object(331)->CreateShaft(470);
	// Left
	Object(420)->CreateShaft(100);
	// Shrooms
	Object(2318)->AddPoisonEffect(0,0);
	Object(2375)->AddPoisonEffect(-20,0);
	// Scorching village
	Object(343)->AddScorch(-20,-10, -45, 50, 1500);
	Object(344)->AddScorch(-15,42, 90, 50, 1200);
	Object(346)->AddScorch(-12,18, 130, 80, 1300);
	// Rules
	CreateObject(Rule_TeamAccount);
	CreateObject(Rule_NoPowerNeed);
	// Intro. Message 250 frames + regular message time
	Dialogue->MessageBoxAll("$MsgIntro1$", Object(2648), true);
	Schedule(nil, "Dialogue->MessageBoxAll(\"$MsgIntro1$\", Object(2648))", 250, 1);
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
