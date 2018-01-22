/**
	Raid
	Intro mission
	
	@authors Sven2
*/

// Set in Objects.c
//static g_chemical, g_cabin, g_sawmill, g_workshop, g_flagpole, g_windmill, npc_newton, npc_lara, npc_lisa, npc_woody, npc_rocky, npc_mave, npc_pyrit, npc_clonko, npc_matthi, npc_dora;

// Created after intro
static g_goal;

// Story progress
static g_is_initialized,      // intro started
        g_attack_started,      // enemy planes arriving
        g_attack_done,         // Clunker village got destroyed
        g_challenge_accepted,  // accepted to kill the king
        g_pyrit_spoken,        // spoke with Pyrit
        g_plane_built,         // built the airplane
        g_mave_oil_spoken,     // got the key offer from Mave
        g_got_maves_key,       // got the key from Mave
        g_dora_spoken,         // got rumour about oil lake from Clonko
        g_clonko_spoken,       // got rumour about oil lake from Dora
        g_oil_delivered;       // oil delivered, all done!

func Initialize()
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
	npc_newton->SetAlternativeSkin("MaleBlackHair");
	npc_pyrit->SetAlternativeSkin("MaleBrownHair");
	npc_woody->SetAlternativeSkin("Youngster");
	g_guidepost1->SetInscription("$Post1$");
	g_guidepost2->SetInscription("$Post2$");
	MakeRuinsOnDamage(); // see System.ocg/Ruins.c
	PlaceGrass(40);
	return true;
}

func DoInit(int first_player)
{
	StartSequence("Intro", 0, GetCrew(first_player));
	//g_goal = CreateObject(Goal_Raid);
	// Prepare trigger for attack sequence
	for (var tree in FindObjects(Find_Func("IsTree")))
	{
		tree.ChopDown_A564F3 = tree.ChopDown;
		tree.ChopDown = Scenario.Tree_Chopdown;
		//tree->ChopDown();
	}
	//g_attack_done = true; GetCrew()->SetPosition(npc_pyrit->GetX(), npc_pyrit->GetY()); GetCrew()->CreateObjectAbove(Airplane); GetCrew()->CreateObjectAbove(MetalBarrel);
	//GetCrew()->CreateContents(Shovel);
	return true;
}

// called in tree context
func Tree_Chopdown(...)
{
	// On tree chopped down: Start attack sequence!
	if (!g_attack_started)
	{
		var chopping_clonk = FindObject(Find_ID(Clonk), Sort_Distance());
		StartSequence("Attack", 0, chopping_clonk);
	}
	// tree falls anyway
	return Call(this.ChopDown_A564F3, ...);
}

func InitializePlayer(int plr)
{
	var crew;
	// Ensure flag has owner
	if (g_flagpole && g_flagpole->GetOwner()<0) g_flagpole->SetOwner(plr);
	// Late join stuff
	if (g_pyrit_spoken) SetPlrKnowledge(plr, Airplane);
	// Join intro listening or regular scenario
	SetPlayerViewLock(plr, true);
	JoinPlayer(plr);
	// Scenario init
	if (!g_is_initialized) g_is_initialized = DoInit(plr);
	return;
}

func JoinPlayer(int plr, object crew, bool no_placement)
{
	if (!crew) crew = GetCrew(plr);
	if (!crew) return false;
	if (!no_placement) crew->SetPosition(471, 338);
	var tools;
	if (g_attack_done) tools = [Shovel, Axe]; else tools = [];
	for (var tool in tools)
		if (!crew->ContentsCount(tool)) crew->CreateContents(tool);
	SetPlayerZoomByViewRange(NO_OWNER, 400,300, PLRZOOM_Set | PLRZOOM_LimitMax);
	SetCursor(crew->GetOwner(), crew);
	return true;
}



/* Attack sequence */

func StartAttackSequence(object chopping_clonk)
{
	if (!chopping_clonk) chopping_clonk = GetCursor(GetPlayerByIndex());
	return StartSequence("Attack", 0, chopping_clonk);
}


/* Finished */

func OnPlaneLoaded(object plane, object oil)
{
	if (!plane || !oil || g_oil_delivered) return false; // disappeared in that one frame?
	g_oil_delivered = true;
	oil->Enter(plane);
	g_goal->SetStageDone();
	g_goal->SetFulfilled();
	return StartSequence("Outro", 0, plane);
}

func OnGoalsFulfilled()
{
	SetNextMission("Missions.ocf/Crash.ocs");
	GainMissionAccess("S2Raid");
	GainScenarioAchievement("Done");
	return true; // GameOver done by outro
}
