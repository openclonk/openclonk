/*--- Shield Gem ---*/

local has_graphics_e;

public func Initialize()
{
	if (Random(2))
	{
		SetGraphics("E");
		has_graphics_e = true;	
	}
	else
	{
		SetGraphics("");
		has_graphics_e = false;
	}
	
	if (this->GetX() < 920)
	{
		SetGraphics("E");
		has_graphics_e = true;
	}
	else if (this->GetX() > 1280)
	{
		SetGraphics("");
		has_graphics_e = false;
	}
	 
	SetR(Random(360));
}

public func Departure()
{
	SetRDir(RandomX(-15, 15));
}

func Hit()
{
	AddEffect("GemShieldCreation", nil, 100, 1, nil, nil, GetX(), GetY(), has_graphics_e);
	RemoveObject();
}

global func FxGemShieldCreationStart(object target, effect, int temporary, int x, int y, bool e)
{
	if (temporary) 
		return 1;

	effect.x = x;
	effect.y = y;
	effect.e = e;
	
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
	if (time > 26) return -1;
	var x = effect.x;
	var y = effect.y;
	var e = effect.e;

	var color;
	if (e)
	{
		color = RGB(190 + Random(10), 0, 20 + Random(20));
	}
	else
	{
		color = RGB(122 + Random(20), 18 + Random(10), 90 + Random(20));
	}
	
	var shield=CreateObjectAbove(CrystalShield, x + Sin(time * 7, 35), y + Cos(time * 7, 35));
	shield->SetR(-time * 7);
	shield->SetClrModulation(color);
	CreateParticle("MagicSpark", x + Sin(time * 7, 39), y + Cos(time * 7, 39), PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(10, 20), effect.particles, 10);

	var shield = CreateObjectAbove(CrystalShield, x - Sin(-7 + time * 7, 35),y + Cos(-7 + time * 7, 35));
	shield->SetR(-7 + time*7);
	shield->SetClrModulation(color);
	CreateParticle("MagicSpark", x - Sin(-7 + time * 7, 39), y + Cos(-7 + time * 7, 39), PV_Random(-10, 10), PV_Random(-10, 10), PV_Random(10, 20), effect.particles, 10);
	return 1;
}

local Collectible = true;
local Name = "$Name$";
local Description = "$Description$";
