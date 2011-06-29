/*-- Moving Bricks --*/

local ox,oy,rx,ry,size;

protected func Initialize()
{

	ox=GetX();
	oy=GetY();
	ry=40;
	rx=0;
	SetAction("Moving");
	SetPosition(ox,oy);
	size=4;
	AddEffect("MovingAround",this,100,1,this,this->GetID());
	return;
}


func Room(int xa, int ya, int a,int xd, int yd) 
{  
	rx=xa; 
	ry=ya; 
	SetXDir(xd);
	SetYDir(yd);
	
		 if(a==1){ 	SetGraphics("S1"); size=1;}
	else if(a==2){ 	SetGraphics("S2"); size=2;}
	else if(a==3){ 	SetGraphics("S3"); size=3;}
	else		 {	SetGraphics(Format("%d",1+Random(4))); size=4;}
}

func FxMovingAroundTimer(object pTarget, effect, int timer)
{
 	pTarget->Move(timer%360);
}

func Move(int a)
{

	if(GetY()<oy)SetYDir(GetYDir()+1);
	if(GetY()>(oy+ry))SetYDir(GetYDir()-1);
	if(GetX()<ox)SetXDir(GetXDir()+1);
	if(GetX()>(ox+rx))SetXDir(GetXDir()-1);
	
	DigFreeRect(GetX()-20,GetY()-6,size*10,12);

}

func Definition(def) 
{

SetProperty("ActMap", {
		Moving = {
			Prototype = Action,
			Name = "Moving",
			Procedure = DFA_FLOAT,
			Length = 1,
			Delay = 1,
			X = 0,
			Y = 0,
			Wdt = 40,
			Hgt = 10,
			NextAction = "Moving",
		},
	}, def);
	SetProperty("Name", "$Name$", def);
}
