/* Boomshire */

func Initialize()
{

	DrawMaterialQuad("Tunnel",1378,1327-5,1860,1327-5,1860,1330,1387,1330,1);
	for(var i = 1380; i<=1800; i+=28)
	{
		CreateObject(Blackpowder,i,1328,-1);
	
	}

	// Create the race goal.
	var pGoal = CreateObject(Goal_Parkour, 0, 0, NO_OWNER);
	pGoal->SetStartpoint(20, 1000);
	pGoal->AddCheckpoint(760,950,  PARKOUR_CP_Ordered);
	pGoal->AddCheckpoint(400,660,  PARKOUR_CP_Ordered);
	pGoal->AddCheckpoint(870,460,  PARKOUR_CP_Respawn|PARKOUR_CP_Ordered);	
	pGoal->AddCheckpoint(1665,1070,PARKOUR_CP_Ordered);
	pGoal->AddCheckpoint(1485,800, PARKOUR_CP_Ordered);
	pGoal->AddCheckpoint(1735,1410,PARKOUR_CP_Ordered);
	pGoal->AddCheckpoint(2110,1180,PARKOUR_CP_Respawn|PARKOUR_CP_Ordered);
	pGoal->AddCheckpoint(3350,1240,PARKOUR_CP_Ordered);
	pGoal->AddCheckpoint(3040,720, PARKOUR_CP_Respawn|PARKOUR_CP_Ordered);
	pGoal->AddCheckpoint(2530,520, PARKOUR_CP_Ordered);
	pGoal->AddCheckpoint(2150,510, PARKOUR_CP_Ordered);
	pGoal->SetFinishpoint(1500,200);
	
	//Items
	CreateObject(Blackpowder,1904,878,-1);
	CreateObject(Blackpowder,1905,878,-1);
	CreateObject(Blackpowder,1906,878,-1);
	
	CreateObject(Blackpowder,2568,918,-1);
	
	CreateObject(Dynamite,3210,1190,-1);
	CreateObject(Dynamite,3205,1190,-1);
	CreateObject(Dynamite,3200,1190,-1);
	CreateObject(Dynamite,3195,1190,-1);

	Edges();
	Doors();
	Decoration();
	AddEffect("DynamiteEruption",0,100,130);
	return 1;
}
global func FxDynamiteEruptionTimer(object nobject, int noum, int timer)
{
	var dyn=CreateObject(Firestone,2460+Random(20),670);
	dyn->SetYDir(-80);
	dyn->SetXDir(RandomX(-1,1));
	dyn->SetRDir(RandomX(-30,30));
}



global func FxSparklingAttentionTimer(object pTarget, int noum, int timer)
{
	CastParticles("Flash",Random(10)+10,60,pTarget->GetX(),pTarget->GetY(),100,120,RGBa(255,235,200,80+Random(36)));
}

protected func Decoration()
{
	PlaceObjects(Rock,300,"Earth");
	PlaceObjects(Loam,200,"Earth");
	PlaceObjects(Gold,100,"Earth");
	PlaceGrass(85);

}

protected func Doors()
{
	var gate = CreateObject(StoneDoor, 865, 1195, NO_OWNER);
	var wheel = CreateObject(SpinWheel, 1045, 1165, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseGateDoor();
	AddEffect("SparklingAttention",wheel,100,100);
	
	var gate = CreateObject(StoneDoor, 1155, 1026, NO_OWNER);
	var	wheel = CreateObject(ArrowWheel, 1906, 778, NO_OWNER);
	var straw = CreateObject(DoorTarget,1850,820,NO_OWNER);
	straw->SetGate(gate);
	straw->SetAction("Float");
	gate->CloseGateDoor();
	AddEffect("SparklingAttention",wheel,100,100);
	
	var gate = CreateObject(StoneDoor, 1875, 761, NO_OWNER);
	var wheel = CreateObject(SpinWheel, 1752, 1148, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseGateDoor();
	AddEffect("SparklingAttention",wheel,100,100);
	
	var gate = CreateObject(StoneDoor, 1875, 864, NO_OWNER);
	var wheel = CreateObject(SpinWheel, 1116, 1038, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseGateDoor();
	AddEffect("SparklingAttention",wheel,100,100);
	
	var gate = CreateObject(StoneDoor, 3115, 685, NO_OWNER);
	var wheel = CreateObject(SpinWheel, 3140, 588, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseGateDoor();
	AddEffect("SparklingAttention",wheel,100,100);
	
	var gate = CreateObject(StoneDoor, 585, 915, NO_OWNER);
	var wheel = CreateObject(SpinWheel, 853, 681, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseGateDoor();
	AddEffect("SparklingAttention",wheel,100,100);
	
	var gate = CreateObject(StoneDoor, 345, 740, NO_OWNER);
	var wheel = CreateObject(SpinWheel, 417, 712, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseGateDoor();
	AddEffect("SparklingAttention",wheel,100,100);
	
	var gate = CreateObject(StoneDoor, 1935, 550, NO_OWNER);
	var wheel = CreateObject(SpinWheel, 1900, 565, NO_OWNER);
	wheel->SetStoneDoor(gate);
	gate->CloseGateDoor();
	AddEffect("SparklingAttention",wheel,100,100);
		
}

protected func Edges()
{
	var x=[2435, 2555, 2565, 2565, 2565, 2565, 2685, 2765, 2755, 2755, 2745, 2705, 2695, 2685, 2695, 2685, 2675, 2655, 2655, 2665, 2675, 2665, 2645, 2655, 2645, 2635, 2615, 2625, 2635, 2625, 2605, 2615, 2605, 2595, 2575, 2585, 2595, 2585, 2565, 2575, 2565, 2555, 3535, 3525, 3545, 3555, 3555, 3575, 3585, 3595, 3605, 3605, 3595, 3585, 3575, 3385, 1935, 1935, 1915, 1895, 735, 685, 745, 755, 775, 815, 825, 595, 605, 365, 1925, 1915, 1895, 1925, 1935, 1915, 1925, 1925, 1915, 1925, 1935, 1905, 1905, 2085, 2135, 2035, 2035, 2045, 2055, 2195, 2215, 2225, 2245, 2235, 2255, 2585, 2515, 2485, 2565, 2785, 3145, 3085, 3015, 3125, 3085, 2995, 2875, 2865, 2865, 2855, 2555, 2945, 2955, 3005, 2745, 2735, 2705, 2715, 2695, 2685, 2655, 2665, 2515, 2505, 2565, 2575, 2405, 2415, 1885, 1945, 1995, 1985, 1965, 1955, 1945, 1935, 1925, 1885, 1865, 1855, 1845, 1725, 1695, 1595, 1575, 1795, 2555, 2545, 2545, 2555, 2555, 2545, 2545, 2555, 2535, 335, 345, 325, 275, 265, 95, 45, 45, 425, 385, 605, 565, 555, 2155];
	var y=[1285, 1305, 1295, 1265, 1245, 1225, 1175, 1225, 1215, 1225, 1215, 1155, 1155, 1145, 1145, 1135, 1135, 1105, 1115, 1115, 1125, 1125, 1105, 1105, 1095, 1095, 1075, 1075, 1085, 1085, 1065, 1065, 1055, 1055, 1035, 1035, 1045, 1045, 1025, 1025, 1015, 1015, 675, 665, 655, 665, 625, 635, 645, 655, 665, 715, 725, 735, 745, 745, 885, 865, 865, 865, 1015, 1155, 1005, 995, 985, 995, 1005, 955, 965, 645, 785, 885, 885, 985, 975, 1065, 1075, 1105, 1115, 1015, 1025, 1055, 1125, 1245, 1245, 1065, 895, 865, 845, 975, 995, 1015, 1035, 1025, 1045, 925, 925, 1205, 1205, 1235, 725, 735, 735, 595, 585, 745, 635, 625, 645, 635, 915, 655, 645, 675, 585, 595, 585, 575, 585, 575, 585, 595, 655, 645, 635, 645, 595, 585, 575, 595, 485, 465, 455, 445, 435, 425, 405, 395, 385, 375, 365, 345, 325, 285, 275, 565, 935, 935, 955, 955, 975, 975, 995, 995, 915, 695, 785, 785, 835, 845, 715, 655, 625, 545, 525, 835, 835, 825, 935];
	for(var i=0; i<GetLength(x); i++)
	{
		CreateObject(BrickEdge,x[i], y[i]+5)->PermaEdge();
	}
}

// Gamecall from Race-goal, on respawning.
protected func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	clonk->CreateContents(JarOfWinds);
	return;
}

func GetEdges()
{
var x=[];
var y=[];
	for(var e in FindObjects(Find_ID(BrickEdge)))
	{
		x[GetLength(x)]=e->GetX();
		y[GetLength(y)]=e->GetY();
	}
	Log("%v",x);
	Log("%v",y);
}



/* Relaunch */

