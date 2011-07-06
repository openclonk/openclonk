local Name = "$Name$";
local Description = "$Description$";

local fade;
local color;

local ActMap = {
Fly = {
	Prototype = Action,
	Name = "Fly",
	Procedure = DFA_FLOAT,
	Directions = 1,
	Length = 1,
	Delay = 0,
	FacetBase=1,
}
};

public func Initialize()
{
	this.Plane=1545;
	fade = 0;
	color = GetPlayerColor(GetOwner());
	SetClrModulation(color|RGBa(0,0,0,fade));
	SetAction("Fly");
	SetComDir(COMD_None);
	return true;
}

func MoveTo(int x, int y, int r)
{
	if(GetEffect("MoveTo", this)) RemoveEffect("MoveTo", this);
	AddEffect("MoveTo", this, 1, 1, this, 0, x, y, r);
}

func FxMoveToStart(target, effect, temp, x, y, r)
{
	effect.x = x;
	effect.y = y;
	effect.r = r;
	effect.distance = Distance(GetX(), GetY(), x, y);
	var r_diff = GetTurnDirection(GetR(), r);
	if(r_diff)
	{
		effect.r_step = Max(1, Abs(effect.distance / r_diff));
		effect.r_step *= BoundBy(r_diff, -1, 1);
	} else effect.r_step = 0;
	effect.x_step = (effect.x - GetX());
	effect.y_step = (effect.y - GetY());
	
	SetXDir(effect.x_step, 100);
	SetYDir(effect.y_step, 100);
	
	effect.x_cnt = 0;
	effect.y_cnt = 0;
}

func FxMoveToTimer(target, effect, time)
{
	if((Abs(GetX() - effect.x) < 2) && (Abs(GetY() - effect.y) < 2))
	{
		SetPosition(effect.x, effect.y);
		SetR(effect.r);
		SetSpeed(0, 0);
		return -1;
	} 
	if(Abs(GetR() - effect.r) >= 2) SetR(GetR() + effect.r_step);
	/*
	effect.x_cnt += 10;
	effect.y_cnt += 10;
	
	if(effect.x_step)
	while(effect.x_cnt >= Abs(effect.x_step))
	{
		effect.x_cnt -= Abs(effect.x_step);
		SetPosition(GetX() + BoundBy(effect.x_step, -1, 1), GetY());
	}
	
	if(effect.y_step)
	while(effect.y_cnt >= Abs(effect.y_step))
	{
		effect.y_cnt -= Abs(effect.y_step);
		SetPosition(GetX(), GetY() + BoundBy(effect.y_step, -1, 1));
	}
	*/
		
	
}

func FadeIn()
{
	if(GetEffect("Fade*", this)) RemoveEffect("Fade*", this);
	AddEffect("FadeIn", this, 1, 1, this);
}

func FadeOut()
{
	if(GetEffect("Fade*", this)) RemoveEffect("Fade*", this);
	AddEffect("FadeOut", this, 1, 1, this);
}

func FxFadeInTimer(target, effect, time)
{
	if(fade == 255) return -1;
	fade = BoundBy(fade+3, 0, 255);
	SetClrModulation(color|RGBa(0,0,0,fade));
}

func FxFadeOutTimer(target, effect, time)
{
	if(fade == 0) return -1;
	fade = BoundBy(fade-3, 0, 255);
	SetClrModulation(color|RGBa(0,0,0,fade));
}