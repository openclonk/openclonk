// Appendto

#appendto Javelin

public func AI_IsRangedWeapon() { return true; }
public func AI_IsLoaded() { return true; }
public func AI_CommandString() { return "AI_JavelinAttack"; }
public func AI_TargetHittable(object target) 
{
	var x = ObjectDistance(target, Contained());
	return Max(0, - x * (x - 100) / 25);
}

protected func AI_JavelinAttack(object clonk, int x, int y, object target)
{
	clonk->AI_Log("Javelin attack on %v", target);
	// Throw a javelin.
	AddEffect("AI_JavelinAim", clonk, 100, 1, this, nil, target);

	clonk->AppendCommand("Wait", nil, 24, nil, nil, nil, 24);
	clonk->AppendCommand("Call", clonk, nil, nil, target, nil, "AI_RangedAttack"); //add+finish
	return;
}

protected func FxAI_JavelinAimStart(object clonk, int num, int temporary, object target)
{
	if (temporary == 1)
		return;
	EffectVar(0, clonk, num) = target;
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var dist = Distance(0, 0, dx, dy);
	dy -= dist / 10;
	ControlUseStart(clonk, dx, dy);
}

protected func FxAI_JavelinAimTimer(object clonk, int num, int time)
{
	if (time > 16)
		return -1;
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var dist = Distance(0, 0, dx, dy);
	dy -= dist / 10;
	ControlUseHolding(clonk, dx, dy);
	return 1;
}

protected func FxAI_JavelinAimStop(object clonk, int num, int reason, bool temporary)
{
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var dist = Distance(0, 0, dx, dy);
	dy -= dist / 10;
	ControlUseStop(clonk, dx, dy);
	return 1;
}