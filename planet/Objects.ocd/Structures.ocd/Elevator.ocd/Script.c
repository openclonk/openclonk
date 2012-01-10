/*-- Elevator --*/

#include Library_PowerConsumer

local case;

// Frees a rectangle for the case
func CreateShaft(int length)
{
	// Move the case out of the way
	case->SetPosition(case->GetX(), GetY()-10);
	ClearFreeRect(GetX() + 6, GetY() + 22, 23, length + 4);
	// Move the case back
	case->SetPosition(case->GetX(), GetY()+12);
}

/* Initialization */

func Initialize()
{
	CreateCase();
}

func CreateCase()
{
	case = CreateObject(ElevatorCase, 18, 26, GetOwner());
	case->Connect(this);
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

local Name = "$Name$";
local Description = "$Description$";