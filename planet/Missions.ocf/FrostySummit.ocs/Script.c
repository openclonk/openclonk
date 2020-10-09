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
	for (var i = 0; i<5; ++i)
	{
		var loc = FindLocation(Loc_InRect(0, 80*8, 40*8, 20*8), Loc_Material("Earth"));
		if (loc)
			CreateObjectAbove(Rock, loc.x, loc.y + 3);
	}
	SetSkyParallax(1, 20, 20, 0, 0, nil, nil);
	var relaunch_rule = GetRelaunchRule();
	relaunch_rule->SetInventoryTransfer(true);
	relaunch_rule->SetFreeCrew(true);
	relaunch_rule->SetRespawnDelay(1);
	relaunch_rule->SetBaseRespawn(false);
	relaunch_rule->SetDefaultRelaunchCount(nil);
	relaunch_rule->SetAllowPlayerRestart(true);
	relaunch_rule->SetLastClonkRespawn(true);
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
	// Position.
	var clonk = GetCrew(plr);
	var pos = RelaunchPosition();
	clonk->SetPosition(pos[0], pos[1]);
	return true;
}

private func InitBase(int owner)
{
	// Create standard base owned by player
	var y = 90 * 8, x = 40 * 8;
	CreateObjectAbove(Flagpole, x + 85, y, owner);
	var hut = CreateObjectAbove(ToolsWorkshop, x + 45, y, owner);
	if (hut)
	{
		hut->CreateContents(Shovel, 1);
		hut->CreateContents(Loam, 1);
	}
	for (var i = 0; i < 3; ++i)
		CreateObjectAbove(Boompack, x + 20 + i*5 + Random(4),y, owner);
	return true;
}

public func RelaunchPlayer(int plr)
{
	var clonk = CreateObjectAbove(Clonk, 50, 1000, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	return true;
}

public func OnPlayerRelaunch(int plr, bool is_relaunch)
{
	if (!is_relaunch)
	{
		var clonk = GetCrew(plr);
		clonk->CreateContents(GrappleBow, 2);
		clonk->CreateContents(WindBag);
		clonk->CreateContents(TeleGlove);
		clonk->CreateContents(Dynamite, 2);
	}
}

public func RelaunchPosition()
{
	return [40 * 8 + Random(40), 90 * 8 - 10];
}

func OnGoalsFulfilled()
{
	GainScenarioAchievement("Done");
	return false;
}
