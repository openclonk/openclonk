/*
 Star for King of the Hill
 Author: Zapper
 */

local origin;
local number;

func Init(o, n)
{	
	origin = o;
	number = n;
	SetR(Angle(origin->GetX(), origin->GetY(), this->GetX(), this->GetY()) + 45);
	
	AddEffect("Timer", this, 1, 1 + Random(3), this);
}

func FxTimerTimer(target, effect, time)
{
	SetClrModulation(origin->GetStarColor(number));
}

public func SaveScenarioObject() { return false; }

local Name = "$Name$";
