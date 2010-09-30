// Appendto

#appendto Bow

public func AI_IsRangedWeapon() { return true; }
public func AI_IsLoaded() { return !!FindObject(Find_Container(this), Find_ID(Arrow)); }
public func AI_CommandString() { return "AI_BowAttack"; }
public func AI_TargetHittable(object target)
{
	var xDist = target->GetX() - Contained()->GetX();
	var yDist = target->GetY() - Contained()->GetY();
	// (vel*vel*vel*vel - g*(g*x*x + 2*y*vel*vel)) <see: AI_BowFirePos()>
	// vel = 100, g = 20
	return Max(1, 100000000 - (400*xDist*xDist + yDist*400000)); 
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
	//var dist = Distance(0, 0, dx, dy);
	//dy -= dist / 10;
	//ControlUseStart(clonk, dx, dy);
	var aimVector = AI_AimPos(dx, dy, 100, 5);
	if(aimVector == -1) {
		ControlUseCancel(clonk,nil,nil);
	}else{
		ControlUseStart(clonk, aimVector[0], aimVector[1]);
	}
}

protected func FxAI_BowAimTimer(object clonk, int num, int time)
{
	if (time > 30)
		return -1;
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	//var dist = Distance(0, 0, dx, dy);
	//dy -= dist / 10;
	//ControlUseHolding(clonk, dx, dy);
	var aimVector = AI_AimPos(dx, dy, 100, 5);
	if(aimVector == -1) {
		ControlUseCancel(clonk,nil,nil);
	}else{
		ControlUseHolding(clonk, aimVector[0], aimVector[1]);
		return 1;
	}
}

protected func FxAI_BowAimStop(object clonk, int num, int reason, bool temporary)
{
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	//var dist = Distance(0, 0, dx, dy);
	//dy -= dist / 10;
	//ControlUseStop(clonk, dx, dy);
	var aimVector = AI_AimPos(dx, dy, 100, 5);
	if(aimVector == -1) {
		ControlUseCancel(clonk,nil,nil);
	}else{
		ControlUseStop(clonk, aimVector[0], aimVector[1]);
		return 1;
	}
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