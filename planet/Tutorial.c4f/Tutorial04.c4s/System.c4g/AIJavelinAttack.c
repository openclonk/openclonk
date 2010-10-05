// Appendto

#appendto Javelin


public func AI_WeaponSpray() { return 10; }
public func AI_IsRangedWeapon() { return true; }
public func AI_IsLoaded() { return true; }
public func AI_CommandString() { return "AI_JavelinAttack"; }
public func AI_CanHitTarget(object target) 
{
	var v = Contained()->GetPhysical("Throw") / 800;
	var x = target->GetX() - Contained()->GetX();
	var y = target->GetY() - Contained()->GetY() + 10;
	if (AI_AimPos(x, y, v) == nil)
		return false;
	return true;
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
	var dy = target->GetY() - clonk->GetY() + 10;
	var angle = AI_AimPos(dx, dy, clonk->GetPhysical("Throw") / 800, AI_WeaponSpray());
	if (angle == nil)
	{
		ControlUseCancel(clonk);
		return -1;
	}
	ControlUseStart(clonk, angle[0], angle[1]);
	return 1;
}

protected func FxAI_JavelinAimTimer(object clonk, int num, int time)
{
	if (time > 16)
		return -1;
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY() + 10;
	var angle = AI_AimPos(dx, dy, clonk->GetPhysical("Throw") / 800, AI_WeaponSpray());
	if (angle == nil)
	{
		ControlUseCancel(clonk);
		return -1;
	}
	ControlUseHolding(clonk, angle[0], angle[1]);
	return 1;
}

protected func FxAI_JavelinAimStop(object clonk, int num, int reason, bool temporary)
{
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY() + 10;
	var angle = AI_AimPos(dx, dy, clonk->GetPhysical("Throw") / 800, AI_WeaponSpray());
	if (angle == nil)
	{
		ControlUseCancel(clonk);
		return -1;
	}
	ControlUseStop(clonk, angle[0], angle[1]);
	return 1;
}

//v = "muzzle speed" (speed at which the projectile is launched)
//spread = variation in aim
private func AI_AimPos(int x, int y, int v, int spread)
{
	var g = GetGravity()/5; //FnGetGravity() multiplies actual gravity by 500
	var root = (v**4 - g*(g*x*x - 2*y*v*v));
	if (root < 0)
		return nil;
	var angle = Angle(0,0,(g*x),v*v-Sqrt(root));
	//allows for finer variation than adding Random(2*spread)-spread to the angle
    return [Sin(angle, 100)+Random(2*spread)-spread, Cos(angle, 100)+Random(2*spread)-spread];
}