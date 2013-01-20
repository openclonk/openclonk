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
	//CreateObject(Goal_Plane);
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
	return true;
}

func InitializePlayer(int plr)
{
	// Players only
	if (GetPlayerType(plr)!=C4PT_User) return;
	// Scenario init
	if (!g_is_initialized) g_is_initialized = DoInit(plr);
	// Start intro if not yet started
	//IntroStart();
	// Place in village
	var crew;
	for(var index = 0; crew = GetCrew(plr, index); ++index)
	{
		var x = 50 + Random(50);
		var y = 850;
		crew->SetPosition(x , y);
	}
}
