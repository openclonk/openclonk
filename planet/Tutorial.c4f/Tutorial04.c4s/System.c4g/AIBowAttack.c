// Appendto

#appendto Bow

public func AI_WeaponSpray() { return 5; }
public func AI_IsRangedWeapon() { return true; }
public func AI_IsLoaded() { return !!FindObject(Find_Container(this), Find_ID(Arrow)); }
public func AI_CommandString() { return "AI_BowAttack"; }
public func AI_CanHitTarget(object target)
{
	var x = target->GetX() - Contained()->GetX();
	var y = target->GetY() - Contained()->GetY();
	if (AI_AimPos(x, y, 100) == nil)
		return false;
	return true;
}

protected func AI_BowAttack(object clonk, int x, int y, object target)
{
	clonk->AI_Log("Bow attack on %v", target);
	// Check for arrows.
	var arrows = FindObject(Find_Container(this), Find_ID(Arrow));
	if (!arrows)
	{
		clonk->AddCommand("Call", this, nil, nil, target, nil, "AI_BowAttack");
		clonk->AddCommand("Acquire", nil, nil, nil, nil, nil, Arrow);
		return;	
	}
	AddEffect("AI_BowAim", clonk, 100, 1, this, nil, target);

	clonk->AppendCommand("Wait", nil, 50, nil, nil, nil, 50);
	clonk->AppendCommand("Call", clonk, nil, nil, target, nil, "AI_RangedAttack"); //add+finish
	return;
}

protected func FxAI_BowAimStart(object clonk, int num, int temporary, object target)
{
	if (temporary == 1)
		return;
	EffectVar(0, clonk, num) = target;
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var angle = AI_AimPos(dx, dy, 100, AI_WeaponSpray());
	if (angle == nil)
	{
		ControlUseCancel(clonk);
		return -1;
	}
	ControlUseStart(clonk, angle[0], angle[1]);
	return 1;
}

protected func FxAI_BowAimTimer(object clonk, int num, int time)
{
	if (time > 30)
		return -1;
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var angle = AI_AimPos(dx, dy, 100, AI_WeaponSpray());
	if (angle == nil)
	{
		ControlUseCancel(clonk);
		return -1;
	}
	ControlUseHolding(clonk, angle[0], angle[1]);
	return 1;
}

protected func FxAI_BowAimStop(object clonk, int num, int reason, bool temporary)
{
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var angle = AI_AimPos(dx, dy, 100, AI_WeaponSpray());
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
	if(root < 0)
		return -1;
	var angle = Angle(0,0,(g*x),v*v-Sqrt(root));
	//allows for finer variation than adding Random(2*spread)-spread to the angle
    return [Sin(angle, 100)+Random(2*spread)-spread, Cos(angle, 100)+Random(2*spread)-spread];
}