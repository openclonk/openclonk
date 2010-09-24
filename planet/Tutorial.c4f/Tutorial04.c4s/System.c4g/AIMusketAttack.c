// Appendto

#appendto Musket

public func AI_IsRangedWeapon() { return true; }
public func AI_IsLoaded() { return !!FindObject(Find_Container(this), Find_ID(LeadShot)); }
public func AI_CommandString() { return "AI_MusketAttack"; }
public func AI_TargetHittable(object target) 
{
	var x = ObjectDistance(target, Contained());
	return Max(0, - x * (x - 400) / 400);
}

protected func AI_MusketAttack(object clonk, int x, int y, object target)
{
	clonk->FinishCommand(false, 1);
	clonk->AI_Log("Musket attack on %v", target);
	// Check for lead shot.
	var shot = FindObject(Find_Container(this), Find_ID(LeadShot));
	if (!shot)
	{
		clonk->AddCommand("Call", target, nil, nil, target, nil, "AI_MusketAttack");
		clonk->AddCommand("Acquire", nil, nil, nil, nil, nil, LeadShot);
		return;	
	}
	// Shoot a lead shot.
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var dist = Distance(0, 0, dx, dy);
	ControlUseStart(clonk, dx, dy);
	ControlUseStop(clonk, dx, dy);

	clonk->AppendCommand("Wait", nil, 36, nil, nil, nil, 36);
	clonk->AppendCommand("Call", clonk, nil, nil, target, nil, "AI_RangedAttack");
	clonk->AI_LogCommandStack();
	
	return;
}