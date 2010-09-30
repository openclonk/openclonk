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
	var dy = target->GetY() - (clonk->GetY()-5);
	//var dist = Distance(0, 0, dx, dy);
	//dy -= dist / 10;
	//ControlUseStart(clonk, dx, dy);
	var aimVector = AI_AimPos(dx, dy, clonk->GetPhysical("Throw") / 800, 0);
	if(aimVector == -1) {
		ControlUseCancel(clonk,nil,nil);
	}else{
		ControlUseStart(clonk, aimVector[0], aimVector[1]);
	}
}

protected func FxAI_JavelinAimTimer(object clonk, int num, int time)
{
	if (time > 16)
		return -1;
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - (clonk->GetY()-5);
	//var dist = Distance(0, 0, dx, dy);
	//dy -= dist / 10;
	//ControlUseHolding(clonk, dx, dy);
	var aimVector = AI_AimPos(dx, dy, clonk->GetPhysical("Throw") / 800, 0);
	if(aimVector == -1) {
		ControlUseCancel(clonk,nil,nil);
	}else{
		ControlUseHolding(clonk, aimVector[0], aimVector[1]);
	}
	return 1;
}

protected func FxAI_JavelinAimStop(object clonk, int num, int reason, bool temporary)
{
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - (clonk->GetY()-5);
	//var dist = Distance(0, 0, dx, dy);
	//dy -= dist / 10;
	//ControlUseStop(clonk, dx, dy);
	var aimVector = AI_AimPos(dx, dy, clonk->GetPhysical("Throw") / 800, 0);
	if(aimVector == -1) {
		ControlUseCancel(clonk,nil,nil);
	}else{
		ControlUseStop(clonk, aimVector[0], aimVector[1]);
	}
	return 1;
}

//vel = "muzzle velocity" (speed at which the projectile is launched)
//spread = variation in aim
private func AI_AimPos(int x, int y, int vel, int spread)
{
	y = -y;
	var g = GetGravity()/5; //FnGetGravity() multiplies actual gravity by 500
	var root = (vel*vel*vel*vel - g*(g*x*x + 2*y*vel*vel));
	if(root < 0)
		return -1;
	var angle = Angle(0,0,(g*x),vel*vel-Sqrt(root));
    return [Sin(angle, 100)+Random(2*spread)-spread, Cos(angle, 100)+Random(2*spread)-spread];
}