/* Sky race */

func Initialize()
{

 CreateObject(DYNA,1050,1150,-1);
 CreateObject(DYNA,1050,1150,-1);
 
 CreateObject(DYNA,500,900,-1);
 CreateObject(DYNA,500,900,-1);
 
 
 CreateObject(DYNA,1336,1116,-1);
 CreateObject(DYNA,1675,1075,-1);
 CreateObject(BPDR,1130,1007,-1);
 
 DrawMaterialQuad("Tunnel",1378,1327-5,1860,1327-5,1860,1330,1387,1330);
 for(var i = 1380; i<=1800; i+=30)
 {
 	CreateObject(BPDR,i,1328,-1);
 
 }
 
 CreateObject(BPDR,2553,918,-1);
 
 CreateObject(DYNA,3208,1188,-1);
 
 CreateObject(DYNA,3361,749,-1);
 CreateObject(DYNA,3243,557,-1);
 
 for(var i=0; i<=6; i++)
 CreateObject(DYNA,3090-(3*i),564,-1);
 
 // Create the race goal.
	var pGoal = CreateObject(PARK, 0, 0, NO_OWNER);
	pGoal->SetStartpoint(20, 1000);
	pGoal->AddCheckpoint(760,950,RACE_CP_Ordered);
	pGoal->AddCheckpoint(400,660,RACE_CP_Ordered);
	pGoal->AddCheckpoint(870,460,RACE_CP_Respawn);
	pGoal->AddCheckpoint(1200,1020,RACE_CP_Ordered);
	pGoal->AddCheckpoint(1665,1070,RACE_CP_Ordered);
	pGoal->AddCheckpoint(1120,1010,RACE_CP_Ordered);
	pGoal->AddCheckpoint(1485,800,RACE_CP_Ordered);
	pGoal->AddCheckpoint(1735,1410,RACE_CP_Ordered);
	pGoal->AddCheckpoint(2110,1180, RACE_CP_Respawn);
	pGoal->AddCheckpoint(3350,1240,RACE_CP_Ordered);	
	pGoal->AddCheckpoint(3040,720, RACE_CP_Respawn);
	pGoal->AddCheckpoint(2530,520,RACE_CP_Ordered);
	pGoal->AddCheckpoint(2150,510,RACE_CP_Ordered);
	pGoal->AddCheckpoint(2740,350,RACE_CP_Ordered);
	pGoal->SetFinishpoint(3490,100);
 
 

 return 1;
 }
 

// Gamecall from Race-goal, on respawning.
protected func PlrHasRespawned(int iPlr, object cp)
{
	var clonk = GetCrew(iPlr);
	clonk->CreateContents(MJOW);
	return;
}


/* Relaunch */

