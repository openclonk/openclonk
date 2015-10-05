/* Boomshire */

private func Initialize()
{

	// Create dynamite below the first lava basin
	DrawMaterialQuad("Tunnel",1378,1327-5,1860,1327-5,1860,1330,1387,1330,1);

	//Sound("BirdsLoop",true,100,nil,+1);
	Cloud->Place(40);
	PlaceObjects(Rock,50,"Earth");
	PlaceObjects(Loam,25,"Earth");
	PlaceObjects(Nugget,25,"Earth");

	AddEffect("PlaneReset",CreateObjectAbove(Plane,3030,315,0),100,10,nil,nil);
	AddEffect("PlaneReset",CreateObjectAbove(Plane,3160,315,1),100,10,nil,nil);

	Doors();

	var concierge = CreateObjectAbove(Clonk, 70, 1030, NO_OWNER);
	concierge->SetDir(DIR_Left);
	concierge->SetSkin(2);
	concierge->SetMeshMaterial("clonkMime");
	concierge->SetColor(0xffffff);
	concierge->SetObjectLayer(concierge);
	concierge->SetName("$NameConcierge$");
	concierge->SetPortrait({ Source = DialogueBoomshire });
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
	DrawMaterialQuad("Tunnel-brickback",339,687,350,687,350,761,339,761);
	DrawMaterialQuad("Tunnel-brickback",1889,866,1955,866,1955,882,1889,882);
	DrawMaterialQuad("Tunnel-brickback",1920,874,1955,874,1930,1000,1920,1000);
	DrawMaterialQuad("Tunnel-brickback",1920,1000,1930,1000,1970,1100,1960,1100);
	DrawMaterialQuad("Tunnel-brickback",1960,1100,1970,1100,1930,1201,1919,1201);

	DrawMaterialQuad("Tunnel-brickback",2549,907,2590,907,2590,920,2549,920);
	DrawMaterialQuad("Tunnel-brickback",2549,920,2560,920,2560,1000,2550,1000);
	DrawMaterialQuad("Tunnel-brickback",2550,1000,2560,1000,2720,1160,2710,1160);
	DrawMaterialQuad("Tunnel-brickback",2710,1160,2720,1160,2710,1190,2700,1190);
	DrawMaterialQuad("Tunnel-brickback",2680,1185,2750,1185,2750,1195,2680,1195);
	DrawMaterialQuad("Tunnel-brickback",2737,1195,2750,1190,2775,1255,2765,1255);

	//AddEffect("DynamiteEruption",nil,100,130);
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
	if(Random(5))
	{
		var dyn=CreateObjectAbove(Firestone,2460+Random(20),670);
		dyn->SetYDir(-80);
		dyn->SetXDir(RandomX(-1,1));
		dyn->SetRDir(RandomX(-30,30));
	}
	else
	{
		var dyn=CreateObjectAbove(Dynamite,2460+Random(20),670);
		dyn->SetYDir(-80);
		dyn->SetXDir(RandomX(-1,1));
		dyn->SetRDir(RandomX(-30,30));
		dyn->Fuse();
	}
}

global func FxAutoOpenTimer(object pTarget, effect, int timer)
{
	if (!effect.fired)
	{
		if (FindObject(Find_ID(Plane),Find_InRect(2710,310,130,40)))
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
	if(FindObject(Find_ID(Plane),Find_InRect(0,0,2000,500)))
	{
		pTarget->OpenDoor();
		return FX_Execute_Kill;
	}
	else return FX_OK;
}

global func FxSparklingAttentionTimer(object pTarget, effect, int timer)
{
	CreateParticle("Flash", 0, 0, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(8, 15), {Prototype = Particles_Flash(), Size = 10}, 10);
}

global func FxPlaneResetTimer(object target, effect, int time)
{
	if(target->GetX() > 3000 || target->Contents())
	{
		effect.count=0;
		return FX_OK;
	}
	else
		effect.count++;
	
	if(effect.count<4) return FX_OK;
	
	var particles =
	{
		Prototype = Particles_Air(),
		Size = PV_Linear(4, 0)
	};
	DrawParticleLine("Air", target->GetX()+3, target->GetY(), 3030, 315, 1, PV_Random(-2, 2), PV_Random(-2, 2), PV_Random(10, 30), particles);
	
	target->SetPosition(3030,315);
	target->SetR(-90);
	target->SetDir(0);
}

private func Doors()
{
	var door=CreateObjectAbove(StoneDoor, 135, 60, NO_OWNER);
	door->CloseDoor();
	AddEffect("AutoOpen",door,1,15);

	var gate = CreateObjectAbove(StoneDoor, 865, 1195, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 1045, 1165, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel,100,100);

	var gate = CreateObjectAbove(StoneDoor, 1155, 1026, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 1906, 778, NO_OWNER);
	wheel->SetArrowWheel();
	var straw = CreateObjectAbove(DoorTarget,1850,820,NO_OWNER);
	straw->SetGate(gate);
	straw->SetAction("Float");
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel,100,100);

	var gate = CreateObjectAbove(StoneDoor, 1875, 761, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 1752, 1148, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel,100,100);

	var gate = CreateObjectAbove(StoneDoor, 1875, 864, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 1116, 1038, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel,100,100);

	var gate = CreateObjectAbove(StoneDoor, 3115, 685, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 3140, 588, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel,100,100);

	var gate = CreateObjectAbove(StoneDoor, 585, 915, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 853, 681, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel,100,100);

	var gate = CreateObjectAbove(StoneDoor, 345, 740, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 60, 644, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel,100,100);

	var gate = CreateObjectAbove(StoneDoor, 1935, 558, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 1900, 565, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseDoor();
	AddEffect("SparklingAttention",wheel,100,100);

	var gate = CreateObjectAbove(StoneDoor, 2965, 316, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 3260, 328, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseDoor();

	var gate = CreateObjectAbove(StoneDoor, 3285, 1150, NO_OWNER);
	var wheel = CreateObjectAbove(SpinWheel, 3220, 1200, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseDoor();
}

/*
global func PlaceEdges()
{
	var x=[25, 115, 155, 125, 195, 205, 215, 225, 125, 265, 145, 245, 145, 235, 2155, 555, 565, 605, 385, 425, 45, 45, 95, 265, 275, 325, 345, 335, 2535, 2555, 2545, 2545, 2555, 2555, 2545, 2545, 2555, 1795, 1575, 1595, 1695, 1725, 1845, 1855, 1865, 1885, 1925, 1935, 1945, 1955, 1965, 1985, 1995, 1945, 1885, 2415, 2405, 2575, 2565, 2505, 2515, 2665, 2655, 2685, 2695, 2715, 2705, 2735, 2745, 3005, 2955, 2945, 2555, 2855, 2865, 2865, 2875, 2995, 3125, 3015, 3085, 3145, 2785, 2565, 2485, 2515, 2585, 2255, 2235, 2245, 2225, 2195, 2055, 2045, 2035, 2035, 2135, 2085, 1905, 1905, 1935, 1925, 1915, 1925, 1925, 1915, 1935, 1925, 1895, 1915, 1925, 605, 595, 825, 815, 775, 755, 745, 735, 1895, 1915, 1935, 1935, 3385, 3575, 3585, 3595, 3605, 3605, 3595, 3585, 3575, 3555, 3555, 3545, 3525, 3535, 2555, 2565, 2575, 2565, 2585, 2595, 2585, 2575, 2595, 2605, 2615, 2605, 2625, 2635, 2625, 2615, 2635, 2645, 2655, 2645, 2665, 2675, 2665, 2655, 2655, 2675, 2685, 2695, 2685, 2695, 2705, 2745, 2755, 2755, 2765, 2685, 2565, 2565, 2565, 2565, 2555, 2435, 386, 3175, 3085, 3265, 2985, 3265, 3325, 1865, 1985, 1975, 1975, 1985, 2005, 1995, 2055, 2065, 2215, 2175, 2575, 2745, 2735];
	var y=[45, 45, 45, 125, 425, 395, 345, 275, 195, 35, 125, 175, 155, 215, 935, 825, 835, 835, 525, 545, 625, 655, 715, 845, 835, 785, 785, 695, 915, 995, 995, 975, 975, 955, 955, 935, 935, 565, 275, 285, 325, 345, 365, 375, 385, 395, 405, 425, 435, 445, 455, 465, 485, 595, 575, 585, 595, 645, 635, 645, 655, 595, 585, 575, 585, 575, 585, 595, 585, 675, 645, 655, 915, 635, 645, 625, 635, 745, 595, 735, 735, 725, 1235, 1205, 1205, 925, 925, 1045, 1025, 1035, 1015, 975, 845, 865, 895, 1065, 1245, 1245, 1125, 1055, 1025, 1015, 1115, 1105, 1075, 1065, 975, 985, 885, 885, 785, 965, 955, 1005, 995, 985, 995, 1005, 1015, 865, 865, 865, 885, 745, 745, 735, 725, 715, 665, 655, 645, 635, 625, 665, 655, 665, 675, 1015, 1015, 1025, 1025, 1045, 1045, 1035, 1035, 1055, 1055, 1065, 1065, 1085, 1085, 1075, 1075, 1095, 1095, 1105, 1105, 1125, 1125, 1115, 1115, 1105, 1135, 1135, 1145, 1145, 1155, 1155, 1215, 1225, 1215, 1225, 1175, 1225, 1245, 1265, 1295, 1305, 1285, 715, 1085, 585, 275, 275, 335, 395, 275, 535, 545, 885, 865, 755, 775, 715, 705, 995, 965, 935, 945, 945];
	var d=[2, 3, 2, 3, 2, 2, 2, 2, 1, 2, 2, 2, 0, 2, 3, 3, 3, 2, 1, 2, 2, 0, 0, 2, 2, 3, 2, 0, 1, 3, 0, 2, 1, 3, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 3, 2, 1, 1, 2, 2, 1, 1, 2, 0, 3, 3, 0, 0, 0, 3, 2, 3, 3, 0, 0, 0, 0, 0, 1, 0, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 3, 1, 1, 2, 0, 3, 1, 2, 3, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 3, 2, 3, 1, 0, 1, 1, 1, 1, 3, 3, 3, 3, 3, 0, 0, 3, 3, 0, 3, 3, 0, 0, 3, 3, 0, 0, 3, 3, 0, 0, 3, 3, 0, 0, 3, 3, 0, 0, 3, 3, 0, 3, 0, 3, 3, 0, 0, 3, 0, 0, 3, 3, 3, 1, 1, 1, 1, 1, 0, 0, 3, 1, 3, 2, 1, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 3];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObjectAbove(Boomshire_BrickEdge, x[i], y[i] + 5, NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]);
		edge->PermaEdge();
	}
	return 1;
}
*/

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
	else if (g_num_chests==MAX_CHESTS/2)
	{
		sAchievement = "|$Achieve7$";
		GainScenarioAchievement("Chests", 2);
	}
	else if (g_num_chests==MAX_CHESTS)
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
