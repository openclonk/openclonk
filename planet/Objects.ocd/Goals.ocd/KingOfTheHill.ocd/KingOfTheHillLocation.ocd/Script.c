/*
Location for King of the Hill
Author: Zapper

This object manages the logic behind the goal.
It creates the marker and a star circle one frame after it is created.
*/

local marker;
local stars;
local color;
local king;
local timer;
local koth_goal;

func Initialize() {
	
	ScheduleCall(this, "PostInitialize", 1, 0);
	timer = 0;
	return(1);
}

func SetKotH(object koth)
{
	koth_goal = koth;
}

func GetKing()
{
	return king;
}

func Destruction()
{
	for (var s in stars)
	{
		if (s)
			s->RemoveObject();
	}
	if (marker)
		marker->RemoveObject();
}

func PostInitialize()
{
	marker = CreateObjectAbove(KingOfTheHill_Marker, 0, -5, NO_OWNER);
	marker->SetOrigin(this);
	CreateStarCircle();
	AddEffect("Timer", this, 10, 10, this);
}

func NewPosition()
{
	if (marker) marker->SetPosition(this->GetX(), this->GetY());
	CreateStarCircle();
}

func FxTimerTimer(target, effect, effect_time)
{
	this->AdjustStarColor();
	this->CheckNewKing();
}

func CheckNewKing()
{
	if (king)
	if (!king->GetAlive())
		king = nil;
		
	if (king) return;
	
	var new = FindObject(Find_Distance(koth_goal->GetRadius()), Find_NoContainer(), Find_OCF(OCF_CrewMember));
	if (new)
	{
		king = new;
	}
}

func SetKing(object to)
{
	
	if (king)
		if (GetEffect("KOTHKing", king))
			RemoveEffect("KOTHKing", king);
	king = to;
	
	if (king != nil)
		AddEffect("KOTHKing", king, 10, 35, this);
}

func FxKOTHKingTimer(target, effect)
{
	//target->DoEnergy(1);
}

func FxKOTHKingStop(target, effect, reason, temp)
{
	if (temp) return;
	if (!target) return;
	if (!this) return;
	
	var killer = target->GetKiller();
	if (killer == NO_OWNER || killer == target->GetOwner())
		this->SetKing(nil);
	else
	{
		var crew = GetCursor(killer);
		if (!(crew->GetOCF() & OCF_CrewMember))
			crew = GetCrew(killer);
		if (!crew) this->SetKing(nil);
		else
			this->SetKing(crew);
	}
}

func GetStarColor(int which)
{
	if (king) return color;
	return RGB(200 + Cos(timer + which, 50), 200 + Sin(timer * 2 + which, 50), 0);
}

func AdjustStarColor()
{
	++timer;
	if (king)
	{
		var percent=(king->GetEnergy() * 100) / (king->GetMaxEnergy());
		var red = 255; if (percent > 50) red=(255*(100-2*(percent-50))) / 100;
		var green = 255; if (percent < 50) green=(255*(2*percent)) / 100;
		color = RGB(red/2, green/2, 0);
	}
	else
	{
		color = RGB(200 + Cos(timer, 50), 200 + Sin(timer * 2, 50), 0);
	}
}

func CreateStarCircle()
{
	var radius = koth_goal->GetRadius();
	if (radius == nil) return FatalError("Goal_KingOfTheHill: radius has to be set before use!");
	
	if (GetType(stars) != C4V_Array)
		stars=[];
	for (var star in stars) star->RemoveObject();
	stars=[];
	var amount = radius / 15;
	
	var cnt = 0;
	for (var i = 0;i<360;i += 360/amount)
	{
		var star = CreateObjectAbove(KingOfTheHill_Star, Sin(i, radius), -Cos(i, radius) + 15);
		star->Init(this, cnt++);
		stars[GetLength(stars)]=star;
	}
}

// stored by main goal
public func SaveScenarioObject() { return false; }

local Name = "$Name$";
