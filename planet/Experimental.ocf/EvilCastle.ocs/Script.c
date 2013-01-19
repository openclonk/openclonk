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
	Object(331)->CreateShaft(500);
	// Left
	Object(420)->CreateShaft(100);
	// Shrooms
	Object(2318)->AddPoisonEffect(0,0);
	Object(2375)->AddPoisonEffect(-20,0);
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
