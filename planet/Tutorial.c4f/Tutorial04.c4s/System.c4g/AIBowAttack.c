// Appendto

#appendto Bow

public func AI_IsRangedWeapon() { return true; }
public func AI_IsLoaded() { return !!FindObject(Find_Container(this), Find_ID(Arrow)); }
public func AI_CommandString() { return "AI_BowAttack"; }
public func AI_TargetHittable(object target)
{
	var x = ObjectDistance(target, Contained());
	return Max(0, - x * (x - 200) / 100);
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
	// Shoot an arrow. Todo: aim better.
	AddEffect("AI_BowAim", clonk, 100, 1, this, nil, target);

	clonk->AppendCommand("Wait", nil, 30, nil, nil, nil, 30);
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
	var dist = Distance(0, 0, dx, dy);
	dy -= dist / 10;
	ControlUseStart(clonk, dx, dy);
}

protected func FxAI_BowAimTimer(object clonk, int num, int time)
{
	if (time > 30)
		return -1;
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var dist = Distance(0, 0, dx, dy);
	dy -= dist / 10;
	ControlUseHolding(clonk, dx, dy);
	return 1;
}

protected func FxAI_BowAimStop(object clonk, int num, int reason, bool temporary)
{
	var target = EffectVar(0, clonk, num);
	var dx = target->GetX() - clonk->GetX();
	var dy = target->GetY() - clonk->GetY();
	var dist = Distance(0, 0, dx, dy);
	dy -= dist / 10;
	ControlUseStop(clonk, dx, dy);
	return 1;
}

private func AI_BowFirePos(x, y)
{
	var v = 10, // 10 px / tick
		f = 200, // Fixpunktfaktor
		g = -GetGravity() * f * f / 1000 / v, // 0,2 px / tick²
		x = x * f / v,
		y = -y * f / v /* Korrektur: */ - Abs(x) * GetGravity() / 2000,
		d = y * y + x * x,
		k = y + f * f * f / 2 / g,
		w = k * k - d;
	if (w < 0)
		return;
	var t1 = Sqrt( (k + Sqrt(w)) * (f * f * f / g) ),
		t2 = Sqrt( (k - Sqrt(w)) * (f * f * f / g) ),
		phi1 = ArcCos(x, t1),
		phi2 = ArcCos(x, t2);
	if(y < g * t1 / f * t1 / f / f) phi1 = -phi1;
	if(y < g * t2 / f * t2 / f / f) phi2 = -phi2;
    // Winkel umrechnen
    phi1 = (270 - phi1) % 360 - 180; 
    phi2 = 90 - phi2;

    var angle = phi1;
	if(t2 < t1 * 3)
    	angle = phi2;
    	
    return [Sin(angle, 100), Cos(angle, 100)];
}