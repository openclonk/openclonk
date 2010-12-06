/*--- Pyre Gem ---*/

local e;
local thrower;

protected func Initialize()
{
	if(Random(2))
	{
		SetGraphics("E");
		e=true;	
	}
	else
	{
		SetGraphics("");
		e=false;
	}
	
	if(this->GetX() < 920)
	{
		SetGraphics("E");
		e=true;
	}
	else if(this->GetX() > 1280)
	{
		SetGraphics("");
		e=false;
	}
	 
	SetR(Random(360));
}

protected func Departure( from)
{
	SetRDir(RandomX(-15,15));
	thrower=from;
}

func Hit()
{

	AddEffect("GemPyre",nil,100,1,nil,nil,[GetX(),GetY()],e,this->GetOwner(),thrower);
	RemoveObject();
}
global func FxGemPyreStart(object target, int num, int temporary, c, e, owner,thrower)
{
	if (temporary) 
		return 1;
	EffectVar(0, target, num)=c[0];
	EffectVar(1, target, num)=c[1];
	EffectVar(2, target, num)=e;
	EffectVar(3, target, num)=thrower;
	EffectVar(4, target, num)=owner;
	EffectVar(5, target, num)=0;
}
global func FxGemPyreTimer(object target, int num, int time)
{
	var x=EffectVar(0, target, num);
	var y=EffectVar(1, target, num);
	var e=EffectVar(2, target, num);
	
	if(time > 32) return -1;
	
	for(var i=0; i<(20 + time); i++)
	{
		var r = Random(360);
		var d = Random((((time/2)+1)*6)-((time/2)*4))+((time/2)*4)+RandomX(-2,2);
		if(!PathFree(x,y,x + Sin(r,d), y - Cos(r,d))) continue;
		var clr=RGB(122+Random(20),18+Random(10),90+Random(20));
		if(e)clr=RGB(190+Random(10),0,20+Random(20));
		if(Random(2))CreateParticle("AirIntake", x + Sin(r,d), y - Cos(r,d),RandomX(-5,5),RandomX(-5,-10),BoundBy((30-time),1,25)*2 + 10 + Random(10),clr);
		else CreateParticle("Magic", x + Sin(r,d), y - Cos(r,d),0,0,BoundBy((40-time),1,25) + 5 + Random(10),clr);
	}
	
	for(var obj in FindObjects(Find_NoContainer(), Find_OCF(OCF_Alive), Find_Distance(((time/2)+1)*6,x,y),Find_Not(Find_Distance((time/2)*4,x,y)),Find_ID(Clonk)))
	{
		var end=false;	
		for(var i = 0; i < EffectVar(5, target, num) ; i++)
			if(obj == EffectVar(6+i, target, num)) end=true;
		if(end) continue;
		if(PathFree(x,y,obj->GetX(),obj->GetY()))
		{
			obj->DoEnergy((-BoundBy((30-time),1,26)*3)/5,0,0,EffectVar(3,target,num));
			obj->CastParticles("MagicFire",20 + (BoundBy((30-time),1,26)*2),(BoundBy((30-time),6,26)*2),0,0,26,50,clr,clr);
			obj->CastParticles("Air",10 + BoundBy((30-time),1,26),10,0,0,16,30,clr,clr);
			obj->Fling(RandomX(-2,2),-2-(BoundBy((30-time),10,30)/10));
			EffectVar(6+EffectVar(5, target, num), target, num) = obj;
			EffectVar(5, target, num)++;
		}	
	}
	return 1;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
