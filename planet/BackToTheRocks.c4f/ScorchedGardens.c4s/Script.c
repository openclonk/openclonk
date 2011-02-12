/*-- 
	Scorches Gardens
	Author: Mimmo_O
	
	A melee in a fiery setting.
--*/



protected func Initialize()
{

	// Goal.
	CreateObject(Goal_LastManStanding, 0, 0, NO_OWNER);
	CreateObject(Rule_KillLogs);
	
	//Enviroment.
	CreateObject(Rule_ObjectFade)->DoFadeTime(10 * 36);
	StartItemSparks(60+GetPlayerCount()*10,false);
	SetSkyAdjust(RGB(255,128,0));	
	CreateObject(Column,160,229);
	CreateObject(Column,448,269);
	
	AddEffect("RandomMeteor", nil, 100, 72, this);
	AddEffect("RemoveCorpses", nil, 100, 1, this);
	// Smooth brick edges.
	PlaceEdges();
	PlaceGras();
	return;
}

protected func GetSparkItem(int x, int y)
{
	if(x==nil)
		return nil;
	var objects=[Firestone, Firestone, Firestone, Firestone, PowderKeg, Dynamite, Dynamite, Sword, Shield, Club, Javelin];
	return objects[Random(GetLength(objects))];
}
global func FxRemoveCorpsesTimer()
{
	//uber effect abuse
	
	for(var dead in FindObjects(Find_ID(Clonk),Find_Not(Find_OCF(OCF_Alive))))
	{
		CastParticles("MagicFire",100,50,dead->GetX(),dead->GetY(),50+Random(30));
		CastParticles("MagicFire",50,30,dead->GetX(),dead->GetY(),70+Random(60));
		dead->RemoveObject();		
	}
	for(var burning in FindObjects(Find_ID(Clonk),Find_OCF(OCF_OnFire)))
	{
		burning->DoEnergy(-3); //lava hurts a lot
	}

}
global func FxRandomMeteorTimer()
{
	if(Random(2)) return ;
	
	var flint = CreateObject(Firestone,50+Random(LandscapeWidth()-100),-10);
	flint->SetYDir(25+Random(6));
	flint->SetXDir(RandomX(-20,20));
	AddEffect("Meteorsparkle",flint,100,1,nil,nil,true);
}

public func FxMeteorSparkleStart(obj, effect, iTemp, natural)
{
	if(iTemp) return;
	effect.n=natural;
}

global func FxMeteorsparkleTimer(obj, effect, time)
{

	var x=obj->GetX(), y=obj->GetY();
	CreateParticle("FireballSmoke",x,y,Sin(Random(360),2),Cos(Random(360),2),RandomX(120,180),RGBa(100,100,100,70));
	for(var i=0; i<6; i++) CreateParticle("MagicFire",x,y,Sin(Random(360),RandomX(5,6)),Cos(Random(360),RandomX(5,6)),RandomX(50,90),HSL(Random(50), 200+Random(25), Random(100)));
	CreateParticle("MagicSpark",x,y,Sin(Random(360),RandomX(15,33)),Cos(Random(360),RandomX(15,33)),RandomX(30,70),RGB(255,255,255));
	if(obj->Contained()) obj->Hit();
	if(Abs(obj->GetXDir())<3 && Abs(obj->GetYDir())<3) effect.count++;
	else effect.count=0;
	if(effect.count>10) obj->Hit();
}

global func FxMeteorsparkleStop (obj, effect, reason, iTemp)
{
	if(iTemp) return;
	for(var i=0; i<30; i++) CreateParticle("MagicSpark",obj->GetX(),obj->GetY(),Sin(Random(360),RandomX(15,33)),Cos(Random(360),RandomX(15,33)),RandomX(30,70),RGB(255,255,255));
	
	return ;
}

global func PlaceEdges()
{
	var x=[65, 65, 75, 85, 485, 75, 565, 565, 535, 565, 575, 525, 535, 595, 585, 575, 35, 45, 65, 65, 255, 245, 475, 465, 275, 35, 95, 235, 535, 565, 665, 675, 685, 775, 685, 775, 235, 265, 535, 765, 695, 765, 765, 365, 525, 505];
	var y=[355, 345, 365, 365, 255, 295, 305, 315, 315, 215, 215, 255, 255, 175, 185, 195, 225, 225, 295, 305, 275, 285, 255, 265, 265, 235, 265, 495, 265, 205, 375, 365, 355, 355, 275, 415, 295, 265, 305, 345, 345, 255, 285, 445, 445, 435];
	var d=[3, 1, 3, 2, 0, 0, 1, 3, 2, 3, 0, 1, 0, 1, 1, 1, 1, 0, 1, 3, 1, 1, 1, 1, 0, 3, 2, 3, 2, 1, 1, 1, 1, 0, 3, 2, 1, 1, 0, 0, 1, 1, 3, 3, 2, 3];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i] + 5, NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]);
		edge->PermaEdge();
	}
	return 1;
}

global func PlaceGras()
{
	var x=[523, 519, 528, 282, 278, 483, 488, 534, 539, 274, 461, 469, 465, 474, 585, 595, 562, 589, 575, 580, 570, 599, 566, 510, 516, 454, 504, 499, 494, 447, 441, 434, 422, 428, 399, 416, 411, 406, 392, 386, 336, 342, 349, 355, 369, 374, 379, 362, 313, 330, 325, 320, 306, 300, 293, 287, 257, 266, 235, 262, 246, 251, 240, 230, 220, 226, 197, 214, 209, 204, 190, 184, 65, 75, 80, 71, 60, 134, 140, 147, 153, 167, 172, 177, 160, 111, 128, 123, 118, 104, 98, 91, 85, 56, 62, 69, 75, 89, 35, 45, 50, 41, 30, 103, 102, 94, 102, 102, 99, 82];
	var y=[253, 258, 248, 269, 264, 252, 256, 251, 256, 259, 267, 257, 262, 252, 183, 172, 207, 178, 192, 188, 197, 166, 202, 259, 259, 270, 259, 259, 259, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 270, 272, 260, 293, 266, 283, 276, 288, 297, 300, 300, 300, 300, 300, 300, 300, 300, 292, 291, 295, 290, 297, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 229, 229, 229, 229, 228, 221, 220, 226, 219, 226, 250, 257, 228, 242, 234, 228, 229];
	var r=[-45, -45, -45, 45, 45, 45, 45, 45, 45, 45, -45, -45, -45, -45, -45, -45, -45, -45, -45, -45, -45, -45, -45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -45, -45, -45, -45, -45, -45, -45, -45, 0, 0, 0, 0, 0, 0, 0, 0, -45, 45, 45, 0, -45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -45, 45, 45, 0, -45, 90, 90, 0, 90, 90, 0, 0];
	for (var i = 0; i < GetLength(x); i++)
	{
		while(GBackSolid(x[i],y[i])) y[i]--;
		var edge=CreateObject(Grass, x[i], y[i] + 5, NO_OWNER);
		edge->SetCategory(C4D_StaticBack);
		edge->SetR(r[i]); 
		edge->Initialize();
		edge->SetClrModulation(RGB(200+Random(50),100+Random(60),100+Random(60)));
		
	}
	return 1;
}

protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
	var x = RandomX(75,500);
	var y=100;
	while(!GBackSolid(x,y)) y+=10;
	y-=30;
	var relaunch = CreateObject(RelaunchContainer, x, y, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	relaunch->SetRelaunchTime(3);
	clonk->CreateContents(TeleGlove);
	return;
}

func OnClonkLeftRelaunch(object clonk)
{
	CastParticles("Magic",36,12,clonk->GetX(),clonk->GetY(),30,60,clonk->GetColor(),clonk->GetColor(),clonk);
	clonk->SetYDir(-5);
	return;
}

func KillsToRelaunch() { return 3; }
func RelaunchWeaponList() { return []; }
