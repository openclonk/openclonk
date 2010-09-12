/*
 Star for King of the Hill
 Author: Zapper
 */

local origin;
local number;

func Init(o, n)
{
	this->SetObjectBlitMode(GFX_BLIT_Mod2);
	origin=o;
	number=n;
	SetR(Angle(origin->GetX(), origin->GetY(), this->GetX(), this->GetY()) + 45);
}

func Initialize()
{
	SetGraphics(0, KingOfTheHill_Marker);
	AddEffect("Timer", this, 10, 1, this);
	this->SetGraphics(nil, KingOfTheHill_Marker);
}

func FxTimerTimer()
{
	this->SetClrModulation(origin->GetStarColor(number));
}

local Name = "$Name$";
