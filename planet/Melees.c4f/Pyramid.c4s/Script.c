/*--
	Pyramid
	Author: Maikel
	
	A king of the hill scenario on a pyramid.
--*/

protected func Initialize()
{
	// Goal settings.
	CreateObject(Goal_KingOfTheHill, 625, 170, NO_OWNER);
	Goal_KingOfTheHill->SetRadius(120);
	Goal_KingOfTheHill->SetPointLimit(6);
	
	// Weapon chests.
	CreateObject(Chest, 690, 210, NO_OWNER);
	CreateObject(Chest, 680, 360, NO_OWNER);
	CreateObject(Chest, 430, 440, NO_OWNER);
	CreateObject(Chest, 200, 760, NO_OWNER);
	CreateObject(Chest, 970, 760, NO_OWNER);
	CreateObject(Chest, 860, 560, NO_OWNER);
	CreateObject(Chest, 670, 610, NO_OWNER);
	AddEffect("IntFillChests", nil, 100, 70, this);
	
	// Brick edges.
	var edges = [
	// Left outside part of pyramid.
	[190,560],[200,550],[210,540],[220,530],[280,470],[290,460],[300,450],[310,440],[360,390],[370,380],[380,370],[390,360],[400,350],[410,340],[420,330],[480,270],[490,260],[500,250],[510,240],[520,230],[530,220],[580,170],[590,160],[600,150],[610,140],[620,130]
	// Right outside part of pyramid.
	,[640,130],[650,140],[660,150],[670,160],[680,170],[730,220],[740,230],[790,280],[800,290],[810,300],[820,310],[830,320],[840,330],[850,340],[900,390],[910,400],[920,410],[930,420],[940,430],[950,440],[960,450],[1010,500],[1020,510],[1030,520],[1040,530],[1050,540],[1060,550],[1070,560],[1080,570]
	// Left bottom inside and middle "stairs".
	,[400,530],[420,540],[440,550],[340,440],[450,450],[530,550],[540,540],[550,530],[560,520],[570,510],[580,500],[590,490],[600,480],[480,490],[490,480],[500,470],[510,460],[520,450],[530,440]
	// Right bottom inside.
	,[630,560],[720,560],[890,570],[730,510],[740,500],[750,490],[760,480],[780,470],[800,460],[820,450],[850,450],[860,460],[870,470],[880,480],[890,490],[950,520],[970,510],[980,500]
	// Top inside area.
	,[470,330],[520,330],[710,310],[730,300],[740,290],[750,280],[630,400],[640,390],[650,380],[660,370],[710,370],[720,400],[730,410],[490,280]
	// Lower and underground area.
	,[630,610],[400,640],[410,630],[420,620],[430,610]
	];
	for(var i = 0; i < GetLength(edges); i++)
		CreateObject(BrickEdge, edges[i][0], edges[i][1], NO_OWNER)->PermaEdge();
	

	return;
}

/*-- Relaunch & spawn player --*/

protected func InitializePlayer(int plr)
{
	JoinPlayer(plr);
	return;
}

protected func RelaunchPlayer(int plr)
{
	var clonk = CreateObject(Clonk, 0, 0, plr);
	clonk->MakeCrewMember(plr);
	SetCursor(plr, clonk);
	JoinPlayer(plr);
	return;
}

protected func JoinPlayer(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
	var x = Random(LandscapeWidth()), y = 590;
	clonk->SetPosition(x, y);
	return;
}

/*-- Fill chests --*/

global func FxIntFillChestsStart()
{
	var chests = FindObjects(Find_ID(Chest));
	var w_list = [Bow,Musket,Shield,Sword,Club,Javelin,Bow,Musket,Shield,Sword,Club,Javelin,DynamiteBox,JarOfWinds];
	
	for(var chest in chests)
		for(var i=0; i<4; ++i)
			chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
}

global func FxIntFillChestsTimer()
{
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	var w_list = [Boompack,Dynamite,Loam,Firestone,Bow,Musket,Sword,Javelin,JarOfWinds];
	
	if (chest && chest->ContentsCount() < 5)
		chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
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
