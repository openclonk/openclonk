/*-- Elevator --*/

#include Library_Structure
#include Library_Ownable

local case, rope;
local partner, slave;

// Frees a rectangle for the case
func CreateShaft(int length)
{
	// Move the case out of the way
	case->SetPosition(case->GetX(), GetY()-10);
	ClearFreeRect(GetX() + 7, GetY() + 20, 24, length + 13);
	// Move the case back
	case->SetPosition(case->GetX(), GetY()+20);
}

/* Initialization */

func Construction(object creator)
{
	SetProperty("MeshTransformation", Trans_Rotate(-44,0,1,0));
	SetAction("Default");
	return _inherited(creator, ...);
}

func Initialize()
{
	CreateCase();
	CreateRope();

	if (partner)
	{
		if (Inside(partner->GetY(), GetY()-3, GetY()+3))
		{
			partner->LetsBecomeFriends(this);
			slave = true; // Note: This is liberal slavery
			case.slave = true; // I guess this is not so liberal
			SetPosition(GetX(), partner->GetY());
		}
		else
			partner = nil;
	}
	return _inherited();
}

func CreateCase()
{
	case = CreateObject(ElevatorCase, -19 * GetCalcDir(), 33, GetOwner());
	case->Connect(this);
}

func CreateRope()
{
	rope = CreateObject(ElevatorRope, -19 * GetCalcDir(), -11, GetOwner());
	rope->SetAction("Be", case.back);
}

/* Destruction */

func Destruction()
{
	rope->RemoveObject();
	if (partner) partner->LoseCombination();
}

/* Effects */

func StartEngine()
{
	Sound("ElevatorStart");
	ScheduleCall(this, "EngineLoop", 34);
	Sound("ElevatorMoving", nil, nil, nil, 1);
}
func EngineLoop()
{
	Sound("ElevatorMoving", nil, nil, nil, 1);
}
func StopEngine()
{
	Sound("ElevatorMoving", nil, nil, nil, -1);
	ClearScheduleCall(this, "EngineLoop");
	Sound("ElevatorStop");
}

/* Construction */

// Sticking to other elevators
func ConstructionCombineWith() { return "IsElevator"; }

// Called to determine if sticking is possible
func IsElevator(object previewer)
{
	if (!previewer) return true;

	if (GetDir() == DIR_Left)
	{
		if (previewer.direction == DIR_Right && previewer->GetX() > GetX()) return true;
	}
	else
	{
		if (previewer.direction == DIR_Left && previewer->GetX() < GetX()) return true;
	}
	return false;
}

// Called when the elevator construction site is created
func CombineWith(object other)
{
	// Save for use in Initialize
	partner = other;
}

/* Combination */

// Called by a new elevator next to this one
// The other elevator will be the slave
func LetsBecomeFriends(object other)
{
	partner = other;
	if (case) case->StartConnection(other);
}

// Partner was destroyed or moved
func LoseCombination()
{
	partner = nil;
	slave = false;
	if (case) case->LoseConnection();
}

// Called by our case because the case has a timer anyway
func CheckSlavery()
{
	// Check if somehow we moved away from our fellow
	if (ObjectDistance(partner) > 62 || !Inside(partner->GetY(), GetY()-1, GetY()+1))
	{
		LoseCombination();
		partner->LoseCombination();
	}
}

local ActMap = {
		Default = {
			Prototype = Action,
			Name = "Default",
			Procedure = DFA_NONE,
			Directions = 2,
			FlipDir = 1,
			Length = 1,
			Delay = 0,
			FacetBase = 1,
			NextAction = "Default",
		},
};

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(-20,1,0), Trans_Rotate(-20, 0, 1, 0)));
}
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 100;
local HitPoints = 70;