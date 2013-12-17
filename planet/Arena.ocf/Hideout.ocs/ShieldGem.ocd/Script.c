/*--- Shield Gem ---*/

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
	AddEffect("GemShieldCreation",nil,100,1,nil,nil,GetX(),GetY(),e);
	RemoveObject();
}
global func FxGemShieldCreationStart(object target, effect, int temporary, x, y, e)
{
	if (temporary) 
		return 1;
	effect.x=x;
	effect.var1=y;
	effect.var2=e;
	
	effect.particles =
	{
		Prototype = Particles_Spark(),
		R = PV_Random(120, 140),
		G = PV_Random(20, 30),
		B = PV_Random(90, 110)
	};
	
	if (e)
	{
		effect.particles.R = PV_Random(190, 200);
		effect.particles.G = 0;
		effect.particles.B = PV_Random(20, 40);
	}
}
global func FxGemShieldCreationTimer(object target, effect, int time)
{
	if(time > 26) return -1;
	var x=effect.x;
	var y=effect.var1;
	var e=effect.var2;
	var clr=RGB(122+Random(20),18+Random(10),90+Random(20));
	if(e)clr=RGB(190+Random(10),0,20+Random(20));
	
	var shield=CreateObject(CrystalShield,x+Sin(time*7,35),y+Cos(time*7,35));
	shield->SetR(-time*7);
	shield->SetClrModulation(clr);
	CreateParticle("MagicSpark", x+Sin(time*7,39),y+Cos(time*7,39), PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(10, 20), effect.particles, 10);
	
	var shield=CreateObject(CrystalShield,x-Sin(-7+time*7,35),y+Cos(-7+time*7,35));
	shield->SetR(-7 + time*7);
	shield->SetClrModulation(clr);
	CreateParticle("MagicSpark", x-Sin(-7+time*7,39),y+Cos(-7+time*7,39), PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(10, 20), effect.particles, 10);
	return 1;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
