/*--- Flint ---*/

local e;

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

protected func Departure()
{
	SetRDir(RandomX(-15,15));
}

func Hit()
{

	AddEffect("GemSlowField",nil,100,1,nil,nil,GetX(),GetY(),e);
	RemoveObject();
}
global func FxGemSlowFieldStart(object target, effect, int temporary, x, y, e)
{
	if (temporary) 
		return 1;
	effect.x=x;
	effect.var1=y;
	effect.var2=e;
	
	effect.particles =
	{
		Prototype = Particles_Spark(),
		Size = PV_Linear(3, 0),
		ForceY = PV_Gravity(-40),
		R = PV_Random(120, 140),
		G = PV_Random(20, 30),
		B = PV_Random(90, 110),
		OnCollision = PC_Die(),
		CollisionVertex = 1000
	};
	
	if (e)
	{
		effect.particles.R = PV_Random(190, 210);
		effect.particles.G = 0;
		effect.particles.B = PV_Random(20, 40);
	}
}
global func FxGemSlowFieldTimer(object target, effect, int time)
{
	var x=effect.x;
	var y=effect.var1;
	var e=effect.var2;
	if(time > (150)) return -1;
	for(var i=0; i<40; i++)
	{
		var r=Random(360);
		var d=Min(Random(20)+Random(130),62);
		if(!PathFree(x,y,x + Sin(r,d), y - Cos(r,d))) continue;
		CreateParticle("MagicFire", x + Sin(r,d), y - Cos(r,d), PV_Random(-2, 2), PV_Random(0, 4), PV_Random(10, 40), effect.particles, 2);
	}
	for(var obj in FindObjects(Find_Distance(62,x,y)))
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
