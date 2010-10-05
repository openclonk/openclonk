// Appendto

#appendto Javelin

local spread_x;
local spread_y;
local lob_shot;

public func AI_Spread(){return 10;}
public func AI_IsRangedWeapon() { return true; }
public func AI_IsLoaded() { return true; }
public func AI_CommandString() { return "AI_JavelinAttack"; }
public func AI_CanHitTarget(object target) 
{
	var v = Contained()->GetPhysical("Throw") / 800;
	var x = target->GetX() - Contained()->GetX();
	var y = target->GetY() - Contained()->GetY();
	
	lob_shot = false;
	var angle = AI_AimPos(x, y, v, lob_shot, true);
	if (angle == nil) return false;
	
	var tolerance = 20**2; // tolerance = margin for error (in px) squared
	
	var end = SimFlight(nil, nil, angle[0], angle[1], 1, nil, angle[2]);
	//Log("end: %d, %d, %d, %d, %d", end[0], end[1], end[2], end[3], end[4]);
	
	var error = (target->GetX() - end[0])**2 + (target->GetY() - end[1])**2;
	//Log("error:%d", error);
	
	if(error < tolerance) return true;
	
	//Log("Low shot misses");
	// Oh, the straight shot didn't work. 
	// Can I shoot over the obstacle instead?
	lob_shot = true;
	angle = AI_AimPos(x, y, v, lob_shot, true);
	//Log("angle: %d, %d, %d",angle[0],angle[1],angle[2]);
	
	end = SimFlight(nil, nil, angle[0], angle[1], 1, nil, angle[2]);
	if (end == nil) return false;
	
	error = (target->GetX() - end[0])**2 + (target->GetY() - end[1])**2;
	
	if(error < tolerance) return true;
	return false;
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
	var angle = AI_AimPos(dx, dy, clonk->GetPhysical("Throw") / 800, lob_shot);
	
	if (angle == nil)
	{
		ControlUseCancel(clonk);
		return -1;
	}
	
	//set random factors here, so it doesn't stutter all over the place.
	spread_x = Random(2*AI_Spread())-AI_Spread();
	spread_y = Random(2*AI_Spread())-AI_Spread();
	
	ControlUseStart(clonk, angle[0]+spread_x, angle[1]+spread_y);
	return 1;
}

protected func FxAI_JavelinAimTimer(object clonk, int num, int time)
{
	if (time > 16)
		return -1;
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY() + 10;
	var angle = AI_AimPos(dx, dy, clonk->GetPhysical("Throw") / 800, lob_shot);
	if (angle == nil)
	{
		ControlUseCancel(clonk);
		return -1;
	}
	ControlUseHolding(clonk, angle[0]+spread_x, angle[1]+spread_y);
	return 1;
}

protected func FxAI_JavelinAimStop(object clonk, int num, int reason, bool temporary)
{
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY() + 10;
	var angle = AI_AimPos(dx, dy, clonk->GetPhysical("Throw") / 800, lob_shot);
	if (angle == nil)
	{
		ControlUseCancel(clonk);
		return -1;
	}
	ControlUseStop(clonk, angle[0]+spread_x, angle[1]+spread_y);
	return 1;
}

//v = "muzzle speed" (speed at which the projectile is launched)
//returns an array of x, y or x, y, t
private func AI_AimPos(int x, int y, int v, bool lob, bool t)
{
	var g = GetGravity()/5; //FnGetGravity() multiplies actual gravity by 500
	var root = (v**4 - g*(g*x*x - 2*y*v*v));
	if(root < 0)
		return nil;
	if(lob)
	{
		var angle = Angle(0, 0, (g*x), v*v+Sqrt(root));
	} else {
		var angle = Angle(0, 0, (g*x), v*v-Sqrt(root));
	}
	if(t){
		var xAng = Sin(angle, v);
		var yAng = Cos(angle, v);
		angle = Angle(0,0,xAng,yAng);
		var yDir = -Cos( Normalize(angle, -180), v);
		var time = 10*( -yDir + Sqrt( (yDir**2) + 2*g*y ) ) / (g) + 1;
		return [xAng, yAng, time];
	}
	return [Sin(angle, v), Cos(angle, v)];
}