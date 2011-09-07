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
	var effect = AddEffect("MoveHorizontal", this, 100, 1, this);
	effect.Left = left;
	effect.Right = right;
	if (speed != nil)
		SetMoveSpeed(10 * speed);
	SetComDir(COMD_Left);
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
	var effect = AddEffect("MoveVertical", this, 100, 1, this);
	effect.Top = top;
	effect.Bottom = bottom;
	if (speed != nil)
		SetMoveSpeed(10 * speed);
	SetComDir(COMD_Up);
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
