// Clears materials on moving bricks.

#appendto MovingBrick

private func FxMoveHorizontalTimer(object target, proplist effect)
{
	if (target->GetX() > effect.Right - 23 + 10*(4 - size))
		if (target->GetComDir() == COMD_Right)
			SetComDir(COMD_Left);
			
	if (target->GetX() < effect.Left + 23)
		if (target->GetComDir() == COMD_Left)
			SetComDir(COMD_Right);	
			
	DigFreeRect(GetX() - 22, GetY() - 6, -22 + 10 * size , 12);

	return 1;
}

private func FxMoveVerticalTimer(object target, proplist effect)
{
	if (target->GetY() > effect.Bottom - 7)
		if (target->GetComDir() == COMD_Down)
			SetComDir(COMD_Up);
			
	if (target->GetY() < effect.Top + 7)
		if (target->GetComDir() == COMD_Up)
			SetComDir(COMD_Down);	
		
	DigFreeRect(GetX() - 22, GetY() - 6, 4 + 10 * size , 12);
	
	return 1;
}