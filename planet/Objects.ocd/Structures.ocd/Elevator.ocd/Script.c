/*-- Elevator --*/

#include Library_Ownable

local case, rope;

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

func Construction()
{
	SetProperty("MeshTransformation", Trans_Rotate(-44,0,1,0));
}

func Initialize()
{
	CreateCase();
	CreateRope();
}

func CreateCase()
{
	case = CreateObject(ElevatorCase, 19, 33, GetOwner());
	case->Connect(this);
}

func CreateRope()
{
	rope = CreateObject(ElevatorRope, 19, -11, GetOwner());
	rope->SetAction("Be", case);
}

/* Destruction */

func Destruction()
{
	rope->RemoveObject();
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

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(-20,1,0), Trans_Rotate(-20, 0, 1, 0)));
}
local Name = "$Name$";
local Description = "$Description$";