/* Boomshire */

private func Initialize()
{
	// Create dynamite below the first lava basin
	DrawMaterialQuad("Tunnel",1378, 1327-5, 1860, 1327-5, 1860, 1330, 1387, 1330, 1);

	//Sound("Environment::BirdsLoop",true, 100, nil,+1);
	Cloud->Place(40);
	PlaceObjects(Rock, 50,"Earth");
	PlaceObjects(Loam, 25,"Earth");
	PlaceObjects(Nugget, 25,"Earth");

	AddEffect("PlaneReset",CreateObjectAbove(Airplane, 3030, 315, 0),100, 10, nil, nil);
	AddEffect("PlaneReset",CreateObjectAbove(Airplane, 3160, 315, 1),100, 10, nil, nil);

	Doors();

	var concierge = CreateObjectAbove(Clonk, 70, 1030, NO_OWNER);
	concierge->SetDir(DIR_Left);
	concierge->SetAlternativeSkin("Mime");
	concierge->SetObjectLayer(concierge);
	concierge->SetName("$NameConcierge$");
	concierge->SetDialogue("Concierge");
	concierge->Sound("Circus", false, nil, nil, +1, 100);
	Dialogue->FindByTarget(concierge)->AddAttention();

	var cannons = FindObjects(Find_ID(Cannon));
	for (var cannon in cannons)
	{
		cannon->TurnCannon(0);
		cannon->SetCannonAngle(45000);
		cannon.Touchable = false;
	}

	// Various tunnels for dynamite to fall through
	DrawMaterialQuad("Tunnel-brickback",339, 687, 350, 687, 350, 761, 339, 761);
	DrawMaterialQuad("Tunnel-brickback",1889, 866, 1955, 866, 1955, 882, 1889, 882);
	DrawMaterialQuad("Tunnel-brickback",1920, 874, 1955, 874, 1930, 1000, 1920, 1000);
	DrawMaterialQuad("Tunnel-brickback",1920, 1000, 1930, 1000, 1970, 1100, 1960, 1100);
	DrawMaterialQuad("Tunnel-brickback",1960, 1100, 1970, 1100, 1930, 1201, 1919, 1201);

	DrawMaterialQuad("Tunnel-brickback",2549, 907, 2590, 907, 2590, 920, 2549, 920);
	DrawMaterialQuad("Tunnel-brickback",2549, 920, 2560, 920, 2560, 1000, 2550, 1000);
	DrawMaterialQuad("Tunnel-brickback",2550, 1000, 2560, 1000, 2720, 1160, 2710, 1160);
	DrawMaterialQuad("Tunnel-brickback",2710, 1160, 2720, 1160, 2710, 1190, 2700, 1190);
	DrawMaterialQuad("Tunnel-brickback",2680, 1185, 2750, 1185, 2750, 1195, 2680, 1195);
	DrawMaterialQuad("Tunnel-brickback",2737, 1195, 2750, 1190, 2775, 1255, 2765, 1255);

	AddEffect("DynamiteEruption",nil, 100, 130);
}

private func InitializePlayer(int plr)
{
	SetPlayerTeam(plr, 1);
}

// Gamecall from Race-goal, on respawning.
public func OnPlayerRespawn(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	clonk->CreateContents(WindBag);
}

global func FxDynamiteEruptionTimer(object nobject, effect, int timer)
{
	if (Random(5))
	{
		var dyn = CreateObjectAbove(Firestone, 2460 + Random(20),670);
		dyn->SetYDir(-80);
		dyn->SetXDir(RandomX(-1, 1));
		dyn->SetRDir(RandomX(-30, 30));
	}
	else
	{
		var dyn = CreateObjectAbove(Dynamite, 2460 + Random(20),670);
		dyn->SetYDir(-80);
		dyn->SetXDir(RandomX(-1, 1));
		dyn->SetRDir(RandomX(-30, 30));
		dyn->Fuse();
	}
}

global func FxAutoOpenTimer(object pTarget, effect, int timer)
{
	if (!effect.fired)
	{
		if (FindObject(Find_ID(Airplane),Find_InRect(2710, 310, 130, 40)))
		{
			effect.fired = true;
			var cannons = FindObjects(Find_ID(Cannon));
			for (var cannon in cannons)
			{
				cannon->CreateContents(Boompack);
				cannon->DoFire(cannon->Contents(), nil, -45000);
			}
		}
	}
	if (FindObject(Find_ID(Airplane),Find_InRect(0, 0, 2000, 500)))
	{
		pTarget->OpenDoor();
		return FX_Execute_Kill;
	}
	else return FX_OK;
}

global func FxSparklingAttentionStart(object target, effect fx, temp)
{
	if (temp) return;
	fx.particles = 
	{
		Size = PV_Linear(2, 0),
		Alpha = PV_KeyFrames(0, 0, 0, 300, 255, 1000, 0),
		Rotation = PV_Random(0, 360),
		R = 255, G = 200, B = 50,
		Stretch = PV_Random(10000, 15000),
		BlitMode = GFX_BLIT_Additive
	};
}

global func FxSparklingAttentionTimer(object target, effect fx, int timer)
{
	// Sparkle quickly every now-and-then.
	var sparkle_cycle = timer % 100;
	if (sparkle_cycle < 30)
	{
		fx.Interval = 1;
		if (Random(2)) return FX_OK;
	}
	else
	{
		fx.Interval = 100;
		return FX_OK;
	}
	var x = RandomX(-10, 10);
	var y = RandomX(-10, 10);
	target->CreateParticle("StarSpark", x, y, 0, 0, PV_Random(1, 7), fx.particles, 10);
	return FX_OK;
}

global func FxPlaneResetTimer(object target, effect, int time)
{
	if (target->GetX() > 3000 || target->Contents())
	{
		effect.count = 0;
		return FX_OK;
	}
	else
		effect.count++;
	
	if (effect.count<4) return FX_OK;
	
	var particles =
	{
		Prototype = Particles_Air(),
		Size = PV_Linear(4, 0)
	};
	DrawParticleLine("Air", target->GetX()+3, target->GetY(), 3030, 315, 1, PV_Random(-2, 2), PV_Random(-2, 2), PV_Random(10, 30), particles);
	
	target->SetPosition(3030, 315);
	target->SetDir(0);
	target->SetR(-90);
	target->~RollPlane();
}

private func Doors()
{
	var door = CreateObjectAbove(StoneDoor, 135, 60, NO_OWNER);
	door->CloseDoor();
	AddEffect("AutoOpen",door, 1, 15);

	var gate = CreateObjectAbove(StoneDoor, 865, 1195, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 1045, 1165, NO_OWNER);
	wheel->SetSwitchTarget(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel, 100, 100);

	var gate = CreateObjectAbove(StoneDoor, 1155, 1026, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 1906, 778, NO_OWNER);
	wheel->SetArrowWheel();
	var straw = CreateObjectAbove(DoorTarget, 1850, 820, NO_OWNER);
	straw->SetGate(gate);
	straw->SetAction("Float");
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel, 100, 100);

	var gate = CreateObjectAbove(StoneDoor, 1875, 761, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 1752, 1148, NO_OWNER);
	wheel->SetSwitchTarget(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel, 100, 100);

	var gate = CreateObjectAbove(StoneDoor, 1875, 864, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 1116, 1038, NO_OWNER);
	wheel->SetSwitchTarget(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel, 100, 100);

	var gate = CreateObjectAbove(StoneDoor, 3115, 685, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 3140, 588, NO_OWNER);
	wheel->SetSwitchTarget(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel, 100, 100);

	var gate = CreateObjectAbove(StoneDoor, 585, 915, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 853, 681, NO_OWNER);
	wheel->SetSwitchTarget(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel, 100, 100);

	var gate = CreateObjectAbove(StoneDoor, 345, 740, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 60, 644, NO_OWNER);
	wheel->SetSwitchTarget(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel, 100, 100);

	var gate = CreateObjectAbove(StoneDoor, 1935, 558, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 1900, 565, NO_OWNER);
	wheel->SetSwitchTarget(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel, 100, 100);

	var gate = CreateObjectAbove(StoneDoor, 2965, 316, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 3260, 328, NO_OWNER);
	wheel->SetSwitchTarget(gate);
	gate->CloseDoor();

	var gate = CreateObjectAbove(StoneDoor, 3285, 1150, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 3220, 1200, NO_OWNER);
	wheel->SetSwitchTarget(gate);
	gate->CloseDoor();
}

static g_num_chests;
static const MAX_CHESTS = 14;

public func OnChestOpened(object collector)
{
	++g_num_chests;
	var sAchievement = "";
	if (g_num_chests == 4)
	{
		sAchievement = "|$Achieve4$";
		GainScenarioAchievement("Chests", 1);
	}
	else if (g_num_chests == MAX_CHESTS/2)
	{
		sAchievement = "|$Achieve7$";
		GainScenarioAchievement("Chests", 2);
	}
	else if (g_num_chests == MAX_CHESTS)
	{
		sAchievement = "|$Achieve14$";
		GainScenarioAchievement("Chests", 3);
	}
	UpdateLeagueScores();
	Dialogue->MessageBoxAll(Format("$MsgChestOpened$%s", g_num_chests, MAX_CHESTS, sAchievement), collector, true);
	return true;
}

public func OnGoalsFulfilled()
{
	GainScenarioAchievement("Done");
	UpdateLeagueScores();
}

public func OnGameOver()
{
	UpdateLeagueScores();
}

private func UpdateLeagueScores()
{
	// +50 for finishing and +5 for every chest
	var goal_finished = (g_goal && g_goal->IsFulfilled());
	return SetLeagueProgressScore(g_num_chests, g_num_chests * 5 + goal_finished * 50);
}
