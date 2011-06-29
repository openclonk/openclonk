// Appendto

#appendto Sword

public func AI_IsMeleeWeapon() { return true; }
public func AI_CanStrike() { return true; }
public func AI_CommandString() { return "AI_SwordAttack"; }
public func AI_TargetInRange(object target)
{
	if (ObjectDistance(target, Contained()) < 14)
		return true;
	return false;
}

protected func AI_SwordAttack(object clonk, int x, int y, object target)
{
	clonk->AI_Log("Sword attack on %s", target->GetName());
	
	// Attack
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	if (dx < 0 && clonk->GetDir() == DIR_Right)
		clonk->SetDir(DIR_Left);
	if (dx > 0 && clonk->GetDir() == DIR_Left)
		clonk->SetDir(DIR_Right);
	
	ControlUse(clonk, dx, dy);
	
	clonk->AppendCommand("Wait", nil, 20, nil, nil, nil, 20);
	clonk->AppendCommand("Call", clonk, nil, nil, target, nil, "AI_MeleeAttack");

}