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
global func FxGemShieldCreationStart(object target, int num, int temporary, x, y, e)
{
	if (temporary) 
		return 1;
	num.var0=x;
	num.var1=y;
	num.var2=e;	
}
global func FxGemShieldCreationTimer(object target, int num, int time)
{
	if(time > 26) return -1;
	var x=num.var0;
	var y=num.var1;
	var e=num.var2;
	var clr=RGB(122+Random(20),18+Random(10),90+Random(20));
	if(e)clr=RGB(190+Random(10),0,20+Random(20));
	var shield=CreateObject(CrystalShield,x+Sin(time*7,35),y+Cos(time*7,35));
	shield->SetR(-time*7);
	shield->SetClrModulation(clr);
	CastParticles("MagicSpark",10+Random(10),20,x+Sin(time*7,39),y+Cos(time*7,39),20,28,shield->GetClrModulation(),shield->GetClrModulation());
	var shield=CreateObject(CrystalShield,x-Sin(-7+time*7,35),y+Cos(-7+time*7,35));
	shield->SetR(-7 + time*7);
	shield->SetClrModulation(clr);
	CastParticles("MagicSpark",10+Random(10),20,x-Sin(-7+time*7,39),y+Cos(-7+time*7,39),20,28,shield->GetClrModulation(),shield->GetClrModulation());
	
	return 1;
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
