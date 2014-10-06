/* Frosty summit */

func Initialize()
{
	// Goal
	var goal = FindObject(Find_ID(Goal_SellGems));
	if (!goal) goal = CreateObject(Goal_SellGems);
	goal->SetTargetAmount(1);
	// Rules
	if (!ObjectCount(Find_ID(Rule_TeamAccount))) CreateObject(Rule_TeamAccount);
	// Environment
	var loc;
	for (var i=0; i<5; ++i)
		if (loc = FindLocation(Loc_InRect(0,80*8,40*8,20*8), Loc_Material("Earth")))
			CreateObject(Rock, loc.x, loc.y+3);
	SetSkyParallax(1, 20,20, 0,0, nil, nil);
}

static g_was_player_init;

func InitializePlayer(int plr)
{
	// First player init base
	if (!g_was_player_init)
	{
		InitBase(plr);
		g_was_player_init = true;
	}
	// Position and materials
	JoinPlayer(plr);
	return true;
}

private func InitBase(int owner)
{
	// Create standard base owned by player
	var y=90*8, x=40*8;
	var flag = CreateObject(Flagpole, x+85,y, owner);
	var hut = CreateObject(ToolsWorkshop, x+45,y, owner);
	if (hut)
	{
		hut->CreateContents(Shovel, 1);
		hut->CreateContents(Loam, 1);
	}
	for (var i=0; i<3; ++i) CreateObject(Boompack, x+20+i*5+Random(4),y, owner);
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
	var i, crew;
	for (i=0; crew=GetCrew(plr,i); ++i)
	{
		crew->SetPosition(40*8+Random(40), 90*8-10);
		if (!i)
		{
			crew->CreateContents(GrappleBow, 2);
			crew->CreateContents(WindBag);
			crew->CreateContents(TeleGlove);
			crew->CreateContents(Dynamite, 2);
		}
	}
	return true;
}

func OnGoalsFulfilled()
{
	GainScenarioAchievement("Done");
	return false;
}
