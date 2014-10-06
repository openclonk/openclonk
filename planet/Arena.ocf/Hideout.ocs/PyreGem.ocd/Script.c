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

	AddEffect("GemPyre",nil,100,1,nil,nil,[GetX(),GetY()],e,this->GetOwner(),thrower->GetOwner());
	RemoveObject();
}
global func FxGemPyreStart(object target, effect, int temporary, c, e, owner,thrower)
{
	if (temporary) 
		return 1;
	effect.x=c[0];
	effect.y=c[1];
	effect.e=e;
	effect.thrower=thrower;
	effect.owner=owner;
	effect.objects=[];
	
	effect.particles =
	{
		Prototype = Particles_Air(),
		Size = PV_Linear(2, 0),
		R = PV_Random(120, 140),
		G = PV_Random(20, 30),
		G = PV_Random(90, 110),
		BlitMode = GFX_BLIT_Additive
	};
	
	if (e)
	{
		effect.particles.R = PV_Random(190, 200);
		effect.particles.G = 0;
		effect.particles.B = PV_Random(20, 40);
	}
}
global func FxGemPyreTimer(object target, effect, int time)
{
	var x=effect.x;
	var y=effect.y;
	var e=effect.e;
	
	if(time > 32) return -1;
	
	for(var i=0; i<(20 + time); i++)
	{
		var r = Random(360);
		var d = Random((((time/2)+1)*6)-((time/2)*4))+((time/2)*4)+RandomX(-2,2);
		if(!PathFree(x,y,x + Sin(r,d), y - Cos(r,d))) continue;
		var clr=RGB(122+Random(20),18+Random(10),90+Random(20));
		if(e)clr=RGB(190+Random(10),0,20+Random(20));
		var xoff = Sin(r, d);
		var yoff = -Cos(r, d);
		CreateParticle("Air", x + xoff, y + yoff, PV_Random(xoff - 3, xoff + 3), PV_Random(yoff - 3, yoff + 3), PV_Random(5, 10), effect.particles, 2);
	}
	
	for(var obj in FindObjects(Find_NoContainer(), Find_OCF(OCF_Alive), Find_Distance(((time/2)+1)*6,x,y),Find_Not(Find_Distance((time/2)*4,x,y)),Find_ID(Clonk)))
	{
		var end=false;	
		for(var i = 0; i < GetLength(effect.objects); i++)
			if(obj == effect.objects[i]) end=true;
		if(end) continue;
		if(PathFree(x,y,obj->GetX(),obj->GetY()))
		{
			obj->DoEnergy((-BoundBy((30-time),1,26)*3)/5,0,0,effect.thrower);
			obj->CreateParticle("MagicFire", 0, 0, PV_Random(-15, 15), PV_Random(-15, 15), PV_Random(5, 10), effect.particles, 20);
			obj->Fling(RandomX(-2,2),-2-(BoundBy((30-time),10,30)/10));
			effect.objects[GetLength(effect.objects)] = obj;
		}	
	}
	return 1;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
