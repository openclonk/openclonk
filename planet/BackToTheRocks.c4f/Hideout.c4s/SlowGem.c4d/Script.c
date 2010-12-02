/*--- Flint ---*/

local e;

protected func Initialize()
{
	if(this->GetX() < LandscapeWidth()/2){
		SetGraphics("E");
		e=true;
	}
	else 
	{
		SetGraphics("");
		e=false;
	}
	SetR(Random(360));
}

protected func Departure()
{
	SetRDir(RandomX(-15,15));
}

func Hit()
{

	AddEffect("GemSlowField",nil,100,1,nil,nil,GetX(),GetY(),e);
	RemoveObject();
}
global func FxGemSlowFieldStart(object target, int num, int temporary, x, y, e)
{
	if (temporary) 
		return 1;
	EffectVar(0, target, num)=x;
	EffectVar(1, target, num)=y;
	EffectVar(2, target, num)=e;	
	Log("OK");
		
}
global func FxGemSlowFieldTimer(object target, int num, int time)
{
	var x=EffectVar(0, target, num);
	var y=EffectVar(1, target, num);
	var e=EffectVar(2, target, num);
	if(time > (36*5)) return -1;
	for(var i=0; i<40; i++)
	{
		var r=Random(360);
		var d=Min(Random(20)+Random(130),50);
		if(!PathFree(x,y,x + Sin(r,d), y - Cos(r,d))) continue;
		var clr=RGB(122+Random(20),18+Random(10),90+Random(20));
		if(e)clr=RGB(190+Random(10),0,20+Random(20));
		if(Random(2))CreateParticle("MagicSpark", x + Sin(r,d), y - Cos(r,d),0,0,10+Random(12),clr);
		else CreateParticle("Magic", x + Sin(r,d), y - Cos(r,d),0,0,10+Random(6),clr);
	}
	for(var obj in FindObjects(Find_Distance(50,x,y)))
	{
		if(!PathFree(x,y,obj->GetX(),obj->GetY())) continue;
		if(Distance(0,0,obj->GetXDir(),obj->GetYDir()) < 16 ) continue;
		var speed=Distance(0,0,obj->GetXDir(),obj->GetYDir());
		var dir = Angle(0,0,obj->GetXDir(),obj->GetYDir());
		obj->SetXDir(obj->GetXDir(100) + Sin(-dir,speed*3) ,100);
		obj->SetYDir(obj->GetYDir(100) + Cos(-dir,speed*3) -10,100);
		obj->SetYDir(obj->GetYDir()-5);
	}
	return 1;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
