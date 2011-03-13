// Appendto

#appendto Bow

local spread_x;
local spread_y;
local lob_shot;

public func AI_Spread(){return 5;}
public func AI_IsRangedWeapon() { return true; }
public func AI_IsLoaded() { return !!FindObject(Find_Container(this), Find_ID(Arrow)); }
public func AI_CommandString() { return "AI_BowAttack"; }
public func AI_CanHitTarget(object target)
{
	var x = target->GetX() - Contained()->GetX();
	var y = target->GetY() - Contained()->GetY();
	lob_shot = false;
	
	var angle = AI_AimPos(x, y, 100, lob_shot, true);
	if (angle == nil) return false;
	//Log("angle: %d, %d, %d",angle[0],angle[1],angle[2]);
	
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
	angle = AI_AimPos(x, y, 100, lob_shot, true);
	//Log("angle: %d, %d, %d",angle[0],angle[1],angle[2]);
	
	end = SimFlight(nil, nil, angle[0], angle[1], 1, nil, angle[2]);
	if (end == nil) return false;
	//Log("end: %d, %d, %d, %d, %d",end[0], end[1], end[2], end[3], end[4]);
	
	error = (target->GetX() - end[0])**2 + (target->GetY() - end[1])**2;
	//Log("error:%d", error);
	
	if(error < tolerance) return true;
	return false;
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

protected func FxAI_BowAimStart(object clonk, effect, int temporary, object target)
{
	if (temporary == 1)
		return;
	effect.var0 = target;
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var angle = AI_AimPos(dx, dy, 100, lob_shot);
	
	if (angle == nil)
	{
		ControlUseCancel(clonk);
		return -1;
	}
	
	//set random factors here, so it doesn't stutter all over the place.
	spread_x = (Random(2*AI_Spread())-AI_Spread()); 
	spread_y = (Random(2*AI_Spread())-AI_Spread());
	
	ControlUseStart(clonk, angle[0]+spread_x, angle[1]+spread_y);
	return 1;
}

protected func FxAI_BowAimTimer(object clonk, effect, int time)
{
	if (time > 30)
		return -1;
	var target = effect.var0;
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var angle = AI_AimPos(dx, dy, 100, lob_shot);
	if (angle == nil)
	{
		ControlUseCancel(clonk);
		return -1;
	}
	ControlUseHolding(clonk, angle[0]+spread_x, angle[1]+spread_y);
	return 1;
}

protected func FxAI_BowAimStop(object clonk, effect, int reason, bool temporary)
{
	var target = effect.var0;
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var angle = AI_AimPos(dx, dy, 100, lob_shot);
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
		var xAng = Sin(angle, 100);
		var yAng = Cos(angle, 100);
		angle = Angle(0,0,xAng,yAng);
		var yDir = Cos( Normalize(angle, -180), -v);
		var time = 10*( -yDir + Sqrt( (yDir**2) + 2*g*y ) ) / (g) + 1;
		return [xAng, yAng, time];
	}
	return [Sin(angle, 100), Cos(angle, 100)];
}