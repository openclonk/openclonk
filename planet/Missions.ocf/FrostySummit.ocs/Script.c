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
			CreateObjectAbove(Rock, loc.x, loc.y+3);
	SetSkyParallax(1, 20,20, 0,0, nil, nil);
	GetRelaunchRule()->Set({
		inventory_transfer = true,
		free_crew = true,
		relaunch_time = 36,
		respawn_at_base = false,
		default_relaunch_count = nil,
		player_restart = true,
		respawn_last_clonk = true
	});
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
	return true;
}

private func InitBase(int owner)
{
	// Create standard base owned by player
	var y=90*8, x=40*8;
	var flag = CreateObjectAbove(Flagpole, x+85,y, owner);
	var hut = CreateObjectAbove(ToolsWorkshop, x+45,y, owner);
	if (hut)
	{
		hut->CreateContents(Shovel, 1);
		hut->CreateContents(Loam, 1);
	}
	for (var i=0; i<3; ++i) CreateObjectAbove(Boompack, x+20+i*5+Random(4),y, owner);
	return true;
}

func RelaunchPlayer(int plr)
{
	var clonk = CreateObjectAbove(Clonk, 50, 1000, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	JoinPlayer(plr);
	return true;
}

public func OnPlayerRelaunch(int plr, bool is_relaunch)
{
	if(!is_relaunch) return OnClonkLeftRelaunch(GetCrew(plr), plr);
}

public func OnClonkLeftRelaunch(object clonk, int plr)
{
	clonk->CreateContents(GrappleBow, 2);
	clonk->CreateContents(WindBag);
	clonk->CreateContents(TeleGlove);
	clonk->CreateContents(Dynamite, 2);
}

public func RelaunchPosition()
{
	return [40*8 + Random(40), 90*8-10];
}

func OnGoalsFulfilled()
{
	GainScenarioAchievement("Done");
	return false;
}
