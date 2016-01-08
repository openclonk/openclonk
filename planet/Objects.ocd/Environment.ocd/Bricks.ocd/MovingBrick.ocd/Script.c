/*-- 
	Moving Bricks 
	
	
--*/

local size; // Length of the brick.

protected func Initialize()
{
	// Size defaults to four.
	SetSize(4);
	
	// Allow for dynamically changing speeds.
	ActMap = { Prototype = this.Prototype.ActMap };
	ActMap.Moving = { Prototype = ActMap.Moving };
	
	// Set floating action.
	SetAction("Moving");
	SetComDir(COMD_None);

	return;
}

public func SetSize(int to_size)
{
	size = BoundBy(to_size, 1, 4);
	// Update graphics.
	var graph = Format("Size%dN%d", size, 1 + Random(1));
	SetGraphics(graph);
	// Update solid
	SetSolidMask(0,size*8-8,10*size,8);
	return;
}

public func SetMoveSpeed(int speed)
{
	ActMap.Moving.Speed = Max(0, speed);
	return;
}

/*-- Horizontal movement --*/

public func MoveHorizontal(int left, int right, int speed)
{
	RemoveEffect("MoveHorizontal", this); RemoveEffect("MoveVertical", this);
	var effect = AddEffect("MoveHorizontal", this, 100, 1, this);
	effect.Left = left;
	effect.Right = right;
	if (speed != nil)
		SetMoveSpeed(10 * speed);
	if (GetComDir() != COMD_Right) SetComDir(COMD_Left);
	return;
}

private func FxMoveHorizontalTimer(object target, proplist effect)
{
	if (target->GetX() > effect.Right - 23 + 10*(4 - size))
		if (target->GetComDir() == COMD_Right)
			SetComDir(COMD_Left);
			
	if (target->GetX() < effect.Left + 23)
		if (target->GetComDir() == COMD_Left)
			SetComDir(COMD_Right);	

	return 1;
}

/*-- Vertical movement --*/

public func MoveVertical(int top, int bottom, int speed)
{
	RemoveEffect("MoveHorizontal", this); RemoveEffect("MoveVertical", this);
	var effect = AddEffect("MoveVertical", this, 100, 1, this);
	effect.Top = top;
	effect.Bottom = bottom;
	if (speed != nil)
		SetMoveSpeed(10 * speed);
	if (GetComDir() != COMD_Down) SetComDir(COMD_Up);
	return;
}

private func FxMoveVerticalTimer(object target, proplist effect)
{
	if (target->GetY() > effect.Bottom - 7)
		if (target->GetComDir() == COMD_Down)
			SetComDir(COMD_Up);
			
	if (target->GetY() < effect.Top + 7)
		if (target->GetComDir() == COMD_Up)
			SetComDir(COMD_Down);	

	return 1;
}

/* Scenario saving */

func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	if (size != 4) props->AddCall("Size", this, "SetSize", size);
	if (ActMap.Moving.Speed != GetID().ActMap.Moving.Speed) props->AddCall("MoveSpeed", this, "SetMoveSpeed", ActMap.Moving.Speed);
	if (GetComDir() == COMD_None) props->Remove("ComDir");
	return true;
}

func FxMoveHorizontalSaveScen(obj, fx, props)
{
	props->AddCall("Move", obj, "MoveHorizontal", fx.Left, fx.Right);
	return true;
}

func FxMoveVerticalSaveScen(obj, fx, props)
{
	props->AddCall("Move", obj, "MoveVertical", fx.Top, fx.Bottom);
	return true;
}

/* Properties */

local ActMap = {
	Moving = {
		Prototype = Action,
		Name = "Moving",
		Procedure = DFA_FLOAT,
		Length = 1,
		Delay = 1,
		X = 0,
		Y = 0,
		Accel = 20,
		Decel = 20,
		Speed = 100,
		Wdt = 40,
		Hgt = 8,
		NextAction = "Moving",
	},
};
local Name = "MovingBrick";
local Plane = 600;
local SolidMaskPlane = 150; // move almost everything so background stuff can be put onto moving bricks
