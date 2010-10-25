/*-- 
	ForgottenHeights
	Author: Mimmo_O
	
	Last man Standing on sky islands for up to 12 players.
--*/



protected func Initialize()
{

	// Goal.
	CreateObject(Goal_LastManStanding, 0, 0, NO_OWNER);
	
	//Enviroment.
	CreateObject(Environment_Clouds);
	CreateObject(Environment_Clouds); //Double the clouds.
	SetSkyAdjust(RGBa(250,250,255,128),RGB(200,200,220));
	Sound("BirdsLoop.ogg",true,100,nil,+1);
	
	// Chests with weapons.
	CreateObject(Chest, 200, 150, NO_OWNER);
	CreateObject(Chest, 400, 460, NO_OWNER);
	CreateObject(Chest, 380, 220, NO_OWNER);
	CreateObject(Chest, 625, 300, NO_OWNER);
	CreateObject(Chest, 100, 480, NO_OWNER);
	CreateObject(Chest, 65, 660, NO_OWNER)->SetClrModulation(RGBa(100,180,255,220));

	AddEffect("IntFillChests", nil, 100, 2 * 36, this);
	// Smooth brick edges.
	AddEffect("ChanneledWind", nil, 100, 1, this);
	PlaceEdges();
	MovingBricks();
	return;
}

global func MovingBricks()
{
	CreateObject(MovingBrick,265,180)->Room(0,85,0,0,8);
	CreateObject(MovingBrick,265,500)->Room(0,40,2,0,7);
	CreateObject(MovingBrick,320,500)->Room(0,50,3,0,6);
	CreateObject(MovingBrick,660,430)->Room(0,75,3,0,9);
	CreateObject(MovingBrick,140,480)->Room(0,180,1,0,17);
	DrawMap(119,490,10,70,"map plainbrick{mat=Tunnel; tex=brickback;};");
	DrawMap(121,490,10,70,"map plainbrick{mat=Tunnel; tex=brickback;};");
}

global func FxChanneledWindTimer()
{
	for(var obj in FindObjects(Find_InRect(110,150,30,280)))
	{
		obj->SetYDir(Min(obj->GetYDir()-3,-14));
	}
	for(var obj in FindObjects(Find_InRect(110,130,70,40)))
	{
		obj->SetXDir(obj->GetXDir()+2);
	}
	for(var obj in FindObjects(Find_InRect(110,0,100,100)))
	{
		obj->SetXDir(obj->GetXDir()+2);
	}
	CreateParticle("AirIntake",110+Random(30),230+Random(185),RandomX(-1,1),-30,60+Random(10),RGB(100+Random(25),128+Random(20),255));
	CreateParticle("AirIntake",110+Random(30),230+Random(180),RandomX(-1,1),-30,60+Random(10),RGB(100+Random(25),128+Random(20),255));
	CreateParticle("AirIntake",110+Random(70),130+Random(30) , 30,Random(3),60+Random(10),RGB(100+Random(25),128+Random(20),255));
	CreateParticle("AirIntake",110+Random(80),Random(100) , 30,Random(3),60+Random(10),RGB(100+Random(25),128+Random(20),255));
}

global func PlaceEdges()
{
	var x=[545, 535, 555, 565, 195, 185, 165, 155, 455, 445, 315, 325, 535, 525, 525, 525, 525, 545, 545, 545, 525, 535, 525, 525, 525, 685, 675, 635, 625, 155, 145, 225, 205, 85, 375, 435, 445, 595, 585, 575, 545, 535, 525, 515, 115, 835, 815, 795, 785, 855, 855, 1315, 1165, 1205, 1415, 1425, 1405, 875, 875, 775, 775, 185, 185, 175, 305, 305, 465, 465, 145, 105, 155, 105, 75, 85, 755, 775, 785, 475, 475, 75, 55, 55, 215, 215, 205, 365, 365, 375, 455, 445, 455, 455, 455, 365, 365, 605, 605, 585, 595, 1195, 1195, 1205, 1165, 1155, 1155, 1425, 1435, 1435, 65, 75, 85, 95, 65, 136, 114, 136, 114];
	var y=[515, 515, 315, 315, 495, 495, 165, 165, 245, 245, 245, 245, 565, 555, 535, 545, 565, 555, 565, 535, 535, 565, 545, 555, 525, 515, 505, 505, 515, 405, 175, 175, 505, 495, 495, 495, 505, 385, 355, 325, 325, 335, 345, 355, 135, 315, 215, 505, 515, 515, 545, 45, 415, 115, 415, 425, 435, 245, 255, 285, 275, 125, 115, 115, 255, 265, 265, 255, 415, 315, 415, 445, 455, 445, 335, 495, 495, 365, 375, 485, 485, 475, 525, 515, 525, 525, 515, 525, 515, 535, 525, 525, 535, 515, 505, 425, 415, 415, 425, 145, 135, 145, 435, 425, 435, 445, 435, 445, 495, 475, 465, 455, 465, 545, 555, 495, 495];
	var d=[0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 2, 1, 1, 3, 3, 0, 2, 2, 3, 3, 1, 3, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 1, 2, 0, 3, 0, 2, 3, 1, 1, 0, 0, 3, 0, 2, 3, 1, 2, 0, 1, 1, 3, 2, 0, 3, 0, 2, 2, 1, 1, 2, 3, 0, 1, 3, 0, 3, 1, 2, 0, 3, 3, 1, 2, 0, 3, 0, 2, 2, 3, 1, 2, 0, 3, 3, 3, 1, 2, 2, 1, 3, 3, 0, 2, 3, 2, 2, 2, 1, 2, 3, 0, 1];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i] + 5, NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]);
		edge->PermaEdge();
	}
	CreateObject(BrickEdge,90,690)->PermaEdge();
	return 1;
}


// Gamecall from LastManStanding goal, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	return;
}


// Refill/fill chests.
global func FxIntFillChestsStart(object target, int num, int temporary)
{
	if(temporary) return 1;
	var chests = FindObjects(Find_ID(Chest),Find_InRect(0,0,LandscapeWidth(),610));
	var w_list = [Shield, Javelin, FireballScroll, Bow, Musket, WindScroll, TeleportScroll];
	
	for(var chest in chests)
		for(var i=0; i<4; ++i)
			chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxIntFillChestsTimer()
{
	SetTemperature(100);
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	var w_list = [Boompack, Dynamite, Shield,Javelin, Bow, Musket, Boompack, Dynamite, Shield,Javelin, Bow, Musket, TeleportScroll, WindScroll, FireballScroll];
	if(chest->GetY()>600)
	{
		w_list = [FireballScroll, FireballScroll, TeleportScroll, WindScroll, WindScroll];
		if(!FindObject(Find_ID(JarOfWinds))) w_list[GetLength(w_list)]=JarOfWinds;
	}
	if (chest->ContentsCount() < 5)
		chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func CreateChestContents(id obj_id)
{
if (!this)
		return;
	var obj = CreateObject(obj_id);
	if (obj_id == Bow)
		obj->CreateContents(Arrow);
	if (obj_id == Musket)
		obj->CreateContents(LeadShot);
	obj->Enter(this);
	return;
}

// GameCall from RelaunchContainer.
func OnClonkLeftRelaunch(object clonk)
{
	var r=Random(5);
	var x,y;
	if(r==0) { x=370; y=220; }
	if(r==1) { x=600; y=500; }
	if(r==2) { x=100; y=470; }
	if(r==3) { x=500; y=330; }
	if(r==4) { x=200; y=150; }
	if(r==5) { x=400; y=470; }
	clonk->SetPosition(x,y);
	CastParticles("Magic",36,12,x,y,30,60,clonk->GetColor(),clonk->GetColor(),clonk);
	clonk->SetYDir(-5);
}

func KillsToRelaunch() { return 0; }
func RelaunchWeaponList() { return [Bow,  Javelin, Musket, FireballScroll, WindScroll, TeleportScroll]; }
